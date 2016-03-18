/** @file
    Defines the Main Editor data type - 
     - Global variables 
     - Instances of the other objects of the editor
     - Main Interfaces
  
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

#include "HexEditor.h"

/**
  Init function for MainEditor

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainEditorInit (
  VOID
  );

/**
  Cleanup function for MainEditor.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_LOAD_ERROR          A load error occured.
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
  @retval EFI_OUT_OF_RESOURCES    A memory allocation occured.
  @retval EFI_LOAD_ERROR          A load error occured.
**/
EFI_STATUS
HMainEditorKeyInput (
  VOID
  );

/**
  Backup function for MainEditor.
**/
VOID
EFIAPI
HMainEditorBackup (
  VOID
  );

#endif
