/** @file
  Defines FileImage - the view of the file that is visible at any point,
  as well as the event handlers for editing the file

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_FILE_IMAGE_H_
#define _LIB_FILE_IMAGE_H_

#include "HexEditor.h"

/**
  Initialization function for HFileImage

  @retval EFI_SUCCESS     The operation was successful.
**/
EFI_STATUS
HFileImageInit (
  VOID
  );

/**
  Cleanup function for HFileImage.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
HFileImageCleanup (
  VOID
  );

/**
  Backup function for HFileImage. Only a few fields need to be backup.
  This is for making the file buffer refresh as few as possible.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HFileImageBackup (
  VOID
  );

/**
  Read a file from disk into HBufferImage.

  @param[in] FileName     filename to read.
  @param[in] Recover      if is for recover, no information print.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occurred.
**/
EFI_STATUS
HFileImageRead (
  IN CONST CHAR16  *FileName,
  IN BOOLEAN       Recover
  );

/**
  Save lines in HBufferImage to disk.

  @param[in] FileName     The file name.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occurred.
**/
EFI_STATUS
HFileImageSave (
  IN CHAR16  *FileName
  );

#endif
