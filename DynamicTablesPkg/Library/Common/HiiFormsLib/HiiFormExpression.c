/** @file
  Dynamic Hii Form Conditional Expressions API functions

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - UEFI specification 2.11, section 33.3.8 Forms Package

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiFormsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "InternalHiiFormsLib.h"

/** Check if this is a valid parent for the condition

  Check if the parent handle is valid for the condition
  being added. This primarily checks if the condition being
  used under the formset node is valid, as not all
  conditionals are allowed under the formset.

  @param [in] ParentHandle    Pointer to the parent.
  @param [in] ConditionType   Type of condition being added.

  @retval TRUE if parent is valid, FALSE otherwise.

**/
static
BOOLEAN
IsValidConditionalParent (
  IN DYN_HII_HANDLE          ParentHandle,
  IN DYN_HII_CONDITION_TYPE  ConditionType
  )
{
  DYN_HII_NODE_HDR  *Hdr;

  ASSERT (ParentHandle != NULL);

  Hdr = ParentHandle;
  switch (Hdr->Type) {
    case DynHiiNodeFormset:
      return ((ConditionType == DynHiiConditionSuppressIf) ||
              (ConditionType == DynHiiConditionDisableIf));

    case DynHiiNodeForm:
    case DynHiiNodeStatement:
    case DynHiiNodeCondition:
      return TRUE;

    default:
      return FALSE;
  }
}

/** Add a conditional node.

  Create a HII condition node and add it to the given parent.

  @param [in]  ConditionType    Type of conditional being added.
  @param [in]  ParentHandle     Pointer to the parent.
  @param [out] ConditionHandle  Condition handle for storing the
                                condition node pointer.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiAddConditional (
  IN  DYN_HII_CONDITION_TYPE  ConditionType,
  IN  DYN_HII_HANDLE          ParentHandle,
  OUT DYN_HII_HANDLE          *ConditionHandle
  )
{
  LIST_ENTRY         *ChildList;
  DYN_HII_NODE_HDR   *Hdr;
  DYN_HII_CONDITION  *NewCondition;

  ASSERT (ParentHandle != NULL);
  ASSERT (ConditionHandle != NULL);

  Hdr = ParentHandle;
  if (Hdr->FormsetHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NewCondition = AllocateZeroPool (sizeof (*NewCondition));
  if (NewCondition == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewCondition->ParentNode = ParentHandle;

  NewCondition->Hdr.Type          = DynHiiNodeCondition;
  NewCondition->Hdr.Scope         = TRUE;
  NewCondition->Hdr.FormsetHandle = Hdr->FormsetHandle;
  InitializeListHead (&NewCondition->Hdr.Link);

  NewCondition->ConditionType   = ConditionType;
  NewCondition->ExpressionCount = 0;

  InitializeListHead (&NewCondition->ExpressionList);
  InitializeListHead (&NewCondition->ChildList);

  ChildList = DynHiiGetNodeChildList (ParentHandle);
  if (ChildList == NULL) {
    FreePool (NewCondition);
    return EFI_INVALID_PARAMETER;
  }

  InsertTailList (ChildList, &NewCondition->Hdr.Link);
  *ConditionHandle = NewCondition;

  return EFI_SUCCESS;
}

/** Add a SuppressIf conditional expression.

  Create a scoped EFI_IFR_SUPPRESS_IF_OP node and add it under the given
  parent node. The parent can be a form, statement or another condition.

  @param [in]  ParentHandle     Parent handle.
  @param [out] ConditionHandle  Condition handle for storing the
                                condition node pointer.

  @retval  EFI_SUCCESS            The Condition was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddSuppressIf (
  IN  DYN_HII_HANDLE  ParentHandle,
  OUT DYN_HII_HANDLE  *ConditionHandle
  )
{
  if ((ParentHandle == NULL) || (ConditionHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidConditionalParent (ParentHandle, DynHiiConditionSuppressIf)) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddConditional (
           DynHiiConditionSuppressIf,
           ParentHandle,
           ConditionHandle
           );
}

/** Add a GrayOutIf conditional expression.

  Create a scoped EFI_IFR_GRAY_OUT_IF_OP node and add it under the given parent
  node. The parent can be a form, statement or another condition.

  @param [in]  ParentHandle     Parent handle.
  @param [out] ConditionHandle  Condition handle for storing the
                                condition node pointer.

  @retval  EFI_SUCCESS            The Condition was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddGrayOutIf (
  IN  DYN_HII_HANDLE  ParentHandle,
  OUT DYN_HII_HANDLE  *ConditionHandle
  )
{
  if ((ParentHandle == NULL) || (ConditionHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidConditionalParent (ParentHandle, DynHiiConditionGrayOutIf)) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddConditional (
           DynHiiConditionGrayOutIf,
           ParentHandle,
           ConditionHandle
           );
}

/** Add a DisableIf conditional expression.

  Create a scoped EFI_IFR_DISABLE_IF_OP node and add it under the given parent
  node. The parent can be a form, statement or another condition.

  @param [in]  ParentHandle     Parent handle.
  @param [out] ConditionHandle  Condition handle for storing the
                                condition node pointer.

  @retval  EFI_SUCCESS            The Condition was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddDisableIf (
  IN  DYN_HII_HANDLE  ParentHandle,
  OUT DYN_HII_HANDLE  *ConditionHandle
  )
{
  if ((ParentHandle == NULL) || (ConditionHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsValidConditionalParent (ParentHandle, DynHiiConditionDisableIf)) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddConditional (
           DynHiiConditionDisableIf,
           ParentHandle,
           ConditionHandle
           );
}

/** Add an expression opcode node to a Condition.

  Create an expression node and add it under the given condition. API-level
  parameter checks are done by the public expression addition functions.

  @param [in] Condition       Condition under which the expression is to be
                              added.
  @param [in] ExpressionType  Type of expression opcode to add.
  @param [in] ExpressionData  Optional expression payload. Required for
                              payload-carrying expression opcodes.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_UNSUPPORTED        The expression type is unsupported.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
EFIAPI
DynHiiAddExpression (
  IN DYN_HII_CONDITION        *Condition,
  IN DYN_HII_EXPRESSION_TYPE  ExpressionType,
  IN DYN_HII_EXPRESSION_DATA  *ExpressionData  OPTIONAL
  )
{
  UINT16              *ValueList;
  UINT16              ValueCount;
  DYN_HII_EXPRESSION  *NewExpression;
  DYN_HII_EXPRESSION  *FirstExpression;

  if ((Condition == NULL) || (Condition->Hdr.Type != DynHiiNodeCondition)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Condition->Hdr.FormsetHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (ExpressionType) {
    case DynHiiExprQuestionRef1:
    case DynHiiExprEqIdVal:
    case DynHiiExprEqIdValList:
    case DynHiiExprUint64:
      if (ExpressionData == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      break;

    case DynHiiExprEqual:
    case DynHiiExprGreaterThan:
    case DynHiiExprAnd:
    case DynHiiExprOr:
    case DynHiiExprNot:
    case DynHiiExprBitwiseAnd:
    case DynHiiExprTrue:
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  NewExpression = AllocateZeroPool (sizeof (*NewExpression));
  if (NewExpression == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewExpression->Hdr.Type          = DynHiiNodeExpression;
  NewExpression->Hdr.Scope         = FALSE;
  NewExpression->Hdr.FormsetHandle = Condition->Hdr.FormsetHandle;
  InitializeListHead (&NewExpression->Hdr.Link);

  NewExpression->ExpressionType = ExpressionType;

  switch (ExpressionType) {
    case DynHiiExprQuestionRef1:
      NewExpression->Data.QuestionRef1 = ExpressionData->QuestionRef1;
      break;

    case DynHiiExprEqIdVal:
      NewExpression->Data.EqIdVal = ExpressionData->EqIdVal;
      break;

    case DynHiiExprEqIdValList:
      ValueCount = ExpressionData->EqIdValList.ValueCount;
      ValueList  = AllocateCopyPool (
                     ValueCount * sizeof (UINT16),
                     ExpressionData->EqIdValList.ValueList
                     );
      if (ValueList == NULL) {
        FreePool (NewExpression);
        return EFI_OUT_OF_RESOURCES;
      }

      NewExpression->Data.EqIdValList.QuestionId =
        ExpressionData->EqIdValList.QuestionId;
      NewExpression->Data.EqIdValList.ValueCount = ValueCount;
      NewExpression->Data.EqIdValList.ValueList  = ValueList;
      break;

    case DynHiiExprUint64:
      NewExpression->Data.Uint64 = ExpressionData->Uint64;
      break;

    default:
      break;
  }

  if (Condition->ExpressionCount == 1) {
    ASSERT (!IsListEmpty (&Condition->ExpressionList));

    FirstExpression = BASE_CR (
                        GetFirstNode (&Condition->ExpressionList),
                        DYN_HII_EXPRESSION,
                        Hdr.Link
                        );
    FirstExpression->Hdr.Scope = TRUE;
  }

  InsertTailList (&Condition->ExpressionList, &NewExpression->Hdr.Link);
  Condition->ExpressionCount++;

  return EFI_SUCCESS;
}

/** Add a QuestionRef1 expression to a Condition.

  @param [in] ConditionHandle  Condition handle under which the expression is
                               to be added.
  @param [in] QuestionId       Question ID referenced by the expression.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddQuestionRef1Expr (
  IN DYN_HII_HANDLE   ConditionHandle,
  IN EFI_QUESTION_ID  QuestionId
  )
{
  DYN_HII_CONDITION        *Condition;
  DYN_HII_EXPRESSION_DATA  ExpressionData;

  if ((ConditionHandle == NULL) || (QuestionId == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ExpressionData, sizeof (ExpressionData));
  ExpressionData.QuestionRef1.QuestionId = QuestionId;

  return DynHiiAddExpression (
           Condition,
           DynHiiExprQuestionRef1,
           &ExpressionData
           );
}

/** Add an unsigned 64-bit value onto the expression stack.

  Add an unsigned 64-bit value that will be pushed on the expression
  stack. Corresponds to EFI_IFR_UIN64_OP

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.
  @param [in] Value             64-bit unsigned value.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddUint64ValueExpr (
  IN DYN_HII_HANDLE  ConditionHandle,
  IN UINT64          Value
  )
{
  DYN_HII_CONDITION        *Condition;
  DYN_HII_EXPRESSION_DATA  ExpressionData;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ExpressionData, sizeof (ExpressionData));
  ExpressionData.Uint64.Value = Value;

  return DynHiiAddExpression (
           Condition,
           DynHiiExprUint64,
           &ExpressionData
           );
}

/** Add an equality operator expression to a condition.

  Add an equality check expression operator to a condition. This pops
  two values from the stack, and returns TRUE if the values are equal
  and FALSE otherwise. Corresponds to EFI_IFR_EQUAL_OP.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddEqualExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprEqual, NULL);
}

/** Add an equal value check expression operator to a condition.

  Add an equal value check expression operator to a condition. This
  fetches the specified question's value, and compares it with the
  specified unsigned 16-bit value. Returns TRUE if the values are
  equal and FALSE otherwise. Corresponds to EFI_IFR_EQ_ID_VAL_OP.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.
  @param [in] QuestionId        Question ID whose value is to be fetched.
  @param [in] Value             16-bit value to be checked.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddEqualValExpr (
  IN DYN_HII_HANDLE   ConditionHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN UINT16           Value
  )
{
  DYN_HII_CONDITION        *Condition;
  DYN_HII_EXPRESSION_DATA  ExpressionData;

  if ((ConditionHandle == NULL) || (QuestionId == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ExpressionData, sizeof (ExpressionData));
  ExpressionData.EqIdVal.QuestionId = QuestionId;
  ExpressionData.EqIdVal.Value      = Value;

  return DynHiiAddExpression (
           Condition,
           DynHiiExprEqIdVal,
           &ExpressionData
           );
}

/** Add an equal val list check expression operator to a condition.

  This expression operator results in a checking the value passed to it
  in a list of unsigned integers. Returns TRUE if the value is found in
  the list, and FALSE otherwise. Corresponds to EFI_IFR_EQ_ID_VAL_LIST_OP.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.
  @param [in] QuestionId        Question ID whose value is to be fetched
                                and compared.
  @param [in] ListLength        Length of the list in which the question
                                is to be searched.
  @param [in] List              Pointer to the list of unsigned integers.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddEqualValListExpr (
  IN DYN_HII_HANDLE   ConditionHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN UINT16           ListLength,
  IN UINT16           *List
  )
{
  DYN_HII_CONDITION        *Condition;
  DYN_HII_EXPRESSION_DATA  ExpressionData;

  if ((ConditionHandle == NULL) || (QuestionId == 0) || (List == NULL) ||
      (ListLength == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ExpressionData, sizeof (ExpressionData));
  ExpressionData.EqIdValList.QuestionId = QuestionId;
  ExpressionData.EqIdValList.ValueCount = ListLength;
  ExpressionData.EqIdValList.ValueList  = List;

  return DynHiiAddExpression (
           Condition,
           DynHiiExprEqIdValList,
           &ExpressionData
           );
}

/** Add a bitwise AND operator expression to a Condition.

  Add a bitwise AND operator expression, which corresponds
  to EFI_IFR_BITWISE_AND_OP. This expression pops two values
  from the expression stack and performs a bitwise AND on them.
  Returns a TRUE if the expression evaluates as such, returns
  FALSE otherwise

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddBitwiseAndExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprBitwiseAnd, NULL);
}

/** Add a logical OR operator expression to a Condition.

  Add a logical OR operator expression, which corresponds
  to EFI_IFR_OR_OP. This expression pops two values
  from the expression stack and performs a logical OR on them.
  Returns a TRUE if the expression evaluates as such, returns
  FALSE otherwise

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOrExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprOr, NULL);
}

/** Add a logical AND operator expression to a Condition.

  Add a logical AND operator expression, which corresponds
  to EFI_IFR_AND_OP. This expression pops two values
  from the expression stack and performs a logical AND on
  them. Returns a TRUE if the expression evaluates as such,
  returns FALSE otherwise

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddAndExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprAnd, NULL);
}

/** Add a greater than operator expression to a Condition.

  Add a greater than operator expression, which corresponds
  to EFI_IFR_GREATER_THAN_OP. This expression pops two values
  from the expression stack and returns TRUE if the first value
  is greater than the second, and returns FALSE otherwise.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddGreaterThanExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprGreaterThan, NULL);
}

/** Push a TRUE onto the expression stack

  Push a TRUE onto the expression stack. Corresponds to EFI_IFR_TRUE_OP.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddTrueExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprTrue, NULL);
}

/** Do a NOT of the value popped from the expression stack.

  Pop a value from the expression stack, and push the logical NOT
  of the popped value. Corresponds to EFI_IFR_NOT_OP.

  @param [in] ConditionHandle   Condition handle under which the expression
                                is to be added.

  @retval  EFI_SUCCESS            The expression was added successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddNotExpr (
  IN DYN_HII_HANDLE  ConditionHandle
  )
{
  DYN_HII_CONDITION  *Condition;

  if (ConditionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Condition = ConditionHandle;
  if (Condition->Hdr.Type != DynHiiNodeCondition) {
    return EFI_INVALID_PARAMETER;
  }

  return DynHiiAddExpression (Condition, DynHiiExprNot, NULL);
}
