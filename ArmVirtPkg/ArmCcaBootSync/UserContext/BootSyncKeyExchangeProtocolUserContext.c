/** @file
  Boot Sync Key Exchange protocol interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiBaseType.h>

#include "Include/BootSyncProtocol.h"
#include "Include/BootSyncProtocolDefines.h"
#include "Include/BootSyncSession.h"

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
EFI_STATUS
EFIAPI
SendKeyXchgResp (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS               Status;
  UINTN                    PayloadSize;
  UINT8                    *PubKeyPem;
  UINTN                    PubKeyPemSize;
  UINT8                    *PemData;
  BOOT_SYNC_KEY_XCHG_RESP  *KeResp;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Generate a ECDH Key
  Status = ArmCcaBootSyncCryptoGenerateKey (&SecChannel->SessionCtx);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 2: Get the ECDHpub in PEM format
  Status = ArmCcaBootSyncCryptoGetPublicKey (
             SecChannel->SessionCtx,
             &PubKeyPem,
             &PubKeyPemSize
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Step 3: Allocate memory for the key exchange response message.
  PayloadSize = sizeof (BOOT_SYNC_KEY_XCHG_RESP) + PubKeyPemSize;
  KeResp      = AllocateZeroPool (PayloadSize);
  if (KeResp == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler1;
  }

  // Step 4: Populate the Key Exchange Request.
  CopyGuid (&KeResp->Header.Name, &gArmBootSyncKeyXchgRespGuid);
  KeResp->Header.Length = PayloadSize;
  KeResp->Version       = ARM_CCA_BIB_PROTOCOL_VERSION;

  KeResp->PemdataLen = PubKeyPemSize;

  // Step 5: Copy the Public Key PEM data.
  PemData = (UINT8 *)(KeResp + 1);
  CopyMem (PemData, PubKeyPem, PubKeyPemSize);

  // Step 6: Send the Key Exchange Request.
  Status = SecureChannelSendMessage (SecChannel, (BOOT_SYNC_GUID_BLOB *)KeResp);
  ASSERT_EFI_ERROR (Status);

  // Step 7: The Key exchange request has been sent so free
  //         the associated memory.
  FreePool (KeResp);

  // Step 8: Free the PEM Public Key data.
  FreePool (PubKeyPem);

  return Status;

exit_handler1:
  FreePool (PubKeyPem);

exit_handler:
  if (SecChannel->SessionCtx != NULL) {
    ArmCcaBootSyncCryptoDeleteKey (SecChannel->SessionCtx);
    SecChannel->SessionCtx = NULL;
  }

  return Status;
}

/**
  Establish a secure channel for communication.

  @param[in]  SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error was detected.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
EstablishSecureChannel (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  BOOT_SYNC_KEY_XCHG_REQ  *KeyXchgReq;
  UINT8                   *PeerPubKeyPem;
  UINTN                   PeerPubKeyPemSize;

  // Step 1: Open a Session with the Guest.
  Status = BootSyncSessionOpen (SecChannel);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to open a RHI Session. Status = %r\n",
      Status
      ));
    ASSERT (FALSE);
    return Status;
  }

  // Step 2: Read the header
  Status = SecureChannelGetMessage (
             SecChannel,
             (BOOT_SYNC_GUID_BLOB **)&KeyXchgReq
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "Request GUID = %g\n", &KeyXchgReq->Header.Name));
  DEBUG ((DEBUG_INFO, "Expected GUID = %g\n", &gArmBootSyncKeyXchgReqGuid));

  // Step 3: Validate the KeyXchg Request data.
  if ((!CompareGuid (&KeyXchgReq->Header.Name, &gArmBootSyncKeyXchgReqGuid) ||
       (KeyXchgReq->Header.Length < sizeof (BOOT_SYNC_KEY_XCHG_REQ) +
        KeyXchgReq->PemdataLen)))
  {
    Status = EFI_PROTOCOL_ERROR;
    ASSERT (FALSE);
    goto exit_handler1;
  }

  // Step 4: Copy Iv and Salt data
  CopyMem (
    SecChannel->SaltKeyBinding,
    KeyXchgReq->SaltKeyBinding,
    SALT_SIZE
    );
  CopyMem (
    SecChannel->SaltKeyEncryption,
    KeyXchgReq->SaltKeyEncryption,
    SALT_SIZE
    );

  // Step 5: Send the Key Xchg Response
  Status = SendKeyXchgResp (SecChannel);
  if (EFI_ERROR (Status)) {
    goto exit_handler1;
  }

  // Step 6: Compute Common Key
  PeerPubKeyPem     = (UINT8 *)(KeyXchgReq + 1);
  PeerPubKeyPemSize = KeyXchgReq->PemdataLen;
  Status            = SecureChannelComputeCommonKey (
                        SecChannel,
                        PeerPubKeyPem,
                        PeerPubKeyPemSize
                        );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler1;
  }

  // Step 7: Derive Keys
  Status = SecureChannelDeriveKeys (SecChannel);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler2;
  }

  // Step 8: Update the session state to connected
  SecChannel->ProtocolStatus.SessionState = ConnectionEstablished;

  // Step 9: Free the Key Xchg Request
  FreePool (KeyXchgReq);
  return Status;

exit_handler2:
  Status1 = SecureChannelDeleteKeys (SecChannel);
  ASSERT_EFI_ERROR (Status1);

exit_handler1:
  FreePool (KeyXchgReq);

exit_handler:
  // Send the FIN message to indicate an error.
  Status1 = SendFin (SecChannel, BOOT_SYNC_COMM_END_PROTOCOL_ERROR);
  ASSERT_EFI_ERROR (Status1);

  Status1 = BootSyncSessionClose (SecChannel);
  ASSERT_EFI_ERROR (Status1);

  SecChannel->ProtocolStatus.SessionState = UnConnected;
  return Status;
}

/**
  Terminate the secure session.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
TerminateSecureChannel (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  Status1;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  // Step 1: Close the secure channel
  Status1 = BootSyncSessionClose (SecChannel);
  if (EFI_ERROR (Status1)) {
    Status = Status1;
    ASSERT (FALSE);
  }

  // Step 2: Delete the secure channel
  Status1 = SecureChannelDeleteKeys (SecChannel);
  if (EFI_ERROR (Status1)) {
    Status = Status1;
    ASSERT (FALSE);
  }

  return Status;
}
