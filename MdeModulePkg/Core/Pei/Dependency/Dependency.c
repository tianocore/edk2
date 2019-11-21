/** @file
  PEI Dispatcher Dependency Evaluator

  This routine evaluates a dependency expression (DEPENDENCY_EXPRESSION) to determine
  if a driver can be scheduled for execution.  The criteria to be scheduled is
  that the dependency expression is satisfied.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"
#include "Dependency.h"

/**

  This routine determines if a PPI has been installed.
  The truth value of a GUID is determined by if the PPI has
  been published and can be queried from the PPI database.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param Stack           Reference to EVAL_STACK_ENTRY that contains PPI GUID to check

  @retval TRUE  if the PPI is already installed.
  @retval FALSE if the PPI has yet to be installed.

**/
BOOLEAN
IsPpiInstalled (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN EVAL_STACK_ENTRY  *Stack
  )
{
  VOID        *PeiInstance;
  EFI_STATUS  Status;
  EFI_GUID    PpiGuid;

  //
  // If there is no GUID to evaluate, just return current result on stack.
  //
  if (Stack->Operator == NULL) {
    return Stack->Result;
  }

  //
  // Copy the GUID into a local variable so that there are no
  // possibilities of alignment faults for cross-compilation
  // environments such as Intel?Itanium(TM).
  //
  CopyMem(&PpiGuid, Stack->Operator, sizeof(EFI_GUID));

  //
  // Check if the PPI is installed.
  //
  Status = PeiServicesLocatePpi(
             &PpiGuid,        // GUID
             0,               // INSTANCE
             NULL,            // EFI_PEI_PPI_DESCRIPTOR
             &PeiInstance     // PPI
             );

  if (EFI_ERROR(Status)) {
    return FALSE;
  }

  return TRUE;
}

/**

  This is the POSTFIX version of the dependency evaluator.  When a
  PUSH [PPI GUID] is encountered, a pointer to the GUID is stored on
  the evaluation stack.  When that entry is popped from the evaluation
  stack, the PPI is checked if it is installed.  This method allows
  some time savings as not all PPIs must be checked for certain
  operation types (AND, OR).


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param DependencyExpression   Pointer to a dependency expression.  The Grammar adheres to
                                the BNF described above and is stored in postfix notation.

  @retval TRUE      if it is a well-formed Grammar
  @retval FALSE     if the dependency expression overflows the evaluation stack
                    if the dependency expression underflows the evaluation stack
                    if the dependency expression is not a well-formed Grammar.

**/
BOOLEAN
PeimDispatchReadiness (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN VOID               *DependencyExpression
  )
{
  DEPENDENCY_EXPRESSION_OPERAND  *Iterator;
  EVAL_STACK_ENTRY               *StackPtr;
  EVAL_STACK_ENTRY               EvalStack[MAX_GRAMMAR_SIZE];

  Iterator  = DependencyExpression;

  StackPtr = EvalStack;

  while (TRUE) {

    switch (*(Iterator++)) {

      //
      // For performance reason we put the frequently used items in front of
      // the rarely used  items
      //

      case (EFI_DEP_PUSH):
        //
        // Check to make sure the dependency grammar doesn't overflow the
        // EvalStack on the push
        //
        if (StackPtr > &EvalStack[MAX_GRAMMAR_SIZE-1]) {
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Underflow Error)\n"));
          return FALSE;
        }

        //
        // Push the pointer to the PUSH opcode operator (pointer to PPI GUID)
        // We will evaluate if the PPI is installed on the POP operation.
        //
        StackPtr->Operator = (VOID *) Iterator;
        Iterator = Iterator + sizeof (EFI_GUID);
        DEBUG ((DEBUG_DISPATCH, "  PUSH GUID(%g) = %a\n", StackPtr->Operator, IsPpiInstalled (PeiServices, StackPtr) ? "TRUE" : "FALSE"));
        StackPtr++;
        break;

      case (EFI_DEP_AND):
      case (EFI_DEP_OR):
        if (*(Iterator - 1) == EFI_DEP_AND) {
          DEBUG ((DEBUG_DISPATCH, "  AND\n"));
        } else {
          DEBUG ((DEBUG_DISPATCH, "  OR\n"));
        }
        //
        // Check to make sure the dependency grammar doesn't underflow the
        // EvalStack on the two POPs for the AND operation.  Don't need to
        // check for the overflow on PUSHing the result since we already
        // did two POPs.
        //
        if (StackPtr < &EvalStack[2]) {
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Underflow Error)\n"));
          return FALSE;
        }

        //
        // Evaluate the first POPed operator only. If the operand is
        // EFI_DEP_AND and the POPed operator evaluates to FALSE, or the
        // operand is EFI_DEP_OR and the POPed operator evaluates to TRUE,
        // we don't need to check the second operator, and the result will be
        // evaluation of the POPed operator. Otherwise, don't POP the second
        // operator since it will now evaluate to the final result on the
        // next operand that causes a POP.
        //
        StackPtr--;
        //
        // Iterator has increased by 1 after we retrieve the operand, so here we
        // should get the value pointed by (Iterator - 1), in order to obtain the
        // same operand.
        //
        if (*(Iterator - 1) == EFI_DEP_AND) {
          if (!(IsPpiInstalled (PeiServices, StackPtr))) {
            (StackPtr-1)->Result = FALSE;
            (StackPtr-1)->Operator = NULL;
          }
        } else {
          if (IsPpiInstalled (PeiServices, StackPtr)) {
            (StackPtr-1)->Result = TRUE;
            (StackPtr-1)->Operator = NULL;
          }
        }
        break;

      case (EFI_DEP_END):
        DEBUG ((DEBUG_DISPATCH, "  END\n"));
        StackPtr--;
        //
        // Check to make sure EvalStack is balanced.  If not, then there is
        // an error in the dependency grammar, so return EFI_INVALID_PARAMETER.
        //
        if (StackPtr != &EvalStack[0]) {
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Underflow Error)\n"));
          return FALSE;
        }
        DEBUG ((DEBUG_DISPATCH, "  RESULT = %a\n", IsPpiInstalled (PeiServices, StackPtr) ? "TRUE" : "FALSE"));
        return IsPpiInstalled (PeiServices, StackPtr);

      case (EFI_DEP_NOT):
        DEBUG ((DEBUG_DISPATCH, "  NOT\n"));
        //
        // Check to make sure the dependency grammar doesn't underflow the
        // EvalStack on the POP for the NOT operation.  Don't need to
        // check for the overflow on PUSHing the result since we already
        // did a POP.
        //
        if (StackPtr < &EvalStack[1]) {
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Underflow Error)\n"));
          return FALSE;
        }
        (StackPtr-1)->Result = (BOOLEAN) !IsPpiInstalled (PeiServices, (StackPtr-1));
        (StackPtr-1)->Operator = NULL;
        break;

      case (EFI_DEP_TRUE):
      case (EFI_DEP_FALSE):
        if (*(Iterator - 1) == EFI_DEP_TRUE) {
          DEBUG ((DEBUG_DISPATCH, "  TRUE\n"));
        } else {
          DEBUG ((DEBUG_DISPATCH, "  FALSE\n"));
        }
        //
        // Check to make sure the dependency grammar doesn't overflow the
        // EvalStack on the push
        //
        if (StackPtr > &EvalStack[MAX_GRAMMAR_SIZE-1]) {
          DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Underflow Error)\n"));
          return FALSE;
        }
        //
        // Iterator has increased by 1 after we retrieve the operand, so here we
        // should get the value pointed by (Iterator - 1), in order to obtain the
        // same operand.
        //
        if (*(Iterator - 1) == EFI_DEP_TRUE) {
          StackPtr->Result = TRUE;
        } else {
          StackPtr->Result = FALSE;
        }
        StackPtr->Operator = NULL;
        StackPtr++;
        break;

      default:
        DEBUG ((DEBUG_DISPATCH, "  RESULT = FALSE (Invalid opcode)\n"));
        //
        // The grammar should never arrive here
        //
        return FALSE;
    }
  }
}
