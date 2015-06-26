/** @file
  Platform PEI module include file.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLATFORM_PEI_H_INCLUDED_
#define _PLATFORM_PEI_H_INCLUDED_

#include <IndustryStandard/E820.h>

VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  );

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  );

VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  );

VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  );

VOID
AddUntestedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  );

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  );

VOID
AddUntestedMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  );

VOID
AddressWidthInitialization (
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

EFI_STATUS
InitializeXen (
  VOID
  );

BOOLEAN
XenDetect (
  VOID
  );

extern BOOLEAN mXen;

VOID
XenPublishRamRegions (
  VOID
  );

extern EFI_BOOT_MODE mBootMode;

extern BOOLEAN mS3Supported;

extern UINT8 mPhysMemAddressWidth;

#endif // _PLATFORM_PEI_H_INCLUDED_
