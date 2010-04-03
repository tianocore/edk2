/** @file
  Save Non-Volatile Variables to a file system.

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
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


/**
  Examines the NvVars file contents, and updates variables based on it.

  @param[in]  VarsBuffer - Buffer with NvVars data
  @param[in]  VarsBufferSize - Size of VarsBuffer in bytes

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
SetVariablesFromBuffer (
  IN VOID   *VarsBuffer,
  IN UINTN  VarsBufferSize
  );


/**
  Writes the variable into the file so it can be restored from
  the file on future boots of the system.

  @param[in]  File - The file to write to
  @param[in]  Name - Variable name string
  @param[in]  NameSize - Size of Name in bytes
  @param[in]  Guid - GUID of variable
  @param[in]  Attributes - Attributes of variable
  @param[in]  Data - Buffer containing Data for variable
  @param[in]  DataSize - Size of Data in bytes

  @return     EFI_STATUS based on the success or failure of the operation

**/
EFI_STATUS
PackVariableIntoFile (
  IN EFI_FILE_HANDLE  File,
  IN CHAR16           *Name,
  IN UINT32           NameSize,
  IN EFI_GUID         *Guid,
  IN UINT32           Attributes,
  IN VOID             *Data,
  IN UINT32           DataSize
  );

#endif

