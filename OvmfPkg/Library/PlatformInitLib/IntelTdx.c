/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>

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
  EFI_PEI_HOB_POINTERS  Hob;

  //
  // PcdOvmfSecGhcbBase is used as the TD_HOB in Tdx guest.
  //
  Hob.Raw = (UINT8 *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        BuildResourceDescriptorHob (
          Hob.ResourceDescriptor->ResourceType,
          Hob.ResourceDescriptor->ResourceAttribute,
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

/**

  Publish memory regions in Intel TDX guest.

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
