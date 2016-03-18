/** @file
  Boot UEFI Linux.

  Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LOAD_LINUX_LIB_INCLUDED_
#define _LOAD_LINUX_LIB_INCLUDED_

#include <Uefi.h>
#include <Library/LoadLinuxLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <IndustryStandard/LinuxBzimage.h>

#include <Protocol/GraphicsOutput.h>

VOID
EFIAPI
JumpToKernel (
  VOID *KernelStart,
  VOID *KernelBootParams
  );

VOID
EFIAPI
JumpToUefiKernel (
  EFI_HANDLE ImageHandle,
  EFI_SYSTEM_TABLE *SystemTable,
  VOID *KernelBootParams,
  VOID *KernelStart
  );

VOID
InitLinuxDescriptorTables (
  VOID
  );

VOID
SetLinuxDescriptorTables (
  VOID
  );

#endif

