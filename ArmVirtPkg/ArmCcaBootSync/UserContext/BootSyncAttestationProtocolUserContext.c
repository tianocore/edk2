/** @file
  Arm CCA Boot Sync Attestation protocol host interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncBsbParser.h"
#include "Include/BootSyncDebug.h"

/*
  Attestation and verification protocol

           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                /            Secure Session           \
                \             Established             /
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
  Send an attestation response message to the guest.

  @param[in]  SecChannel          Pointer to the secure channel.
  @param[in]  AttestationResult   The Attestaion Result.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
SendAttestationResp (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          AttestationResult
  )
{
  EFI_STATUS             Status;
  UINTN                  BsbPayloadSize;
  BOOT_SYNC_BSB_HEADER   *AttResp;
  BOOT_SYNC_BSB_ELEMENT  *BsbElement;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Compute the payload size required for
  // the attestation response message
  BsbPayloadSize  = sizeof (BOOT_SYNC_BSB_HEADER);
  BsbPayloadSize += sizeof (BOOT_SYNC_ATTESTATION_RESULT);

  // Step 2: Allocate memory for the Attestation Response message.
  AttResp = AllocateZeroPool (BsbPayloadSize);
  if (AttResp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 3: Poulate the Attestation Response message BSB header.
  CopyGuid (&AttResp->Header.Name, &gArmBootSyncAttRespGuid);
  AttResp->Header.Length = BsbPayloadSize;
  AttResp->ElementCount  = 1;

  // Step 4: Populate the Attestation Result BSB Element
  BsbElement = (BOOT_SYNC_BSB_ELEMENT *)(AttResp + 1);
  CopyGuid (&BsbElement->Header.Name, &gArmBootSyncAttResult);
  BsbElement->Header.Length                            = sizeof (BOOT_SYNC_ATTESTATION_RESULT);
  ((BOOT_SYNC_ATTESTATION_RESULT *)BsbElement)->Result =
    AttestationResult;

  // Step 5: Encrypt and send the Attestation Request message
  Status = SendEncryptBsbMessage (SecChannel, AttResp);
  ASSERT_EFI_ERROR (Status);

  // Step 6: Free any resources that were allocated.
  FreePool (AttResp);

  return Status;
}

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
  IN SECURE_CHANNEL       *SecChannel,
  IN BOOT_SYNC_GUID_BLOB  *Msg,
  OUT BOOLEAN             *BootSyncCompleted
  )
{
  EFI_STATUS            Status;
  BOOT_SYNC_BSB_HEADER  *AttReq;
  UINT64                AttestationResult;

  if ((SecChannel == NULL) || (Msg == NULL) || (BootSyncCompleted == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Get the Attestation request message.
  AttReq = (BOOT_SYNC_BSB_HEADER *)Msg;

  DBG_DUMP_RAW ("ATT REQ", (UINT8 *)AttReq, AttReq->Header.Length);

  // Step 2: Check the Attestation request message.
  if (!CompareGuid (&AttReq->Header.Name, &gArmBootSyncAttReqGuid)) {
    DEBUG ((DEBUG_ERROR, "Invalid Response = %g\n", &AttReq->Header.Name));
    Status = EFI_ABORTED;
    ASSERT (FALSE);
    return Status;
  }

  // Step 3: Perform Verification
  // Until the verfication support is added always return attestation
  // verification result as successful and Boot Sync already completed as FALSE.
  AttestationResult  = ATTESTATION_RESULT_VERIFY_SUCCESS;
  *BootSyncCompleted = FALSE;

  // Step 4: Update the attestation status in the protocol status
  SecChannel->ProtocolStatus.AttestationState =
    (AttestationResult == ATTESTATION_RESULT_VERIFY_SUCCESS) ?
    AttSuccess :
    AttFailed;

  // Step 5: Send the Attestation Response message
  Status = SendAttestationResp (
             SecChannel,
             AttestationResult
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
