/** @file
  This file contains DXE driver for publishing empty HSTI table

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2024, Red Hat. Inc

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/HstiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformInitLib.h>

#include <IndustryStandard/Hsti.h>
#include <IndustryStandard/I440FxPiix4.h>
#include <IndustryStandard/Q35MchIch9.h>

#include "VirtHstiDxe.h"

VOID
VirtHstiSetSupported (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti,
  IN UINT32                            ByteIndex,
  IN UINT8                             BitMask
  )
{
  ASSERT (ByteIndex < VIRT_HSTI_SECURITY_FEATURE_SIZE);
  VirtHsti->SecurityFeaturesRequired[ByteIndex]    |= BitMask;
  VirtHsti->SecurityFeaturesImplemented[ByteIndex] |= BitMask;
}

BOOLEAN
VirtHstiIsSupported (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti,
  IN UINT32                            ByteIndex,
  IN UINT8                             BitMask
  )
{
  ASSERT (ByteIndex < VIRT_HSTI_SECURITY_FEATURE_SIZE);
  return VirtHsti->SecurityFeaturesImplemented[ByteIndex] & BitMask;
}

VOID
VirtHstiTestResult (
  CHAR16     *ErrorMsg,
  IN UINT32  ByteIndex,
  IN UINT8   BitMask
  )
{
  EFI_STATUS  Status;

  ASSERT (ByteIndex < VIRT_HSTI_SECURITY_FEATURE_SIZE);

  if (ErrorMsg) {
    DEBUG ((DEBUG_ERROR, "VirtHsti: Test failed: %s\n", ErrorMsg));
    Status = HstiLibAppendErrorString (
               PLATFORM_SECURITY_ROLE_PLATFORM_REFERENCE,
               NULL,
               ErrorMsg
               );
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = HstiLibSetFeaturesVerified (
               PLATFORM_SECURITY_ROLE_PLATFORM_REFERENCE,
               NULL,
               ByteIndex,
               BitMask
               );
    ASSERT_EFI_ERROR (Status);
  }
}

STATIC
UINT16
VirtHstiGetHostBridgeDevId (
  VOID
  )
{
  EFI_HOB_GUID_TYPE      *GuidHob;
  EFI_HOB_PLATFORM_INFO  *PlatformInfo;

  GuidHob = GetFirstGuidHob (&gUefiOvmfPkgPlatformInfoGuid);
  ASSERT (GuidHob);
  PlatformInfo = (EFI_HOB_PLATFORM_INFO *)GET_GUID_HOB_DATA (GuidHob);
  return PlatformInfo->HostBridgeDevId;
}

STATIC
VOID
EFIAPI
VirtHstiOnReadyToBoot (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  switch (VirtHstiGetHostBridgeDevId ()) {
    case INTEL_82441_DEVICE_ID:
      VirtHstiQemuPCVerify ();
      VirtHstiQemuCommonVerify ();
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      VirtHstiQemuQ35Verify ();
      VirtHstiQemuCommonVerify ();
      break;
    default:
      ASSERT (FALSE);
  }

  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
VirtHstiDxeEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti;
  UINT16                               DevId;
  EFI_STATUS                           Status;
  EFI_EVENT                            Event;

  if (PcdGet64 (PcdConfidentialComputingGuestAttr)) {
    DEBUG ((DEBUG_INFO, "%a: confidential guest\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DevId = VirtHstiGetHostBridgeDevId ();
  switch (DevId) {
    case INTEL_82441_DEVICE_ID:
      VirtHsti = VirtHstiQemuPCInit ();
      VirtHstiQemuCommonInit (VirtHsti);
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      VirtHsti = VirtHstiQemuQ35Init ();
      VirtHstiQemuCommonInit (VirtHsti);
      break;
    default:
      DEBUG ((DEBUG_INFO, "%a: unknown platform (0x%x)\n", __func__, DevId));
      return EFI_UNSUPPORTED;
  }

  Status = HstiLibSetTable (VirtHsti, sizeof (*VirtHsti));
  if (EFI_ERROR (Status)) {
    if (Status != EFI_ALREADY_STARTED) {
      ASSERT_EFI_ERROR (Status);
    }
  }

  EfiCreateEventReadyToBootEx (
    TPL_NOTIFY,
    VirtHstiOnReadyToBoot,
    NULL,
    &Event
    );

  return EFI_SUCCESS;
}
