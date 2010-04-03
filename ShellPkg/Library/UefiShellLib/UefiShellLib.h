/** @file
  Provides interface to shell functionality for shell commands and applications.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

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
