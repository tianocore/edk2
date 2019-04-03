/** @file

  This library class defines a set of interfaces for how to process capsule image updates.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CAPSULE_LIB_H__
#define __CAPSULE_LIB_H__

/**
  The firmware checks whether the capsule image is supported
  by the CapsuleGuid in CapsuleHeader or if there is other specific information in
  the capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Pointer to the UEFI capsule image to be checked.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  );

/**
  The firmware-specific implementation processes the capsule image
  if it recognized the format of this capsule image.

  Caution: This function may receive untrusted input.

  @param  CapsuleHeader    Pointer to the UEFI capsule image to be processed.

  @retval EFI_SUCESS       Capsule Image processed successfully.
  @retval EFI_UNSUPPORTED  Capsule image is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  );

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
  );

#endif
