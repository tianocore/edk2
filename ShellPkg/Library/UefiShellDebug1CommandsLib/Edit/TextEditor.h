/** @file
  Main include file for Edit shell Debug1 function.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_EDIT_H_
#define _EFI_EDIT_H_

#include "TextEditorTypes.h"

#include "MainTextEditor.h"
#include "FileBuffer.h"
#include "EditTitleBar.h"
#include "EditStatusBar.h"
#include "EditInputBar.h"
#include "EditMenuBar.h"
#include "Misc.h"

extern EFI_EDITOR_GLOBAL_EDITOR MainEditor;
extern BOOLEAN                  EditorFirst;
extern BOOLEAN                  EditorExit;

#endif // _EFI_EDIT_H_
