/** @file
  Declares titlebar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_TITLE_BAR_H_
#define _LIB_TITLE_BAR_H_

/**
  Initialize a title bar.

  @param[in] Prompt             The prompt to print in the title bar.

  @retval EFI_SUCCESS           The initialization was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
MainTitleBarInit (
  CONST CHAR16 *Prompt
  );

/**
  Clean up the memory used.
**/
VOID
EFIAPI
MainTitleBarCleanup (
  VOID
  );

typedef enum {
  FileTypeNone,
  FileTypeAscii,
  FileTypeUnicode,
  FileTypeDiskBuffer,
  FileTypeMemBuffer,
  FileTypeFileBuffer
} EDIT_FILE_TYPE;

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
EFIAPI
MainTitleBarRefresh (
  IN CONST CHAR16                 *FileName OPTIONAL,
  IN CONST EDIT_FILE_TYPE         FileType,
  IN CONST BOOLEAN                ReadOnly,
  IN CONST BOOLEAN                Modified,
  IN CONST UINTN                  LastCol,
  IN CONST UINTN                  LastRow,
  IN CONST UINTN                  Offset,
  IN CONST UINTN                  Size
  );

#endif
