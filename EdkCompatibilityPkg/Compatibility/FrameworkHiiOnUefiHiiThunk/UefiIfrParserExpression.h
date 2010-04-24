/** @file
  Internal Function and Macro defintions for IFR Expression evaluation used in Ifr Parsing. This header file should only
  be included by UefiIfrParserExpression.c and UefiIfrParser.c

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_PARSER_EXPRESSION_
#define _HII_THUNK_UEFI_IFR_PARSER_EXPRESSION_

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
  Zero extend integer/boolean/date/time to UINT64 for comparing.

  @param  Value                  HII Value to be converted.

  @return None.

**/
VOID
ExtendValueToU64 (
  IN  EFI_HII_VALUE   *Value
  );

/**
  Compare two Hii value.

  @param  Value1                 Expression value to compare on left-hand
  @param  Value2                 Expression value to compare on right-hand
  @param  HiiHandle              Only required for string compare

  @retval EFI_INVALID_PARAMETER  Could not perform comparation on two values
  @retval 0                      Two operators equeal
  @retval 0                      Value1 is greater than Value2
  @retval 0                      Value1 is less than Value2

**/
INTN
CompareHiiValue (
  IN  EFI_HII_VALUE   *Value1,
  IN  EFI_HII_VALUE   *Value2,
  IN  EFI_HII_HANDLE  HiiHandle OPTIONAL
  );

#endif
