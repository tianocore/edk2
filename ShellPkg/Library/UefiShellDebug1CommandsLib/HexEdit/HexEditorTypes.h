/** @file
  data types that are used by editor

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HEDITOR_TYPE_H_
#define _HEDITOR_TYPE_H_

#include "UefiShellDebug1CommandsLib.h"
#include "EditTitleBar.h"

#define EFI_EDITOR_LINE_LIST  SIGNATURE_32 ('e', 'e', 'l', 'l')

#define ASCII_POSITION        ((0x10 * 3) + 12)


typedef struct {
  UINTN Row;
  UINTN Column;
} HEFI_EDITOR_POSITION;

typedef
EFI_STATUS
(*HEFI_MENU_ITEM_FUNCTION) (
  VOID
  );

typedef struct {
  CHAR16                  Name[50];
  CHAR16                  Key[3];
  HEFI_MENU_ITEM_FUNCTION Function;
} HMENU_ITEMS;

typedef struct _HEFI_EDITOR_LINE {
  UINTN           Signature;
  UINT8           Buffer[0x10];
  UINTN           Size;                             // unit is Unicode
  LIST_ENTRY  Link;
} HEFI_EDITOR_LINE;

typedef struct _HEFI_EDITOR_MENU_ITEM {
  CHAR16                  NameToken;
  CHAR16                  FunctionKeyToken;
  HEFI_MENU_ITEM_FUNCTION Function;
} HEFI_EDITOR_MENU_ITEM;

typedef struct {
  UINT32  Foreground : 4;
  UINT32  Background : 4;
} HEFI_EDITOR_COLOR_ATTRIBUTES;

typedef union {
  HEFI_EDITOR_COLOR_ATTRIBUTES  Colors;
  UINTN                         Data;
} HEFI_EDITOR_COLOR_UNION;

typedef struct {
  UINTN Columns;
  UINTN Rows;
} HEFI_EDITOR_TEXT_MODE;


typedef struct {
  CHAR16  *Name;

  UINTN   BlockSize;
  UINTN   Size;
  UINTN   Offset;
} HEFI_EDITOR_DISK_IMAGE;

typedef struct {
  EFI_CPU_IO2_PROTOCOL *IoFncs;
  UINTN                Offset;
  UINTN                Size;
} HEFI_EDITOR_MEM_IMAGE;

typedef struct {
  CHAR16  *FileName;
  UINTN   Size;                                     // file size
  BOOLEAN ReadOnly;                                 // file is read-only or not
} HEFI_EDITOR_FILE_IMAGE;

typedef struct {
  LIST_ENTRY                      *ListHead;        // list head of lines
  HEFI_EDITOR_LINE                *Lines;           // lines of current file
  UINTN                           NumLines;         // number of lines
  HEFI_EDITOR_LINE                *CurrentLine;     // current line cursor is at
  HEFI_EDITOR_POSITION            DisplayPosition;  // cursor position in screen
  HEFI_EDITOR_POSITION            MousePosition;    // mouse position in screen
  HEFI_EDITOR_POSITION            BufferPosition;   // cursor position in buffer
  UINTN                           LowVisibleRow;    // the lowest visible row of file position
  BOOLEAN                         HighBits;         // cursor is at the high4 bits or low4 bits
  BOOLEAN                         Modified;         // BUFFER is modified or not
  EDIT_FILE_TYPE                  BufferType;

  HEFI_EDITOR_FILE_IMAGE          *FileImage;
  HEFI_EDITOR_DISK_IMAGE          *DiskImage;
  HEFI_EDITOR_MEM_IMAGE           *MemImage;

} HEFI_EDITOR_BUFFER_IMAGE;

typedef struct {
  HEFI_EDITOR_BUFFER_IMAGE          *BufferImage;

  HEFI_EDITOR_COLOR_UNION           ColorAttributes;
  HEFI_EDITOR_POSITION              ScreenSize;           // row number and column number
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *TextInputEx;
  BOOLEAN                           MouseSupported;
  EFI_SIMPLE_POINTER_PROTOCOL       *MouseInterface;
  INT32                             MouseAccumulatorX;
  INT32                             MouseAccumulatorY;

  UINTN                             SelectStart;          // starting from 1
  UINTN                             SelectEnd;            // starting from 1
} HEFI_EDITOR_GLOBAL_EDITOR;

#endif
