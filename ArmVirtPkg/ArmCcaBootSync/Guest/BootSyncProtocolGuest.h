/** @file
  Arm CCA Boot Sync Protocol guest interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SYNC_PROTOCOL_GUEST_H_
#define BOOT_SYNC_PROTOCOL_GUEST_H_

/**
  Perform attestation over the secure channel.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncPerformAttestation (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Get BIB over the secure channel.

  @param[in]    SecChannel  Pointer to the secure channel.
  @param[out]   Pointer to the BIB to be returned.

  Note: The BIB needs to be freed by the caller by calling FreePool ().

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetBib (
  IN SECURE_CHANNEL         *SecChannel,
  OUT BOOT_SYNC_BSB_HEADER  **Bib
  );

#endif // BOOT_SYNC_PROTOCOL_GUEST_H_
