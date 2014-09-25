/** @file
  Declares editor types.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EDITOR_TYPE_H_
#define _EDITOR_TYPE_H_

#include "UefiShellDebug1CommandsLib.h"
#include "EditTitleBar.h"
#include "EditMenuBar.h"

#define MIN_POOL_SIZE         125
#define MAX_STRING_LENGTH     127

typedef struct {
  UINTN Row;
  UINTN Column;
} EFI_EDITOR_POSITION;

typedef
EFI_STATUS
(*EFI_MENU_ITEM_FUNCTION) (
  VOID
  );

typedef enum {
  NewLineTypeDefault,
  NewLineTypeLineFeed,
  NewLineTypeCarriageReturn,
  NewLineTypeCarriageReturnLineFeed,
  NewLineTypeLineFeedCarriageReturn,
  NewLineTypeUnknown
} EE_NEWLINE_TYPE;

#define LINE_LIST_SIGNATURE  SIGNATURE_32 ('e', 'e', 'l', 'l')
typedef struct _EFI_EDITOR_LINE {
  UINTN           Signature;
  CHAR16          *Buffer;
  UINTN           Size;                   // unit is Unicode
  UINTN           TotalSize;              // unit is Unicode, exclude CHAR_NULL
  EE_NEWLINE_TYPE Type;
  LIST_ENTRY      Link;
} EFI_EDITOR_LINE;

typedef struct {
  UINT32  Foreground : 4;
  UINT32  Background : 4;
} EFI_EDITOR_COLOR_ATTRIBUTES;

typedef union {
  EFI_EDITOR_COLOR_ATTRIBUTES Colors;
  UINTN                       Data;
} EFI_EDITOR_COLOR_UNION;

typedef struct {
  UINTN Columns;
  UINTN Rows;
} EFI_EDITOR_TEXT_MODE;

typedef struct {
  CHAR16                *FileName;        // file name current edited in editor
  EDIT_FILE_TYPE        FileType;         // Unicode file or ASCII file
  LIST_ENTRY            *ListHead;        // list head of lines
  EFI_EDITOR_LINE       *Lines;           // lines of current file
  UINTN                 NumLines;         // total line numbers
  EFI_EDITOR_POSITION   DisplayPosition;  // cursor position in screen
  EFI_EDITOR_POSITION   FilePosition;     // cursor position in file
  EFI_EDITOR_POSITION   MousePosition;    // mouse position in screen
  // file position of first byte displayed on screen
  //
  EFI_EDITOR_POSITION   LowVisibleRange;

  BOOLEAN               FileModified;     // file is modified or not
  BOOLEAN               ModeInsert;       // input mode INS or OVR
  BOOLEAN               ReadOnly;         // file is read-only or not
  EFI_EDITOR_LINE       *CurrentLine;     // current line cursor is at
} EFI_EDITOR_FILE_BUFFER;

typedef struct {
  EFI_EDITOR_FILE_BUFFER      *FileBuffer;

  EFI_EDITOR_COLOR_UNION      ColorAttributes;
  EFI_EDITOR_POSITION         ScreenSize; // row number and column number
  EFI_EDITOR_LINE             *CutLine;   // clip board
  BOOLEAN                     MouseSupported;
  EFI_SIMPLE_POINTER_PROTOCOL *MouseInterface;
  INT32                       MouseAccumulatorX;
  INT32                       MouseAccumulatorY;

} EFI_EDITOR_GLOBAL_EDITOR;

#endif
