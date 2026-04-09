/** @file
  Null Dxe Capsule Library instance does nothing and returns unsupport status.

Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/CapsuleLib.h>

/**
  The firmware checks whether the capsule image is supported
  by the CapsuleGuid in CapsuleHeader or other specific information in capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Point to the UEFI capsule image to be checked.

  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  return EFI_UNSUPPORTED;
}

/**
  The firmware specific implementation processes the capsule image
  if it recognized the format of this capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Point to the UEFI capsule image to be processed.

  @retval EFI_UNSUPPORTED  Capsule image is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  return EFI_UNSUPPORTED;
}

/**

  This routine is called to process capsules.

  Caution: This function may receive untrusted input.

  The capsules reported in EFI_HOB_UEFI_CAPSULE are processed.
  If there is no EFI_HOB_UEFI_CAPSULE, this routine does nothing.

  This routine should be called twice in BDS.
  1) The first call must be before EndOfDxe. The system capsules is processed.
     If device capsule FMP protocols are exposted at this time and device FMP
     capsule has zero EmbeddedDriverCount, the device capsules are processed.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule and
     all capsules are processed.
     If not all capsules are processed, reset will be defered to second call.

  2) The second call must be after EndOfDxe and after ConnectAll, so that all
     device capsule FMP protocols are exposed.
     The system capsules are skipped. If the device capsules are NOT processed
     in first call, they are processed here.
     Each individual capsule result is recorded in capsule record variable.
     System may reset in this function, if reset is required by capsule
     processed in first call and second call.

  @retval EFI_SUCCESS             There is no error when processing capsules.
  @retval EFI_OUT_OF_RESOURCES    No enough resource to process capsules.

**/
EFI_STATUS
EFIAPI
ProcessCapsules (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This routine is called to check if CapsuleOnDisk flag in OsIndications Variable
  is enabled.

  @retval TRUE     Flag is enabled
  @retval FALSE    Flag is not enabled

**/
BOOLEAN
EFIAPI
CoDCheckCapsuleOnDiskFlag (
  VOID
  )
{
  return FALSE;
}

/**
  This routine is called to clear CapsuleOnDisk flags including OsIndications and BootNext variable.

  @retval EFI_SUCCESS   All Capsule On Disk flags are cleared

**/
EFI_STATUS
EFIAPI
CoDClearCapsuleOnDiskFlag (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Relocate Capsule on Disk from EFI system partition.

  Two solution to deliver Capsule On Disk:
  Solution A: If PcdCapsuleInRamSupport is enabled, relocate Capsule On Disk to memory and call UpdateCapsule().
  Solution B: If PcdCapsuleInRamSupport is disabled, relocate Capsule On Disk to a platform-specific NV storage
  device with BlockIo protocol.

  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  Side Effects:
    Capsule Delivery Supported Flag in OsIndication variable and BootNext variable will be cleared.
    Solution B: Content corruption. Block IO write directly touches low level write. Orignal partitions, file
  systems of the relocation device will be corrupted.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Capsule on Disk images are successfully relocated.

**/
EFI_STATUS
EFIAPI
CoDRelocateCapsule (
  UINTN  MaxRetry
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Remove the temp file from the root of EFI System Partition.
  Device enumeration like USB costs time, user can input MaxRetry to tell function to retry.
  Function will stall 100ms between each retry.

  @param[in]    MaxRetry             Max Connection Retry. Stall 100ms between each connection try to ensure
                                     devices like USB can get enumerated. Input 0 means no retry.

  @retval EFI_SUCCESS   Remove the temp file successfully.

**/
EFI_STATUS
EFIAPI
CoDRemoveTempFile (
  UINTN  MaxRetry
  )
{
  return EFI_UNSUPPORTED;
}
