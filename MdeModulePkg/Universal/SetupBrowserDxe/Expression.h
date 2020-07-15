/** @file
Private structure, MACRO and function definitions for User Interface related functionalities.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EXPRESSION_H_
#define _EXPRESSION_H_

/**
  Get the expression list count.

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval >=0                    The expression count
  @retval -1                     Input parameter error.

**/
INTN
GetConditionalExpressionCount (
  IN EXPRESS_LEVEL       Level
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetCurrentExpressionStack (
  VOID
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetMapExpressionListStack (
  VOID
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetScopeStack (
  VOID
  );

/**
  Push an Operand onto the Stack

  @param  Operand                Operand to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PushScope (
  IN UINT8   Operand
  );

/**
  Get the expression Buffer pointer.

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval  The start pointer of the expression buffer or NULL.

**/
FORM_EXPRESSION **
GetConditionalExpressionList (
  IN EXPRESS_LEVEL       Level
  );

/**
  Pop an Operand from the Stack

  @param  Operand                Operand to pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PopScope (
  OUT UINT8     *Operand
  );

/**
  Push the list of map expression onto the Stack

  @param  Pointer                Pointer to the list of map expression to be pushed.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushMapExpressionList (
  IN VOID  *Pointer
  );

/**
  Push current expression onto the Stack

  @param  Pointer                Pointer to current expression.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushCurrentExpression (
  IN VOID  *Pointer
  );

/**
  Zero extend integer/boolean/date/time to UINT64 for comparing.

  @param  Value                  HII Value to be converted.

**/
VOID
ExtendValueToU64 (
  IN  EFI_HII_VALUE   *Value
  );

/**
  Push the expression options onto the Stack.

  @param  Pointer                Pointer to the current expression.
  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushConditionalExpression (
  IN FORM_EXPRESSION   *Pointer,
  IN EXPRESS_LEVEL     Level
  );

/**
  Pop the expression options from the Stack

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopConditionalExpression (
  IN  EXPRESS_LEVEL      Level
  );

/**
  Pop the list of map expression from the Stack

  @param  Pointer                Pointer to the list of map expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopMapExpressionList (
  OUT VOID    **Pointer
  );

/**
  Pop current expression from the Stack

  @param  Pointer                Pointer to current expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopCurrentExpression (
  OUT VOID    **Pointer
  );

/**
  Evaluate the result of a HII expression.

  If Expression is NULL, then ASSERT.

  @param  FormSet                FormSet associated with this expression.
  @param  Form                   Form associated with this expression.
  @param  Expression             Expression to be evaluated.

  @retval EFI_SUCCESS            The expression evaluated successfuly
  @retval EFI_NOT_FOUND          The Question which referenced by a QuestionId
                                 could not be found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack
  @retval EFI_INVALID_PARAMETER  Syntax error with the Expression

**/
EFI_STATUS
EvaluateExpression (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form,
  IN OUT FORM_EXPRESSION   *Expression
  );
/**
  Return the result of the expression list. Check the expression list and
  return the highest priority express result.
  Priority: DisableIf > SuppressIf > GrayOutIf > FALSE

  @param  ExpList             The input expression list.
  @param  Evaluate            Whether need to evaluate the expression first.
  @param  FormSet             FormSet associated with this expression.
  @param  Form                Form associated with this expression.

  @retval EXPRESS_RESULT      Return the higher priority express result.
                              DisableIf > SuppressIf > GrayOutIf > FALSE

**/
EXPRESS_RESULT
EvaluateExpressionList (
  IN FORM_EXPRESSION_LIST *ExpList,
  IN BOOLEAN              Evaluate,
  IN FORM_BROWSER_FORMSET *FormSet, OPTIONAL
  IN FORM_BROWSER_FORM    *Form OPTIONAL
  );

/**
  Get Form given its FormId.

  @param  FormSet                The formset which contains this form.
  @param  FormId                 Id of this form.

  @retval Pointer                The form.
  @retval NULL                   Specified Form is not found in the formset.

**/
FORM_BROWSER_FORM *
IdToForm (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN UINT16                FormId
  );

#endif // _EXPRESSION_H
