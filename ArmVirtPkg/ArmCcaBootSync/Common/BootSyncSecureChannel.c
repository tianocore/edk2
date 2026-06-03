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
#include <Library/BaseMemoryLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncSession.h"

/**
  A helper function to update the rolling hash with the app info for the
  derived key(s) and the messages in bytes.

  @param[in] SecChannel   Pointer to the secure channel.
  @param[in] Data         Pointer to the data for extending the hash.
  @param[in] DataLen      Length of the data buffer.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
SecureChannelUpdateRollingHash (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT8           *Data,
  IN UINTN           DataLen
  )
{
  BOOLEAN  Result;
  UINT8    InterimHash[ROLLING_MSG_HASH_SIZE*2];

  if ((SecChannel == NULL) ||
      (Data == NULL) ||
      (DataLen == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Copy the RmHash as the first part
  CopyMem (InterimHash, SecChannel->RmHash, ROLLING_MSG_HASH_SIZE);

  // Compute the hash of the data in the second part
  Result = Sha256HashAll (Data, DataLen, &InterimHash[ROLLING_MSG_HASH_SIZE]);
  if (!Result) {
    return EFI_ABORTED;
  }

  // Compute the FinalHash = Hash (RmHash+DataHash)
  Result = Sha256HashAll (
             InterimHash,
             sizeof (InterimHash),
             SecChannel->RmHash
             );
  if (!Result) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

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
  Derive Binding Key and Encryption Key from the Common Key.

  @param[in] SecChannel         Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelDeriveKeys (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Result;
  UINT8       *AppInfo;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AppInfo = (UINT8 *)PcdGetPtr (PcdBootSyncAppInfoBindingKeyString);
  // Extend the BindingKey App Info to the RmHash
  Status = SecureChannelUpdateRollingHash (
             SecChannel,
             AppInfo,
             AsciiStrSize ((CHAR8 *)AppInfo)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Derive Binding Key
  Result = ArmCcaBootSyncCryptoDeriveKey (
             SecChannel->SessionCtx,
             SecChannel->SaltKeyBinding,
             SALT_SIZE,
             SecChannel->RmHash,
             sizeof (SecChannel->RmHash),
             SecChannel->Kb,
             BINDING_KEY_SIZE
             );
  if (!Result) {
    return EFI_ABORTED;
  }

  AppInfo = (UINT8 *)PcdGetPtr (PcdBootSyncAppInfoEncryptionKeyString);
  // Extend the BindingKey App Info to the RmHash
  Status = SecureChannelUpdateRollingHash (
             SecChannel,
             AppInfo,
             AsciiStrSize ((CHAR8 *)AppInfo)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Derive Encryption Key
  Result = ArmCcaBootSyncCryptoDeriveKey (
             SecChannel->SessionCtx,
             SecChannel->SaltKeyEncryption,
             SALT_SIZE,
             SecChannel->RmHash,
             sizeof (SecChannel->RmHash),
             SecChannel->Ke,
             ENCRYPTION_KEY_SIZE
             );
  if (!Result) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
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

  // Scrub the Rolling hash of the messages.
  ZeroMem (SecChannel->RmHash, ROLLING_MSG_HASH_SIZE);

  // Scrub the Binding Key Salt.
  ZeroMem (SecChannel->SaltKeyBinding, SALT_SIZE);

  // Scrub the Encryption Key Salt.
  ZeroMem (SecChannel->SaltKeyEncryption, SALT_SIZE);

  // Scrub the Binding Key.
  ZeroMem (SecChannel->Kb, BINDING_KEY_SIZE);

  // Scrub the Encryption Key.
  ZeroMem (SecChannel->Ke, ENCRYPTION_KEY_SIZE);

  // Scrub the RPV.
  ZeroMem (SecChannel->Rpv, ARM_CCA_REALM_CFG_RPV_SIZE);

  return Status;
}

/**
  Send a message over an active session and update the rolling hash.

  Note: This function does not encrypt the data.

  @param[in]      SecChannel    Pointer to the secure channel.
  @param[in]      Msg           Pointer to the message data to send.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelSendMessage (
  IN  SECURE_CHANNEL       *SecChannel,
  IN  BOOT_SYNC_GUID_BLOB  *Msg
  )
{
  EFI_STATUS  Status;

  if ((SecChannel == NULL) ||
      (Msg == NULL) ||
      (Msg->Length == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_INFO,
    "BootSyncSessionSendData: Msg = 0x%p, Len = 0x%x\n",
    Msg,
    Msg->Length
    ));

  Status = BootSyncSessionSendData (SecChannel, (UINT8 *)Msg, Msg->Length);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SecureChannelUpdateRollingHash (
             SecChannel,
             (UINT8 *)Msg,
             Msg->Length
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Get the response message and update the rolling hash.

  Note: This function does not decypt the data.

  @param[in]  SecChannel  Pointer to the secure channel.
  @param[out] Resp        Pointer to a buffer to store the response message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SecureChannelGetMessage (
  IN SECURE_CHANNEL        *SecChannel,
  OUT BOOT_SYNC_GUID_BLOB  **Resp
  )
{
  EFI_STATUS           Status;
  BOOT_SYNC_GUID_BLOB  *Response;
  UINTN                ResponseSize;
  UINTN                RemainingSize;
  UINT8                *RespData;
  UINT8                *RemainingData;

  if ((SecChannel == NULL) || (Resp == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Allocate memory for retriving the BOOT_SYNC_GUID_BLOB.
  // Note: We always first read the BOOT_SYNC_GUID_BLOB as we do not know
  // the response type. e.g. Even though we expect a KeyXchgResponse
  // we could get a NACK response if the server is busy.
  ResponseSize = sizeof (BOOT_SYNC_GUID_BLOB);
  Response     = AllocateZeroPool (ResponseSize);
  if (Response == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 2: Read the BOOT_SYNC_GUID_BLOB.
  Status = BootSyncSessionReceiveData (
             SecChannel,
             (UINT8 *)Response,
             ResponseSize
             );
  if (EFI_ERROR (Status) || (ResponseSize < sizeof (BOOT_SYNC_GUID_BLOB))) {
    goto exit_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "ResponseSize = %d, Response->Length = %d, MSG {%g}\n",
    ResponseSize,
    Response->Length,
    &Response->Name
    ));

  if (Response->Length < sizeof (BOOT_SYNC_GUID_BLOB)) {
    Status = EFI_INVALID_PARAMETER;
    goto exit_handler;
  }

  if (Response->Length > sizeof (BOOT_SYNC_GUID_BLOB)) {
    // Step 3: Re allocate a buffer large enough to store the response
    RespData = ReallocatePool (ResponseSize, Response->Length, Response);
    if (RespData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exit_handler;
    }

    // Retarget the Response pointer after relloc as the old pointer
    // is no longer valid.
    Response = (BOOT_SYNC_GUID_BLOB *)RespData;

    // Store the Response Size before relocating the buffer
    ResponseSize = Response->Length;

    // More data to be received.
    RemainingSize = ResponseSize - sizeof (BOOT_SYNC_GUID_BLOB);
    RemainingData = RespData + sizeof (BOOT_SYNC_GUID_BLOB);

    DEBUG ((
      DEBUG_INFO,
      "ResponseSize = %d, RemainingSize = %d\n",
      ResponseSize,
      RemainingSize
      ));

    // Step 4: Read the remaining payload.
    Status = BootSyncSessionReceiveData (
               SecChannel,
               (UINT8 *)RemainingData,
               RemainingSize
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  // Step 5: Update the rolling hash
  Status = SecureChannelUpdateRollingHash (
             SecChannel,
             (UINT8 *)Response,
             ResponseSize
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler;
  }

  *Resp = (BOOT_SYNC_GUID_BLOB *)Response;
  return Status;

exit_handler:
  FreePool (Response);
  return Status;
}
