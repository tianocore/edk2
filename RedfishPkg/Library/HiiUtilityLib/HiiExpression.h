/** @file
  Definitions of Hii Expression.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_EXPRESSION_H_
#define HII_EXPRESSION_H_

#include <Library/HiiUtilityLib.h>

/**
  Get the expression list count.

  @param[in]  Level              Which type this expression belong to. Form,
                                 statement or option?

  @retval >=0                    The expression count
  @retval -1                     Input parameter error.

**/
INTN
GetConditionalExpressionCount (
  IN EXPRESS_LEVEL  Level
  );

/**
  Get the expression Buffer pointer.

  @param[in]  Level              Which type this expression belong to. Form,
                                 statement or option?

  @retval  The start pointer of the expression buffer or NULL.

**/
HII_EXPRESSION **
GetConditionalExpressionList (
  IN EXPRESS_LEVEL  Level
  );

/**
  Push the expression options onto the Stack.

  @param[in]  Pointer            Pointer to the current expression.
  @param[in]  Level              Which type this expression belong to. Form,
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushConditionalExpression (
  IN HII_EXPRESSION  *Pointer,
  IN EXPRESS_LEVEL   Level
  );

/**
  Pop the expression options from the Stack

  @param[in]  Level              Which type this expression belong to. Form,
                                 statement or option?

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopConditionalExpression (
  IN EXPRESS_LEVEL  Level
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetCurrentExpressionStack (
  VOID
  );

/**
  Push current expression onto the Stack

  @param[in]  Pointer            Pointer to current expression.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushCurrentExpression (
  IN VOID  *Pointer
  );

/**
  Pop current expression from the Stack

  @param[in]  Pointer            Pointer to current expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopCurrentExpression (
  OUT VOID  **Pointer
  );

/**
  Reset stack pointer to begin of the stack.

**/
VOID
ResetMapExpressionListStack (
  VOID
  );

/**
  Push the list of map expression onto the Stack

  @param[in]  Pointer            Pointer to the list of map expression to be pushed.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PushMapExpressionList (
  IN VOID  *Pointer
  );

/**
  Pop the list of map expression from the Stack

  @param[in]  Pointer            Pointer to the list of map expression to be pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the stack.

**/
EFI_STATUS
PopMapExpressionList (
  OUT VOID  **Pointer
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

  @param[in]  Operand            Operand to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PushScope (
  IN UINT8  Operand
  );

/**
  Pop an Operand from the Stack

  @param[out]  Operand           Operand to pop.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.

**/
EFI_STATUS
PopScope (
  OUT UINT8  *Operand
  );

#endif
