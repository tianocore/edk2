/** @file
  Defines MemImage - the view of the file that is visible at any point,
  as well as the event handlers for editing the file

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_MEM_IMAGE_H_
#define _LIB_MEM_IMAGE_H_

#include "HexEditor.h"

/**
  Initialization function for HDiskImage.

  @retval EFI_SUCCESS       The operation was successful.
  @retval EFI_LOAD_ERROR    A load error occurred.
**/
EFI_STATUS
HMemImageInit (
  VOID
  );

/**
  Backup function for HDiskImage. Only a few fields need to be backup.
  This is for making the Disk buffer refresh as few as possible.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
HMemImageBackup (
  VOID
  );

/**
  Set FileName field in HFileImage.

  @param[in] Offset   The offset.
  @param[in] Size     The size.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageSetMemOffsetSize (
  IN UINTN  Offset,
  IN UINTN  Size
  );

/**
  Read a disk from disk into HBufferImage.

  @param[in] Offset   The offset.
  @param[in] Size     The size.
  @param[in] Recover  if is for recover, no information print.

  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageRead (
  IN UINTN    Offset,
  IN UINTN    Size,
  IN BOOLEAN  Recover
  );

/**
  Save lines in HBufferImage to disk.

  @param[in] Offset   The offset.
  @param[in] Size     The size.

  @retval EFI_LOAD_ERROR        A load error occurred.
  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
HMemImageSave (
  IN UINTN  Offset,
  IN UINTN  Size
  );

#endif
