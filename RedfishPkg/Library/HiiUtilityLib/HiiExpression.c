/** @file
  The implementation of HII expression.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "HiiInternal.h"

//
// Global stack used to evaluate boolean expressions
//
EFI_HII_VALUE  *mOpCodeScopeStack        = NULL;
EFI_HII_VALUE  *mOpCodeScopeStackEnd     = NULL;
EFI_HII_VALUE  *mOpCodeScopeStackPointer = NULL;

//
// Stack used for Suppressif/grayoutif/disableif expression list.
//
HII_EXPRESSION  **mFormExpressionStack   = NULL;
HII_EXPRESSION  **mFormExpressionEnd     = NULL;
HII_EXPRESSION  **mFormExpressionPointer = NULL;

HII_EXPRESSION  **mStatementExpressionStack   = NULL;
HII_EXPRESSION  **mStatementExpressionEnd     = NULL;
HII_EXPRESSION  **mStatementExpressionPointer = NULL;

HII_EXPRESSION  **mOptionExpressionStack   = NULL;
HII_EXPRESSION  **mOptionExpressionEnd     = NULL;
HII_EXPRESSION  **mOptionExpressionPointer = NULL;

//
// Stack used for the sub expresion in map expression.
//
EFI_HII_VALUE  *mCurrentExpressionStack   = NULL;
EFI_HII_VALUE  *mCurrentExpressionEnd     = NULL;
EFI_HII_VALUE  *mCurrentExpressionPointer = NULL;

//
// Stack used for the map expression list.
//
EFI_HII_VALUE  *mMapExpressionListStack   = NULL;
EFI_HII_VALUE  *mMapExpressionListEnd     = NULL;
EFI_HII_VALUE  *mMapExpressionListPointer = NULL;

//
// Stack used for dependency expression.
//
HII_DEPENDENCY_EXPRESSION  **mExpressionDependencyStack   = NULL;
HII_DEPENDENCY_EXPRESSION  **mExpressionDependencyEnd     = NULL;
HII_DEPENDENCY_EXPRESSION  **mExpressionDependencyPointer = NULL;

/**
  Grow size of the stack.

  This is an internal function.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end

  @retval EFI_SUCCESS            Grow stack success.
  @retval EFI_OUT_OF_RESOURCES   No enough memory for stack space.

**/
EFI_STATUS
GrowStack (
  IN OUT EFI_HII_VALUE  **Stack,
  IN OUT EFI_HII_VALUE  **StackPtr,
  IN OUT EFI_HII_VALUE  **StackEnd
  )
{
  UINTN          Size;
  EFI_HII_VALUE  *NewStack;

  Size = EXPRESSION_STACK_SIZE_INCREMENT;
  if (*StackPtr != NULL) {
    Size = Size + (*StackEnd - *Stack);
  }

  NewStack = AllocatePool (Size * sizeof (EFI_HII_VALUE));
  if (NewStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (*StackPtr != NULL) {
    //
    // Copy from Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      *Stack,
      (*StackEnd - *Stack) * sizeof (EFI_HII_VALUE)
      );

    //
    // Free The Old Stack
    //
    FreePool (*Stack);
  }

  //
  // Make the Stack pointer point to the old data in the new stack
  //
  *StackPtr = NewStack + (*StackPtr - *Stack);
  *Stack    = NewStack;
  *StackEnd = NewStack + Size;

  return EFI_SUCCESS;
}

/**
  Push an element onto the Boolean Stack.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end
  @param  Data                   Data to push.

  @retval EFI_SUCCESS            Push stack success.

**/
EFI_STATUS
PushStack (
  IN OUT EFI_HII_VALUE  **Stack,
  IN OUT EFI_HII_VALUE  **StackPtr,
  IN OUT EFI_HII_VALUE  **StackEnd,
  IN     EFI_HII_VALUE  *Data
  )
{
  EFI_STATUS  Status;

  //
  // Check for a stack overflow condition
  //
  if (*StackPtr >= *StackEnd) {
    //
    // Grow the stack
    //
    Status = GrowStack (Stack, StackPtr, StackEnd);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Push the item onto the stack
  //
  CopyMem (*StackPtr, Data, sizeof (EFI_HII_VALUE));
  if (Data->Type == EFI_IFR_TYPE_BUFFER) {
    (*StackPtr)->Buffer = AllocateCopyPool (Data->BufferLen, Data->Buffer);
    if ((*StackPtr)->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  *StackPtr = *StackPtr + 1;

  return EFI_SUCCESS;
}

/**
  Pop an element from the stack.

  @param  Stack                  On input: old stack
  @param  StackPtr               On input: old stack pointer; On output: new stack pointer
  @param  Data                   Data to pop.

  @retval EFI_SUCCESS            The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack

**/
EFI_STATUS
PopStack (
  IN     EFI_HII_VALUE  *Stack,
  IN OUT EFI_HII_VALUE  **StackPtr,
  OUT EFI_HII_VALUE     *Data
  )
{
  //
  // Check for a stack underflow condition
  //
  if (*StackPtr == Stack) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  *StackPtr = *StackPtr - 1;
  CopyMem (Data, *StackPtr, sizeof (EFI_HII_VALUE));
  return EFI_SUCCESS;
}

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetCurrentExpressionStack (
  VOID
  )
{
  mCurrentExpressionPointer   = mCurrentExpressionStack;
  mFormExpressionPointer      = mFormExpressionStack;
  mStatementExpressionPointer = mStatementExpressionStack;
  mOptionExpressionPointer    = mOptionExpressionStack;
}

/**
  Push current expression onto the Stack

  @param  Pointer                Pointer to current expression.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushCurrentExpression (
  IN VOID  *Pointer
  )
{
  EFI_HII_VALUE  Data;

  Data.Type      = EFI_IFR_TYPE_NUM_SIZE_64;
  Data.Value.u64 = (UINT64)(UINTN)Pointer;

  return PushStack (
           &mCurrentExpressionStack,
           &mCurrentExpressionPointer,
           &mCurrentExpressionEnd,
           &Data
           );
}

/**
  Pop current expression from the Stack

  @param  Pointer                Pointer to current expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopCurrentExpression (
  OUT VOID  **Pointer
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Data;

  Status = PopStack (
             mCurrentExpressionStack,
             &mCurrentExpressionPointer,
             &Data
             );

  *Pointer = (VOID *)(UINTN)Data.Value.u64;

  return Status;
}

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetMapExpressionListStack (
  VOID
  )
{
  mMapExpressionListPointer = mMapExpressionListStack;
}

/**
  Grow size of the stack.

  This is an internal function.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end
  @param  MemberSize             The stack member size.

  @retval EFI_SUCCESS            Grow stack success.
  @retval EFI_OUT_OF_RESOURCES   No enough memory for stack space.

**/
EFI_STATUS
GrowConditionalStack (
  IN OUT HII_EXPRESSION  ***Stack,
  IN OUT HII_EXPRESSION  ***StackPtr,
  IN OUT HII_EXPRESSION  ***StackEnd,
  IN     UINTN           MemberSize
  )
{
  UINTN           Size;
  HII_EXPRESSION  **NewStack;

  Size = EXPRESSION_STACK_SIZE_INCREMENT;
  if (*StackPtr != NULL) {
    Size = Size + (*StackEnd - *Stack);
  }

  NewStack = AllocatePool (Size * MemberSize);
  if (NewStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (*StackPtr != NULL) {
    //
    // Copy from Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      *Stack,
      (*StackEnd - *Stack) * MemberSize
      );

    //
    // Free The Old Stack
    //
    FreePool (*Stack);
  }

  //
  // Make the Stack pointer point to the old data in the new stack
  //
  *StackPtr = NewStack + (*StackPtr - *Stack);
  *Stack    = NewStack;
  *StackEnd = NewStack + Size;

  return EFI_SUCCESS;
}

/**
  Push an element onto the Stack.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end
  @param  Data                   Data to push.

  @retval EFI_SUCCESS            Push stack success.

**/
EFI_STATUS
PushConditionalStack (
  IN OUT HII_EXPRESSION  ***Stack,
  IN OUT HII_EXPRESSION  ***StackPtr,
  IN OUT HII_EXPRESSION  ***StackEnd,
  IN     HII_EXPRESSION  **Data
  )
{
  EFI_STATUS  Status;

  //
  // Check for a stack overflow condition
  //
  if (*StackPtr >= *StackEnd) {
    //
    // Grow the stack
    //
    Status = GrowConditionalStack (Stack, StackPtr, StackEnd, sizeof (HII_EXPRESSION *));
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Push the item onto the stack
  //
  CopyMem (*StackPtr, Data, sizeof (HII_EXPRESSION *));
  *StackPtr = *StackPtr + 1;

  return EFI_SUCCESS;
}

/**
  Pop an element from the stack.

  @param  Stack                  On input: old stack
  @param  StackPtr               On input: old stack pointer; On output: new stack pointer
  @param  Data                   Data to pop.

  @retval EFI_SUCCESS            The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack

**/
EFI_STATUS
PopConditionalStack (
  IN     HII_EXPRESSION  **Stack,
  IN OUT HII_EXPRESSION  ***StackPtr,
  OUT HII_EXPRESSION     **Data
  )
{
  //
  // Check for a stack underflow condition
  //
  if (*StackPtr == Stack) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  *StackPtr = *StackPtr - 1;
  CopyMem (Data, *StackPtr, sizeof (HII_EXPRESSION  *));
  return EFI_SUCCESS;
}

/**
  Get the expression list count.

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval >=0                    The expression count
  @retval -1                     Input parameter error.

**/
INTN
GetConditionalExpressionCount (
  IN EXPRESS_LEVEL  Level
  )
{
  switch (Level) {
    case ExpressForm:
      return mFormExpressionPointer - mFormExpressionStack;
    case ExpressStatement:
      return mStatementExpressionPointer - mStatementExpressionStack;
    case ExpressOption:
      return mOptionExpressionPointer - mOptionExpressionStack;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/**
  Get the expression Buffer pointer.

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval  The start pointer of the expression buffer or NULL.

**/
HII_EXPRESSION **
GetConditionalExpressionList (
  IN EXPRESS_LEVEL  Level
  )
{
  switch (Level) {
    case ExpressForm:
      return mFormExpressionStack;
    case ExpressStatement:
      return mStatementExpressionStack;
    case ExpressOption:
      return mOptionExpressionStack;
    default:
      ASSERT (FALSE);
      return NULL;
  }
}

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
  IN HII_EXPRESSION  *Pointer,
  IN EXPRESS_LEVEL   Level
  )
{
  switch (Level) {
    case ExpressForm:
      return PushConditionalStack (
               &mFormExpressionStack,
               &mFormExpressionPointer,
               &mFormExpressionEnd,
               &Pointer
               );
    case ExpressStatement:
      return PushConditionalStack (
               &mStatementExpressionStack,
               &mStatementExpressionPointer,
               &mStatementExpressionEnd,
               &Pointer
               );
    case ExpressOption:
      return PushConditionalStack (
               &mOptionExpressionStack,
               &mOptionExpressionPointer,
               &mOptionExpressionEnd,
               &Pointer
               );
    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }
}

/**
  Pop the expression options from the Stack

  @param  Level                  Which type this expression belong to. Form,
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopConditionalExpression (
  IN EXPRESS_LEVEL  Level
  )
{
  HII_EXPRESSION  *Pointer;

  switch (Level) {
    case ExpressForm:
      return PopConditionalStack (
               mFormExpressionStack,
               &mFormExpressionPointer,
               &Pointer
               );

    case ExpressStatement:
      return PopConditionalStack (
               mStatementExpressionStack,
               &mStatementExpressionPointer,
               &Pointer
               );

    case ExpressOption:
      return PopConditionalStack (
               mOptionExpressionStack,
               &mOptionExpressionPointer,
               &Pointer
               );

    default:
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }
}

/**
  Push the list of map expression onto the Stack

  @param  Pointer                Pointer to the list of map expression to be pushed.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushMapExpressionList (
  IN VOID  *Pointer
  )
{
  EFI_HII_VALUE  Data;

  Data.Type      = EFI_IFR_TYPE_NUM_SIZE_64;
  Data.Value.u64 = (UINT64)(UINTN)Pointer;

  return PushStack (
           &mMapExpressionListStack,
           &mMapExpressionListPointer,
           &mMapExpressionListEnd,
           &Data
           );
}

/**
  Pop the list of map expression from the Stack

  @param  Pointer                Pointer to the list of map expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopMapExpressionList (
  OUT VOID  **Pointer
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Data;

  Status = PopStack (
             mMapExpressionListStack,
             &mMapExpressionListPointer,
             &Data
             );

  *Pointer = (VOID *)(UINTN)Data.Value.u64;

  return Status;
}

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetScopeStack (
  VOID
  )
{
  mOpCodeScopeStackPointer = mOpCodeScopeStack;
}

/**
  Push an Operand onto the Stack

  @param  Operand                Operand to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PushScope (
  IN UINT8  Operand
  )
{
  EFI_HII_VALUE  Data;

  Data.Type     = EFI_IFR_TYPE_NUM_SIZE_8;
  Data.Value.u8 = Operand;

  return PushStack (
           &mOpCodeScopeStack,
           &mOpCodeScopeStackPointer,
           &mOpCodeScopeStackEnd,
           &Data
           );
}

/**
  Pop an Operand from the Stack

  @param  Operand                Operand to pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PopScope (
  OUT UINT8  *Operand
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Data;

  Status = PopStack (
             mOpCodeScopeStack,
             &mOpCodeScopeStackPointer,
             &Data
             );

  *Operand = Data.Value.u8;

  return Status;
}

/**
  Grow size of the stack for Expression Dependencies.

  This is an internal function.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end

  @retval EFI_SUCCESS            Grow Dependency stack success.
  @retval EFI_OUT_OF_RESOURCES   No enough memory for stack space.

**/
EFI_STATUS
GrowDependencyStack (
  IN OUT HII_DEPENDENCY_EXPRESSION  ***Stack,
  IN OUT HII_DEPENDENCY_EXPRESSION  ***StackPtr,
  IN OUT HII_DEPENDENCY_EXPRESSION  ***StackEnd
  )
{
  UINTN                      Size;
  HII_DEPENDENCY_EXPRESSION  **NewStack;

  Size = EXPRESSION_STACK_SIZE_INCREMENT;
  if (*StackPtr != NULL) {
    Size = Size + (*StackEnd - *Stack);
  }

  NewStack = AllocatePool (Size * sizeof (HII_DEPENDENCY_EXPRESSION *));
  if (NewStack == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (*StackPtr != NULL) {
    //
    // Copy from Old Stack to the New Stack
    //
    CopyMem (
      NewStack,
      *Stack,
      (*StackEnd - *Stack) * sizeof (HII_DEPENDENCY_EXPRESSION *)
      );

    //
    // Free The Old Stack
    //
    FreePool (*Stack);
  }

  //
  // Make the Stack pointer point to the old data in the new stack
  //
  *StackPtr = NewStack + (*StackPtr - *Stack);
  *Stack    = NewStack;
  *StackEnd = NewStack + Size;

  return EFI_SUCCESS;
}

/**
  Push an element onto the Stack for Expression Dependencies.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end
  @param  Data                   Data to push.

  @retval EFI_SUCCESS            Push stack success.

**/
EFI_STATUS
PushDependencyStack (
  IN OUT HII_DEPENDENCY_EXPRESSION  ***Stack,
  IN OUT HII_DEPENDENCY_EXPRESSION  ***StackPtr,
  IN OUT HII_DEPENDENCY_EXPRESSION  ***StackEnd,
  IN     HII_DEPENDENCY_EXPRESSION  **Data
  )
{
  EFI_STATUS  Status;

  //
  // Check for a stack overflow condition
  //
  if (*StackPtr >= *StackEnd) {
    //
    // Grow the stack
    //
    Status = GrowDependencyStack (Stack, StackPtr, StackEnd);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Push the item onto the stack
  //
  CopyMem (*StackPtr, Data, sizeof (HII_DEPENDENCY_EXPRESSION *));
  *StackPtr = *StackPtr + 1;

  return EFI_SUCCESS;
}

/**
  Pop the Expression Dependency options from the Stack

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  Data                   Data to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopDependencyStack (
  IN     HII_DEPENDENCY_EXPRESSION  **Stack,
  IN OUT HII_DEPENDENCY_EXPRESSION  ***StackPtr,
  OUT HII_DEPENDENCY_EXPRESSION     **Data
  )
{
  //
  // Check for a stack underflow condition
  //
  if (*StackPtr == Stack) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Pop the item off the stack
  //
  *StackPtr = *StackPtr - 1;
  CopyMem (Data, *StackPtr, sizeof (HII_DEPENDENCY_EXPRESSION *));
  return EFI_SUCCESS;
}

/**
  Push the list of Expression Dependencies onto the Stack

  @param  Pointer                Pointer to the list of map expression to be pushed.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushDependencyExpDes (
  IN HII_DEPENDENCY_EXPRESSION  **Pointer
  )
{
  return PushDependencyStack (
           &mExpressionDependencyStack,
           &mExpressionDependencyPointer,
           &mExpressionDependencyEnd,
           Pointer
           );
}

/**
  Pop the list of Expression Dependencies from the Stack

  @param  Pointer                Pointer to the list of map expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopDependencyExpDes (
  OUT HII_DEPENDENCY_EXPRESSION  **Pointer
  )
{
  return PopDependencyStack (
           mExpressionDependencyStack,
           &mExpressionDependencyPointer,
           Pointer
           );
}

/**
  Retrieve dependencies within an expression. These dependencies can express how
  this expression will be evaluated.

  @param[in,out]  Expression     Expression to retrieve dependencies.

  @retval EFI_SUCCESS            The dependencies were successfully retrieved.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory.

**/
EFI_STATUS
GetHiiExpressionDependency (
  IN OUT HII_EXPRESSION  *Expression
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *Link;
  HII_EXPRESSION_OPCODE      *ExpressionOpCode;
  HII_DEPENDENCY_EXPRESSION  *DepExpressionOpCode;
  LIST_ENTRY                 *SubExpressionLink;
  HII_EXPRESSION             *SubExpression;
  UINT8                      MapPairCount;

  Link = GetFirstNode (&Expression->OpCodeListHead);
  while (!IsNull (&Expression->OpCodeListHead, Link)) {
    ExpressionOpCode = HII_EXPRESSION_OPCODE_FROM_LINK (Link);
    Link             = GetNextNode (&Expression->OpCodeListHead, Link);
    Status           = EFI_SUCCESS;

    DepExpressionOpCode = (HII_DEPENDENCY_EXPRESSION *)AllocateZeroPool (sizeof (HII_DEPENDENCY_EXPRESSION));
    if (DepExpressionOpCode == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    switch (ExpressionOpCode->Operand) {
      //
      // Constant
      //
      case EFI_IFR_FALSE_OP:
      case EFI_IFR_ONE_OP:
      case EFI_IFR_ONES_OP:
      case EFI_IFR_TRUE_OP:
      case EFI_IFR_UINT8_OP:
      case EFI_IFR_UINT16_OP:
      case EFI_IFR_UINT32_OP:
      case EFI_IFR_UINT64_OP:
      case EFI_IFR_UNDEFINED_OP:
      case EFI_IFR_VERSION_OP:
      case EFI_IFR_ZERO_OP:
        DepExpressionOpCode->ConstantExp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->ConstantExp.Value, &ExpressionOpCode->ExtraData.Value, sizeof (EFI_HII_VALUE));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      //
      // Built-in functions
      //
      case EFI_IFR_DUP_OP:
        DepExpressionOpCode->DupExp.Operand = ExpressionOpCode->Operand;
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_EQ_ID_VAL_OP:
        DepExpressionOpCode->EqIdValExp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->EqIdValExp.QuestionId, &ExpressionOpCode->ExtraData.EqIdValData.QuestionId, sizeof (EFI_QUESTION_ID));
        CopyMem (&DepExpressionOpCode->EqIdValExp.Value, &ExpressionOpCode->ExtraData.EqIdValData.Value, sizeof (EFI_HII_VALUE));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        DepExpressionOpCode->EqIdIdExp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->EqIdIdExp.QuestionId1, &ExpressionOpCode->ExtraData.EqIdIdData.QuestionId1, sizeof (EFI_QUESTION_ID));
        CopyMem (&DepExpressionOpCode->EqIdIdExp.QuestionId2, &ExpressionOpCode->ExtraData.EqIdIdData.QuestionId2, sizeof (EFI_QUESTION_ID));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_EQ_ID_VAL_LIST_OP:
        DepExpressionOpCode->EqIdListExp.Operand    = ExpressionOpCode->Operand;
        DepExpressionOpCode->EqIdListExp.ListLength = ExpressionOpCode->ExtraData.EqIdListData.ListLength;
        CopyMem (&DepExpressionOpCode->EqIdListExp.QuestionId, &ExpressionOpCode->ExtraData.EqIdListData.QuestionId, sizeof (EFI_QUESTION_ID));
        PushDependencyExpDes (&DepExpressionOpCode);
        DepExpressionOpCode->EqIdListExp.ValueList = AllocateCopyPool (
                                                       DepExpressionOpCode->EqIdListExp.ListLength * sizeof (UINT16),
                                                       ExpressionOpCode->ExtraData.EqIdListData.ValueList
                                                       );
        if (DepExpressionOpCode->EqIdListExp.ValueList == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_GET_OP:
        DepExpressionOpCode->GetExp.Operand    = ExpressionOpCode->Operand;
        DepExpressionOpCode->GetExp.VarStorage = ExpressionOpCode->ExtraData.GetSetData.VarStorage;
        CopyMem (&DepExpressionOpCode->GetExp.VarStoreInfo.VarName, &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarName, sizeof (EFI_STRING_ID));
        CopyMem (&DepExpressionOpCode->GetExp.VarStoreInfo.VarOffset, &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset, sizeof (UINT16));
        DepExpressionOpCode->GetExp.VarStoreInfo = ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo;
        if (ExpressionOpCode->ExtraData.GetSetData.ValueName != NULL) {
          DepExpressionOpCode->GetExp.ValueName = (CHAR16 *)AllocateCopyPool (sizeof (ExpressionOpCode->ExtraData.GetSetData.ValueName), ExpressionOpCode->ExtraData.GetSetData.ValueName);
          if (DepExpressionOpCode->GetExp.ValueName == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        }

        DepExpressionOpCode->GetExp.ValueType  = ExpressionOpCode->ExtraData.GetSetData.ValueType;
        DepExpressionOpCode->GetExp.ValueWidth = ExpressionOpCode->ExtraData.GetSetData.ValueWidth;
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_QUESTION_REF1_OP:
        DepExpressionOpCode->QuestionRef1Exp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->QuestionRef1Exp.QuestionId, &ExpressionOpCode->ExtraData.QuestionRef1Data.QuestionId, sizeof (EFI_QUESTION_ID));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_RULE_REF_OP:
        DepExpressionOpCode->RuleRefExp.Operand = ExpressionOpCode->Operand;
        DepExpressionOpCode->RuleRefExp.RuleId  = ExpressionOpCode->ExtraData.RuleId;
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_STRING_REF1_OP:
        DepExpressionOpCode->StringRef1Exp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->StringRef1Exp.Value.Value.string, &ExpressionOpCode->ExtraData.Value.Value.string, sizeof (EFI_STRING_ID));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_THIS_OP:
        DepExpressionOpCode->ThisExp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->ThisExp.QuestionId, &ExpressionOpCode->ExtraData.QuestionRef1Data.QuestionId, sizeof (EFI_QUESTION_ID));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SECURITY_OP:
        DepExpressionOpCode->SecurityExp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->SecurityExp.Permissions, &ExpressionOpCode->ExtraData.Guid, sizeof (EFI_GUID));
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      //
      // unary-op
      //
      case EFI_IFR_LENGTH_OP:
        DepExpressionOpCode->LengthExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->LengthExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_NOT_OP:
        DepExpressionOpCode->NotExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->NotExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_BITWISE_NOT_OP:
        DepExpressionOpCode->BitWiseNotExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->BitWiseNotExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_QUESTION_REF2_OP:
        DepExpressionOpCode->QuestionRef2Exp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->QuestionRef2Exp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_QUESTION_REF3_OP:
        DepExpressionOpCode->QuestionRef3Exp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->QuestionRef3Exp.DevicePath, &ExpressionOpCode->ExtraData.QuestionRef3Data.DevicePath, sizeof (EFI_DEVICE_PATH));
        CopyMem (&DepExpressionOpCode->QuestionRef3Exp.Guid, &ExpressionOpCode->ExtraData.QuestionRef3Data.Guid, sizeof (EFI_GUID));
        PopDependencyExpDes (&DepExpressionOpCode->QuestionRef3Exp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SET_OP:
        DepExpressionOpCode->SetExp.Operand    = ExpressionOpCode->Operand;
        DepExpressionOpCode->SetExp.VarStorage = ExpressionOpCode->ExtraData.GetSetData.VarStorage;
        CopyMem (&DepExpressionOpCode->SetExp.VarStoreInfo.VarName, &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarName, sizeof (EFI_STRING_ID));
        CopyMem (&DepExpressionOpCode->SetExp.VarStoreInfo.VarOffset, &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset, sizeof (UINT16));
        DepExpressionOpCode->SetExp.VarStoreInfo = ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo;
        if (ExpressionOpCode->ExtraData.GetSetData.ValueName != NULL) {
          DepExpressionOpCode->SetExp.ValueName = (CHAR16 *)AllocateCopyPool (sizeof (ExpressionOpCode->ExtraData.GetSetData.ValueName), ExpressionOpCode->ExtraData.GetSetData.ValueName);
          if (DepExpressionOpCode->SetExp.ValueName == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        }

        DepExpressionOpCode->SetExp.ValueType  = ExpressionOpCode->ExtraData.GetSetData.ValueType;
        DepExpressionOpCode->SetExp.ValueWidth = ExpressionOpCode->ExtraData.GetSetData.ValueWidth;
        PopDependencyExpDes (&DepExpressionOpCode->SetExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_STRING_REF2_OP:
        DepExpressionOpCode->StringRef2Exp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->StringRef2Exp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TO_BOOLEAN_OP:
        DepExpressionOpCode->ToBooleanExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ToBooleanExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TO_STRING_OP:
        DepExpressionOpCode->ToStringExp.Operand = ExpressionOpCode->Operand;
        DepExpressionOpCode->ToStringExp.Format  = ExpressionOpCode->ExtraData.Format;
        PopDependencyExpDes (&DepExpressionOpCode->ToStringExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TO_UINT_OP:
        DepExpressionOpCode->ToUintExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ToUintExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TO_LOWER_OP:
        DepExpressionOpCode->ToLowerExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ToLowerExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TO_UPPER_OP:
        DepExpressionOpCode->ToUpperExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ToUpperExp.SubExpression);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      //
      // binary-op
      //
      case EFI_IFR_ADD_OP:
        DepExpressionOpCode->AddExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->AddExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->AddExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_AND_OP:
        DepExpressionOpCode->AndExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->AndExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->AndExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_BITWISE_AND_OP:
        DepExpressionOpCode->BitwiseAndExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->BitwiseAndExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->BitwiseAndExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_BITWISE_OR_OP:
        DepExpressionOpCode->BitwiseOrExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->BitwiseOrExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->BitwiseOrExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_CATENATE_OP:
        DepExpressionOpCode->CatenateExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->CatenateExp.LeftStringExp);
        PopDependencyExpDes (&DepExpressionOpCode->CatenateExp.RightStringExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_DIVIDE_OP:
        DepExpressionOpCode->DivExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->DivExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->DivExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_EQUAL_OP:
        DepExpressionOpCode->EqualExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->EqualExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->EqualExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_GREATER_EQUAL_OP:
        DepExpressionOpCode->GreaterEqualExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->GreaterEqualExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->GreaterEqualExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_GREATER_THAN_OP:
        DepExpressionOpCode->GreaterThanExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->GreaterThanExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->GreaterThanExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_LESS_EQUAL_OP:
        DepExpressionOpCode->LessEqualExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->LessEqualExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->LessEqualExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_LESS_THAN_OP:
        DepExpressionOpCode->LessThanExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->LessThanExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->LessThanExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_MATCH_OP:
        DepExpressionOpCode->MatchExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->MatchExp.PatternExp);
        PopDependencyExpDes (&DepExpressionOpCode->MatchExp.StringExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_MATCH2_OP:
        DepExpressionOpCode->Match2Exp.Operand = ExpressionOpCode->Operand;
        CopyMem (&DepExpressionOpCode->Match2Exp.SyntaxType, &ExpressionOpCode->ExtraData.Guid, sizeof (EFI_GUID));
        PopDependencyExpDes (&DepExpressionOpCode->Match2Exp.PatternExp);
        PopDependencyExpDes (&DepExpressionOpCode->Match2Exp.StringExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_MODULO_OP:
        DepExpressionOpCode->ModExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ModExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->ModExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_MULTIPLY_OP:
        DepExpressionOpCode->MultExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->MultExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->MultExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_NOT_EQUAL_OP:
        DepExpressionOpCode->NotEqualExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->NotEqualExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->NotEqualExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_OR_OP:
        DepExpressionOpCode->OrExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->OrExp.SubExpression1);
        PopDependencyExpDes (&DepExpressionOpCode->OrExp.SubExpression2);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SHIFT_LEFT_OP:
        DepExpressionOpCode->ShiftLeftExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ShiftLeftExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->ShiftLeftExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SHIFT_RIGHT_OP:
        DepExpressionOpCode->ShiftRightExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ShiftRightExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->ShiftRightExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SUBTRACT_OP:
        DepExpressionOpCode->SubtractExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->SubtractExp.LeftHandExp);
        PopDependencyExpDes (&DepExpressionOpCode->SubtractExp.RightHandExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      //
      // ternary-op
      //
      case EFI_IFR_CONDITIONAL_OP:
        DepExpressionOpCode->ConditionalExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->ConditionalExp.CondTrueValExp);
        PopDependencyExpDes (&DepExpressionOpCode->ConditionalExp.CondFalseValExp);
        PopDependencyExpDes (&DepExpressionOpCode->ConditionalExp.ConditionExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_FIND_OP:
        DepExpressionOpCode->FindExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->FindExp.StringToSearchExp);
        PopDependencyExpDes (&DepExpressionOpCode->FindExp.StringToCompWithExp);
        PopDependencyExpDes (&DepExpressionOpCode->FindExp.IndexExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_MID_OP:
        DepExpressionOpCode->MidExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->MidExp.StringOrBufferExp);
        PopDependencyExpDes (&DepExpressionOpCode->MidExp.IndexExp);
        PopDependencyExpDes (&DepExpressionOpCode->MidExp.LengthExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_TOKEN_OP:
        DepExpressionOpCode->TokenExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->TokenExp.StringToSearchExp);
        PopDependencyExpDes (&DepExpressionOpCode->TokenExp.DelimiterExp);
        PopDependencyExpDes (&DepExpressionOpCode->TokenExp.IndexExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      case EFI_IFR_SPAN_OP:
        DepExpressionOpCode->SpanExp.Operand = ExpressionOpCode->Operand;
        PopDependencyExpDes (&DepExpressionOpCode->SpanExp.StringToSearchExp);
        PopDependencyExpDes (&DepExpressionOpCode->SpanExp.CharsetExp);
        PopDependencyExpDes (&DepExpressionOpCode->SpanExp.IndexExp);
        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      //
      // Map
      //
      case EFI_IFR_MAP_OP:
        //
        // Go through map expression list.
        //
        DepExpressionOpCode->MapExp.Operand = ExpressionOpCode->Operand;
        MapPairCount                        = 0;
        SubExpressionLink                   = GetFirstNode (&ExpressionOpCode->MapExpressionList);
        while (!IsNull (&ExpressionOpCode->MapExpressionList, SubExpressionLink)) {
          MapPairCount++;
          SubExpressionLink = GetNextNode (&ExpressionOpCode->MapExpressionList, SubExpressionLink);
          if (IsNull (&ExpressionOpCode->MapExpressionList, SubExpressionLink)) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }

          //
          // Goto the first expression on next pair.
          //
          SubExpressionLink = GetNextNode (&ExpressionOpCode->MapExpressionList, SubExpressionLink);
        }

        DepExpressionOpCode->MapExp.ExpPair = (HII_DEPENDENCY_EXPRESSION_PAIR *)AllocateZeroPool (
                                                                                  MapPairCount * sizeof (HII_DEPENDENCY_EXPRESSION_PAIR)
                                                                                  );
        if (DepExpressionOpCode->MapExp.ExpPair == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }

        DepExpressionOpCode->MapExp.ExpPairNo = MapPairCount;
        MapPairCount                          = 0;
        PopDependencyExpDes (&DepExpressionOpCode->MapExp.SubExp);

        //
        // Go through map expression list.
        //
        SubExpressionLink = GetFirstNode (&ExpressionOpCode->MapExpressionList);
        while (!IsNull (&ExpressionOpCode->MapExpressionList, SubExpressionLink)) {
          SubExpression = HII_EXPRESSION_FROM_LINK (SubExpressionLink);
          //
          // Get the first expression description in this pair.
          //
          GetHiiExpressionDependency (SubExpression);
          DepExpressionOpCode->MapExp.ExpPair[MapPairCount].MatchExp = SubExpression->RootDependencyExp;

          //
          // Get the second expression description in this pair.
          //
          SubExpressionLink = GetNextNode (&ExpressionOpCode->MapExpressionList, SubExpressionLink);
          SubExpression     = HII_EXPRESSION_FROM_LINK (SubExpressionLink);
          GetHiiExpressionDependency (SubExpression);
          DepExpressionOpCode->MapExp.ExpPair[MapPairCount].ReturnExp = SubExpression->RootDependencyExp;
          //
          // Goto the first expression on next pair.
          //
          SubExpressionLink = GetNextNode (&ExpressionOpCode->MapExpressionList, SubExpressionLink);
          MapPairCount++;
        }

        PushDependencyExpDes (&DepExpressionOpCode);
        break;

      default:
        break;
    }
  }

  PopDependencyExpDes (&Expression->RootDependencyExp);

Done:
  return Status;
}

/**
  Return the result of the expression list. Check the expression list and
  return the highest priority express result.
  Priority: DisableIf > SuppressIf > GrayOutIf > FALSE

  @param[in]  ExpList         The input expression list.
  @param[in]  Evaluate        Whether need to evaluate the expression first.
  @param[in]  FormSet         FormSet associated with this expression.
  @param[in]  Form            Form associated with this expression.

  @retval EXPRESS_RESULT      Return the higher priority express result.
                              DisableIf > SuppressIf > GrayOutIf > FALSE

**/
EXPRESS_RESULT
EvaluateExpressionList (
  IN HII_EXPRESSION_LIST  *ExpList,
  IN BOOLEAN              Evaluate,
  IN HII_FORMSET          *FormSet  OPTIONAL,
  IN HII_FORM             *Form OPTIONAL
  )
{
  UINTN           Index;
  EXPRESS_RESULT  ReturnVal;
  EXPRESS_RESULT  CompareOne;
  EFI_STATUS      Status;

  if (ExpList == NULL) {
    return ExpressFalse;
  }

  ASSERT (ExpList->Signature == HII_EXPRESSION_LIST_SIGNATURE);
  Index = 0;

  //
  // Check whether need to evaluate the expression first.
  //
  if (Evaluate) {
    while (ExpList->Count > Index) {
      Status = EvaluateHiiExpression (FormSet, Form, ExpList->Expression[Index++]);
      if (EFI_ERROR (Status)) {
        return ExpressFalse;
      }
    }
  }

  //
  // Run the list of expressions.
  //
  ReturnVal = ExpressFalse;
  for (Index = 0; Index < ExpList->Count; Index++) {
    if (IsHiiValueTrue (&ExpList->Expression[Index]->Result)) {
      switch (ExpList->Expression[Index]->Type) {
        case EFI_HII_EXPRESSION_SUPPRESS_IF:
          CompareOne = ExpressSuppress;
          break;

        case EFI_HII_EXPRESSION_GRAY_OUT_IF:
          CompareOne = ExpressGrayOut;
          break;

        case EFI_HII_EXPRESSION_DISABLE_IF:
          CompareOne = ExpressDisable;
          break;

        default:
          return ExpressFalse;
      }

      ReturnVal = ReturnVal < CompareOne ? CompareOne : ReturnVal;
    }
  }

  return ReturnVal;
}
