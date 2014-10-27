/** @file
*
*  Copyright (c) 2014, ARM Ltd. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution. The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
*  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __BOOTMONFS_LOADER_H__
#define __BOOTMONFS_LOADER_H__

/**
  Check that loading the file is supported.

  Not all information is checked, only the properties that matters to us in
  our simplified loader.

  BootMonFS file properties is not in a file header but in the file-system
  metadata, so we need to pass a handle to the file to allow access to the
  information.

  @param[in] FileHandle  Handle of the file to check.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the header is invalid.
  @retval EFI_UNSUPPORTED if the file type/platform is not supported.
**/
EFI_STATUS
BootMonFsCheckFile (
  IN  CONST EFI_FILE_HANDLE  FileHandle
  );

/**
  Load a binary file from BootMonFS.

  @param[in]  FileHandle    Handle of the file to load.

  @param[in]  FileData      Address  of the file data in memory.

  @param[out] EntryPoint    Will be filled with the ELF entry point address.

  @param[out] ImageSize     Will be filled with the file size in memory. This
                            will effectively be equal to the sum of the load
                            region sizes.

  This function assumes the file is valid and supported as checked with
  BootMonFsCheckFile().

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER if the file is invalid.
**/
EFI_STATUS
BootMonFsLoadFile (
  IN  CONST EFI_FILE_HANDLE   FileHandle,
  IN  CONST VOID             *FileData,
  OUT VOID                  **EntryPoint,
  OUT LIST_ENTRY             *LoadList
  );

#endif // __BOOTMONFS_LOADER_H__
