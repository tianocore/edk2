/** @file
  This file provides edk2 PLDM SMBIOS Transfer Protocol implementation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <IndustryStandard/Pldm.h>
#include <Protocol/PldmProtocol.h>

#include "PldmProtocolCommon.h"

MANAGEABILITY_TRANSPORT_TOKEN  *mTransportToken = NULL;
CHAR16                         *mTransportName;
UINT8                          mPldmRequestInstanceId;
UINT32                         TransportMaximumPayload;

/**
  This service enables submitting commands via EDKII PLDM protocol.

  @param[in]         This                       EDKII_PLDM_PROTOCOL instance.
  @param[in]         PldmType                   PLDM message type.
  @param[in]         Command                    PLDM Command of PLDM message type.
  @param[in]         PldmTerminusSourceId       PLDM source teminus ID.
  @param[in]         PldmTerminusDestinationId  PLDM destination teminus ID.
  @param[in]         RequestData                Command Request Data.
  @param[in]         RequestDataSize            Size of Command Request Data.
  @param[out]        ResponseData               Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize           Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          PLDM transport interface is not ready for PLDM command access.
  @retval EFI_DEVICE_ERROR       PLDM transport interface Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allcation is out of resource or data size error.
  @retval EFI_INVALID_PARAMETER  Both RequestData and ResponseData are NULL
**/
EFI_STATUS
EFIAPI
PldmSubmitCommand (
  IN     EDKII_PLDM_PROTOCOL  *This,
  IN     UINT8                PldmType,
  IN     UINT8                Command,
  IN     UINT8                PldmTerminusSourceId,
  IN     UINT8                PldmTerminusDestinationId,
  IN     UINT8                *RequestData,
  IN     UINT32               RequestDataSize,
  OUT    UINT8                *ResponseData,
  IN OUT UINT32               *ResponseDataSize
  )
{
  EFI_STATUS  Status;

  //
  // Check the given input parameters.
  //
  if ((RequestData == NULL) && (RequestDataSize != 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RequestDataSize != 0, however RequestData is NULL for PLDM type: 0x%x, Command: 0x%x.\n",
      __func__,
      PldmType,
      Command
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((RequestData != NULL) && (RequestDataSize == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RequestDataSize == 0, however RequestData is not NULL for PLDM type: 0x%x, Command: 0x%x.\n",
      __func__,
      PldmType,
      Command
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((ResponseData == NULL) && (*ResponseDataSize != 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: *ResponseDataSize != 0, however ResponseData is NULL for PLDM type: 0x%x, Command: 0x%x.\n",
      __func__,
      PldmType,
      Command
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((ResponseData != NULL) && (*ResponseDataSize == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: *ResponseDataSize == 0, however ResponseData is not NULL for PLDM type: 0x%x, Command: 0x%x.\n",
      __func__,
      PldmType,
      Command
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Source terminus ID: 0x%x, Destination terminus ID: 0x%x.\n"));
  Status = CommonPldmSubmitCommand (
             mTransportToken,
             PldmType,
             Command,
             PldmTerminusSourceId,
             PldmTerminusDestinationId,
             RequestData,
             RequestDataSize,
             ResponseData,
             ResponseDataSize
             );
  return Status;
}

EDKII_PLDM_PROTOCOL_V1_0  mPldmProtocolV10 = {
  PldmSubmitCommand
};

EDKII_PLDM_PROTOCOL  mPldmProtocol;

/**
  The entry point of the PLDM SMBIOS Transfer DXE driver.

  @param[in] ImageHandle - Handle of this driver image
  @param[in] SystemTable - Table containing standard EFI services

  @retval EFI_SUCCESS    - PLDM Protocol is installed successfully.
  @retval Otherwise      - Other errors.
**/
EFI_STATUS
EFIAPI
DxePldmProtocolEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                    Status;
  EFI_HANDLE                                    Handle;
  MANAGEABILITY_TRANSPORT_CAPABILITY            TransportCapability;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS     TransportAdditionalStatus;
  MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  HardwareInfo;

  Status = HelperAcquireManageabilityTransport (
             &gManageabilityProtocolPldmGuid,
             &mTransportToken
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to acquire transport interface for PLDM protocol - %r\n", __func__, Status));
    return Status;
  }

  Status = GetTransportCapability (mTransportToken, &TransportCapability);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to GetTransportCapability().\n", __func__));
    return Status;
  }

  TransportMaximumPayload = MANAGEABILITY_TRANSPORT_PAYLOAD_SIZE_FROM_CAPABILITY (TransportCapability);
  if (TransportMaximumPayload == (1 << MANAGEABILITY_TRANSPORT_CAPABILITY_MAXIMUM_PAYLOAD_NOT_AVAILABLE)) {
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Transport interface maximum payload is undefined.\n", __func__));
  } else {
    TransportMaximumPayload -= 1;
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Transport interface for PLDM protocol has maximum payload 0x%x.\n", __func__, TransportMaximumPayload));
  }

  mTransportName = HelperManageabilitySpecName (mTransportToken->Transport->ManageabilityTransportSpecification);
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: PLDM protocol over %s.\n", __func__, mTransportName));

  // Initial transport interface with the hardware information assigned.
  HardwareInfo.Pointer = NULL;
  Status               = HelperInitManageabilityTransport (
                           mTransportToken,
                           HardwareInfo,
                           &TransportAdditionalStatus
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mPldmRequestInstanceId             = 0;
  mPldmProtocol.ProtocolVersion      = EDKII_PLDM_PROTOCOL_VERSION;
  mPldmProtocol.Functions.Version1_0 = &mPldmProtocolV10;
  Handle                             = NULL;
  Status                             = gBS->InstallProtocolInterface (
                                              &Handle,
                                              &gEdkiiPldmProtocolGuid,
                                              EFI_NATIVE_INTERFACE,
                                              (VOID **)&mPldmProtocol
                                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install EDKII PLDM protocol - %r\n", __func__, Status));
  }

  return Status;
}

/**
  This is the unload handler of PLDM SMBIOS Transfer DXE driver.

  @param[in] ImageHandle           The driver's image handle.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
PldmProtocolUnloadImage (
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
