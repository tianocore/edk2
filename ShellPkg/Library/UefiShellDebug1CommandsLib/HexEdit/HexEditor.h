/** @file
  Main include file for hex editor
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_SHELL_HEXEDIT_H_
#define _EFI_SHELL_HEXEDIT_H_

#include "UefiShellDebug1CommandsLib.h"
#include "HexEditorTypes.h"

#include "MainHexEditor.h"

#include "BufferImage.h"
#include "FileImage.h"
#include "DiskImage.h"
#include "MemImage.h"

#include "EditTitleBar.h"
#include "EditStatusBar.h"
#include "EditInputBar.h"
#include "EditMenuBar.h"

#include "Misc.h"

#include "Clipboard.h"

extern HEFI_EDITOR_GLOBAL_EDITOR  HMainEditor;
extern BOOLEAN                    HEditorFirst;
extern BOOLEAN                    HEditorExit;

#endif // _HEDITOR_H
