/** @file
  This file provides edk2 MCTP Protocol implementation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MctpProtocol.h>

#include <IndustryStandard/Mctp.h>

#include "MctpProtocolCommon.h"

extern MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  mHardwareInformation;

MANAGEABILITY_TRANSPORT_TOKEN  *mTransportToken = NULL;
CHAR16                         *mTransportName;
UINT32                         mTransportMaximumPayload;

/**
  This service enables submitting message via EDKII MCTP protocol.

  @param[in]         This                       EDKII_MCTP_PROTOCOL instance.
  @param[in]         MctpType                   MCTP message type.
  @param[in]         MctpSourceEndpointId       Pointer of MCTP source endpoint ID.
                                                Set to NULL means use platform PCD value
                                                (PcdMctpSourceEndpointId).
  @param[in]         MctpDestinationEndpointId  Pointer of MCTP destination endpoint ID.
                                                Set to NULL means use platform PCD value
                                                (PcdMctpDestinationEndpointId).
  @param[in]         RequestDataIntegrityCheck  Indicates whether MCTP message has
                                                integrity check byte.
  @param[in]         RequestData                Message Data.
  @param[in]         RequestDataSize            Size of message Data.
  @param[in]         RequestTimeout             Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        ResponseData               Message Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Message Response Data.
  @param[in]         ResponseTimeout            Timeout value in milliseconds.
                                                MANAGEABILITY_TRANSPORT_NO_TIMEOUT means no timeout value.
  @param[out]        AdditionalTransferError    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS.

  @retval EFI_SUCCESS            The message was successfully send to transport interface and a
                                 response was successfully received.
  @retval EFI_NOT_FOUND          The message was not successfully sent to transport interface or a response
                                 was not successfully received from transport interface.
  @retval EFI_NOT_READY          MCTP transport interface is not ready for MCTP message.
  @retval EFI_DEVICE_ERROR       MCTP transport interface Device hardware error.
  @retval EFI_TIMEOUT            The message time out.
  @retval EFI_UNSUPPORTED        The message was not successfully sent to the transport interface.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  Both RequestData and ResponseData are NULL
**/
EFI_STATUS
EFIAPI
MctpSubmitMessage (
  IN     EDKII_MCTP_PROTOCOL                        *This,
  IN     UINT8                                      MctpType,
  IN     UINT8                                      *MctpSourceEndpointId,
  IN     UINT8                                      *MctpDestinationEndpointId,
  IN     BOOLEAN                                    RequestDataIntegrityCheck,
  IN     UINT8                                      *RequestData,
  IN     UINT32                                     RequestDataSize,
  IN     UINT32                                     RequestTimeout,
  OUT    UINT8                                      *ResponseData,
  IN OUT UINT32                                     *ResponseDataSize,
  IN     UINT32                                     ResponseTimeout,
  OUT    MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalTransferError
  )
{
  EFI_STATUS  Status;
  UINT8       SourceEid;
  UINT8       DestinationEid;

  if ((RequestData == NULL) && (ResponseData == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Both RequestData and ResponseData are NULL\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (MctpSourceEndpointId == NULL) {
    SourceEid = PcdGet8 (PcdMctpSourceEndpointId);
    DEBUG ((DEBUG_MANAGEABILITY, "%a: Use PcdMctpSourceEndpointId for MCTP source EID: %x\n", __func__, SourceEid));
  } else {
    SourceEid = *MctpSourceEndpointId;
    DEBUG ((DEBUG_MANAGEABILITY, "%a: MCTP source EID: %x\n", __func__, SourceEid));
  }

  if (MctpDestinationEndpointId == NULL) {
    DestinationEid = PcdGet8 (PcdMctpDestinationEndpointId);
    DEBUG ((DEBUG_MANAGEABILITY, "%a: Use PcdMctpDestinationEndpointId for MCTP destination EID: %x\n", __func__, DestinationEid));
  } else {
    DestinationEid = *MctpDestinationEndpointId;
    DEBUG ((DEBUG_MANAGEABILITY, "%a: MCTP destination EID: %x\n", __func__, DestinationEid));
  }

  //
  // Check source EID and destination EID
  //
  if ((SourceEid >= MCTP_RESERVED_ENDPOINT_START_ID) &&
      (SourceEid <= MCTP_RESERVED_ENDPOINT_END_ID)
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: The value of MCTP source EID (%x) is reserved.\n", __func__, MctpSourceEndpointId));
    return EFI_INVALID_PARAMETER;
  }

  if ((DestinationEid >= MCTP_RESERVED_ENDPOINT_START_ID) &&
      (DestinationEid <= MCTP_RESERVED_ENDPOINT_END_ID)
      )
  {
    DEBUG ((DEBUG_ERROR, "%a: The value of MCTP destination EID (%x) is reserved.\n", __func__, MctpDestinationEndpointId));
    return EFI_INVALID_PARAMETER;
  }

  Status = CommonMctpSubmitMessage (
             mTransportToken,
             MctpType,
             SourceEid,
             DestinationEid,
             RequestDataIntegrityCheck,
             RequestData,
             RequestDataSize,
             RequestTimeout,
             ResponseData,
             ResponseDataSize,
             ResponseTimeout,
             AdditionalTransferError
             );
  return Status;
}

EDKII_MCTP_PROTOCOL_V1_0  mMctpProtocolV10 = {
  MctpSubmitMessage
};

EDKII_MCTP_PROTOCOL  mMctpProtocol;

/**
  The entry point of the MCTP DXE driver.

  @param[in] ImageHandle - Handle of this driver image
  @param[in] SystemTable - Table containing standard EFI services

  @retval EFI_SUCCESS    - edkii MCTP Protocol is installed successfully.
  @retval Otherwise      - Other errors.
**/
EFI_STATUS
EFIAPI
DxeMctpEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                 Status;
  EFI_HANDLE                                 Handle;
  MANAGEABILITY_TRANSPORT_CAPABILITY         TransportCapability;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  TransportAdditionalStatus;

  Status = HelperAcquireManageabilityTransport (
             &gManageabilityProtocolMctpGuid,
             &mTransportToken
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to acquire transport interface for MCTP protocol - %r\n", __func__, Status));
    return Status;
  }

  Status = GetTransportCapability (mTransportToken, &TransportCapability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to GetTransportCapability().\n", __func__));
    return Status;
  }

  mTransportMaximumPayload = MANAGEABILITY_TRANSPORT_PAYLOAD_SIZE_FROM_CAPABILITY (TransportCapability);
  if (mTransportMaximumPayload == (1 << MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_NOT_AVAILABLE)) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Transport interface maximum payload is undefined.\n", __func__));
  } else {
    mTransportMaximumPayload -= 1;
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Transport interface for MCTP protocol has maximum payload 0x%x.\n", __func__, mTransportMaximumPayload));
  }

  mTransportName = HelperManageabilitySpecName (mTransportToken->Transport->ManageabilityTransportSpecification);
  DEBUG ((DEBUG_ERROR, "%a: MCTP protocol over %s.\n", __func__, mTransportName));

  //
  // Setup hardware information according to the transport interface.
  Status = SetupMctpTransportHardwareInformation (
             mTransportToken,
             &mHardwareInformation
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "%a: No hardware information of %s transport interface.\n", __func__, mTransportName));
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Failed to setup hardware information of %s transport interface.\n", __func__, mTransportName));
    }

    return Status;
  }

  // Initial transport interface with the hardware information assigned.
  Status = HelperInitManageabilityTransport (
             mTransportToken,
             mHardwareInformation,
             &TransportAdditionalStatus
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mMctpProtocol.ProtocolVersion      = EDKII_MCTP_PROTOCOL_VERSION;
  mMctpProtocol.Functions.Version1_0 = &mMctpProtocolV10;
  Handle                             = NULL;
  Status                             = gBS->InstallProtocolInterface (
                                              &Handle,
                                              &gEdkiiMctpProtocolGuid,
                                              EFI_NATIVE_INTERFACE,
                                              (VOID **)&mMctpProtocol
                                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install EDKII MCTP protocol - %r\n", __func__, Status));
  }

  return Status;
}

/**
  This is the unload handler for MCTP protocol module.

  Release the MANAGEABILITY_TRANSPORT_TOKEN acquired at entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
MctpUnloadImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if (mTransportToken != NULL) {
    Status = ReleaseTransportSession (mTransportToken);
  }

  return Status;
}
