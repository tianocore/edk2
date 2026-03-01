/** @file

  Serial instance of Manageability Transport Library

  Copyright (c) 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

*/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>

#include "ManageabilityTransportSerial.h"

MANAGEABILITY_TRANSPORT_SERIAL  *mSingleSessionToken = NULL;

EFI_GUID  *SupportedManageabilityProtocol[] = {
  &gManageabilityProtocolIpmiGuid,
};

UINT8  mNumberOfSupportedProtocol = (sizeof (SupportedManageabilityProtocol)/sizeof (EFI_GUID *));

MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO  mSerialHardwareInfo;

/**
  This function initializes the transport interface.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  HardwareInfo             The hardware information
                                        assigned to Serial transport interface.

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
SerialTransportInit (
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL
  )
{
  CHAR16      *ManageabilityProtocolName;
  EFI_STATUS  Status;

  Status = SerialPortInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to initialize SerialPort (%r).\n", __func__, Status));
    return Status;
  }

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (HardwareInfo.Serial == NULL) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Hardware information is not provided, use dfault settings.\n", __func__));
    mSerialHardwareInfo.IpmiRequesterAddress = FixedPcdGet8 (PcdIpmiSerialRequesterAddress);
    mSerialHardwareInfo.IpmiResponderAddress = FixedPcdGet8 (PcdIpmiSerialResponderAddress);
    mSerialHardwareInfo.IpmiRequesterLUN     = FixedPcdGet8 (PcdIpmiSerialRequesterLun);
    mSerialHardwareInfo.IpmiResponderLUN     = FixedPcdGet8 (PcdIpmiSerialResponderLun);
  } else {
    mSerialHardwareInfo.IpmiRequesterAddress = ((MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO *)HardwareInfo.Serial)->IpmiRequesterAddress;
    mSerialHardwareInfo.IpmiResponderAddress = ((MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO *)HardwareInfo.Serial)->IpmiResponderAddress;
    mSerialHardwareInfo.IpmiRequesterLUN     = ((MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO *)HardwareInfo.Serial)->IpmiRequesterLUN;
    mSerialHardwareInfo.IpmiResponderLUN     = ((MANAGEABILITY_TRANSPORT_SERIAL_HARDWARE_INFO *)HardwareInfo.Serial)->IpmiResponderLUN;
  }

  // Get protocol specification name.
  ManageabilityProtocolName = HelperManageabilitySpecName (TransportToken->ManageabilityProtocolSpecification);

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Serial transport hardware for %s is:\n", __func__, ManageabilityProtocolName));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "IPMI requester address       : 0x%08x\n", mSerialHardwareInfo.IpmiRequesterAddress));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "IPMI responder address       : 0x%08x\n", mSerialHardwareInfo.IpmiResponderAddress));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "IPMI requester LUN           : 0x%08x\n", mSerialHardwareInfo.IpmiRequesterLUN));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "IPMI responder LUN           : 0x%08x\n", mSerialHardwareInfo.IpmiResponderLUN));

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
SerialTransportStatus (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportAdditionalStatus == NULL) {
    return EFI_SUCCESS;
  }

  *TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;

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
SerialTransportReset (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
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
SerialTransportTransmitReceive (
  IN  MANAGEABILITY_TRANSPORT_TOKEN  *TransportToken,
  IN  MANAGEABILITY_TRANSFER_TOKEN   *TransferToken
  )
{
  EFI_STATUS                                 Status;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  AdditionalStatus;

  if ((TransportToken == NULL) || (TransferToken == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token or transfer token.\n", __func__));
    return;
  }

  Status = SerialTransportSendCommand (
                                       TransferToken->TransmitHeader,
                                       TransferToken->TransmitHeaderSize,
                                       TransferToken->TransmitTrailer,
                                       TransferToken->TransmitTrailerSize,
                                       TransferToken->TransmitPackage.TransmitPayload,
                                       TransferToken->TransmitPackage.TransmitSizeInByte,
                                       TransferToken->ReceivePackage.ReceiveBuffer,
                                       &TransferToken->ReceivePackage.ReceiveSizeInByte,
                                       &AdditionalStatus
                                       );

  TransferToken->TransferStatus = Status;
  SerialTransportStatus (TransportToken, &TransferToken->TransportAdditionalStatus);
  TransferToken->TransportAdditionalStatus |= AdditionalStatus;
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
  EFI_STATUS                      Status;
  MANAGEABILITY_TRANSPORT_SERIAL  *SerialTransportToken;

  if (ManageabilityProtocolSpec == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No Manageability protocol specification specified.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TransportToken is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = HelperManageabilityCheckSupportedSpec (
                                                  &gManageabilityTransportSerialGuid,
                                                  SupportedManageabilityProtocol,
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

  SerialTransportToken = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_SERIAL));
  if (SerialTransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_SERIAL\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  SerialTransportToken->Token.Transport = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT));
  if (SerialTransportToken->Token.Transport == NULL) {
    FreePool (SerialTransportToken);
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  SerialTransportToken->Signature                                            = MANAGEABILITY_TRANSPORT_SERIAL_SIGNATURE;
  SerialTransportToken->Token.ManageabilityProtocolSpecification             = ManageabilityProtocolSpec;
  SerialTransportToken->Token.Transport->TransportVersion                    = MANAGEABILITY_TRANSPORT_TOKEN_VERSION;
  SerialTransportToken->Token.Transport->ManageabilityTransportSpecification = &gManageabilityTransportSerialGuid;
  SerialTransportToken->Token.Transport->TransportName                       = L"SERIAL";
  SerialTransportToken->Token.Transport->Function.Version1_0                 = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_FUNCTION_V1_0));
  if (SerialTransportToken->Token.Transport->Function.Version1_0 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_FUNCTION_V1_0\n", __func__));
    FreePool (SerialTransportToken);
    FreePool (SerialTransportToken->Token.Transport);
    return EFI_OUT_OF_RESOURCES;
  }

  SerialTransportToken->Token.Transport->Function.Version1_0->TransportInit            = SerialTransportInit;
  SerialTransportToken->Token.Transport->Function.Version1_0->TransportReset           = SerialTransportReset;
  SerialTransportToken->Token.Transport->Function.Version1_0->TransportStatus          = SerialTransportStatus;
  SerialTransportToken->Token.Transport->Function.Version1_0->TransportTransmitReceive = SerialTransportTransmitReceive;

  mSingleSessionToken = SerialTransportToken;
  *TransportToken     = &SerialTransportToken->Token;
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
  if (CompareGuid (
                   TransportToken->ManageabilityProtocolSpecification,
                   &gManageabilityProtocolIpmiGuid
                   ))
  {
    *TransportCapability |=
      (MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_NOT_AVAILABLE << MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_BIT_POSITION);
  }

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
  EFI_STATUS                      Status;
  MANAGEABILITY_TRANSPORT_SERIAL  *SerialTransportToken;

  if (TransportToken == NULL) {
    Status = EFI_INVALID_PARAMETER;
  }

  SerialTransportToken = MANAGEABILITY_TRANSPORT_SERIAL_FROM_LINK (TransportToken);
  if (mSingleSessionToken != SerialTransportToken) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (SerialTransportToken != NULL) {
    FreePool (SerialTransportToken->Token.Transport->Function.Version1_0);
    FreePool (SerialTransportToken->Token.Transport);
    FreePool (SerialTransportToken);
    mSingleSessionToken = NULL;
    Status              = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to release Serial transport token (%r).\n", __func__, Status));
  }

  return Status;
}
