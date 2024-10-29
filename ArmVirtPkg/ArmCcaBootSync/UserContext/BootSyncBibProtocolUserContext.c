/** @file
  Arm CCA Boot Sync Boot Information Blocks (BIB) Protocol host.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Include/BootSyncBsbMsg.h"
#include "Include/BootSyncDebug.h"
#include "Include/BootSyncSecureChannel.h"

/**
  Boot Information Blocks protocol

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
  Get the UEFI Variable Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                UEFI Variable data.
  @param[out]  DataSize            UEFI Variable data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetVariableData (
  IN  SECURE_CHANNEL  *SecChannel,
  OUT UINT8           **Data,
  OUT UINTN           *DataSize
  )
{
  // Not implemented as yet.
  ASSERT (0);
  return EFI_UNSUPPORTED;
}

/**
  Get the Secret Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                Secret data.
  @param[out]  DataSize            Secret data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetSecretData (
  IN SECURE_CHANNEL  *SecChannel,
  OUT UINT8          **Data,
  OUT UINTN          *DataSize
  )
{
  // Not implemented as yet.
  ASSERT (0);
  return EFI_UNSUPPORTED;
}

/**
  Pack the response data element.

  @param[in]  Element             Pointer to the element.
  @param[in]  ElementGuidName     Guid for the element.
  @param[in]  Data                Element data.
  @param[in]  DataSize            Element data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
STATIC
EFI_STATUS
EFIAPI
BootSyncPackResponseDataElement (
  IN BOOT_SYNC_BSB_ELEMENT  *Element,
  IN EFI_GUID               *ElementGuidName,
  IN UINT8                  *Data,
  IN UINTN                  DataSize
  )
{
  UINT8  *ElementData;

  if ((Element == NULL) ||
      (ElementGuidName == NULL) ||
      (Data == NULL) ||
      (DataSize == 0)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  CopyGuid (&Element->Header.Name, ElementGuidName);
  Element->Header.Length = sizeof (BOOT_SYNC_BSB_ELEMENT) + DataSize;

  ElementData = (UINT8 *)(Element + 1);
  CopyMem (ElementData, Data, DataSize);

  return EFI_SUCCESS;
}

/**
  Perform Boot Sync and send the guest requested data.

  @param[in]    SecChannel  Pointer to the secure channel.
  @param[in]    Msg         Pointer to the received Attestation Request.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_PROTOCOL_ERROR      A protocol error was detected.
  @retval EFI_ABORTED             An operation failed.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncPerformSync (
  IN SECURE_CHANNEL       *SecChannel,
  IN BOOT_SYNC_GUID_BLOB  *Msg
  )
{
  EFI_STATUS            Status;
  BOOT_SYNC_BSB_HEADER  *BibReq;
  BOOT_SYNC_BSB_HEADER  *BibResp;

  BOOT_SYNC_REQUEST_OPTIONS  *BibReqOptions;
  UINT64                     Options;
  UINTN                      PayloadSize;
  UINTN                      ElementCount;
  BOOT_SYNC_BSB_ELEMENT      *Element;
  UINT8                      *VariableData;
  UINTN                      VariableDataLen;
  UINT8                      *SecretData;
  UINTN                      SecretDataLen;

  if ((SecChannel == NULL) || (Msg == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1: Get the BIB request message.
  BibReq = (BOOT_SYNC_BSB_HEADER *)Msg;

  DBG_DUMP_RAW ("BIB REQ", (UINT8 *)BibReq, BibReq->Header.Length);
  if (BibReq->Header.Length < (sizeof (BOOT_SYNC_BSB_HEADER) +
                               sizeof (BOOT_SYNC_REQUEST_OPTIONS)))
  {
    return EFI_PROTOCOL_ERROR;
  }

  // Step 2: Check the BIB request message.
  if (!CompareGuid (&BibReq->Header.Name, &gArmBootSyncBibReqGuid)) {
    DEBUG ((DEBUG_ERROR, "Invalid Response = %g\n", &BibReq->Header.Name));
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  // Step 3: Get the BIB Request options.
  BibReqOptions = (BOOT_SYNC_REQUEST_OPTIONS *)(BibReq + 1);

  if (BibReqOptions->ElementHdr.Header.Length !=
      sizeof (BOOT_SYNC_REQUEST_OPTIONS))
  {
    return EFI_PROTOCOL_ERROR;
  }

  if (!CompareGuid (
         &BibReqOptions->ElementHdr.Header.Name,
         &gArmBootSyncRequestOptions
         ))
  {
    DEBUG ((DEBUG_ERROR, "Invalid BIB Request = %g\n", &BibReq->Header.Name));
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Options = BibReqOptions->Options;
  if (Options == 0) {
    // No Options specified
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: No BIB Options Specified, Options = 0x%x\n",
      Options
      ));
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  // Step 4: Compute the size of the response payloads.
  // Step 4a: Get the size of the BSB Header.
  PayloadSize     = sizeof (BOOT_SYNC_BSB_HEADER);
  ElementCount    = 0;
  VariableData    = NULL;
  VariableDataLen = 0;
  SecretData      = NULL;
  SecretDataLen   = 0;

  // Step 4b: Get the size of the UEFI Variable Data.
  if ((Options & BIB_REQUEST_OPTION_VARIABLE_DATA) ==
      BIB_REQUEST_OPTION_VARIABLE_DATA)
  {
    DEBUG ((DEBUG_ERROR, "INFO: BIB Variable Data Requested\n"));
    ElementCount++;
    Status = BootSyncGetVariableData (
               SecChannel,
               &VariableData,
               &VariableDataLen
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    PayloadSize += sizeof (BOOT_SYNC_BSB_ELEMENT) +  VariableDataLen;
  }

  // Step 4c: Get the size of the Secret Data.
  if ((Options & BIB_REQUEST_OPTION_SECRET_DATA) ==
      BIB_REQUEST_OPTION_SECRET_DATA)
  {
    DEBUG ((DEBUG_ERROR, "INFO: BIB Secret Data Requested\n"));
    ElementCount++;
    Status = BootSyncGetSecretData (
               SecChannel,
               &SecretData,
               &SecretDataLen
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    PayloadSize += sizeof (BOOT_SYNC_BSB_ELEMENT) + SecretDataLen;
  }

  DEBUG ((DEBUG_INFO, "INFO: BIB Response Payload Size = %d\n", PayloadSize));
  DEBUG ((DEBUG_INFO, "INFO: VariableDataLen = %d\n", VariableDataLen));
  DEBUG ((DEBUG_INFO, "INFO: SecretDataLen = %d\n", SecretDataLen));

  // Step 5: Allocate memory for the BIB Resp message
  BibResp = AllocateZeroPool (PayloadSize);
  if (BibResp == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit_handler;
  }

  // Step 6: Poulate the Response message.
  // Step 6a: Pack the BIB Response message Header
  CopyGuid (&BibResp->Header.Name, &gArmBootSyncBibRespGuid);
  BibResp->Header.Length = PayloadSize;
  BibResp->ElementCount  = ElementCount;

  // Populate the BIB Response elements.
  Element = (BOOT_SYNC_BSB_ELEMENT *)(BibResp + 1);

  // Step 6b: Pack the UEFI Variable Data.
  if ((Options & BIB_REQUEST_OPTION_VARIABLE_DATA) ==
      BIB_REQUEST_OPTION_VARIABLE_DATA)
  {
    Status = BootSyncPackResponseDataElement (
               Element,
               &gArmBootSyncVarData,
               VariableData,
               VariableDataLen
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler1;
    }

    Element = (BOOT_SYNC_BSB_ELEMENT *)((UINT8 *)Element +
                                        Element->Header.Length);
  }

  // Step 6c: Pack the Secret Data.
  if ((Options & BIB_REQUEST_OPTION_SECRET_DATA) ==
      BIB_REQUEST_OPTION_SECRET_DATA)
  {
    Status = BootSyncPackResponseDataElement (
               Element,
               &gArmBootSyncSecretData,
               SecretData,
               SecretDataLen
               );
    if (EFI_ERROR (Status)) {
      goto exit_handler1;
    }
  }

  // Step 7: Send the BIB Request message.
  Status = SendEncryptBsbMessage (SecChannel, BibResp);
  ASSERT_EFI_ERROR (Status);

  // Free any resources that were allocated.
exit_handler1:
  FreePool (BibResp);

exit_handler:
  if (VariableData != NULL) {
    FreePool (VariableData);
  }

  if (SecretData != NULL) {
    FreePool (SecretData);
  }

  return Status;
}
