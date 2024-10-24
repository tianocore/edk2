/** @file
  Arm CCA Boot Sync Boot Information Blob (BIB) Protocol.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncProtocol.h"
#include "Include/BootSyncSecureChannel.h"

/**
  BIB protocol

           +---------+                           +----------+
           |  User   |                           | Arm CCA  |
           | Context |                           | Guest FW |
           +---------+                           +----------+
                |                                     |
                /            Secure Session           \
                \             Established             /
                |                                     |
                /      Attestation & Verification     \
                \             Completed               /
                |                                     |
                |<--------------BIB-REQ---------------|
                |                                     |
                |---------------BIB-RESP------------->|
                |                                     |
                /                                     \
                \                                     /
                |                                     |
*/

/*

  BIB Request message
  ---------------------------

  +------
  |  BOOT_SYNC_ENCRYPTED_DATA
  |  .Header.Name = gArmBootSyncKeyEncData
  |  .Tag[]
  |  .EncDataLen = Total length
  +-+----
    |  BOOT_SYNC_BSB_HEADER
    |  .Header.Name = gArmBootSyncBibReqGuid
    |  .ElementCount = Count of BSB elements, i.e. 1
    +-+-----------
      |  BOOT_SYNC_BSB_ELEMENT
      |  .Header.Name = gArmBootSyncRequestOptions
      |  .Length = sizeof (BOOT_SYNC_BSB_ELEMENT) +
      |             Request Options Length
      |  .Data[] = Request Options Data.
      +-----------


  BIB Response message
  ---------------------------

  +------
  |  BOOT_SYNC_ENCRYPTED_DATA
  |  .Header.Name = gArmBootSyncKeyEncData
  |  .Tag[]
  |  .EncDataLen = Total length
  +-+----
    |  BOOT_SYNC_BSB_HEADER
    |  .Header.Name = gArmBootSyncBibRespGuid
    |  .ElementCount = Count of BSB elements, i.e. 1
    +-+-----------
      |  BOOT_SYNC_BSB_ELEMENT
      |  .Header.Name = gArmBootSyncVarData
      |  .Length = sizeof (BOOT_SYNC_BSB_ELEMENT) +
      |             Var Data Length
      |  .Data[] = Var Data.
      +-----------
      |  BOOT_SYNC_BSB_ELEMENT
      |  .Header.Name = gArmBootSyncSecretData
      |  .Length = sizeof (BOOT_SYNC_BSB_ELEMENT) +
      |             Secret Data Length
      |  .Data[] = Secret Data.
      +-----------
*/

/**
  Send a BIB request message to get the Boot Information Blob.

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
SendBibReq (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          Options
  )
{
  EFI_STATUS                 Status;
  UINTN                      PayloadSize;
  BOOT_SYNC_BSB_HEADER       *BibReq;
  BOOT_SYNC_REQUEST_OPTIONS  *BibReqOptions;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Allocate memory for the BIB Request message.
  PayloadSize = sizeof (BOOT_SYNC_BSB_HEADER) +
                sizeof (BOOT_SYNC_REQUEST_OPTIONS);
  BibReq = AllocateZeroPool (PayloadSize);
  if (BibReq == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Step 2: Poulate the Bib Request message header.
  CopyGuid (&BibReq->Header.Name, &gArmBootSyncBibReqGuid);
  BibReq->Header.Length = PayloadSize;
  BibReq->ElementCount  = 1;

  // Step 3: Poulate the Bib Request options.
  BibReqOptions = (BOOT_SYNC_REQUEST_OPTIONS *)(BibReq + 1);
  CopyGuid (
    &BibReqOptions->ElementHdr.Header.Name,
    &gArmBootSyncRequestOptions
    );
  BibReqOptions->ElementHdr.Header.Length =
    sizeof (BOOT_SYNC_REQUEST_OPTIONS);
  BibReqOptions->Options = Options;

  // Step 4: Send the BIB Request message.
  Status = SendEncryptBsbMessage (SecChannel, BibReq);
  ASSERT_EFI_ERROR (Status);

  // Step 5: Free any resources that were allocated.
  FreePool (BibReq);

  return Status;
}

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
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  BOOT_SYNC_BSB_HEADER  *BibResp;

  if ((SecChannel == NULL) || (Bib == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Send BIB Request.
  Status = SendBibReq (
             SecChannel,
             (BIB_REQUEST_OPTION_VARIABLE_DATA | BIB_REQUEST_OPTION_SECRET_DATA)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 2: Get the BIB response message.
  Status = ReceiveDecryptBsbMsg (
             SecChannel,
             (BOOT_SYNC_GUID_BLOB **)&BibResp
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "BibResp = %p\n", BibResp));

  if (CompareGuid (&BibResp->Header.Name, &gArmBootSyncFinGuid) ||
      CompareGuid (&BibResp->Header.Name, &gArmBootSyncNackGuid))
  {
    Status = EFI_PROTOCOL_ERROR;
    goto exit_handler;
  }

  // Step 3: Check the response message.
  if (!CompareGuid (&BibResp->Header.Name, &gArmBootSyncBibRespGuid)) {
    DEBUG ((DEBUG_ERROR, "Invalid Response = %g\n", &BibResp->Header.Name));
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  DBG_DUMP_RAW ("BIB RESPONSE", (UINT8 *)BibResp, BibResp->Header.Length);

  // Validate that the BIB response length.
  if (BibResp->Header.Length < sizeof (BOOT_SYNC_BSB_HEADER)) {
    DEBUG ((DEBUG_ERROR, "Invalid BIB Response size.\n"));
    Status = EFI_ABORTED;
    goto exit_handler;
  }

  // Step 4: Return the Bib.
  *Bib = BibResp;
  return Status;

exit_handler:
  // Send the FIN message to indicate an error.
  Status1 = SendFin (SecChannel, BOOT_SYNC_COMM_END_PROTOCOL_ERROR);
  ASSERT_EFI_ERROR (Status1);

  FreePool (BibResp);

  return Status;
}
