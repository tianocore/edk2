/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <Library/PeiServicesLib.h>
#include <Pi/PrePiHob.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>

/**
 * Build ResourceDescriptorHob for the unaccepted memory region.
 * This memory region may be splitted into 2 parts because of lazy accept.
 *
 * @param Hob     Point to the EFI_HOB_RESOURCE_DESCRIPTOR
 * @return VOID
 */
VOID
BuildResourceDescriptorHobForUnacceptedMemory (
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *Hob
  )
{
  EFI_PHYSICAL_ADDRESS         PhysicalStart;
  EFI_PHYSICAL_ADDRESS         PhysicalEnd;
  UINT64                       ResourceLength;
  EFI_RESOURCE_TYPE            ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  UINT64                       MaxAcceptedMemoryAddress;

  ASSERT (Hob->ResourceType == BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED);

  ResourceType      = BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED;
  ResourceAttribute = Hob->ResourceAttribute;
  PhysicalStart     = Hob->PhysicalStart;
  ResourceLength    = Hob->ResourceLength;
  PhysicalEnd       = PhysicalStart + ResourceLength;

  //
  // In the first stage of lazy-accept, all the memory under 4G will be accepted.
  // The memory above 4G will not be accepted.
  //
  MaxAcceptedMemoryAddress = BASE_4GB;

  if (PhysicalEnd <= MaxAcceptedMemoryAddress) {
    //
    // This memory region has been accepted.
    //
    ResourceType       = EFI_RESOURCE_SYSTEM_MEMORY;
    ResourceAttribute |= (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED);
  } else if (PhysicalStart >= MaxAcceptedMemoryAddress) {
    //
    // This memory region hasn't been accepted.
    // So keep the ResourceType and ResourceAttribute unchange.
    //
  }

  BuildResourceDescriptorHob (
    ResourceType,
    ResourceAttribute,
    PhysicalStart,
    ResourceLength
    );
}

/**
  Transfer the incoming HobList for the TD to the final HobList for Dxe.
  The Hobs transferred in this function are ResourceDescriptor hob and
  MemoryAllocation hob.

  @param[in] VmmHobList    The Hoblist pass the firmware

**/
VOID
EFIAPI
TransferTdxHobList (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_RESOURCE_TYPE            ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  VOID                         *GuidedData;

  //
  // PcdOvmfSecGhcbBase is used as the TD_HOB in Tdx guest.
  //
  Hob.Raw = (UINT8 *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        ResourceType      = Hob.ResourceDescriptor->ResourceType;
        ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;

        if (ResourceType == BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED) {
          BuildResourceDescriptorHobForUnacceptedMemory (Hob.ResourceDescriptor);
        } else {
          BuildResourceDescriptorHob (
            ResourceType,
            ResourceAttribute,
            Hob.ResourceDescriptor->PhysicalStart,
            Hob.ResourceDescriptor->ResourceLength
            );
        }

        break;
      case EFI_HOB_TYPE_MEMORY_ALLOCATION:
        BuildMemoryAllocationHob (
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
          Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
          Hob.MemoryAllocation->AllocDescriptor.MemoryType
          );
        break;
      case EFI_HOB_TYPE_GUID_EXTENSION:
        GuidedData = (VOID *)(&Hob.Guid->Name + 1);
        BuildGuidDataHob (&Hob.Guid->Name, GuidedData, Hob.Guid->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE));
        break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

/**
  In Tdx guest, the system memory is passed in TdHob by host VMM. So
  the major task of PlatformTdxPublishRamRegions is to walk thru the
  TdHob list and transfer the ResourceDescriptorHob and MemoryAllocationHob
  to the hobs in DXE phase.

  MemoryAllocationHob should also be created for Mailbox and Ovmf work area.
**/
VOID
EFIAPI
PlatformTdxPublishRamRegions (
  VOID
  )
{
  if (!TdIsEnabled ()) {
    return;
  }

  TransferTdxHobList ();

  //
  // The memory region defined by PcdOvmfSecGhcbBackupBase is pre-allocated by
  // host VMM and used as the td mailbox at the beginning of system boot.
  //
  BuildMemoryAllocationHob (
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );

  if (FixedPcdGet32 (PcdOvmfWorkAreaSize) != 0) {
    //
    // Reserve the work area.
    //
    // Since this memory range will be used by the Reset Vector on S3
    // resume, it must be reserved as ACPI NVS.
    //
    // If S3 is unsupported, then various drivers might still write to the
    // work area. We ought to prevent DXE from serving allocation requests
    // such that they would overlap the work area.
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase),
      (UINT64)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaSize),
      EfiBootServicesData
      );
  }
}
