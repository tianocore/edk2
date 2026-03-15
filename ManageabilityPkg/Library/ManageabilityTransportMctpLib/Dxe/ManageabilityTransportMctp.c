/** @file

  MCTP instance of Manageability Transport Library

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

*/

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportMctpLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MctpProtocol.h>

#include "ManageabilityTransportMctp.h"

MANAGEABILITY_TRANSPORT_MCTP  *mSingleSessionToken = NULL;
EDKII_MCTP_PROTOCOL           *mMctpProtocol       = NULL;

EFI_GUID  *mSupportedManageabilityProtocol[] = {
  &gManageabilityProtocolPldmGuid
};

UINT8  mNumberOfSupportedProtocol = (sizeof (mSupportedManageabilityProtocol)/sizeof (EFI_GUID *));

/**
  This function initializes the transport interface.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  HardwareInfo             The hardware information
                                        assigned to MCTP transport interface.

  @retval      EFI_SUCCESS              Transport interface is initialized
                                        successfully.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_NOT_READY            The transport interface works fine but
  @retval                               is not ready.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems.
  @retval      EFI_ALREADY_STARTED      Teh protocol interface has already initialized.
  @retval      Otherwise                Other errors.

**/
EFI_STATUS
EFIAPI
MctpTransportInit (
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL
  )
{
  return EFI_SUCCESS;
}

/**
  This function returns the transport interface status.
  The generic EFI_STATUS is returned to caller directly, The additional
  information of transport interface could be optionally returned in
  TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_STATUS.

  @param [in]   TransportToken             The transport token acquired through
                                           AcquireTransportSession function.
  @param [out]  TransportAdditionalStatus  The additional status of transport
                                           interface.
                                           NULL means no additional status of this
                                           transport interface.

  @retval      EFI_SUCCESS              Transport interface status is returned.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems to return
  @retval      EFI_UNSUPPORTED          The transport interface doesn't have status report.
               Otherwise                Other errors.

**/
EFI_STATUS
EFIAPI
MctpTransportStatus (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportAdditionalStatus != NULL) {
    *TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;
  }

  return EFI_SUCCESS;
}

/**
  This function resets the transport interface.
  The generic EFI_STATUS is returned to caller directly after reseting transport
  interface. The additional information of transport interface could be optionally
  returned in TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @param [in]   TransportToken             The transport token acquired through
                                           AcquireTransportSession function.
  @param [out]  TransportAdditionalStatus  The additional status of specific transport
                                           interface after the reset.
                                           NULL means no additional status of this
                                           transport interface.

  @retval      EFI_SUCCESS              Transport interface status is returned.
  @retval      EFI_INVALID_PARAMETER    The invalid transport token.
  @retval      EFI_TIMEOUT              The reset process is time out.
  @retval      EFI_DEVICE_ERROR         The transport interface has problems to return
                                        status.
               Otherwise                Other errors.

**/
EFI_STATUS
EFIAPI
MctpTransportReset (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
  if (TransportAdditionalStatus != NULL) {
    *TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NOT_AVAILABLE;
  }

  return EFI_UNSUPPORTED;
}

/**
  This function transmit the request over target transport interface.
  The generic EFI_STATUS is returned to caller directly after reseting transport
  interface. The additional information of transport interface could be optionally
  returned in TransportAdditionalStatus to describes the status that can't be
  described obviously through EFI_STATUS.
  See the definition of MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  TransferToken            The transfer token, see the definition of
                                        MANAGEABILITY_TRANSFER_TOKEN.

  @retval      The EFI status is returned in MANAGEABILITY_TRANSFER_TOKEN.

**/
VOID
EFIAPI
MctpTransportTransmitReceive (
  IN  MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken,
  IN  MANAGEABILITY_TRANSFER_TOKEN   *TransferToken
  )
{
  EFI_STATUS                           Status;
  MANAGEABILITY_MCTP_TRANSPORT_HEADER  *TransmitHeader;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    TransferToken->TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NOT_AVAILABLE;
    return;
  }

  TransmitHeader = (MANAGEABILITY_MCTP_TRANSPORT_HEADER *)TransferToken->TransmitHeader;
  if (TransmitHeader == NULL) {
    TransferToken->TransferStatus            = EFI_INVALID_PARAMETER;
    TransferToken->TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NOT_AVAILABLE;
    return;
  }

  if (mMctpProtocol == NULL) {
    Status = gBS->LocateProtocol (
                    &gEdkiiMctpProtocolGuid,
                    NULL,
                    (VOID **)&mMctpProtocol
                    );
    if (EFI_ERROR (Status)) {
      //
      // Dxe MCTP Protocol is not installed.
      //
      DEBUG ((DEBUG_ERROR, "%a: EDKII MCTP protocol is not found - %r\n", __func__, Status));
      return;
    }
  }

  DEBUG ((
    DEBUG_MANAGEABILITY_INFO,
    "%a: MCTP message type: 0x%x, SourceEndpointId: 0x%x, DestinationEndpointId: 0x%x\n",
    __func__,
    TransmitHeader->MessageHeader.MessageType,
    TransmitHeader->SourceEndpointId,
    TransmitHeader->DestinationEndpointId
    ));
  DEBUG ((
    DEBUG_MANAGEABILITY_INFO,
    "  - Request message size: 0x%x, Response message size: %x\n",
    TransferToken->TransmitPackage.TransmitSizeInByte,
    TransferToken->ReceivePackage.ReceiveSizeInByte
    ));
  Status = mMctpProtocol->Functions.Version1_0->MctpSubmitCommand (
                                                  mMctpProtocol,
                                                  TransmitHeader->MessageHeader.MessageType,
                                                  &TransmitHeader->SourceEndpointId,
                                                  &TransmitHeader->DestinationEndpointId,
                                                  (BOOLEAN)TransmitHeader->MessageHeader.IntegrityCheck,
                                                  TransferToken->TransmitPackage.TransmitPayload,
                                                  TransferToken->TransmitPackage.TransmitSizeInByte,
                                                  TransferToken->TransmitPackage.TransmitTimeoutInMillisecond,
                                                  TransferToken->ReceivePackage.ReceiveBuffer,
                                                  &TransferToken->ReceivePackage.ReceiveSizeInByte,
                                                  TransferToken->ReceivePackage.TransmitTimeoutInMillisecond,
                                                  &TransferToken->TransportAdditionalStatus
                                                  );
  TransferToken->TransferStatus = Status;
}

/**
  This function acquires to create a transport session to transmit manageability
  packet. A transport token is returned to caller for the follow up operations.

  @param [in]   ManageabilityProtocolSpec  The protocol spec the transport interface is acquired.
  @param [out]  TransportToken             The pointer to receive the transport token created by
                                           the target transport interface library.
  @retval       EFI_SUCCESS                Token is created successfully.
  @retval       EFI_OUT_OF_RESOURCES       Out of resource to create a new transport session.
  @retval       EFI_UNSUPPORTED            Protocol is not supported on this transport interface.
  @retval       Otherwise                  Other errors.

**/
EFI_STATUS
AcquireTransportSession (
  IN  EFI_GUID                       *ManageabilityProtocolSpec,
  OUT MANAGEABILITY_TRANSPORT_TOKEN  **TransportToken
  )
{
  EFI_STATUS                    Status;
  MANAGEABILITY_TRANSPORT_MCTP  *MctpTransportToken;

  if (ManageabilityProtocolSpec == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No Manageability protocol specification specified.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = HelperManageabilityCheckSupportedSpec (
             &gManageabilityTransportMctpGuid,
             mSupportedManageabilityProtocol,
             mNumberOfSupportedProtocol,
             ManageabilityProtocolSpec
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Protocol is not supported on this transport interface.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  if (mSingleSessionToken != NULL) {
    DEBUG ((DEBUG_ERROR, "%a: This manageability transport library only supports one session transport token.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  MctpTransportToken = (MANAGEABILITY_TRANSPORT_MCTP *)AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_MCTP));
  if (MctpTransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_MCTP\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  MctpTransportToken->Token.Transport = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT));
  if (MctpTransportToken->Token.Transport == NULL) {
    FreePool (MctpTransportToken);
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  MctpTransportToken->Signature                                            = MANAGEABILITY_TRANSPORT_MCTP_SIGNATURE;
  MctpTransportToken->Token.ManageabilityProtocolSpecification             = ManageabilityProtocolSpec;
  MctpTransportToken->Token.Transport->TransportVersion                    = MANAGEABILITY_TRANSPORT_TOKEN_VERSION;
  MctpTransportToken->Token.Transport->ManageabilityTransportSpecification = &gManageabilityTransportMctpGuid;
  MctpTransportToken->Token.Transport->TransportName                       = L"MCTP";
  MctpTransportToken->Token.Transport->Function.Version1_0                 = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_FUNCTION_V1_0));
  if (MctpTransportToken->Token.Transport->Function.Version1_0 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_FUNCTION_V1_0\n", __func__));
    FreePool (MctpTransportToken);
    FreePool (MctpTransportToken->Token.Transport);
    return EFI_OUT_OF_RESOURCES;
  }

  MctpTransportToken->Token.Transport->Function.Version1_0->TransportInit            = MctpTransportInit;
  MctpTransportToken->Token.Transport->Function.Version1_0->TransportReset           = MctpTransportReset;
  MctpTransportToken->Token.Transport->Function.Version1_0->TransportStatus          = MctpTransportStatus;
  MctpTransportToken->Token.Transport->Function.Version1_0->TransportTransmitReceive = MctpTransportTransmitReceive;

  mSingleSessionToken = MctpTransportToken;
  *TransportToken     = &MctpTransportToken->Token;
  return EFI_SUCCESS;
}

/**
  This function returns the transport capabilities according to
  the manageability protocol.

  @param [in]   TransportToken             Transport token acquired from manageability
                                           transport library.
  @param [out]  TransportFeature           Pointer to receive transport capabilities.
                                           See the definitions of
                                           MANAGEABILITY_TRANSPORT_CAPABILITY.
  @retval       EFI_SUCCESS                TransportCapability is returned successfully.
  @retval       EFI_INVALID_PARAMETER      TransportToken is not a valid token.
**/
EFI_STATUS
GetTransportCapability (
  IN MANAGEABILITY_TRANSPORT_TOKEN        *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_CAPABILITY  *TransportCapability
  )
{
  if ((TransportToken == NULL) || (TransportCapability == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *TransportCapability = 0;
  return EFI_SUCCESS;
}

/**
  This function releases the manageability session.

  @param [in]  TransportToken         The transport token acquired through
                                      AcquireTransportSession.
  @retval      EFI_SUCCESS            Token is released successfully.
  @retval      EFI_INVALID_PARAMETER  Invalid TransportToken.
  @retval      Otherwise              Other errors.

**/
EFI_STATUS
ReleaseTransportSession (
  IN MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken
  )
{
  EFI_STATUS                    Status;
  MANAGEABILITY_TRANSPORT_MCTP  *MctpTransportToken;

  if (TransportToken == NULL) {
    Status = EFI_INVALID_PARAMETER;
  }

  MctpTransportToken = MANAGEABILITY_TRANSPORT_MCTP_FROM_LINK (TransportToken);
  if (mSingleSessionToken != MctpTransportToken) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (MctpTransportToken != NULL) {
    FreePool (MctpTransportToken->Token.Transport->Function.Version1_0);
    FreePool (MctpTransportToken->Token.Transport);
    FreePool (MctpTransportToken);
    mSingleSessionToken = NULL;
    Status              = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to release MCTP transport token (%r).\n", __func__, Status));
  }

  return Status;
}
