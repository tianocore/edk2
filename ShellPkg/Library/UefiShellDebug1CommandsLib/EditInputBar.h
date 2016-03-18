/** @file
  Declares imputbar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_INPUT_BAR_H_
#define _LIB_INPUT_BAR_H_

/**
  Initialize the input bar.
**/
VOID
EFIAPI
InputBarInit (
  VOID
  );

/**
  Cleanup function for input bar.
**/
VOID
EFIAPI
InputBarCleanup (
  VOID
  );

/**
  The refresh function for InputBar, it will wait for user input

  @param[in] LastRow            The last printable row.
  @param[in] LastColumn         The last printable column.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
InputBarRefresh (
  UINTN LastRow,
  UINTN LastColumn
  );

/**
  SetPrompt and wait for input.

  @param[in] Str                The prompt string.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
InputBarSetPrompt (
  IN CONST CHAR16 *Str
  );

/**
  Set the size of the string in characters.

  @param[in] Size               The max number of characters to accept.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
InputBarSetStringSize (
  UINTN   Size
  );

/**
  Function to retrieve the input from the user.

  @retval NULL                  No input has been received.
  @return The string that was input.
**/
CONST CHAR16*
EFIAPI
InputBarGetString (
  VOID
  );

#endif
