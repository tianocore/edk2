/** @file
  This file provides the IPMI Standalone MM Protocol implementation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/MmServicesTableLib.h>

#include <Protocol/IpmiProtocol.h>

#include "IpmiProtocolCommon.h"

MANAGEABILITY_TRANSPORT_TOKEN                 *mTransportToken = NULL;
CHAR16                                        *mTransportName;
UINT32                                        TransportMaximumPayload;
MANAGEABILITY_TRANSPORT_HARDWARE_INFORMATION  mHardwareInformation;

/**
  This service enables submitting commands via IPMI.

  @param[in]         This              This pointer for IPMI_PROTOCOL structure.
  @param[in]         NetFunction       Net function of the command.
  @param[in]         Command           IPMI command.
  @param[in]         RequestData       Command request data.
  @param[in]         RequestDataSize   Size of command request data.
  @param[out]        ResponseData      Command response data. The completion code is the first byte of response data.
  @param[in, out]    ResponseDataSize  Size of command response data.

  @retval EFI_SUCCESS            The command was submitted and a response was received.
  @retval EFI_NOT_FOUND          The command or response transfer failed.
  @retval EFI_NOT_READY          The IPMI device is not ready.
  @retval EFI_DEVICE_ERROR       The IPMI device reported a hardware error.
  @retval EFI_TIMEOUT            The command timed out.
  @retval EFI_UNSUPPORTED        The command could not be sent.
  @retval EFI_OUT_OF_RESOURCES   A resource or data size limit was reached.
**/
EFI_STATUS
EFIAPI
StandaloneMmIpmiSubmitCommand (
  IN     IPMI_PROTOCOL  *This,
  IN     UINT8          NetFunction,
  IN     UINT8          Command,
  IN     UINT8          *RequestData,
  IN     UINT32         RequestDataSize,
  OUT    UINT8          *ResponseData,
  IN OUT UINT32         *ResponseDataSize
  )
{
  return CommonIpmiSubmitCommand (
           mTransportToken,
           NetFunction,
           Command,
           RequestData,
           RequestDataSize,
           ResponseData,
           ResponseDataSize
           );
}

STATIC IPMI_PROTOCOL  mIpmiProtocol = {
  StandaloneMmIpmiSubmitCommand
};

/**
  The entry point of the IPMI Standalone MM driver.

  @param[in] ImageHandle    Handle of this driver image.
  @param[in] MmSystemTable  Pointer to the MM system table.

  @retval EFI_SUCCESS  The IPMI protocol was installed successfully.
  @retval Others       An error occurred while initializing the transport or
                       installing the protocol.
**/
EFI_STATUS
EFIAPI
StandaloneMmIpmiEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
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

  Status = SetupIpmiTransportHardwareInformation (
             mTransportToken,
             &mHardwareInformation
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "%a: No hardware information for %s transport interface.\n", __func__, mTransportName));
    }

    return Status;
  }

  Status = HelperInitManageabilityTransport (
             mTransportToken,
             mHardwareInformation,
             &TransportAdditionalStatus
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = NULL;
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gSmmIpmiProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mIpmiProtocol
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install IPMI Standalone MM protocol - %r\n", __func__, Status));
  }

  return Status;
}
