/** @file
  NULL library that implements runtime capsule update

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/**
  The firmware implements to process the capsule image at runtime

  Caution: This function may receive untrusted input.

  @param[in]  CapsuleHeader         Points to a capsule header.

  @retval EFI_SUCCESS           Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessCaspuleImageAtRuntime (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  return EFI_OUT_OF_RESOURCES;
}
