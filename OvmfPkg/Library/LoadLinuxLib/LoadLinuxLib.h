/** @file
  Boot UEFI Linux.

  Copyright (c) 2008 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  VOID  *KernelStart,
  VOID  *KernelBootParams
  );

VOID
EFIAPI
JumpToUefiKernel (
  EFI_HANDLE        ImageHandle,
  EFI_SYSTEM_TABLE  *SystemTable,
  VOID              *KernelBootParams,
  VOID              *KernelStart
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
