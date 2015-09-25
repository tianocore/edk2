/** @file
  header file for NULL named library for 'tftp' Shell command functions.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _UEFI_SHELL_TFTP_COMMAND_LIB_H_
#define _UEFI_SHELL_TFTP_COMMAND_LIB_H_

#include <Uefi.h>
#include <ShellBase.h>

#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Mtftp4.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>
#include <Library/PrintLib.h>

extern EFI_HANDLE gShellTftpHiiHandle;

typedef struct {
  UINTN  FileSize;
  UINTN  DownloadedNbOfBytes;
  UINTN  LastReportedNbOfBytes;
} DOWNLOAD_CONTEXT;

/**
  Function for 'tftp' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunTftp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif /* _UEFI_SHELL_TFTP_COMMAND_LIB_H_ */
