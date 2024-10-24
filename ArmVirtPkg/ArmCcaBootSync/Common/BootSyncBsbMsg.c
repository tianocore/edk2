/** @file
  Interfaces to send/receive encrypted messages over a Secure Channel.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RngLib.h>

#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"

/**
  Initialise the initial vector to be used for cryptographic operations.

  @param[in, out] Iv    Pointer to the buffer to store the initial vector.
**/
STATIC
EFI_STATUS
EFIAPI
InitIv (
  IN  SECURE_CHANNEL  *SecChannel,
  OUT UINT8           *Iv
  )
{
  if ((SecChannel == NULL) || (Iv == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Iv, IV_SIZE);

  SecChannel->IvSequenceNo++;
  if (SecChannel->IvSequenceNo == MAX_UINT64) {
    SecChannel->IvSequenceNo = 0;
    // Generate a new prefix
    if (!GetRandomNumber32 (&SecChannel->IvPrefix)) {
      return EFI_ABORTED;
    }
  }

  // Copy the prefix
  CopyMem (Iv, &SecChannel->IvPrefix, sizeof (UINT32));
  CopyMem (&Iv[4], &SecChannel->IvSequenceNo, sizeof (UINT64));
  return EFI_SUCCESS;
}

/**
  Encrypt and send a BSB message over the secure channel.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Bsb           Pointer to the BSB message to send.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendEncryptBsbMessage (
  IN SECURE_CHANNEL        *SecChannel,
  IN BOOT_SYNC_BSB_HEADER  *Bsb
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   Result;
  UINTN                     BsbPayloadSize;
  UINTN                     EncPayloadSize;
  UINT8                     *EncData;
  UINTN                     EncDataSize;
  BOOT_SYNC_ENCRYPTED_DATA  *EncMsg;

  if ((SecChannel == NULL) || (Bsb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DBG_PRINT_MSGINFO (&Bsb->Header.Name, TRUE);
  DBG_DUMP_RAW ("Send Data", (UINT8 *)Bsb, Bsb->Header.Length);

  // Step 1: Compute the encrypted payload size.
  BsbPayloadSize = Bsb->Header.Length;
  EncPayloadSize = sizeof (BOOT_SYNC_ENCRYPTED_DATA) + BsbPayloadSize;

  EncMsg = AllocateZeroPool (EncPayloadSize);
  if (EncMsg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 2: Update the encrypted data message header.
  CopyGuid (&EncMsg->Header.Name, &gArmBootSyncKeyEncData);
  EncMsg->Header.Length = EncPayloadSize;
  EncMsg->EncDataLen    = BsbPayloadSize;

  Status = InitIv (SecChannel, EncMsg->Iv);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  // Step 3: Encrypt the BSB payload.
  EncData     = (UINT8 *)(EncMsg + 1);
  EncDataSize = BsbPayloadSize;
  Result      = ArmCcaBootSyncCryptoEncrypt (
                  SecChannel->Ke,                             // Key
                  ENCRYPTION_KEY_SIZE,                        // Key Size
                  EncMsg->Iv,                                 // IV
                  IV_SIZE,                                    // IV Size
                  (UINT8 *)EncMsg,                            // Adata
                  OFFSET_OF (BOOT_SYNC_ENCRYPTED_DATA, Tag),  // AdataSize
                  (UINT8 *)Bsb,                               // DataIn
                  BsbPayloadSize,                             // DataInSize
                  EncMsg->Tag,                                // Tag Out
                  TAG_SIZE,                                   // Tag Size
                  EncData,                                    // DataOut
                  &EncDataSize                                // DataOutSize
                  );
  if (!Result) {
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  // Step 4: Check that the Encrypted Data Size is the same as we
  // populated in step 2, i.e. DataOutSize == DataInSize
  if (EncDataSize != BsbPayloadSize) {
    Status = EFI_ABORTED;
    ASSERT (FALSE);
    goto exit_handler;
  }

  // Step 5: Send the Encrypted message.
  Status = SecureChannelSendMessage (SecChannel, (BOOT_SYNC_GUID_BLOB *)EncMsg);
  ASSERT_EFI_ERROR (Status);

  // Step 6: Free any resources that were allocated.
exit_handler:
  FreePool (EncMsg);
  return Status;
}

/**
  Decrypt the BSB Message.

  @param[in]  SecChannel  Pointer to the secure channel.
  @param[in]  EncBsbMsg   Pointer to the encrypted message.
  @param[out] DecMsg      Pointer to the decrypted BSB message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
DecryptBsbMsg (
  IN  SECURE_CHANNEL            *SecChannel,
  IN  BOOT_SYNC_ENCRYPTED_DATA  *EncBsbMsg,
  OUT BOOT_SYNC_BSB_HEADER      **DecMsg
  )
{
  BOOLEAN     Result;
  EFI_STATUS  Status;
  UINT8       *EncData;
  UINT8       *BsbData;
  UINTN       BsbDataLen;

  if ((SecChannel == NULL) ||
      (EncBsbMsg == NULL) ||
      (DecMsg == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((EncBsbMsg->Header.Length < sizeof (BOOT_SYNC_ENCRYPTED_DATA)) ||
      (EncBsbMsg->Header.Length < (sizeof (BOOT_SYNC_ENCRYPTED_DATA) +
                                   EncBsbMsg->EncDataLen)))
  {
    return EFI_INVALID_PARAMETER;
  }

  BsbDataLen = EncBsbMsg->EncDataLen;
  BsbData    = AllocateZeroPool (BsbDataLen);
  if (BsbData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EncData = (UINT8 *)(EncBsbMsg + 1);
  Result  = ArmCcaBootSyncCryptoDecrypt (
              SecChannel->Ke,                            // Key
              ENCRYPTION_KEY_SIZE,                       // KeySize
              EncBsbMsg->Iv,                             // Iv
              IV_SIZE,                                   // IvSize
              (UINT8 *)EncBsbMsg,                        // AData
              OFFSET_OF (BOOT_SYNC_ENCRYPTED_DATA, Tag), // ADataSize
              EncData,                                   // DataIn
              EncBsbMsg->EncDataLen,                     // DataInSize
              EncBsbMsg->Tag,                            // Tag
              TAG_SIZE,                                  // TagSize
              BsbData,                                   // DataOut
              &BsbDataLen                                // DataOutSize
              );
  if (!Result) {
    Status = EFI_ABORTED;
    goto error_handler;
  }

  // Validate that the decrypted data matches the length in the header.
  if (((BOOT_SYNC_BSB_HEADER *)BsbData)->Header.Length != BsbDataLen) {
    Status = EFI_ABORTED;
    goto error_handler;
  }

  *DecMsg = (BOOT_SYNC_BSB_HEADER *)BsbData;
  return EFI_SUCCESS;

error_handler:
  FreePool (BsbData);
  return Status;
}

/**
  Receive a message and decrypt if encrypted.

  Note: Caller is expected to free the returned Message

  @param[in]  SecChannel  Pointer to the secure channel.
  @param[out] Message     Pointer to the received message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error was detected, resulting in
                                  the connection being terminated.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
ReceiveDecryptBsbMsg (
  IN  SECURE_CHANNEL       *SecChannel,
  OUT BOOT_SYNC_GUID_BLOB  **Message
  )
{
  EFI_STATUS           Status;
  BOOT_SYNC_GUID_BLOB  *Msg;

  if ((SecChannel == NULL) || (Message == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Message = NULL;

  Status = SecureChannelGetMessage (SecChannel, &Msg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check if the message received is an Encrypted Data message.
  if (!CompareGuid (&Msg->Name, &gArmBootSyncKeyEncData)) {
    // The message is a protocol communication message
    if (CompareGuid (&Msg->Name, &gArmBootSyncFinGuid)) {
      DEBUG ((
        DEBUG_ERROR,
        "INFO: Communication termination requested (FIN), " \
        "MSG GUID = %g, Reason = 0x%x\n",
        &Msg->Name,
        ((BOOT_SYNC_FIN *)Msg)->Reason
        ));
    } else if (CompareGuid (&Msg->Name, &gArmBootSyncNackGuid)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: NACK - Communication refused and terminated (NACK), " \
        "MSG GUID = %g, Reason = 0x%x\n",
        &Msg->Name,
        ((BOOT_SYNC_NACK *)Msg)->Reason
        ));
    } else {
      // Invalid message received over channel.
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Communication terminated unexpectedly (PROTOCOL ERROR), " \
        "MSG GUID = %g, Invalid Data received.\n",
        &Msg->Name
        ));
      Status = EFI_PROTOCOL_ERROR;
      ASSERT (FALSE);

      // Free the message data
      FreePool (Msg);
      *Message = NULL;
      goto exit_handler;
    }

    *Message = Msg;
  } else {
    // The Message is an encrypted BSB message, so decrypt it.
    Status = DecryptBsbMsg (
               SecChannel,
               (BOOT_SYNC_ENCRYPTED_DATA *)Msg,
               (BOOT_SYNC_BSB_HEADER **)Message
               );
    ASSERT_EFI_ERROR (Status);

    // Free the encrypted message data
    FreePool (Msg);
  }

  if (*Message != NULL) {
    DBG_PRINT_MSGINFO (&(*Message)->Name, FALSE);
    DBG_DUMP_RAW ("Recv Data", (UINT8 *)(*Message), (*Message)->Length);
  }

exit_handler:
  return Status;
}
