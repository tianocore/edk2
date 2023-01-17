/** @file
  Main SEC phase code. Handles initial TDX Hob List Processing

  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PciLib.h>
#include <Library/PrePiLib.h>
#include <Library/QemuFwCfgLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/PlatformInitLib.h>
#include <OvmfPlatforms.h>
#include <Pi/PrePiHob.h>
#include "PeilessStartupInternal.h"

/**
 * Construct the HobList in SEC phase.
 *
 * @return EFI_SUCCESS      Successfully construct the firmware hoblist.
 * @return EFI_NOT_FOUND    Cannot find a memory region to be the fw hoblist.
 */
EFI_STATUS
EFIAPI
ConstructSecHobList (
  )
{
  UINT32  LowMemorySize;
  UINT32  LowMemoryStart;

  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_HOB_PLATFORM_INFO       PlatformInfoHob;

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));
  PlatformInfoHob.HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  PlatformGetSystemMemorySizeBelow4gb (&PlatformInfoHob);
  LowMemorySize = PlatformInfoHob.LowMemory;
  ASSERT (LowMemorySize != 0);
  LowMemoryStart = FixedPcdGet32 (PcdOvmfDxeMemFvBase) + FixedPcdGet32 (PcdOvmfDxeMemFvSize);
  LowMemorySize -= LowMemoryStart;

  DEBUG ((DEBUG_INFO, "LowMemory Start and End: %x, %x\n", LowMemoryStart, LowMemoryStart + LowMemorySize));
  HobList = HobConstructor (
              (VOID *)(UINTN)LowMemoryStart,
              LowMemorySize,
              (VOID *)(UINTN)LowMemoryStart,
              (VOID *)(UINTN)(LowMemoryStart + LowMemorySize)
              );

  SetHobList ((VOID *)(UINT64)HobList);

  return EFI_SUCCESS;
}

/**
 * This function is to find a memory region which is the largest one below 4GB.
 * It will be used as the firmware hoblist.
 *
 * @param VmmHobList    Vmm passed hoblist which constains the memory information.
 * @return EFI_SUCCESS  Successfully construct the firmware hoblist.
 */
EFI_STATUS
EFIAPI
ConstructFwHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  PhysicalEnd;
  UINT64                ResourceLength;
  EFI_PHYSICAL_ADDRESS  LowMemoryStart;
  UINT64                LowMemoryLength;

  ASSERT (VmmHobList != NULL);

  Hob.Raw = (UINT8 *)VmmHobList;

  LowMemoryLength = 0;
  LowMemoryStart  = 0;

  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED) {
        PhysicalEnd    = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;
        ResourceLength = Hob.ResourceDescriptor->ResourceLength;

        if (PhysicalEnd <= BASE_4GB) {
          if (ResourceLength > LowMemoryLength) {
            LowMemoryStart  = Hob.ResourceDescriptor->PhysicalStart;
            LowMemoryLength = ResourceLength;
          }
        } else {
          break;
        }
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  if (LowMemoryLength == 0) {
    DEBUG ((DEBUG_ERROR, "Cannot find a memory region under 4GB for Fw hoblist.\n"));
    return EFI_NOT_FOUND;
  }

  //
  // HobLib doesn't like HobStart at address 0 so adjust is needed
  //
  if (LowMemoryStart == 0) {
    LowMemoryStart  += EFI_PAGE_SIZE;
    LowMemoryLength -= EFI_PAGE_SIZE;
  }

  DEBUG ((DEBUG_INFO, "LowMemory Start and End: %x, %x\n", LowMemoryStart, LowMemoryStart + LowMemoryLength));
  HobConstructor (
    (VOID *)LowMemoryStart,
    LowMemoryLength,
    (VOID *)LowMemoryStart,
    (VOID *)(LowMemoryStart + LowMemoryLength)
    );

  SetHobList ((VOID *)(UINT64)LowMemoryStart);

  return EFI_SUCCESS;
}
