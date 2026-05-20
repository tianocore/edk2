/** @file
  This file provides IPMI SMM Protocol implementation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/IpmiProtocol.h>

#include "IpmiProtocolCommon.h"

MANAGEABILITY_TRANSPORT_TOKEN                 *mTransportToken = NULL;
CHAR16                                        *mTransportName;
UINT32                                        TransportMaximumPayload;
MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  mHardwareInformation;

/**
  This service enables submitting commands via Ipmi.

  @param[in]         This              This point for IPMI_PROTOCOL structure.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI Command.
  @param[in]         RequestData       Command Request Data.
  @param[in]         RequestDataSize   Size of Command Request Data.
  @param[out]        ResponseData      Command Response Data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of Command Response Data.

  @retval EFI_SUCCESS            The command byte stream was successfully submit to the device and a response was successfully received.
  @retval EFI_NOT_FOUND          The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_NOT_READY          Ipmi Device is not ready for Ipmi command access.
  @retval EFI_DEVICE_ERROR       Ipmi Device hardware error.
  @retval EFI_TIMEOUT            The command time out.
  @retval EFI_UNSUPPORTED        The command was not successfully sent to the device.
  @retval EFI_OUT_OF_RESOURCES   The resource allocation is out of resource or data size error.
**/
EFI_STATUS
EFIAPI
SmmIpmiSubmitCommand (
  IN     IPMI_PROTOCOL  *This,
  IN     UINT8          NetFunction,
  IN     UINT8          Command,
  IN     UINT8          *RequestData,
  IN     UINT32         RequestDataSize,
  OUT    UINT8          *ResponseData,
  IN OUT UINT32         *ResponseDataSize
  )
{
  EFI_STATUS  Status;

  Status = CommonIpmiSubmitCommand (
             mTransportToken,
             NetFunction,
             Command,
             RequestData,
             RequestDataSize,
             ResponseData,
             ResponseDataSize
             );
  return Status;
}

static IPMI_PROTOCOL  mIpmiProtocol = {
  SmmIpmiSubmitCommand
};

/**
  The entry point of the Ipmi DXE driver.

  @param[in] ImageHandle - Handle of this driver image
  @param[in] SystemTable - Table containing standard EFI services

  @retval EFI_SUCCESS    - IPMI Protocol is installed successfully.
  @retval Otherwise      - Other errors.
**/
EFI_STATUS
EFIAPI
SmmIpmiEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                                 Status;
  EFI_HANDLE                                 Handle;
  MANAGEABILITY_TRANSPORT_CAPABILITY         TransportCapability;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  TransportAdditionalStatus;

  Status = HelperAcquireManageabilityTransport (
             &gManageabilityProtocolIpmiGuid,
             &mTransportToken
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to acquire transport interface for IPMI protocol - %r\n", __func__, Status));
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
    DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Transport interface for IPMI protocol has maximum payload 0x%x.\n", __func__, TransportMaximumPayload));
  }

  mTransportName = HelperManageabilitySpecName (mTransportToken->Transport->ManageabilityTransportSpecification);
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: IPMI protocol over %s.\n", __func__, mTransportName));

  //
  // Setup hardware information according to the transport interface.
  Status = SetupIpmiTransportHardwareInformation (
             mTransportToken,
             &mHardwareInformation
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "%a: No hardware information of %s transport interface.\n", __func__, mTransportName));
    }

    return Status;
  }

  //
  // Initial transport interface with the hardware information assigned.
  Status = HelperInitManageabilityTransport (
             mTransportToken,
             mHardwareInformation,
             &TransportAdditionalStatus
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = NULL;
  Status = gSmst->SmmInstallProtocolInterface (
                    &Handle,
                    &gSmmIpmiProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID **)&mIpmiProtocol
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install IPMI SMM protocol - %r\n", __func__, Status));
  }

  return Status;
}
