/** @file
  Provides interface to shell functionality for shell commands and applications.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UEFI_SHELL_LIB_INTERNAL_H_
#define _UEFI_SHELL_LIB_INTERNAL_H_

#include <Uefi.h>

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/FileHandleLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/ShellLib.h>

typedef struct  {
  EFI_SHELL_GET_FILE_INFO                   GetFileInfo;
  EFI_SHELL_SET_FILE_INFO                   SetFileInfo;
  EFI_SHELL_READ_FILE                       ReadFile;
  EFI_SHELL_WRITE_FILE                      WriteFile;
  EFI_SHELL_CLOSE_FILE                      CloseFile;
  EFI_SHELL_DELETE_FILE                     DeleteFile;
  EFI_SHELL_GET_FILE_POSITION               GetFilePosition;
  EFI_SHELL_SET_FILE_POSITION               SetFilePosition;
  EFI_SHELL_FLUSH_FILE                      FlushFile;
  EFI_SHELL_GET_FILE_SIZE                   GetFileSize;
} FILE_HANDLE_FUNCTION_MAP;

/**
  Function to determin if an entire string is a valid number.

  If Hex it must be preceeded with a 0x or has ForceHex, set TRUE.

  @param[in] String       The string to evaluate.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  TRUE to halt upon finding a space, FALSE to keep going.

  @retval TRUE        It is all numeric (dec/hex) characters.
  @retval FALSE       There is a non-numeric character.
**/
BOOLEAN
EFIAPI
InternalShellIsHexOrDecimalNumber (
  IN CONST CHAR16   *String,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace
  );

#endif

