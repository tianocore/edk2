/** @file
    Defines the Main Editor data type -
     - Global variables
     - Instances of the other objects of the editor
     - Main Interfaces

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_EDITOR_H_
#define _LIB_EDITOR_H_

#include "HexEditor.h"

/**
  Init function for MainEditor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occurred.
**/
EFI_STATUS
HMainEditorInit (
  VOID
  );

/**
  Cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occurred.
**/
EFI_STATUS
HMainEditorCleanup (
  VOID
  );

/**
  Refresh function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
**/
EFI_STATUS
HMainEditorRefresh (
  VOID
  );

/**
  Handle user key input. will route it to other components handle function.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occurred.
  @retval EFI_LOAD_ERROR          A load error occurred.
**/
EFI_STATUS
HMainEditorKeyInput (
  VOID
  );

/**
  Backup function for MainEditor.
**/
VOID
HMainEditorBackup (
  VOID
  );

#endif
