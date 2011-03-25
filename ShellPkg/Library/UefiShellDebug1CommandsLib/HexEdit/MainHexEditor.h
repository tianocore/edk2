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

EFI_STATUS
HMainEditorInit (
  VOID
  );
EFI_STATUS
HMainEditorCleanup (
  VOID
  );
EFI_STATUS
HMainEditorRefresh (
  VOID
  );
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
