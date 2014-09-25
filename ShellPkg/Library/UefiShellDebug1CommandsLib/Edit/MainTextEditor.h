/** @file
  Declares editor interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_EDITOR_H_
#define _LIB_EDITOR_H_

#include "TextEditorTypes.h"

/**
  The initialization function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
EFIAPI
MainEditorInit (
  VOID
  );

/**
  The cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
EFIAPI
MainEditorCleanup (
  VOID
  );

/**
  Refresh the main editor component.
**/
VOID
EFIAPI
MainEditorRefresh (
  VOID
  );

/**
  Handle user key input. This routes to other functions for the actions.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation failed.
**/
EFI_STATUS
EFIAPI
MainEditorKeyInput (
  VOID
  );

/**
  Backup function for MainEditor

  @retval EFI_SUCCESS The operation was successful.
**/
EFI_STATUS
EFIAPI
MainEditorBackup (
  VOID
  );

#endif
