/** @file
  Arm CCA Boot Sync Attestation protocol verification interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SYNC_VERIFICATION_H_
#define BOOT_SYNC_VERIFICATION_H_

/**
  Perform verification of the Attestation Report.

  @param[in]  SecChannel          Pointer to the secure channel.
  @param[in]  AttReq              Pointer to the Attestation request.
  @param[out] AttestationResult   The result of the attestation report
                                  verification.
  @param[out] BootSyncCompleted   Was Boot Sync already completed?

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncPerformVerification (
  IN  SECURE_CHANNEL        *SecChannel,
  IN  BOOT_SYNC_BSB_HEADER  *AttReq,
  OUT UINT64                *AttestationResult,
  OUT BOOLEAN               *BootSyncCompleted
  );

#endif // BOOT_SYNC_VERIFICATION_H_
