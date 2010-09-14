/** @file
  Main file for NULL named library for Profile1 shell command functions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <ShellBase.h>

#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DevicePathToText.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>


extern        EFI_HANDLE                        gShellDriver1HiiHandle;
extern CONST  EFI_GUID                          gShellDriver1HiiGuid;

EFI_HANDLE*
EFIAPI
GetHandleListByPotocol (
  IN CONST EFI_GUID *ProtocolGuid
  );

SHELL_STATUS
EFIAPI
ShellCommandRunConnect (
  VOID                *RESERVED
  );

SHELL_STATUS
EFIAPI
ShellCommandRunDevices (
  VOID                *RESERVED
  );

SHELL_STATUS
EFIAPI
ShellCommandRunOpenInfo (
  VOID                *RESERVED
  );




