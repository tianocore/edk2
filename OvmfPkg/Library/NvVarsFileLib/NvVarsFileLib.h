/** @file
  Save Non-Volatile Variables to a file system.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __NV_VARS_FILE_LIB_INSTANCE__
#define __NV_VARS_FILE_LIB_INSTANCE__

#include <Uefi.h>

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/FileHandleLib.h>
#include <Library/SerializeVariablesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

/**
  Loads the non-volatile variables from the NvVars file on the
  given file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     EFI_STATUS based on the success or failure of load operation

**/
EFI_STATUS
LoadNvVarsFromFs (
  EFI_HANDLE                            FsHandle
  );


/**
  Saves the non-volatile variables into the NvVars file on the
  given file system.

  @param[in]  FsHandle - Handle for a gEfiSimpleFileSystemProtocolGuid instance

  @return     EFI_STATUS based on the success or failure of load operation

**/
EFI_STATUS
SaveNvVarsToFs (
  EFI_HANDLE                            FsHandle
  );

#endif

