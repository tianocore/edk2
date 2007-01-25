/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  dependency.c

Abstract:

  PEI Dispatcher Dependency Evaluator

  This routine evaluates a dependency expression (DEPENDENCY_EXPRESSION) to determine
  if a driver can be scheduled for execution.  The criteria for
  schedulability is that the dependency expression is satisfied.
  
--*/

#include <PeiMain.h>
#include "dependency.h"

STATIC
BOOLEAN
IsPpiInstalled (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN EVAL_STACK_ENTRY  *Stack
  )
/*++

Routine Description:

  This routine determines if a PPI has been installed.
  The truth value of a GUID is determined by if the PPI has
  been published and can be queried from the PPI database.

Arguments:
  PeiServices - The PEI core services table.
  Stack       - Reference to EVAL_STACK_ENTRY that contains PPI GUID to check

Returns:

  True if the PPI is already installed.
  False if the PPI has yet to be installed.

--*/
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
  // Copy the Guid into a locale variable so that there are no
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


EFI_STATUS
PeimDispatchReadiness (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN VOID               *DependencyExpression,
  OUT BOOLEAN           *Runnable
  )
/*++

Routine Description:

  This is the POSTFIX version of the dependency evaluator.  When a
  PUSH [PPI GUID] is encountered, a pointer to the GUID is stored on
  the evaluation stack.  When that entry is poped from the evaluation
  stack, the PPI is checked if it is installed.  This method allows
  some time savings as not all PPIs must be checked for certain
  operation types (AND, OR).

Arguments:

  PeiServices               - Calling context.

  DependencyExpression      - Pointer to a dependency expression.  The Grammar adheres to 
                              the BNF described above and is stored in postfix notation.
  Runnable                  - is True if the driver can be scheduled and False if the driver 
                              cannot be scheduled.  This is the value that the schedulers 
                              should use for deciding the state of the driver.

Returns:

  Status = EFI_SUCCESS            if it is a well-formed Grammar
           EFI_INVALID_PARAMETER  if the dependency expression overflows
                                  the evaluation stack
           EFI_INVALID_PARAMETER  if the dependency expression underflows
                                  the evaluation stack
           EFI_INVALID_PARAMETER  if the dependency expression is not a
                                  well-formed Grammar.
--*/
{
  DEPENDENCY_EXPRESSION_OPERAND  *Iterator;
  EVAL_STACK_ENTRY               *StackPtr;
  EVAL_STACK_ENTRY               EvalStack[MAX_GRAMMAR_SIZE];

  Iterator  = DependencyExpression;
  *Runnable = FALSE;

  StackPtr = &EvalStack[0];

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
          return EFI_INVALID_PARAMETER;
        }

        //
        // Push the pointer to the PUSH opcode operator (pointer to PPI GUID)
        // We will evaluate if the PPI is insalled on the POP operation.
        //
        StackPtr->Operator = (VOID *) Iterator;
        Iterator = Iterator + sizeof (EFI_GUID);
        StackPtr++;
        break;

      case (EFI_DEP_AND):    
      case (EFI_DEP_OR):     
        //
        // Check to make sure the dependency grammar doesn't underflow the
        // EvalStack on the two POPs for the AND operation.  Don't need to
        // check for the overflow on PUSHing the result since we already
        // did two POPs.
        //
        if (StackPtr < &EvalStack[2]) {
          return EFI_INVALID_PARAMETER;
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
        StackPtr--;
        //
        // Check to make sure EvalStack is balanced.  If not, then there is
        // an error in the dependency grammar, so return EFI_INVALID_PARAMETER.
        //
        if (StackPtr != &EvalStack[0]) {
          return EFI_INVALID_PARAMETER;
        }
        *Runnable = IsPpiInstalled (PeiServices, StackPtr);
        return EFI_SUCCESS;
        break;

      case (EFI_DEP_NOT):    
        //
        // Check to make sure the dependency grammar doesn't underflow the
        // EvalStack on the POP for the NOT operation.  Don't need to
        // check for the overflow on PUSHing the result since we already
        // did a POP.
        //
        if (StackPtr < &EvalStack[1]) {
          return EFI_INVALID_PARAMETER;
        }
        (StackPtr-1)->Result = (BOOLEAN) !IsPpiInstalled (PeiServices, (StackPtr-1));
        (StackPtr-1)->Operator = NULL;
        break;

      case (EFI_DEP_TRUE):
      case (EFI_DEP_FALSE):
        //
        // Check to make sure the dependency grammar doesn't overflow the
        // EvalStack on the push
        //
        if (StackPtr > &EvalStack[MAX_GRAMMAR_SIZE-1]) {
          return EFI_INVALID_PARAMETER;
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
        //
        // The grammar should never arrive here
        //
        return EFI_INVALID_PARAMETER;
        break;
    }
  }
}
