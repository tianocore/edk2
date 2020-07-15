/** @file
  The file operation functions for WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_WIFI_MGR_FILE_UTIL__
#define __EFI_WIFI_MGR_FILE_UTIL__

#include "WifiConnectionMgrDxe.h"

/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AddtionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AddtionAllocateSize   Addtion size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE           FileHandle,
  IN OUT  VOID                      **BufferPtr,
     OUT  UINTN                     *FileSize,
  IN      UINTN                     AddtionAllocateSize
  );

/**
  Update the CA cert base on the input file path info.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  FilePath            Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdateCAFromFile (
  IN  WIFI_MGR_PRIVATE_DATA           *Private,
  IN  EFI_DEVICE_PATH_PROTOCOL        *FilePath
  );

/**
  Update the Private Key base on the input file path info.

  @param[in]  Private             The pointer to the global private data structure.
  @param[in]  FilePath            Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdatePrivateKeyFromFile (
  IN  WIFI_MGR_PRIVATE_DATA           *Private,
  IN  EFI_DEVICE_PATH_PROTOCOL        *FilePath
  );

#endif
