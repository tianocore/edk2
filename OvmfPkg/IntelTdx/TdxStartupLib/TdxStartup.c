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
#include <Library/TdxPlatformLib.h>
#include <Library/PrePiLib.h>
#include <Library/TdxStartupLib.h>
#include "TdxStartupInternal.h"

#define GET_GPAW_INIT_STATE(INFO)  ((UINT8) ((INFO) & 0x3f))

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
TdxStartup (
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
  BOOLEAN                     CfgSysStateDefault;
  BOOLEAN                     CfgNxStackDefault;

  Status      = EFI_SUCCESS;
  BootFv      = NULL;
  SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Context;
  VmmHobList  = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);

  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  ASSERT (Status == EFI_SUCCESS);

  DEBUG ((
    EFI_D_INFO,
    "Tdx started with(Hob: 0x%x, Gpaw: 0x%x, Cpus: %d)\n",
    (UINT32)(UINTN)VmmHobList,
    GET_GPAW_INIT_STATE (TdReturnData.TdInfo.Gpaw),
    TdReturnData.TdInfo.NumVcpus
    ));

  ZeroMem (&PlatformInfoHob, sizeof (PlatformInfoHob));

  //
  // Construct the Fw hoblist.
  //
  Status = ConstructFwHobList (VmmHobList);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  //
  // Tranfer the Hoblist to the final Hoblist for DXE
  //
  TransferHobList (VmmHobList);

  //
  // Initialize Platform
  //
  TdxPlatformInitialize (&PlatformInfoHob, &CfgSysStateDefault, &CfgNxStackDefault);

  //
  // TDVF must not use any CpuHob from input HobList.
  // It must create its own using GPWA from VMM and 0 for SizeOfIoSpace
  //
  BuildCpuHob (GET_GPAW_INIT_STATE (TdReturnData.TdInfo.Gpaw), 16);

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

  BuildGuidDataHob (&gUefiOvmfPkgTdxPlatformGuid, &PlatformInfoHob, sizeof (EFI_HOB_PLATFORM_INFO));

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

  BuildMemoryAllocationHob (
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );

  //
  // Load the DXE Core and transfer control to it.
  // DXE FV is the 1st FvInstance. (base 0)
  //
  Status = DxeLoadCore (1);

  //
  // Never arrive here.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}
