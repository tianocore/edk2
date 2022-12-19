/** @file
  Platform PEI module include file.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_PEI_H_INCLUDED_
#define _PLATFORM_PEI_H_INCLUDED_

#include <IndustryStandard/E820.h>
#include <Library/PlatformInitLib.h>

VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize,
  BOOLEAN               Cacheable
  );

VOID
AddReservedMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit,
  BOOLEAN               Cacheable
  );

VOID
AddressWidthInitialization (
  VOID
  );

VOID
Q35TsegMbytesInitialization (
  VOID
  );

EFI_STATUS
PublishPeiMemory (
  VOID
  );

UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  );

VOID
InitializeRamRegions (
  VOID
  );

EFI_STATUS
PeiFvInitialization (
  VOID
  );

VOID
InstallClearCacheCallback (
  VOID
  );

EFI_STATUS
XenConnect (
  VOID
  );

BOOLEAN
XenDetect (
  VOID
  );

BOOLEAN
XenHvmloaderDetected (
  VOID
  );

BOOLEAN
XenPvhDetected (
  VOID
  );

VOID
AmdSevInitialize (
  VOID
  );

VOID
XenPublishRamRegions (
  VOID
  );

EFI_STATUS
XenGetE820Map (
  EFI_E820_ENTRY64  **Entries,
  UINT32            *Count
  );

EFI_STATUS
PhysicalAddressIdentityMapping (
  IN EFI_PHYSICAL_ADDRESS  AddressToMap
  );

VOID
CalibrateLapicTimer (
  VOID
  );

extern EFI_BOOT_MODE  mBootMode;

extern UINT8  mPhysMemAddressWidth;

extern UINT16  mHostBridgeDevId;

#endif // _PLATFORM_PEI_H_INCLUDED_
