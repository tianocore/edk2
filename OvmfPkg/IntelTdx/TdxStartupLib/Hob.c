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
#include <Library/PrePiLib.h>
#include <Library/TdxPlatformLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/QemuFwCfgLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include "TdxStartupInternal.h"

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
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
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

/**
  Transfer the incoming HobList for the TD to the final HobList for Dxe

  @param[in] VmmHobList    The Hoblist pass the firmware

**/
VOID
EFIAPI
TransferHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  EFI_PHYSICAL_ADDRESS         PhysicalEnd;

  Hob.Raw = (UINT8 *)VmmHobList;

  Hob.Raw = (UINT8 *)VmmHobList;
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;
        PhysicalEnd       = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

        if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
          ResourceAttribute |= EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED;

          if (PhysicalEnd <= BASE_4GB) {
            ResourceAttribute |= EFI_RESOURCE_ATTRIBUTE_ENCRYPTED;
          }
        }

        BuildResourceDescriptorHob (
          Hob.ResourceDescriptor->ResourceType,
          ResourceAttribute,
          Hob.ResourceDescriptor->PhysicalStart,
          Hob.ResourceDescriptor->ResourceLength
          );
        break;

      case EFI_HOB_TYPE_MEMORY_ALLOCATION:
        BuildMemoryAllocationHob (
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
          Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
          Hob.MemoryAllocation->AllocDescriptor.MemoryType
          );
        break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}
