/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    Hub.c
    
  Abstract:

    Usb Hub Request

  Revision History

--*/

#include "usbbus.h"

EFI_STATUS
HubGetPortStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Port,
  OUT UINT32                  *PortStatus
  )
/*++
  
  Routine Description:
    Get a given hub port status
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    Port            -   Usb hub port number (starting from 1).
    PortStatus      -   Current Hub port status and change status.
    
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
    EFI_INVALID_PARAMETER
    
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_GET_PORT_STATUS_REQ_TYPE;
  DevReq.Request      = HUB_GET_PORT_STATUS;
  DevReq.Value        = 0;
  DevReq.Index        = Port;
  DevReq.Length       = sizeof (UINT32);

  Timeout             = 3000;

  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbDataIn,
                      Timeout,
                      PortStatus,
                      sizeof (UINT32),
                      &UsbStatus
                      );

  return EfiStatus;
}

EFI_STATUS
HubSetPortFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Port,
  IN UINT8                   Value
  )
/*++
  
  Routine Description:
    Set specified feature to a give hub port
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    Port            -   Usb hub port number (starting from 1).
    Value           -   New feature value.
  
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
    EFI_INVALID_PARAMETER
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_SET_PORT_FEATURE_REQ_TYPE;
  DevReq.Request      = HUB_SET_PORT_FEATURE;
  DevReq.Value        = Value;
  DevReq.Index        = Port;
  DevReq.Length       = 0;

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbNoData,
                      Timeout,
                      NULL,
                      0,
                      &UsbStatus
                      );

  return EfiStatus;
}

EFI_STATUS
HubClearPortFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Port,
  IN UINT8                   Value
  )
/*++
  
  Routine Description:
    Clear a specified feature of a given hub port
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    Port            -   Usb hub port number (starting from 1).
    Value           -   Feature value that will be cleared from
                        that hub port.
  
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
    EFI_INVALID_PARAMETER
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_CLEAR_FEATURE_PORT_REQ_TYPE;
  DevReq.Request      = HUB_CLEAR_FEATURE_PORT;
  DevReq.Value        = Value;
  DevReq.Index        = Port;
  DevReq.Length       = 0;

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbNoData,
                      Timeout,
                      NULL,
                      0,
                      &UsbStatus
                      );

  return EfiStatus;
}

EFI_STATUS
HubGetHubStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT32                  *HubStatus
  )
/*++
  
  Routine Description:
    Get Hub Status
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    HubStatus       -   Current Hub status and change status.
  
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_GET_HUB_STATUS_REQ_TYPE;
  DevReq.Request      = HUB_GET_HUB_STATUS;
  DevReq.Value        = 0;
  DevReq.Index        = 0;
  DevReq.Length       = sizeof (UINT32);

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbDataIn,
                      Timeout,
                      HubStatus,
                      sizeof (UINT32),
                      &UsbStatus
                      );

  return EfiStatus;
}

EFI_STATUS
HubSetHubFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Value
  )
/*++
  
  Routine Description:
    Set a specified feature to the hub
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    Value           -   Feature value that will be set to the hub.
  
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_SET_HUB_FEATURE_REQ_TYPE;
  DevReq.Request      = HUB_SET_HUB_FEATURE;
  DevReq.Value        = Value;
  DevReq.Index        = 0;
  DevReq.Length       = 0;

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbNoData,
                      Timeout,
                      NULL,
                      0,
                      &UsbStatus
                      );

  return EfiStatus;
}

EFI_STATUS
HubClearHubFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Value
  )
/*++
  
  Routine Description:
    Set a specified feature to the hub
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    Value           -   Feature value that will be cleared from the hub.
  
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = HUB_CLEAR_FEATURE_REQ_TYPE;
  DevReq.Request      = HUB_CLEAR_FEATURE;
  DevReq.Value        = Value;
  DevReq.Index        = 0;
  DevReq.Length       = 0;

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbNoData,
                      Timeout,
                      NULL,
                      0,
                      &UsbStatus
                      );

  return EfiStatus;

}

EFI_STATUS
GetHubDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINTN                      DescriptorSize,
  OUT EFI_USB_HUB_DESCRIPTOR     *HubDescriptor
  )
/*++
  
  Routine Description:
    Get the hub descriptor
  
  Arguments:
    UsbIo           -   EFI_USB_IO_PROTOCOL instance
    DescriptorSize  -   The length of Hub Descriptor buffer.
    HubDescriptor   -   Caller allocated buffer to store the hub descriptor
                        if successfully returned.
                              
  Returns:
    EFI_SUCCESS
    EFI_DEVICE
    EFI_TIME_OUT
  
--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              EfiStatus;
  UINT32                  UsbStatus;
  UINT32                  Timeout;

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  //
  // Fill Device request packet
  //
  DevReq.RequestType  = USB_RT_HUB | 0x80;
  DevReq.Request      = HUB_GET_DESCRIPTOR;
  DevReq.Value        = USB_DT_HUB << 8;
  DevReq.Index        = 0;
  DevReq.Length       = (UINT16) DescriptorSize;

  Timeout             = 3000;
  EfiStatus = UsbIo->UsbControlTransfer (
                      UsbIo,
                      &DevReq,
                      EfiUsbDataIn,
                      Timeout,
                      HubDescriptor,
                      (UINT16) DescriptorSize,
                      &UsbStatus
                      );

  return EfiStatus;

}

EFI_STATUS
DoHubConfig (
  IN USB_IO_CONTROLLER_DEVICE     *HubController
  )
/*++
  
  Routine Description:
    Configure the hub
  
  Arguments:
    HubController         -   Indicating the hub controller device that
                              will be configured
                                
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  EFI_USB_HUB_DESCRIPTOR  HubDescriptor;
  EFI_STATUS              Status;
  EFI_USB_HUB_STATUS      HubStatus;
  UINTN                   Index;
  UINT32                  PortStatus;

  UsbIo = &HubController->UsbIo;

  ZeroMem (&HubDescriptor, sizeof (HubDescriptor));

  //
  // First get the hub descriptor length
  //
  Status = GetHubDescriptor (UsbIo, 2, &HubDescriptor);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
    
  //
  // First get the whole descriptor, then
  // get the number of hub ports
  //
  Status = GetHubDescriptor (
            UsbIo,
            HubDescriptor.Length,
            &HubDescriptor
            );
  if (EFI_ERROR (Status)) {
    DEBUG ((gUSBErrorLevel, "Get hub descriptor fail\n"));
    return EFI_DEVICE_ERROR;
  }

  HubController->DownstreamPorts  = HubDescriptor.NbrPorts;

  Status                          = HubGetHubStatus (UsbIo, (UINT32 *) &HubStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((gUSBErrorLevel, "Get hub status fail when configure\n"));
    return EFI_DEVICE_ERROR;
  }
  
  //
  //  Get all hub ports status
  //
  for (Index = 0; Index < HubController->DownstreamPorts; Index++) {

    Status = HubGetPortStatus (UsbIo, (UINT8) (Index + 1), &PortStatus);
    if (EFI_ERROR (Status)) {
      continue;
    }
  }
  //
  //  Power all the hub ports
  //
  for (Index = 0; Index < HubController->DownstreamPorts; Index++) {
    Status = HubSetPortFeature (
              UsbIo,
              (UINT8) (Index + 1),
              EfiUsbPortPower
              );
    if (EFI_ERROR (Status)) {
      continue;
    }
  }
  
  //
  // Clear Hub Status Change
  //
  Status = HubGetHubStatus (UsbIo, (UINT32 *) &HubStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((gUSBErrorLevel, "Get hub status fail\n"));
    return EFI_DEVICE_ERROR;
  } else {
    //
    // Hub power supply change happens
    //
    if (HubStatus.HubChange & HUB_CHANGE_LOCAL_POWER) {
      HubClearHubFeature (UsbIo, C_HUB_LOCAL_POWER);
    }
    //
    // Hub change overcurrent happens
    //
    if (HubStatus.HubChange & HUB_CHANGE_OVERCURRENT) {
      HubClearHubFeature (UsbIo, C_HUB_OVER_CURRENT);
    }
  }

  return EFI_SUCCESS;

}
