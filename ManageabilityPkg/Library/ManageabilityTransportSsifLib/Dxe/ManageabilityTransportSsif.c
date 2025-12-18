/** @file

  SSIF instance of Manageability Transport Library

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

*/

#include <Uefi.h>
#include <IndustryStandard/IpmiSsif.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/PlatformBmcReadyLib.h>

#include "ManageabilityTransportSsif.h"

MANAGEABILITY_TRANSPORT_SSIF  *mSingleSessionToken = NULL;

EFI_GUID  *mSupportedManageabilityProtocol[] = {
  &gManageabilityProtocolIpmiGuid
};

UINT8  mNumberOfSupportedProtocol = (sizeof (mSupportedManageabilityProtocol) / sizeof (EFI_GUID *));

MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO  mSsifHardwareInfo;

//
// Initialize SSIF Interface capabilities
//
BOOLEAN  mPecSupport         = FALSE;
UINT8    mMaxRequestSize     = IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES;
UINT8    mMaxResponseSize    = IPMI_SSIF_MAXIMUM_PACKET_SIZE_IN_BYTES;
UINT8    mTransactionSupport = IPMI_GET_SYSTEM_INTERFACE_CAPABILITIES_SSIF_TRANSACTION_SUPPORT_SINGLE_PARTITION_RW;

/**
  This function initializes the transport interface.

  @param [in]  TransportToken           The transport token acquired through
                                        AcquireTransportSession function.
  @param [in]  HardwareInfo             The hardware information
                                        assigned to SSIF transport interface.

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
SsifTransportInit (
  IN  MANAGEABILITY_TRANSPORT_TOKEN                 *TransportToken,
  IN  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo OPTIONAL
  )
{
  EFI_STATUS  Status;
  CHAR16      *ManageabilityProtocolName;

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (HardwareInfo.Ssif == NULL) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Hardware information is not provided, use default settings.\n", __func__));
    mSsifHardwareInfo.BmcSlaveAddress = FixedPcdGet8 (PcdIpmiSsifSmbusSlaveAddr);
  } else {
    mSsifHardwareInfo.BmcSlaveAddress = ((MANAGEABILITY_TRANSPORT_SSIF_HARDWARE_INFO *)HardwareInfo.Ssif)->BmcSlaveAddress;
  }

  Status = SsifTransportGetCapabilities (
             &mPecSupport,
             &mTransactionSupport,
             &mMaxRequestSize,
             &mMaxResponseSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Could not retrieve IPMI SSIF capabilites, use default settings.\n", __func__));
  }

  // Get protocol specification name.
  ManageabilityProtocolName = HelperManageabilitySpecName (TransportToken->ManageabilityProtocolSpecification);

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: transport hardware for %s is:\n", __func__, ManageabilityProtocolName));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "BMC Slave Address:   0x%x\n", mSsifHardwareInfo.BmcSlaveAddress));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "PEC Support:         %d\n", mPecSupport));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "Transaction Support: %d\n", mTransactionSupport));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "Max Request Size:    %d\n", mMaxRequestSize));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "Max Response Size:   %d\n", mMaxResponseSize));

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
SsifTransportStatus (
  IN  MANAGEABILITY_TRANSPORT_TOKEN              *TransportToken,
  OUT MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *TransportAdditionalStatus OPTIONAL
  )
{
  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid transport token.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // SMBUS does not provide transport status checking capabilities. This only determines
  // if the BMC is ready or not, without updating any additional status.
  //
  if (!PlatformBmcReady ()) {
    return EFI_NOT_READY;
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
SsifTransportReset (
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
SsifTransportTransmitReceive (
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

  Status = SsifTransportSendCommand (
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

  TransferToken->TransferStatus            = Status;
  TransferToken->TransportAdditionalStatus = AdditionalStatus;
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
  MANAGEABILITY_TRANSPORT_SSIF  *SsifTransportToken;

  if (ManageabilityProtocolSpec == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No Manageability protocol specification specified.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (TransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: TransportToken is NULL.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Status = HelperManageabilityCheckSupportedSpec (
             &gManageabilityTransportSmbusI2cGuid,
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

  SsifTransportToken = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_SSIF));
  if (SsifTransportToken == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_SSIF\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  SsifTransportToken->Token.Transport = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT));
  if (SsifTransportToken->Token.Transport == NULL) {
    FreePool (SsifTransportToken);
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  SsifTransportToken->Signature                                            = MANAGEABILITY_TRANSPORT_SSIF_SIGNATURE;
  SsifTransportToken->Token.ManageabilityProtocolSpecification             = ManageabilityProtocolSpec;
  SsifTransportToken->Token.Transport->TransportVersion                    = MANAGEABILITY_TRANSPORT_TOKEN_VERSION;
  SsifTransportToken->Token.Transport->ManageabilityTransportSpecification = &gManageabilityTransportSmbusI2cGuid;
  SsifTransportToken->Token.Transport->TransportName                       = L"SSIF";
  SsifTransportToken->Token.Transport->Function.Version1_0                 = AllocateZeroPool (sizeof (MANAGEABILITY_TRANSPORT_FUNCTION_V1_0));
  if (SsifTransportToken->Token.Transport->Function.Version1_0 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to allocate memory for MANAGEABILITY_TRANSPORT_FUNCTION_V1_0\n", __func__));
    FreePool (SsifTransportToken->Token.Transport);
    FreePool (SsifTransportToken);
    return EFI_OUT_OF_RESOURCES;
  }

  SsifTransportToken->Token.Transport->Function.Version1_0->TransportInit            = SsifTransportInit;
  SsifTransportToken->Token.Transport->Function.Version1_0->TransportReset           = SsifTransportReset;
  SsifTransportToken->Token.Transport->Function.Version1_0->TransportStatus          = SsifTransportStatus;
  SsifTransportToken->Token.Transport->Function.Version1_0->TransportTransmitReceive = SsifTransportTransmitReceive;

  mSingleSessionToken = SsifTransportToken;
  *TransportToken     = &SsifTransportToken->Token;
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
  EFI_STATUS                    Status;
  MANAGEABILITY_TRANSPORT_SSIF  *SsifTransportToken;

  Status = EFI_INVALID_PARAMETER;

  if (TransportToken != NULL) {
    SsifTransportToken = MANAGEABILITY_TRANSPORT_SSIF_FROM_LINK (TransportToken);

    if (SsifTransportToken != NULL) {
      if ((mSingleSessionToken != NULL) &&
          (mSingleSessionToken == SsifTransportToken))
      {
        mSingleSessionToken = NULL;
        Status              = EFI_SUCCESS;
      }

      FreePool (SsifTransportToken->Token.Transport->Function.Version1_0);
      FreePool (SsifTransportToken->Token.Transport);
      FreePool (SsifTransportToken);
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to release SSIF transport token (%r).\n", __func__, Status));
  }

  return Status;
}
