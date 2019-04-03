/** @file
  Provides functions to save and restore NV variables in a file.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __NV_VARS_FILE_LIB__
#define __NV_VARS_FILE_LIB__

/**
  Attempts to connect the NvVarsFileLib to the specified file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     The EFI_STATUS while attempting to connect the NvVarsFileLib
              to the file system instance.
  @retval     EFI_SUCCESS - The given file system was connected successfully

**/
EFI_STATUS
EFIAPI
ConnectNvVarsToFileSystem (
  IN EFI_HANDLE    FsHandle
  );


/**
  Update non-volatile variables stored on the file system.

  @return     The EFI_STATUS while attempting to update the variable on
              the connected file system.
  @retval     EFI_SUCCESS - The non-volatile variables were saved to the disk
  @retval     EFI_NOT_STARTED - A file system has not been connected

**/
EFI_STATUS
EFIAPI
UpdateNvVarsOnFileSystem (
  );


#endif

