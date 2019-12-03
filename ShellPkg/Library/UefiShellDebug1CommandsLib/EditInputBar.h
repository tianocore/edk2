/** @file
  Declares imputbar interface functions.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_INPUT_BAR_H_
#define _LIB_INPUT_BAR_H_

/**
  Initialize the input bar.

  @param[in] TextInEx  Pointer to SimpleTextInEx instance in System Table.
**/
VOID
InputBarInit (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *TextInEx
  );

/**
  Cleanup function for input bar.
**/
VOID
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
InputBarSetStringSize (
  UINTN   Size
  );

/**
  Function to retrieve the input from the user.

  @retval NULL                  No input has been received.
  @return The string that was input.
**/
CONST CHAR16*
InputBarGetString (
  VOID
  );

#endif
