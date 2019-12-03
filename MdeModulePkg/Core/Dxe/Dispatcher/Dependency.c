/** @file
  DXE Dispatcher Dependency Evaluator.

  This routine evaluates a dependency expression (DEPENDENCY_EXPRESSION) to determine
  if a driver can be scheduled for execution.  The criteria for
  schedulability is that the dependency expression is satisfied.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

//
// Global stack used to evaluate dependency expressions
//
BOOLEAN *mDepexEvaluationStack        = NULL;
BOOLEAN *mDepexEvaluationStackEnd     = NULL;
BOOLEAN *mDepexEvaluationStackPointer = NULL;

//
// Worker functions
//


/**
  Grow size of the Depex stack

  @retval EFI_SUCCESS           Stack successfully growed.
  @retval EFI_OUT_OF_RESOURCES  There is not enough system memory to grow the stack.

**/
EFI_STATUS
GrowDepexStack (
  VOID
  )
{
  BOOLEAN     *NewStack;
  UINTN       Size;

  Size = DEPEX_STACK_SIZE_INCREMENT;
  if (mDepexEvaluationStack != NULL) {
    Size = Size + (mDepexEvaluationStackEnd - mDepexEvaluationStack);
  }

  NewStack = AllocatePool (Size * sizeof (BOOLEAN));
  if (NewStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mDepexEvaluationStack != NULL) {
    //
    // Copy to Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      mDepexEvaluationStack,
      (mDepexEvaluationStackEnd - mDepexEvaluationStack) * sizeof (BOOLEAN)
      );

    //
    // Free The Old Stack
    //
    FreePool (mDepexEvaluationStack);
  }

  //
  // Make the Stack pointer point to the old data in the new stack
  //
  mDepexEvaluationStackPointer = NewStack + (mDepexEvaluationStackPointer - mDepexEvaluationStack);
  mDepexEvaluationStack        = NewStack;
  mDepexEvaluationStackEnd     = NewStack + Size;

  return EFI_SUCCESS;
}



/**
  Push an element onto the Boolean Stack.

  @param  Value                 BOOLEAN to push.

  @retval EFI_SUCCESS           The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES  There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushBool (
  IN BOOLEAN  Value
  )
{
  EFI_STATUS  Status;

  //
  // Check for a stack overflow condition
  //
  if (mDepexEvaluationStackPointer == mDepexEvaluationStackEnd) {
    //
    // Grow the stack
    //
    Status = GrowDepexStack ();
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Push the item onto the stack
  //
  *mDepexEvaluationStackPointer = Value;
  mDepexEvaluationStackPointer++;

  return EFI_SUCCESS;
}



/**
  Pop an element from the Boolean stack.

  @param  Value                 BOOLEAN to pop.

  @retval EFI_SUCCESS           The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED     The pop operation underflowed the stack.

**/
EFI_STATUS
PopBool (
  OUT BOOLEAN  *Value
  )
{
  //
  // Check for a stack underflow condition
  //
  if (mDepexEvaluationStackPointer == mDepexEvaluationStack) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  mDepexEvaluationStackPointer--;
  *Value = *mDepexEvaluationStackPointer;
  return EFI_SUCCESS;
}



/**
  Preprocess dependency expression and update DriverEntry to reflect the
  state of  Before, After, and SOR dependencies. If DriverEntry->Before
  or DriverEntry->After is set it will never be cleared. If SOR is set
  it will be cleared by CoreSchedule(), and then the driver can be
  dispatched.

  @param  DriverEntry           DriverEntry element to update .

  @retval EFI_SUCCESS           It always works.

**/
EFI_STATUS
CorePreProcessDepex (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry
  )
{
  UINT8  *Iterator;

  Iterator = DriverEntry->Depex;
  if (*Iterator == EFI_DEP_SOR) {
    DriverEntry->Unrequested = TRUE;
  } else {
    DriverEntry->Dependent = TRUE;
  }

  if (*Iterator == EFI_DEP_BEFORE) {
    DriverEntry->Before = TRUE;
  } else if (*Iterator == EFI_DEP_AFTER) {
    DriverEntry->After = TRUE;
  }

  if (DriverEntry->Before || DriverEntry->After) {
    CopyMem (&DriverEntry->BeforeAfterGuid, Iterator + 1, sizeof (EFI_GUID));
  }

  return EFI_SUCCESS;
}



/**
  This is the POSTFIX version of the dependency evaluator.  This code does
  not need to handle Before or After, as it is not valid to call this
  routine in this case. The SOR is just ignored and is a nop in the grammer.
  POSTFIX means all the math is done on top of the stack.

  @param  DriverEntry           DriverEntry element to update.

  @retval TRUE                  If driver is ready to run.
  @retval FALSE                 If driver is not ready to run or some fatal error
                                was found.

**/
BOOLEAN
CoreIsSchedulable (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry
  )
{
  EFI_STATUS  Status;
  UINT8       *Iterator;
  BOOLEAN     Operator;
  BOOLEAN     Operator2;
  EFI_GUID    DriverGuid;
  VOID        *Interface;

  Operator = FALSE;
  Operator2 = FALSE;

  if (DriverEntry->After || DriverEntry->Before) {
    //
    // If Before or After Depex skip as CoreInsertOnScheduledQueueWhileProcessingBeforeAndAfter ()
    // processes them.
    //
    return FALSE;
  }

  DEBUG ((DEBUG_DISPATCH, "Evaluate DXE DEPEX for FFS(%g)\n", &DriverEntry->FileName));

  if (DriverEntry->Depex == NULL) {
    //
    // A NULL Depex means treat the driver like an UEFI 2.0 thing.
    //
    Status = CoreAllEfiServicesAvailable ();
    DEBUG ((DEBUG_DISPATCH, "  All UEFI Services Available                     = "));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_DISPATCH, "FALSE\n  RESULT = FALSE\n"));
      return FALSE;
    }
    DEBUG ((DEBUG_DISPATCH, "TRUE\n  RESULT = TRUE\n"));
    return TRUE;
  }

  //
  // Clean out memory leaks in Depex Boolean stack. Leaks are only caused by
  // incorrectly formed DEPEX expressions
  //
  mDepexEvaluationStackPointer = mDepexEvaluationStack;


  Iterator = DriverEntry->Depex;

  while (TRUE) {
    //
    // Check to see if we are attempting to fetch dependency expression instructions
    // past the end of the dependency expression.
    //
    if (((UINTN)Iterator - (UINTN)DriverEntry->Depex) >= DriverEntry->DepexSize) {
      DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Attempt to fetch past end of depex)\n"));
      return FALSE;
    }

    //
    // Look at the opcode of the dependency expression instruction.
    //
    switch (*Iterator) {
    case EFI_DEP_BEFORE:
    case EFI_DEP_AFTER:
      //
      // For a well-formed Dependency Expression, the code should never get here.
      // The BEFORE and AFTER are processed prior to this routine's invocation.
      // If the code flow arrives at this point, there was a BEFORE or AFTER
      // that were not the first opcodes.
      //
      DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected BEFORE or AFTER opcode)\n"));
      ASSERT (FALSE);
    case EFI_DEP_SOR:
      //
      // These opcodes can only appear once as the first opcode.  If it is found
      // at any other location, then the dependency expression evaluates to FALSE
      //
      if (Iterator != DriverEntry->Depex) {
        DEBUG ((DEBUG_DISPATCH, "  SOR\n"));
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected SOR opcode)\n"));
        return FALSE;
      }
      DEBUG ((DEBUG_DISPATCH, "  SOR                                             = Requested\n"));
      //
      // Otherwise, it is the first opcode and should be treated as a NOP.
      //
      break;

    case EFI_DEP_PUSH:
      //
      // Push operator is followed by a GUID. Test to see if the GUID protocol
      // is installed and push the boolean result on the stack.
      //
      CopyMem (&DriverGuid, Iterator + 1, sizeof (EFI_GUID));

      Status = CoreLocateProtocol (&DriverGuid, NULL, &Interface);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  PUSH GUID(%g) = FALSE\n", &DriverGuid));
        Status = PushBool (FALSE);
      } else {
        DEBUG ((DEBUG_DISPATCH, "  PUSH GUID(%g) = TRUE\n", &DriverGuid));
        *Iterator = EFI_DEP_REPLACE_TRUE;
        Status = PushBool (TRUE);
      }
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Iterator += sizeof (EFI_GUID);
      break;

    case EFI_DEP_AND:
      DEBUG ((DEBUG_DISPATCH, "  AND\n"));
      Status = PopBool (&Operator);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Status = PopBool (&Operator2);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Status = PushBool ((BOOLEAN)(Operator && Operator2));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      break;

    case EFI_DEP_OR:
      DEBUG ((DEBUG_DISPATCH, "  OR\n"));
      Status = PopBool (&Operator);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Status = PopBool (&Operator2);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Status = PushBool ((BOOLEAN)(Operator || Operator2));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      break;

    case EFI_DEP_NOT:
      DEBUG ((DEBUG_DISPATCH, "  NOT\n"));
      Status = PopBool (&Operator);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Status = PushBool ((BOOLEAN)(!Operator));
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      break;

    case EFI_DEP_TRUE:
      DEBUG ((DEBUG_DISPATCH, "  TRUE\n"));
      Status = PushBool (TRUE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      break;

    case EFI_DEP_FALSE:
      DEBUG ((DEBUG_DISPATCH, "  FALSE\n"));
      Status = PushBool (FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      break;

    case EFI_DEP_END:
      DEBUG ((DEBUG_DISPATCH, "  END\n"));
      Status = PopBool (&Operator);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }
      DEBUG ((DEBUG_DISPATCH, "  RESULT = %a\n", Operator ? "TRUE" : "FALSE"));
      return Operator;

    case EFI_DEP_REPLACE_TRUE:
      CopyMem (&DriverGuid, Iterator + 1, sizeof (EFI_GUID));
      DEBUG ((DEBUG_DISPATCH, "  PUSH GUID(%g) = TRUE\n", &DriverGuid));

      Status = PushBool (TRUE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unexpected error)\n"));
        return FALSE;
      }

      Iterator += sizeof (EFI_GUID);
      break;

    default:
      DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Unknown opcode)\n"));
      goto Done;
    }

    //
    // Skip over the Dependency Op Code we just processed in the switch.
    // The math is done out of order, but it should not matter. That is
    // we may add in the sizeof (EFI_GUID) before we account for the OP Code.
    // This is not an issue, since we just need the correct end result. You
    // need to be careful using Iterator in the loop as it's intermediate value
    // may be strange.
    //
    Iterator++;
  }

Done:
  return FALSE;
}


