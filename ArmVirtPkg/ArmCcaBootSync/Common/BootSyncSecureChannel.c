/** @file
  Boot Sync Secure Channel interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

/**
  Boot Sync Key Exchange protocol

           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                |<---------Key Xchg Request-----------|
                |                                     |
                |                                     |
                |---------Key Xchg Response---------->|
  Compute       |                                     |     Compute
  Common        |                                     |     Common
  Key (Kcomm)   |                                     |     Key (Kcomm)
                |                                     |
  Derive        |                                     |     Derive
  Keys (Ke, Kb) |                                     |     Keys (Ke, Kb)
                |                                     |
                /            Other Protocol           \
                \            Communication            /
                |                                     |
                |<----------------FIN-----------------|
  End of        |                                     |     End of
  Communication |                                     |     Communication

                                  OR

                .                                     .
                .                                     .
                .                                     .
                /                                     \
                \                                     /
                |                                     |
                |----------------NACK---------------->|
  End of        |                                     |     End of
  Communication |                                     |     Communication
*/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "Include/BootSyncSecureChannel.h"

/**
  Compute the common key using the peer public key PEM data.

  Note: Common key is freed when SecChannel is deleted.

  @param[in] SecChannel         Pointer to the secure channel.
  @param[in] PeerPubKeyPem      Pointer to the peer public key PEM data.
  @param[in] PeerPubKeyPemSize  Length of the peer public key PEM data.


  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelComputeCommonKey (
  IN  SECURE_CHANNEL  *SecChannel,
  IN  UINT8           *PeerPubKeyPem,
  IN  UINTN           PeerPubKeyPemSize
  )
{
  EFI_STATUS  Status;

  if ((SecChannel == NULL) ||
      (PeerPubKeyPem == NULL) ||
      (PeerPubKeyPemSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = ArmCcaBootSyncCryptoGeneratePeerKey (
             &SecChannel->PeerSessionCtx,
             PeerPubKeyPem,
             PeerPubKeyPemSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Status = ArmCcaBootSyncCryptoGenerateCommonKey (
             SecChannel->SessionCtx,
             SecChannel->PeerSessionCtx
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    ArmCcaBootSyncCryptoDeleteKey (SecChannel->PeerSessionCtx);
    SecChannel->PeerSessionCtx = NULL;
  }

  return Status;
}

/**
  Delete the secure channel keys.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelDeleteKeys (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  Status1;

  Status = EFI_SUCCESS;
  if (SecChannel->SessionCtx != NULL) {
    Status1 = ArmCcaBootSyncCryptoDeleteKey (SecChannel->SessionCtx);
    if (EFI_ERROR (Status1)) {
      Status = Status1;
      ASSERT (FALSE);
    }

    SecChannel->SessionCtx = NULL;
  }

  if (SecChannel->PeerSessionCtx != NULL) {
    Status1 = ArmCcaBootSyncCryptoDeleteKey (SecChannel->PeerSessionCtx);
    if (EFI_ERROR (Status1)) {
      Status = Status1;
      ASSERT (FALSE);
    }

    SecChannel->PeerSessionCtx = NULL;
  }

  return Status;
}
