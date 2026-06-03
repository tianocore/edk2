/** @file
  Boot Sync Key Exchange protocol interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RngLib.h>

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

/**
  Initialise the initial vector params to be used for cryptographic operations.

  @param[in, out] Iv    Pointer to the buffer to store the initial vector.
**/
STATIC
EFI_STATUS
EFIAPI
InitIvParams (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  // Generate a 32 bit random number as the IV Prefix.
  if (!GetRandomNumber32 (&SecChannel->IvPrefix)) {
    return EFI_ABORTED;
  }

  // Initialise the IvSequence to zero.
  SecChannel->IvSequenceNo = 0;
  return EFI_SUCCESS;
}

/**
  Initialise the salt to be used for cryptographic operations.

  @param[in, out] Salt    Pointer to the buffer to store the salt.
**/
STATIC
EFI_STATUS
EFIAPI
InitSalt (
  IN OUT UINT8  *Salt
  )
{
  // This code assumes Salt Size of 32 bytes.
  // So assert if that is not the case.
  STATIC_ASSERT (SALT_SIZE == 32);

  ZeroMem (Salt, SALT_SIZE);
  // Generate a 256 bit random number as an inital seed.
  if (!GetRandomNumber128 ((UINT64 *)Salt)) {
    return EFI_ABORTED;
  }

  if (!GetRandomNumber128 ((UINT64 *)&Salt[16])) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Send a key exchange request to establish a secure session.

  @param[in]      SecChannel    Pointer to the secure channel.
  @param[in, out] Sid           Pointer retrive the Session ID.
  @param[out]     Cookie        Pointer retrive the cookie.
  @param[out]     RespSize      Pointer retrive the response data size.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
SendKeyXchgReq (
  IN  SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS              Status;
  UINTN                   PayloadSize;
  UINT8                   *PubKeyPem;
  UINTN                   PubKeyPemSize;
  UINT8                   *PemData;
  BOOT_SYNC_KEY_XCHG_REQ  *KeReq;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Initialise channel Iv and Salt data.
  Status = InitIvParams (SecChannel);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InitSalt (SecChannel->SaltKeyBinding);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = InitSalt (SecChannel->SaltKeyEncryption);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 2: Generate a ECDH Key.
  Status = ArmCcaBootSyncCryptoGenerateKey (&SecChannel->SessionCtx);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 3: Get the ECDHpub in PEM format.
  Status = ArmCcaBootSyncCryptoGetPublicKey (
             SecChannel->SessionCtx,
             &PubKeyPem,
             &PubKeyPemSize
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Step 4: Allocate memory for the key exchange request message.
  PayloadSize = sizeof (BOOT_SYNC_KEY_XCHG_REQ) + PubKeyPemSize;
  KeReq       = AllocateZeroPool (PayloadSize);
  if (KeReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler1;
  }

  // Step 5: Populate the Key Exchange Request.
  CopyGuid (&KeReq->Header.Name, &gArmBootSyncKeyXchgReqGuid);
  KeReq->Header.Length = PayloadSize;
  KeReq->Version       = ARM_CCA_BIB_PROTOCOL_VERSION;

  CopyMem (KeReq->SaltKeyBinding, SecChannel->SaltKeyBinding, SALT_SIZE);
  CopyMem (
    KeReq->SaltKeyEncryption,
    SecChannel->SaltKeyEncryption,
    SALT_SIZE
    );

  KeReq->PemdataLen = PubKeyPemSize;

  // Step 6: Copy the Public Key PEM data.
  PemData = (UINT8 *)(KeReq + 1);
  CopyMem (PemData, PubKeyPem, PubKeyPemSize);

  // Step 7: Send the Key Exchange Request.
  Status = SecureChannelSendMessage (SecChannel, (BOOT_SYNC_GUID_BLOB *)KeReq);
  ASSERT_EFI_ERROR (Status);

  // Step 8: The Key exchange request has been sent so free
  //         the associated memory.
  FreePool (KeReq);

exit_handler1:
  // Step 9: Free the PEM Public Key data.
  FreePool (PubKeyPem);

  return Status;

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
  EFI_STATUS  Status;
  EFI_STATUS  Status1;

  BOOT_SYNC_KEY_XCHG_RESP  *KeResp;
  UINT8                    *PeerPubKeyPem;
  UINTN                    PeerPubKeyPemSize;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Open a RHI Host Session.
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

  // Step 2: Send the Key Exchange Request message.
  Status = SendKeyXchgReq (SecChannel);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Step 3: Get the Key Exchange Response message.
  Status = SecureChannelGetMessage (
             SecChannel,
             (BOOT_SYNC_GUID_BLOB **)&KeResp
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  DEBUG ((DEBUG_INFO, "Response GUID = %g\n", &KeResp->Header.Name));
  DEBUG ((DEBUG_INFO, "Expected GUID = %g\n", &gArmBootSyncKeyXchgRespGuid));

  // Step 4: Validate the KeyXchg Response data.
  if ((!CompareGuid (&KeResp->Header.Name, &gArmBootSyncKeyXchgRespGuid)) ||
      (KeResp->Header.Length < (sizeof (BOOT_SYNC_KEY_XCHG_RESP) +
                                KeResp->PemdataLen)))
  {
    Status = EFI_PROTOCOL_ERROR;
    ASSERT (FALSE);
    goto exit_handler1;
  }

  PeerPubKeyPem     = (UINT8 *)(KeResp + 1);
  PeerPubKeyPemSize = KeResp->PemdataLen;

  // Step 5: Compute the common key.
  Status = SecureChannelComputeCommonKey (
             SecChannel,
             PeerPubKeyPem,
             PeerPubKeyPemSize
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler1;
  }

  // Step 6: Derive Keys
  Status = SecureChannelDeriveKeys (SecChannel);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler2;
  }

  // Step 8: Update the session state to connected
  SecChannel->ProtocolStatus.SessionState = ConnectionEstablished;

  // Step 9: Free the Key Xchg Response
  FreePool (KeResp);
  return Status;

exit_handler2:
  Status1 = SecureChannelDeleteKeys (SecChannel);
  ASSERT_EFI_ERROR (Status1);

exit_handler1:
  FreePool (KeResp);

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
