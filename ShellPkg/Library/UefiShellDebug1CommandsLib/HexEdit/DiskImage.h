/** @file
  Defines DiskImage - the view of the file that is visible at any point, 
  as well as the event handlers for editing the file
  
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_DISK_IMAGE_H_
#define _LIB_DISK_IMAGE_H_

#include "HexEditor.h"

/**
  Initialization function for HDiskImage.
 
  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_LOAD_ERROR  A load error occured.
**/
EFI_STATUS
HDiskImageInit (
  VOID
  );

/**
  Cleanup function for HDiskImage.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
HDiskImageCleanup (
  VOID
  );

/**
  Backup function for HDiskImage. Only a few fields need to be backup.   
  This is for making the Disk buffer refresh as few as possible.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  gST->ConOut of resources.
**/
EFI_STATUS
HDiskImageBackup (
  VOID
  );

/**
  Read a disk from disk into HBufferImage.

  @param[in] DeviceName   filename to read.
  @param[in] Offset       The offset.
  @param[in] Size         The size.
  @param[in] Recover      if is for recover, no information print.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occured.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.  
**/
EFI_STATUS
HDiskImageRead (
  IN CONST CHAR16   *DeviceName,
  IN UINTN    Offset,
  IN UINTN    Size,
  IN BOOLEAN  Recover
  );

/**
  Save lines in HBufferImage to disk.
  NOT ALLOW TO WRITE TO ANOTHER DISK!!!!!!!!!

  @param[in] DeviceName   The device name.
  @param[in] Offset       The offset.
  @param[in] Size         The size.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        A load error occured.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.  
**/
EFI_STATUS
HDiskImageSave (
  IN CHAR16 *DeviceName,
  IN UINTN  Offset,
  IN UINTN  Size
  );

#endif
