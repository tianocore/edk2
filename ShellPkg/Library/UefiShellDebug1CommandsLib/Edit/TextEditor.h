/** @file
  Main include file for Edit shell Debug1 function.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
