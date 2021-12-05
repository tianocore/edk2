/** @file
  Implements titlebar interface functions.

  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EditTitleBar.h"
#include "UefiShellDebug1CommandsLib.h"

CHAR16  *Title = NULL;

/**
  Initialize a title bar.

  @param[in] Prompt             The prompt to print in the title bar.

  @retval EFI_SUCCESS           The initialization was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
MainTitleBarInit (
  CONST CHAR16  *Prompt
  )
{
  SHELL_FREE_NON_NULL (Title);
  if (Prompt == NULL) {
    Title = CatSPrint (NULL, L"");
  } else {
    //
    // set Title
    //
    Title = CatSPrint (NULL, L"%s", Prompt);
  }

  if (Title == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Clean up the memory used.
**/
VOID
MainTitleBarCleanup (
  VOID
  )
{
  SHELL_FREE_NON_NULL (Title);
  Title = NULL;
}

typedef struct {
  UINT32    Foreground : 4;
  UINT32    Background : 4;
} TITLE_BAR_COLOR_ATTRIBUTES;

typedef union {
  TITLE_BAR_COLOR_ATTRIBUTES    Colors;
  UINTN                         Data;
} TITLE_BAR_COLOR_UNION;

/**
  Refresh function for MainTitleBar

  @param[in] FileName           The open file's name (or NULL).
  @param[in] FileType           The type fo the file.
  @param[in] ReadOnly           TRUE if the file is read only.  FALSE otherwise.
  @param[in] Modified           TRUE if the file was modified.  FALSE otherwise.
  @param[in] LastCol            The last printable column.
  @param[in] LastRow            The last printable row.
  @param[in] Offset             The offset into the file. (only for mem/disk)
  @param[in] Size               The file's size. (only for mem/disk)

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
MainTitleBarRefresh (
  IN CONST CHAR16          *FileName OPTIONAL,
  IN CONST EDIT_FILE_TYPE  FileType,
  IN CONST BOOLEAN         ReadOnly,
  IN CONST BOOLEAN         Modified,
  IN CONST UINTN           LastCol,
  IN CONST UINTN           LastRow,
  IN CONST UINTN           Offset,
  IN CONST UINTN           Size
  )
{
  TITLE_BAR_COLOR_UNION  Orig;
  TITLE_BAR_COLOR_UNION  New;
  CONST CHAR16           *FileNameTmp;
  INTN                   TempInteger;

  //
  // backup the old screen attributes
  //
  Orig.Data             = gST->ConOut->Mode->Attribute;
  New.Data              = 0;
  New.Colors.Foreground = Orig.Colors.Background & 0xF;
  New.Colors.Background = Orig.Colors.Foreground & 0x7;

  gST->ConOut->SetAttribute (gST->ConOut, New.Data & 0x7F);

  //
  // clear the title line
  //
  EditorClearLine (1, LastCol, LastRow);

  if (Title != NULL) {
    //
    // print the new title bar prefix
    //
    ShellPrintEx (
      0,
      0,
      L"%s ",
      Title
      );
  }

  if (FileName == NULL) {
    gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
    return EFI_SUCCESS;
  }

  //
  // First Extract the FileName from fullpath
  //
  FileNameTmp = FileName;
  for (TempInteger = StrLen (FileNameTmp) - 1; TempInteger >= 0; TempInteger--) {
    if (FileNameTmp[TempInteger] == L'\\') {
      break;
    }
  }

  FileNameTmp = FileNameTmp + TempInteger + 1;

  //
  // the space for file name is 20 characters
  //
  if (StrLen (FileNameTmp) <= 20) {
    ShellPrintEx (-1, -1, L"%s   ", FileNameTmp);
    for (TempInteger = StrLen (FileNameTmp); TempInteger < 20; TempInteger++) {
      ShellPrintEx (-1, -1, L" ");
    }
  } else {
    for (TempInteger = 0; TempInteger < 17; TempInteger++) {
      ShellPrintEx (-1, -1, L"%c", FileNameTmp[TempInteger]);
    }

    //
    // print "..."
    //
    ShellPrintEx (-1, -1, L"...   ");
  }

  //
  // print file type field
  //
  switch (FileType) {
    case FileTypeAscii:
    case FileTypeUnicode:
      if (FileType == FileTypeAscii) {
        ShellPrintEx (-1, -1, L"     ASCII     ");
      } else {
        ShellPrintEx (-1, -1, L"     UNICODE   ");
      }

      //
      // print read-only field for text files
      //
      if (ReadOnly) {
        ShellPrintEx (-1, -1, L"ReadOnly   ");
      } else {
        ShellPrintEx (-1, -1, L"           ");
      }

      break;
    case FileTypeDiskBuffer:
    case FileTypeMemBuffer:
      //
      // Print the offset.
      //
      ShellPrintEx (-1, -1, L"Offset %X | Size %X", Offset, Size);
    case FileTypeFileBuffer:
      break;
    default:
      break;
  }

  //
  // print modified field
  //
  if (Modified) {
    ShellPrintEx (-1, -1, L"Modified");
  }

  //
  // restore the old attribute
  //
  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);

  return EFI_SUCCESS;
}
