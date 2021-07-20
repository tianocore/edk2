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
#include <Library/TdxMailboxLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/QemuFwCfg.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/PeiServicesLib.h>
#include <WorkArea.h>
#include "Platform.h"

/**
  Check if it is Tdx guest

  @retval    TRUE   It is Tdx guest
  @retval    FALSE  It is not Tdx guest
**/
BOOLEAN
PlatformPeiIsTdxGuest (
  VOID
  )
{
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER   *CcWorkAreaHeader;

  CcWorkAreaHeader = (CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER *) FixedPcdGet32 (PcdOvmfWorkAreaBase);
  return (CcWorkAreaHeader != NULL && CcWorkAreaHeader->GuestType == GUEST_TYPE_INTEL_TDX);
}


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
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_RESOURCE_ATTRIBUTE_TYPE ResourceAttribute;

  //
  // PcdOvmfSecGhcbBase is used as the TD_HOB in Tdx guest.
  //
  Hob.Raw = (UINT8 *) (UINTN) PcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
    case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
      ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;

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

/**

  Publish memory regions in Intel TDX guest.

**/
VOID
TdxPublishRamRegions (
  VOID
  )
{
  TransferTdxHobList ();

  //
  // The memory region defined by PcdOvmfSecGhcbBackupBase is pre-allocated by
  // host VMM and used as the td mailbox at the beginning of system boot.
  //
  BuildMemoryAllocationHob (
    PcdGet32 (PcdOvmfSecGhcbBackupBase),
    PcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );
}

/**
  This function check the system status from QEMU via fw_cfg.
  If the system status from QEMU is retrieved, its value is set
  into PlatformInfoHob.

  @param[in]  PlatformInfoHob   The data structure of PlatformInfo hob
**/
VOID
EFIAPI
CheckSystemStatsForOverride (
  IN EFI_HOB_PLATFORM_INFO    * PlatformInfoHob
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

/**
  At the beginning of system boot, a 4K-aligned, 4K-size memory (Td mailbox) is
  pre-allocated by host VMM. BSP & APs do the page accept together in that memory
  region.

  After that TDVF is designed to relocate the mailbox to a 4K-aligned, 4K-size
  memory block which is allocated in the ACPI Nvs memory. APs are waken up and
  spin around the relocated mailbox for further command.

  @return   UINT64    Address of the relocated mailbox
**/
UINT64
EFIAPI
TdxRelocateMailbox (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        Address;
  VOID                        *ApLoopFunc = NULL;
  UINT32                      RelocationPages;
  MP_RELOCATION_MAP           RelocationMap;
  MP_WAKEUP_MAILBOX           *RelocatedMailBox;

  //
  // Get information needed to setup aps running in their
  // run loop in allocated acpi reserved memory
  // Add another page for mailbox
  //
  AsmGetRelocationMap (&RelocationMap);
  RelocationPages  = EFI_SIZE_TO_PAGES ((UINT32)RelocationMap.RelocateApLoopFuncSize) + 1;

  Status = PeiServicesAllocatePages (EfiACPIMemoryNVS, RelocationPages, &Address);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate pages to relocate Td mailbox. %r\n", Status));
    ASSERT (FALSE);
    return 0;
  }

  ApLoopFunc = (VOID *) ((UINTN) Address + EFI_PAGE_SIZE);

  CopyMem (
    ApLoopFunc,
    RelocationMap.RelocateApLoopFuncAddress,
    RelocationMap.RelocateApLoopFuncSize
    );

  DEBUG ((DEBUG_INFO, "Ap Relocation: mailbox %llx, loop %p\n",
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

/**

  This Function checks if TDX is available, if present then it sets
  the dynamic PcdTdxIsEnabled and PcdIa32EferChangeAllowed.

  It relocates the td mailbox and create the PlatformInfo Hob which includes
  the TDX specific information which will be consumed in DXE phase.

  **/
VOID
IntelTdxInitialize (
  VOID
  )
{
  EFI_HOB_PLATFORM_INFO       PlatformInfoHob;
  RETURN_STATUS               PcdStatus;
  UINT32                      ConfidentialComputingCategory;
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER   *CcWorkAreaHeader;

  if (!PlatformPeiIsTdxGuest ()) {
    return;
  }

  CcWorkAreaHeader = (CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER *) FixedPcdGet32 (PcdOvmfWorkAreaBase);
  ConfidentialComputingCategory = *((UINT32 *)CcWorkAreaHeader);

  PcdStatus = PcdSet32S (PcdConfidentialComputingCategory, ConfidentialComputingCategory);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSetBoolS (PcdIa32EferChangeAllowed, FALSE);
  ASSERT_RETURN_ERROR (PcdStatus);

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));
  PlatformInfoHob.HostBridgePciDevId = mHostBridgeDevId;

  PlatformInfoHob.RelocatedMailBox = TdxRelocateMailbox ();

  CheckSystemStatsForOverride (&PlatformInfoHob);

  BuildGuidDataHob (&gUefiOvmfPkgTdxPlatformGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));
}
