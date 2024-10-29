/** @file
  Arm CCA Boot Sync Protocol host interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SYNC_PROTOCOL_HOST_H_
#define BOOT_SYNC_PROTOCOL_HOST_H_

/**
  Perform validation of the attestation report.

  @param[in]    SecChannel          Pointer to the secure channel.
  @param[in]    Msg                 Pointer to the received Attestation Request.
  @param[out]   BootSyncCompleted   Was Boot Sync already completed?

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncValidateAttestation (
  IN  SECURE_CHANNEL       *SecChannel,
  IN  BOOT_SYNC_GUID_BLOB  *Msg,
  OUT BOOLEAN              *BootSyncCompleted
  );

#endif // BOOT_SYNC_PROTOCOL_HOST_H_
