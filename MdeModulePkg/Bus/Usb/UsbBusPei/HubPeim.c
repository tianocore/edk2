/** @file
Usb Hub Request Support In PEI Phase

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbPeim.h"
#include "HubPeim.h"
#include "PeiUsbLib.h"

/**
  Get a given hub port status.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  PortStatus    Current Hub port status and change status.

  @retval EFI_SUCCESS       Port status is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the port status due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubGetPortStatus (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_USB_IO_PPI    *UsbIoPpi,
  IN  UINT8             Port,
  OUT UINT32            *PortStatus
  )
{
  EFI_USB_DEVICE_REQUEST  DeviceRequest;

  ZeroMem (&DeviceRequest, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DeviceRequest.RequestType = USB_HUB_GET_PORT_STATUS_REQ_TYPE;
  DeviceRequest.Request     = USB_HUB_GET_PORT_STATUS;
  DeviceRequest.Index       = Port;
  DeviceRequest.Length      = (UINT16)sizeof (UINT32);

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DeviceRequest,
                     EfiUsbDataIn,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     PortStatus,
                     sizeof (UINT32)
                     );
}

/**
  Set specified feature to a given hub port.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  Value         New feature value.

  @retval EFI_SUCCESS       Port feature is set successfully.
  @retval EFI_DEVICE_ERROR  Cannot set the port feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubSetPortFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             Port,
  IN UINT8             Value
  )
{
  EFI_USB_DEVICE_REQUEST  DeviceRequest;

  ZeroMem (&DeviceRequest, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DeviceRequest.RequestType = USB_HUB_SET_PORT_FEATURE_REQ_TYPE;
  DeviceRequest.Request     = USB_HUB_SET_PORT_FEATURE;
  DeviceRequest.Value       = Value;
  DeviceRequest.Index       = Port;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DeviceRequest,
                     EfiUsbNoData,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     NULL,
                     0
                     );
}

/**
  Clear specified feature on a given hub port.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  Value         Feature value that will be cleared from the hub port.

  @retval EFI_SUCCESS       Port feature is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the port feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubClearPortFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             Port,
  IN UINT8             Value
  )
{
  EFI_USB_DEVICE_REQUEST  DeviceRequest;

  ZeroMem (&DeviceRequest, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DeviceRequest.RequestType = USB_HUB_CLEAR_FEATURE_PORT_REQ_TYPE;
  DeviceRequest.Request     = USB_HUB_CLEAR_FEATURE_PORT;
  DeviceRequest.Value       = Value;
  DeviceRequest.Index       = Port;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DeviceRequest,
                     EfiUsbNoData,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     NULL,
                     0
                     );
}

/**
  Get a given hub status.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  HubStatus     Current Hub status and change status.

  @retval EFI_SUCCESS       Hub status is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the hub status due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubGetHubStatus (
  IN  EFI_PEI_SERVICES  **PeiServices,
  IN  PEI_USB_IO_PPI    *UsbIoPpi,
  OUT UINT32            *HubStatus
  )
{
  EFI_USB_DEVICE_REQUEST  DeviceRequest;

  ZeroMem (&DeviceRequest, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DeviceRequest.RequestType = USB_HUB_GET_HUB_STATUS_REQ_TYPE;
  DeviceRequest.Request     = USB_HUB_GET_HUB_STATUS;
  DeviceRequest.Length      = (UINT16)sizeof (UINT32);

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DeviceRequest,
                     EfiUsbDataIn,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     HubStatus,
                     sizeof (UINT32)
                     );
}

/**
  Clear specified feature on a given hub.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Value         Feature value that will be cleared from the hub port.

  @retval EFI_SUCCESS       Hub feature is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the hub feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubClearHubFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             Value
  )
{
  EFI_USB_DEVICE_REQUEST  DeviceRequest;

  ZeroMem (&DeviceRequest, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DeviceRequest.RequestType = USB_HUB_CLEAR_FEATURE_REQ_TYPE;
  DeviceRequest.Request     = USB_HUB_CLEAR_FEATURE;
  DeviceRequest.Value       = Value;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DeviceRequest,
                     EfiUsbNoData,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     NULL,
                     0
                     );
}

/**
  Get a given (SuperSpeed) hub descriptor.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice   Indicates the hub controller device.
  @param  UsbIoPpi       Indicates the PEI_USB_IO_PPI instance.
  @param  DescriptorSize The length of Hub Descriptor buffer.
  @param  HubDescriptor  Caller allocated buffer to store the hub descriptor if
                         successfully returned.

  @retval EFI_SUCCESS       Hub descriptor is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the hub descriptor due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiGetHubDescriptor (
  IN  EFI_PEI_SERVICES        **PeiServices,
  IN  PEI_USB_DEVICE          *PeiUsbDevice,
  IN  PEI_USB_IO_PPI          *UsbIoPpi,
  IN  UINTN                   DescriptorSize,
  OUT EFI_USB_HUB_DESCRIPTOR  *HubDescriptor
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT8                   DescType;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DescType = (PeiUsbDevice->DeviceSpeed == EFI_USB_SPEED_SUPER) ?
             USB_DT_SUPERSPEED_HUB :
             USB_DT_HUB;

  //
  // Fill Device request packet
  //
  DevReq.RequestType = USB_RT_HUB | 0x80;
  DevReq.Request     = USB_HUB_GET_DESCRIPTOR;
  DevReq.Value       = (UINT16)(DescType << 8);
  DevReq.Length      = (UINT16)DescriptorSize;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DevReq,
                     EfiUsbDataIn,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     HubDescriptor,
                     (UINT16)DescriptorSize
                     );
}

/**
  Read the whole usb hub descriptor. It is necessary
  to do it in two steps because hub descriptor is of
  variable length.

  @param  PeiServices       General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice      Indicates the hub controller device.
  @param  UsbIoPpi          Indicates the PEI_USB_IO_PPI instance.
  @param  HubDescriptor     Caller allocated buffer to store the hub descriptor if
                            successfully returned.

  @retval EFI_SUCCESS       Hub descriptor is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the hub descriptor due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbHubReadDesc (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_DEVICE           *PeiUsbDevice,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  OUT EFI_USB_HUB_DESCRIPTOR  *HubDescriptor
  )
{
  EFI_STATUS  Status;

  //
  // First get the hub descriptor length
  //
  Status = PeiGetHubDescriptor (PeiServices, PeiUsbDevice, UsbIoPpi, 2, HubDescriptor);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the whole hub descriptor
  //
  return PeiGetHubDescriptor (PeiServices, PeiUsbDevice, UsbIoPpi, HubDescriptor->Length, HubDescriptor);
}

/**
  USB hub control transfer to set the hub depth.

  @param  PeiServices       General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice      Indicates the hub controller device.
  @param  UsbIoPpi          Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS       Depth of the hub is set.
  @retval Others            Failed to set the depth.

**/
EFI_STATUS
PeiUsbHubCtrlSetHubDepth (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_DEVICE    *PeiUsbDevice,
  IN PEI_USB_IO_PPI    *UsbIoPpi
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType = USB_RT_HUB;
  DevReq.Request     = USB_HUB_REQ_SET_DEPTH;
  DevReq.Value       = PeiUsbDevice->Tier;
  DevReq.Length      = 0;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DevReq,
                     EfiUsbNoData,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     NULL,
                     0
                     );
}

/**
  Configure a given hub.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice   Indicating the hub controller device that will be configured

  @retval EFI_SUCCESS       Hub configuration is done successfully.
  @retval EFI_DEVICE_ERROR  Cannot configure the hub due to a hardware error.

**/
EFI_STATUS
PeiDoHubConfig (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_DEVICE    *PeiUsbDevice
  )
{
  UINT8                   HubDescBuffer[256];
  EFI_USB_HUB_DESCRIPTOR  *HubDescriptor;
  EFI_STATUS              Status;
  EFI_USB_HUB_STATUS      HubStatus;
  UINTN                   Index;
  PEI_USB_IO_PPI          *UsbIoPpi;

  UsbIoPpi = &PeiUsbDevice->UsbIoPpi;

  //
  // The length field of descriptor is UINT8 type, so the buffer
  // with 256 bytes is enough to hold the descriptor data.
  //
  HubDescriptor = (EFI_USB_HUB_DESCRIPTOR *)HubDescBuffer;

  //
  // Get the hub descriptor
  //
  Status = PeiUsbHubReadDesc (
             PeiServices,
             PeiUsbDevice,
             UsbIoPpi,
             HubDescriptor
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  PeiUsbDevice->DownStreamPortNo = HubDescriptor->NbrPorts;

  if (PeiUsbDevice->DeviceSpeed == EFI_USB_SPEED_SUPER) {
    DEBUG ((DEBUG_INFO, "PeiDoHubConfig: Set Hub Depth as 0x%x\n", PeiUsbDevice->Tier));
    PeiUsbHubCtrlSetHubDepth (
      PeiServices,
      PeiUsbDevice,
      UsbIoPpi
      );
  } else {
    //
    //  Power all the hub ports
    //
    for (Index = 0; Index < PeiUsbDevice->DownStreamPortNo; Index++) {
      Status = PeiHubSetPortFeature (
                 PeiServices,
                 UsbIoPpi,
                 (UINT8)(Index + 1),
                 EfiUsbPortPower
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "PeiDoHubConfig: PeiHubSetPortFeature EfiUsbPortPower failed %x\n", Index));
        continue;
      }
    }

    DEBUG ((DEBUG_INFO, "PeiDoHubConfig: HubDescriptor.PwrOn2PwrGood: 0x%x\n", HubDescriptor->PwrOn2PwrGood));
    if (HubDescriptor->PwrOn2PwrGood > 0) {
      MicroSecondDelay (HubDescriptor->PwrOn2PwrGood * USB_SET_PORT_POWER_STALL);
    }

    //
    // Clear Hub Status Change
    //
    Status = PeiHubGetHubStatus (
               PeiServices,
               UsbIoPpi,
               (UINT32 *)&HubStatus
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    } else {
      //
      // Hub power supply change happens
      //
      if ((HubStatus.HubChangeStatus & HUB_CHANGE_LOCAL_POWER) != 0) {
        PeiHubClearHubFeature (
          PeiServices,
          UsbIoPpi,
          C_HUB_LOCAL_POWER
          );
      }

      //
      // Hub change overcurrent happens
      //
      if ((HubStatus.HubChangeStatus & HUB_CHANGE_OVERCURRENT) != 0) {
        PeiHubClearHubFeature (
          PeiServices,
          UsbIoPpi,
          C_HUB_OVER_CURRENT
          );
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Send reset signal over the given root hub port.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  UsbIoPpi       Indicates the PEI_USB_IO_PPI instance.
  @param  PortNum        Usb hub port number (starting from 1).

**/
VOID
PeiResetHubPort (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             PortNum
  )
{
  EFI_STATUS           Status;
  UINTN                Index;
  EFI_USB_PORT_STATUS  HubPortStatus;

  MicroSecondDelay (100 * 1000);

  //
  // reset root port
  //
  PeiHubSetPortFeature (
    PeiServices,
    UsbIoPpi,
    PortNum,
    EfiUsbPortReset
    );

  //
  // Drive the reset signal for worst 20ms. Check USB 2.0 Spec
  // section 7.1.7.5 for timing requirements.
  //
  MicroSecondDelay (USB_SET_PORT_RESET_STALL);

  //
  // Check USB_PORT_STAT_C_RESET bit to see if the resetting state is done.
  //
  ZeroMem (&HubPortStatus, sizeof (EFI_USB_PORT_STATUS));

  for (Index = 0; Index < USB_WAIT_PORT_STS_CHANGE_LOOP; Index++) {
    Status = PeiHubGetPortStatus (
               PeiServices,
               UsbIoPpi,
               PortNum,
               (UINT32 *)&HubPortStatus
               );

    if (EFI_ERROR (Status)) {
      return;
    }

    if (USB_BIT_IS_SET (HubPortStatus.PortChangeStatus, USB_PORT_STAT_C_RESET)) {
      break;
    }

    MicroSecondDelay (USB_WAIT_PORT_STS_CHANGE_STALL);
  }

  if (Index == USB_WAIT_PORT_STS_CHANGE_LOOP) {
    DEBUG ((DEBUG_ERROR, "PeiResetHubPort: reset not finished in time on port %d\n", PortNum));
    return;
  }

  //
  // clear reset change root port
  //
  PeiHubClearPortFeature (
    PeiServices,
    UsbIoPpi,
    PortNum,
    EfiUsbPortResetChange
    );

  MicroSecondDelay (1 * 1000);

  PeiHubClearPortFeature (
    PeiServices,
    UsbIoPpi,
    PortNum,
    EfiUsbPortConnectChange
    );

  //
  // Set port enable
  //
  PeiHubSetPortFeature (
    PeiServices,
    UsbIoPpi,
    PortNum,
    EfiUsbPortEnable
    );

  //
  // Clear any change status
  //

  PeiHubClearPortFeature (
    PeiServices,
    UsbIoPpi,
    PortNum,
    EfiUsbPortEnableChange
    );

  MicroSecondDelay (10 * 1000);

  return;
}
