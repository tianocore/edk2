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
#include <Library/MemoryAllocationLib.h>
#include <Library/TdxMailboxLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/TdxProbeLib.h>
#include "Platform.h"

VOID
EFIAPI
DEBUG_HOBLIST (
  IN CONST VOID             *HobStart
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  Hob.Raw = (UINT8 *) HobStart;
  //
  // Parse the HOB list until end of list or matching type is found.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    DEBUG ((DEBUG_INFO, "HOB(%p) : %x %x\n", Hob, Hob.Header->HobType, Hob.Header->HobLength));
    switch (Hob.Header->HobType) {
    case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
      DEBUG ((DEBUG_INFO, "\t: %x %x %llx %llx\n",
        Hob.ResourceDescriptor->ResourceType,
        Hob.ResourceDescriptor->ResourceAttribute,
        Hob.ResourceDescriptor->PhysicalStart,
        Hob.ResourceDescriptor->ResourceLength));

      break;
    case EFI_HOB_TYPE_MEMORY_ALLOCATION:
      DEBUG ((DEBUG_INFO, "\t: %llx %llx %x\n",
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
        Hob.MemoryAllocation->AllocDescriptor.MemoryType));
      break;
    default:
      break;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

/**
  Transfer the incoming HobList for the TD to the final HobList for Dxe

  @param[in] VmmHobList    The Hoblist pass the firmware

**/
VOID
EFIAPI
TransferTdxHobList (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;
  EFI_PHYSICAL_ADDRESS        PhysicalEnd;

  Hob.Raw = (UINT8 *) (UINTN) PcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
    case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
      ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;
      PhysicalEnd = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

      //
      // We mark each resource that we issue AcceptPage to with EFI_RESOURCE_SYSTEM_MEMORY
      //
      if ((Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
        (PhysicalEnd <= BASE_4GB)) {
        ResourceAttribute |= EFI_RESOURCE_ATTRIBUTE_ENCRYPTED;
      }
      BuildResourceDescriptorHob (
        Hob.ResourceDescriptor->ResourceType,
        ResourceAttribute,
        Hob.ResourceDescriptor->PhysicalStart,
        Hob.ResourceDescriptor->ResourceLength);
      break;
    case EFI_HOB_TYPE_MEMORY_ALLOCATION:
      BuildMemoryAllocationHob (
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
        Hob.MemoryAllocation->AllocDescriptor.MemoryType);
      break;
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }
  DEBUG_HOBLIST (GetHobList ());
}

VOID
TdxPublishRamRegions (
  VOID
  )
{
  TransferTdxHobList();

  BuildMemoryAllocationHob (
    PcdGet32 (PcdOvmfSecGhcbBackupBase),
    PcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );
}

VOID
EFIAPI
CheckSystemStatsForOverride (
  EFI_HOB_PLATFORM_INFO    * PlatformInfoHob
  )
{
  EFI_STATUS              Status;
  FIRMWARE_CONFIG_ITEM    FwCfgItem;
  UINTN                   FwCfgSize;

  //
  // check for overrides
  //
  Status = QemuFwCfgFindFile ("etc/system-states", &FwCfgItem, &FwCfgSize);
  if (Status != RETURN_SUCCESS || FwCfgSize != sizeof PlatformInfoHob->SystemStates) {
    DEBUG ((DEBUG_INFO, "ACPI using S3/S4 defaults\n"));
    return;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (PlatformInfoHob->SystemStates), PlatformInfoHob->SystemStates);
}

UINT64
EFIAPI
TdxRelocateMailbox (
  VOID
  )
{
  VOID                        * Address;
  VOID                        * ApLoopFunc = NULL;
  UINT32                      RelocationPages;
  MP_RELOCATION_MAP           RelocationMap;
  MP_WAKEUP_MAILBOX           * RelocatedMailBox;

  //
  // Get information needed to setup aps running in their
  // run loop in allocated acpi reserved memory
  // Add another page for mailbox
  //
  AsmGetRelocationMap (&RelocationMap);
  RelocationPages  = EFI_SIZE_TO_PAGES ((UINT32)RelocationMap.RelocateApLoopFuncSize) + 1;

  Address = AllocatePagesWithMemoryType (EfiACPIMemoryNVS, RelocationPages);
  ApLoopFunc = (VOID *) ((UINTN) Address + EFI_PAGE_SIZE);

  CopyMem (
    ApLoopFunc,
    RelocationMap.RelocateApLoopFuncAddress,
    RelocationMap.RelocateApLoopFuncSize
    );

  DEBUG ((DEBUG_INFO, "Ap Relocation: mailbox %p, loop %p\n",
    Address, ApLoopFunc));

  //
  // Initialize mailbox
  //
  RelocatedMailBox = (MP_WAKEUP_MAILBOX *)Address;
  RelocatedMailBox->Command = MpProtectedModeWakeupCommandNoop;
  RelocatedMailBox->ApicId = MP_CPU_PROTECTED_MODE_MAILBOX_APICID_INVALID;
  RelocatedMailBox->WakeUpVector = 0;

  //
  // Wakup APs and have been move to the finalized run loop
  // They will spin until guest OS wakes them
  //
  MpSerializeStart ();

  MpSendWakeupCommand (
    MpProtectedModeWakeupCommandWakeup,
    (UINT64)ApLoopFunc,
    (UINT64)RelocatedMailBox,
    0,
    0,
    0);

  return (UINT64)RelocatedMailBox;
}

VOID
BuildTdxFvHobs (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS BaseAddress;
  UINT64  Length;
  BaseAddress = 0xffc00000;
  Length = 0x40000;
  BuildMemoryAllocationHob(BaseAddress, Length, EfiBootServicesData);
}

VOID
IntelTdxInitialize (
  VOID
  )
{
  EFI_HOB_PLATFORM_INFO       PlatformInfoHob;
  RETURN_STATUS               PcdStatus;

  if (!TdxIsEnabled ()) {
    return;
  }

  PcdStatus = PcdSetBoolS (PcdTdxIsEnabled, TRUE);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSetBoolS (PcdIa32EferChangeAllowed, FALSE);
  ASSERT_RETURN_ERROR (PcdStatus);

  BuildTdxFvHobs();
  // PlatformInfoHob
  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));
  PlatformInfoHob.HostBridgePciDevId = mHostBridgeDevId;

  // Relocate TdMailbox
  PlatformInfoHob.RelocatedMailBox = TdxRelocateMailbox ();

  // CheckSystemStatsForOverride
  CheckSystemStatsForOverride (&PlatformInfoHob);

  // Build PlatformInfoHob
  BuildGuidDataHob (&gUefiOvmfPkgTdxPlatformGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));
}
