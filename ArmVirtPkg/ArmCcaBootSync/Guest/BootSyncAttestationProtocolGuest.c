/** @file
  Arm CCA Boot Sync Attestation protocol guest interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Rsi or RSI   - Realm Service Interface
    - BS           - Boot Sync

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncProtocol.h"
#include "Include/BootSyncSecureChannel.h"

/*
  Attestation and verification protocol

           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                /            Secure session           \
                \            Established              /
                |                                     |
                |<---------Attestation Request--------|
                |          (Attestation Report)       |
                |                                     |
                |---------Attestation Response------->|
                |         (Attestation Result)        |
                |                                     |
                /            Other Protocol           \
                \            Communication            /
                |                                     |

*/

/*

  Attestation Request message
  ---------------------------

  +------
  |  BOOT_SYNC_ENCRYPTED_DATA
  |  .Header.Name = gArmBootSyncKeyEncData
  |  .Tag[]
  |  .EncDataLen = Total length
  +-+----
    |  BOOT_SYNC_BSB_HEADER
    |  .Header.Name = gArmBootSyncAttReqGuid
    |  .ElementCount = Count of BSB elements, i.e. 1
    +-+-----------
      |  BOOT_SYNC_BSB_ELEMENT
      |  .Header.Name = gArmBootSyncAttReport
      |  .Length = sizeof (BOOT_SYNC_BSB_ELEMENT) +
      |             Att Report Length
      |  .Data[] = Attestation Report Data.
      +-----------

  Attestation Response message
  ---------------------------

  +------
  |  BOOT_SYNC_ENCRYPTED_DATA
  |  .Header.Name = gArmBootSyncKeyEncData
  |  .Tag[]
  |  .EncDataLen = Total length
  +-+----
    |  BOOT_SYNC_BSB_HEADER
    |  .Header.Name = gArmBootSyncAttRespGuid
    |  .ElementCount = Count of BSB elements, i.e. 1
    +-+-----------
      |  BOOT_SYNC_BSB_ELEMENT
      |  .Header.Name = gArmBootSyncAttResult
      |  .Length = sizeof (BOOT_SYNC_BSB_ELEMENT) +
      |             Att Report Length
      |  .Data[] = Attestation Result Data.
      +-----------

*/

/**
  Send an attestation request to perform remote attestation.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[out] Cookie        Pointer retrive the cookie.
  @param[out] RespSize      Pointer retrive the response data size.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
SendAttestationReq (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS             Status;
  UINT8                  *TokenBuffer;
  UINT64                 TokenBufferSize;
  UINTN                  BsbPayloadSize;
  UINT32                 ElementCount;
  BOOT_SYNC_BSB_HEADER   *AttReq;
  BOOT_SYNC_BSB_ELEMENT  *BsbElement;
  UINT8                  *Data;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1.a: Retrive the attestation token.
  DEBUG ((DEBUG_INFO, "Get Attestation Token.\n"));
  Status = ArmCcaRsiGetAttestationToken (
             SecChannel->Kb,
             (BINDING_KEY_SIZE << 3),
             &TokenBuffer,
             &TokenBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get Attestation Token.\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Attestation Token Read Successfully.\n"));
  DBG_DUMP_RAW ("Token", (UINT8 *)TokenBuffer, TokenBufferSize);

  // Compute the payload size required for the attestation token
  BsbPayloadSize  = sizeof (BOOT_SYNC_BSB_HEADER);
  BsbPayloadSize += (sizeof (BOOT_SYNC_BSB_ELEMENT) + TokenBufferSize);
  ElementCount    = 1;

  // Step 2: Allocate memory for the Attestation Request message.
  AttReq = AllocateZeroPool (BsbPayloadSize);
  if (AttReq == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_handler;
  }

  // Step 3: Poulate the Attestation Request message BSB header.
  CopyGuid (&AttReq->Header.Name, &gArmBootSyncAttReqGuid);
  AttReq->Header.Length = BsbPayloadSize;
  AttReq->ElementCount  = ElementCount;

  // Step 4.a: Populate the Attestation Report BSB Element
  BsbElement = (BOOT_SYNC_BSB_ELEMENT *)(AttReq + 1);
  CopyGuid (&BsbElement->Header.Name, &gArmBootSyncAttReport);
  BsbElement->Header.Length = sizeof (BOOT_SYNC_BSB_ELEMENT) + TokenBufferSize;
  Data                      = (UINT8 *)(BsbElement + 1);
  CopyMem (Data, TokenBuffer, TokenBufferSize);

  // Step 5: Encrypt and send the Attestation Request message
  Status = SendEncryptBsbMessage (SecChannel, AttReq);
  ASSERT_EFI_ERROR (Status);

  // Free any resources that were allocated.
  FreePool (AttReq);

error_handler:
  // Free the attestation token.
  ArmCcaRsiFreeAttestationToken (TokenBuffer, TokenBufferSize);
  return Status;
}

/**
  Perform attestation over the secure channel.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error was detected.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncPerformAttestation (
  IN SECURE_CHANNEL  *SecChannel
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    Status1;
  BOOT_SYNC_BSB_HEADER          *AttResp;
  BOOT_SYNC_ATTESTATION_RESULT  *AttResult;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Send the attestation request message.
  Status = SendAttestationReq (SecChannel);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 2: Get the attestation response message.
  Status = ReceiveDecryptBsbMsg (
             SecChannel,
             (BOOT_SYNC_GUID_BLOB **)&AttResp
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (CompareGuid (&AttResp->Header.Name, &gArmBootSyncFinGuid) ||
      CompareGuid (&AttResp->Header.Name, &gArmBootSyncNackGuid))
  {
    Status = EFI_PROTOCOL_ERROR;
    goto exit_handler;
  }

  // Step 3: Check the Attestation response message.
  if (!CompareGuid (&AttResp->Header.Name, &gArmBootSyncAttRespGuid)) {
    DEBUG ((DEBUG_ERROR, "Invalid Response = %g\n", &AttResp->Header.Name));
    Status = EFI_ABORTED;
    ASSERT (FALSE);
    goto exit_handler;
  }

  DBG_DUMP_RAW ("ATT RESPONSE", (UINT8 *)AttResp, AttResp->Header.Length);

  // Step 4: Get the Attestation Result BSB Element
  Status = BsbGetElement (
             AttResp,
             &gArmBootSyncAttResult,
             (BOOT_SYNC_BSB_ELEMENT **)&AttResult
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    goto exit_handler;
  }

  if (AttResult->ElementHdr.Header.Length < sizeof (BOOT_SYNC_ATTESTATION_RESULT)) {
    Status = EFI_PROTOCOL_ERROR;
    goto exit_handler;
  }

  // Step 5: Check the attestation result and return failure if the
  // attestation was not successful.
  if (AttResult->Result != ATTESTATION_RESULT_VERIFY_SUCCESS) {
    Status = EFI_ABORTED;
    goto exit_handler;
  }

exit_handler:
  if (EFI_ERROR (Status)) {
    // Send the FIN message to indicate an error.
    Status1 = SendFin (SecChannel, BOOT_SYNC_COMM_END_PROTOCOL_ERROR);
    ASSERT_EFI_ERROR (Status1);
  }

  FreePool (AttResp);
  return Status;
}
