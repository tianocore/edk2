/** @file
  Main include file for hex editor

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
