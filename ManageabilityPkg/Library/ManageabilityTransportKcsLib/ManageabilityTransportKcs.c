/** @file

  KCS instance of Manageability Transport Library

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

*/

#include <Uefi.h>
#include <IndustryStandard/IpmiKcs.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>

#include "ManageabilityTransportKcs.h"

MANAGEABILITY_TRANSPORT_KCS  *mSingleSessionToken = NULL;

EFI_GUID  *SupportedManageabilityProtocol[] = {
  &gManageabilityProtocolIpmiGuid,
  &gManageabilityProtocolMctpGuid
};

UINT8  NumberOfSupportedProtocol = (sizeof (SupportedManageabilityProtocol)/sizeof (EFI_GUID *));

MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO  mKcsHardwareInfo;

/**
  This function initializes the transport interface.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  HardwareInfo             The hardware information
                                        assigned to KCS transport interface.

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
KcsTransportInit (
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL
  )
{
  CHAR16  *ManageabilityProtocolName;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (HardwareInfo.Kcs == NULL) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Hardware information is not provided, use dfault settings.\n", __func__));
    mKcsHardwareInfo.MemoryMap                    = MANAGEABILITY_TRANSPORT_KCS_IO_MAP_IO;
    mKcsHardwareInfo.IoBaseAddress.IoAddress16    = PcdGet16 (PcdIpmiKcsIoBaseAddress);
    mKcsHardwareInfo.IoDataInAddress.IoAddress16  = mKcsHardwareInfo.IoBaseAddress.IoAddress16 + IPMI_KCS_DATA_IN_REGISTER_OFFSET;
    mKcsHardwareInfo.IoDataOutAddress.IoAddress16 = mKcsHardwareInfo.IoBaseAddress.IoAddress16 + IPMI_KCS_DATA_OUT_REGISTER_OFFSET;
    mKcsHardwareInfo.IoCommandAddress.IoAddress16 = mKcsHardwareInfo.IoBaseAddress.IoAddress16 + IPMI_KCS_COMMAND_REGISTER_OFFSET;
    mKcsHardwareInfo.IoStatusAddress.IoAddress16  = mKcsHardwareInfo.IoBaseAddress.IoAddress16 + IPMI_KCS_STATUS_REGISTER_OFFSET;
  } else {
    mKcsHardwareInfo.MemoryMap        = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->MemoryMap;
    mKcsHardwareInfo.IoBaseAddress    = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->IoBaseAddress;
    mKcsHardwareInfo.IoDataInAddress  = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->IoDataInAddress;
    mKcsHardwareInfo.IoDataOutAddress = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->IoDataOutAddress;
    mKcsHardwareInfo.IoCommandAddress = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->IoCommandAddress;
    mKcsHardwareInfo.IoStatusAddress  = ((MANAGEABILITY_TRANSPORT_KCS_HARDWARE_INFO *)HardwareInfo.Kcs)->IoStatusAddress;
  }

  // Get protocol specification name.
  ManageabilityProtocolName = HelperManageabilitySpecName (TransportToken->ManageabilityProtocolSpecification);

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: KCS transport hardware for %s is:\n", __func__, ManageabilityProtocolName));
  if (mKcsHardwareInfo.MemoryMap) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Memory Map I/O\n", __func__));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Base Memory Address : 0x%08x\n", mKcsHardwareInfo.IoBaseAddress.IoAddress32));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Data in Address     : 0x%08x\n", mKcsHardwareInfo.IoDataInAddress.IoAddress32));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Data out Address    : 0x%08x\n", mKcsHardwareInfo.IoDataOutAddress.IoAddress32));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Command Address     : 0x%08x\n", mKcsHardwareInfo.IoCommandAddress.IoAddress32));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Status Address      : 0x%08x\n", mKcsHardwareInfo.IoStatusAddress.IoAddress32));
  } else {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "I/O Map I/O\n"));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Base I/O port    : 0x%04x\n", mKcsHardwareInfo.IoBaseAddress.IoAddress16));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Data in I/O port : 0x%04x\n", mKcsHardwareInfo.IoDataInAddress.IoAddress16));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Data out I/O port: 0x%04x\n", mKcsHardwareInfo.IoDataOutAddress.IoAddress16));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Command I/O port : 0x%04x\n", mKcsHardwareInfo.IoCommandAddress.IoAddress16));
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "Status I/O port  : 0x%04x\n", mKcsHardwareInfo.IoStatusAddress.IoAddress16));
  }

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
KcsTransportStatus (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
  UINT8  TransportStatus;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportAdditionalStatus == NULL) {
    return EFI_SUCCESS;
  }

  TransportStatus            = IPMI_KCS_GET_STATE (KcsRegisterRead8 (KCS_REG_STATUS));
  *TransportAdditionalStatus = MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS;
  if (TransportStatus != IpmiKcsIdleState) {
    if (TransportStatus == IpmiKcsReadState) {
      //
      // Transport is in read state.
      *TransportAdditionalStatus |= MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_BUSY_IN_READ;
      return EFI_NOT_READY;
    } else if (TransportStatus == IpmiKcsWriteState) {
      //
      // Transport is in read state.
      *TransportAdditionalStatus |= MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_BUSY_IN_WRITE;
      return EFI_NOT_READY;
    } else {
      return EFI_DEVICE_ERROR;
    }
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
KcsTransportReset (
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
KcsTransportTransmitReceive (
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

  Status = KcsTransportSendCommand (
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
  KcsTransportStatus (TransportToken, &TransferToken->TransportAdditionalStatus);
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
  EFI_STATUS                   Status;
  MANAGEABILITY_TRANSPORT_KCS  *KcsTransportToken;

  if (ManageabilityProtocolSpec == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No Manageability protocol specification specified.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TransportToken is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = HelperManageabilityCheckSupportedSpec (
             &gManageabilityTransportKcsGuid,
             SupportedManageabilityProtocol,
             NumberOfSupportedProtocol,
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

  KcsTransportToken = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_KCS));
  if (KcsTransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_KCS\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  KcsTransportToken->Token.Transport = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT));
  if (KcsTransportToken->Token.Transport == NULL) {
    FreePool (KcsTransportToken);
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  KcsTransportToken->Signature                                            = MANAGEABILITY_TRANSPORT_KCS_SIGNATURE;
  KcsTransportToken->Token.ManageabilityProtocolSpecification             = ManageabilityProtocolSpec;
  KcsTransportToken->Token.Transport->TransportVersion                    = MANAGEABILITY_TRANSPORT_TOKEN_VERSION;
  KcsTransportToken->Token.Transport->ManageabilityTransportSpecification = &gManageabilityTransportKcsGuid;
  KcsTransportToken->Token.Transport->TransportName                       = L"KCS";
  KcsTransportToken->Token.Transport->Function.Version1_0                 = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_FUNCTION_V1_0));
  if (KcsTransportToken->Token.Transport->Function.Version1_0 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_FUNCTION_V1_0\n", __func__));
    FreePool (KcsTransportToken);
    FreePool (KcsTransportToken->Token.Transport);
    return EFI_OUT_OF_RESOURCES;
  }

  KcsTransportToken->Token.Transport->Function.Version1_0->TransportInit            = KcsTransportInit;
  KcsTransportToken->Token.Transport->Function.Version1_0->TransportReset           = KcsTransportReset;
  KcsTransportToken->Token.Transport->Function.Version1_0->TransportStatus          = KcsTransportStatus;
  KcsTransportToken->Token.Transport->Function.Version1_0->TransportTransmitReceive = KcsTransportTransmitReceive;

  mSingleSessionToken = KcsTransportToken;
  *TransportToken     = &KcsTransportToken->Token;
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
  } else if (CompareGuid (
               TransportToken->ManageabilityProtocolSpecification,
               &gManageabilityProtocolMctpGuid
               ))
  {
    *TransportCapability |=
      (MCTP_KCS_MTU_IN_POWER_OF_2 << MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_BIT_POSITION);
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
  EFI_STATUS                   Status;
  MANAGEABILITY_TRANSPORT_KCS  *KcsTransportToken;

  if (TransportToken == NULL) {
    Status = EFI_INVALID_PARAMETER;
  }

  KcsTransportToken = MANAGEABILITY_TRANSPORT_KCS_FROM_LINK (TransportToken);
  if (mSingleSessionToken != KcsTransportToken) {
    Status = EFI_INVALID_PARAMETER;
  }

  if (KcsTransportToken != NULL) {
    FreePool (KcsTransportToken->Token.Transport->Function.Version1_0);
    FreePool (KcsTransportToken->Token.Transport);
    FreePool (KcsTransportToken);
    mSingleSessionToken = NULL;
    Status              = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to release KCS transport token (%r).\n", __func__, Status));
  }

  return Status;
}
