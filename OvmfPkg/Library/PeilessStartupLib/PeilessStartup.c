/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Protocol/DebugSupport.h>
#include <Library/TdxLib.h>
#include <IndustryStandard/Tdx.h>
#include <Library/PrePiLib.h>
#include <Library/PeilessStartupLib.h>
#include <Library/PlatformInitLib.h>
#include <Library/TdxHelperLib.h>
#include <ConfidentialComputingGuestAttr.h>
#include <Guid/MemoryTypeInformation.h>
#include <OvmfPlatforms.h>
#include "PeilessStartupInternal.h"

#define GET_GPAW_INIT_STATE(INFO)  ((UINT8) ((INFO) & 0x3f))

EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};

EFI_STATUS
EFIAPI
InitializePlatform (
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  VOID  *VariableStore;

  DEBUG ((DEBUG_INFO, "InitializePlatform in Pei-less boot\n"));
  PlatformDebugDumpCmos ();

  PlatformInfoHob->DefaultMaxCpuNumber = 64;
  PlatformInfoHob->PcdPciMmio64Size    = 0x800000000;

  PlatformInfoHob->HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  DEBUG ((DEBUG_INFO, "HostBridgeDeviceId = 0x%x\n", PlatformInfoHob->HostBridgeDevId));

  PlatformAddressWidthInitialization (PlatformInfoHob);
  DEBUG ((
    DEBUG_INFO,
    "PhysMemAddressWidth=0x%x, Pci64Base=0x%llx, Pci64Size=0x%llx\n",
    PlatformInfoHob->PhysMemAddressWidth,
    PlatformInfoHob->PcdPciMmio64Base,
    PlatformInfoHob->PcdPciMmio64Size
    ));

  PlatformMaxCpuCountInitialization (PlatformInfoHob);
  DEBUG ((
    DEBUG_INFO,
    "MaxCpuCount=%d, BootCpuCount=%d\n",
    PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber,
    PlatformInfoHob->PcdCpuBootLogicalProcessorNumber
    ));

  PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);
  PlatformQemuUc32BaseInitialization (PlatformInfoHob);
  DEBUG ((
    DEBUG_INFO,
    "Uc32Base = 0x%x, Uc32Size = 0x%x, LowerMemorySize = 0x%x\n",
    PlatformInfoHob->Uc32Base,
    PlatformInfoHob->Uc32Size,
    PlatformInfoHob->LowMemory
    ));

  VariableStore                                  = PlatformReserveEmuVariableNvStore ();
  PlatformInfoHob->PcdEmuVariableNvStoreReserved = (UINT64)(UINTN)VariableStore;
 #ifdef SECURE_BOOT_FEATURE_ENABLED
  PlatformInitEmuVariableNvStore (VariableStore);
 #endif

  if (TdIsEnabled ()) {
    PlatformTdxPublishRamRegions ();
  } else {
    PlatformQemuInitializeRam (PlatformInfoHob);
    PlatformQemuInitializeRamForS3 (PlatformInfoHob);
  }

  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof (mDefaultMemoryTypeInformation)
    );

  PlatformMemMapInitialization (PlatformInfoHob);

  PlatformNoexecDxeInitialization (PlatformInfoHob);

  if (TdIsEnabled ()) {
    PlatformInfoHob->PcdConfidentialComputingGuestAttr = CCAttrIntelTdx;
    PlatformInfoHob->PcdTdxSharedBitMask               = TdSharedPageMask ();
    PlatformInfoHob->PcdSetNxForStack                  = TRUE;
  }

  PlatformMiscInitialization (PlatformInfoHob);

  return EFI_SUCCESS;
}

/**
 * This function brings up the Tdx guest from SEC phase to DXE phase.
 * PEI phase is skipped because most of the components in PEI phase
 * is not needed for Tdx guest, for example, MP Services, TPM etc.
 * In this way, the attack surfaces are reduced as much as possible.
 *
 * @param Context   The pointer to the SecCoreData
 * @return VOID     This function never returns
 */
VOID
EFIAPI
PeilessStartup (
  IN VOID  *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_FIRMWARE_VOLUME_HEADER  *BootFv;
  EFI_STATUS                  Status;
  EFI_HOB_PLATFORM_INFO       PlatformInfoHob;
  UINT32                      DxeCodeBase;
  UINT32                      DxeCodeSize;
  TD_RETURN_DATA              TdReturnData;
  VOID                        *VmmHobList;

  Status      = EFI_SUCCESS;
  BootFv      = NULL;
  VmmHobList  = NULL;
  SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Context;

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));

  if (TdIsEnabled ()) {
    VmmHobList = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
    Status     = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
    ASSERT (Status == EFI_SUCCESS);

    DEBUG ((
      DEBUG_INFO,
      "Tdx started with(Hob: 0x%x, Gpaw: 0x%x, Cpus: %d)\n",
      (UINT32)(UINTN)VmmHobList,
      GET_GPAW_INIT_STATE (TdReturnData.TdInfo.Gpaw),
      TdReturnData.TdInfo.NumVcpus
      ));

    Status = ConstructFwHobList (VmmHobList);
  } else {
    DEBUG ((DEBUG_INFO, "Ovmf started\n"));
    Status = ConstructSecHobList ();
  }

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  DEBUG ((DEBUG_INFO, "HobList: %p\n", GetHobList ()));

  if (TdIsEnabled ()) {
    //
    // Build GuidHob for the tdx measurements which were done in SEC phase.
    //
    Status = TdxHelperBuildGuidHobForTdxMeasurement ();
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      CpuDeadLoop ();
    }
  }

  //
  // Initialize the Platform
  //
  Status = InitializePlatform (&PlatformInfoHob);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  BuildGuidDataHob (&gUefiOvmfPkgPlatformInfoGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));

  //
  // SecFV
  //
  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  BuildFvHob ((UINTN)BootFv, BootFv->FvLength);

  //
  // DxeFV
  //
  DxeCodeBase = PcdGet32 (PcdBfvBase);
  DxeCodeSize = PcdGet32 (PcdBfvRawDataSize) - (UINT32)BootFv->FvLength;
  BuildFvHob (DxeCodeBase, DxeCodeSize);

  DEBUG ((DEBUG_INFO, "SecFv : %p, 0x%x\n", BootFv, BootFv->FvLength));
  DEBUG ((DEBUG_INFO, "DxeFv : %x, 0x%x\n", DxeCodeBase, DxeCodeSize));

  BuildStackHob ((UINTN)SecCoreData->StackBase, SecCoreData->StackSize <<= 1);

  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    (UINT64)SecCoreData->TemporaryRamBase,
    (UINT64)SecCoreData->TemporaryRamSize
    );

  //
  // Load the DXE Core and transfer control to it.
  // Only DxeFV is in the compressed section.
  //
  Status = DxeLoadCore (1);

  //
  // Never arrive here.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}
