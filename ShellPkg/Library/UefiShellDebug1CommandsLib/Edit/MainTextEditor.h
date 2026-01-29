/** @file
  Declares editor interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_EDITOR_H_
#define _LIB_EDITOR_H_

#include "TextEditorTypes.h"

/**
  The initialization function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occurred.
**/
EFI_STATUS
MainEditorInit (
  VOID
  );

/**
  The cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occurred.
**/
EFI_STATUS
MainEditorCleanup (
  VOID
  );

/**
  Refresh the main editor component.
**/
VOID
MainEditorRefresh (
  VOID
  );

/**
  Handle user key input. This routes to other functions for the actions.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occurred.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
MainEditorKeyInput (
  VOID
  );

/**
  Backup function for MainEditor

  @retval EFI_SUCCESS The operation was successful.
**/
EFI_STATUS
MainEditorBackup (
  VOID
  );

#endif
