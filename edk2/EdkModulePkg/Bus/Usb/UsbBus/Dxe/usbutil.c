/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:
    usbutil.c

  Abstract:

    Helper functions for USB

  Revision History

--*/

#include "usbbus.h"

//
// Following APIs are used to query Port Status
//
BOOLEAN
IsPortConnect (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if there is a device connected to that port according to
    the Port Status.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 0 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_CONNECTION) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortEnable (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if Port is enabled.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE  - Port is enable
    FALSE - Port is disable

--*/
{
  //
  // return the bit 1 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_ENABLE) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortInReset (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if the port is being reset.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 4 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_RESET) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortPowerApplied (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if there is power applied to that port.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 8 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_POWER) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortLowSpeedDeviceAttached (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if the connected device is a low device.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 9 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_LOW_SPEED) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortSuspend (
  IN UINT16  PortStatus
  )
/*++

  Routine Description:
    Tell if the port is suspend.

  Arguments:
    PortStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 2 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_SUSPEND) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}
//
// Following APIs are used to query Port Change Status
//
BOOLEAN
IsPortConnectChange (
  IN UINT16  PortChangeStatus
  )
/*++

  Routine Description:
    Tell if there is a Connect Change status in that port.

  Arguments:
    PortChangeStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 0 value of PortChangeStatus
  //
  if ((PortChangeStatus & USB_PORT_STAT_C_CONNECTION) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortEnableDisableChange (
  IN UINT16  PortChangeStatus
  )
/*++

  Routine Description:
    Tell if there is a Enable/Disable change in that port.

  Arguments:
    PortChangeStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 1 value of PortChangeStatus
  //
  if ((PortChangeStatus & USB_PORT_STAT_C_ENABLE) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortResetChange (
  IN UINT16  PortChangeStatus
  )
/*++

  Routine Description:
    Tell if there is a Port Reset Change status in that port.

  Arguments:
    PortChangeStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 4 value of PortChangeStatus
  //
  if ((PortChangeStatus & USB_PORT_STAT_C_RESET) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsPortSuspendChange (
  IN UINT16  PortChangeStatus
  )
/*++

  Routine Description:
    Tell if there is a Suspend Change Status in that port.

  Arguments:
    PortChangeStatus  -   The status value of that port.

  Returns:
    TRUE
    FALSE

--*/
{
  //
  // return the bit 2 value of PortChangeStatus
  //
  if ((PortChangeStatus & USB_PORT_STAT_C_SUSPEND) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

INTERFACE_DESC_LIST_ENTRY *
FindInterfaceListEntry (
  IN EFI_USB_IO_PROTOCOL    *This
  )
/*++

  Routine Description:
    Find Interface ListEntry.

  Arguments:
    This         -  EFI_USB_IO_PROTOCOL   
  
  Returns:
    INTERFACE_DESC_LIST_ENTRY pointer

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;
  USB_IO_DEVICE             *UsbIoDev;
  LIST_ENTRY                *InterfaceListHead;
  INTERFACE_DESC_LIST_ENTRY *InterfaceListEntry;

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);
  UsbIoDev        = UsbIoController->UsbDevice;

  if (!UsbIoDev->IsConfigured) {
    return NULL;
  }

  InterfaceListHead   = &UsbIoDev->ActiveConfig->InterfaceDescListHead;
  InterfaceListEntry  = (INTERFACE_DESC_LIST_ENTRY *) (InterfaceListHead->ForwardLink);

  //
  // Loop all interface descriptor to get match one.
  //
  while (InterfaceListEntry != (INTERFACE_DESC_LIST_ENTRY *) InterfaceListHead) {
    if (InterfaceListEntry->InterfaceDescriptor.InterfaceNumber == UsbIoController->InterfaceNumber) {
      return InterfaceListEntry;
    }

    InterfaceListEntry = (INTERFACE_DESC_LIST_ENTRY *) InterfaceListEntry->Link.ForwardLink;
  }

  return NULL;
}

ENDPOINT_DESC_LIST_ENTRY* 
FindEndPointListEntry (
  IN EFI_USB_IO_PROTOCOL    *This,
  IN UINT8                  EndPointAddress
  )
/*++

  Routine Description:
    Find EndPoint ListEntry.

  Arguments:
    This            -  EFI_USB_IO_PROTOCOL   
    EndPointAddress -  Endpoint address.
 
  Returns:
    ENDPOINT_DESC_LIST_ENTRY pointer

--*/
{
  INTERFACE_DESC_LIST_ENTRY *InterfaceListEntry;
  LIST_ENTRY                *EndpointDescListHead;
  ENDPOINT_DESC_LIST_ENTRY  *EndPointListEntry;

  InterfaceListEntry = FindInterfaceListEntry (This);
  if (InterfaceListEntry != NULL) {
    EndpointDescListHead  = &InterfaceListEntry->EndpointDescListHead;
    EndPointListEntry     = (ENDPOINT_DESC_LIST_ENTRY *) (EndpointDescListHead->ForwardLink);

    //
    // Loop all interface descriptor to get match one.
    //
    while (EndPointListEntry != (ENDPOINT_DESC_LIST_ENTRY *) EndpointDescListHead) {
      if (EndPointListEntry->EndpointDescriptor.EndpointAddress == EndPointAddress) {
        return EndPointListEntry;
      }

      EndPointListEntry = (ENDPOINT_DESC_LIST_ENTRY *) EndPointListEntry->Link.ForwardLink;
    }
  }

  return NULL;
}

VOID
GetDataToggleBit (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN  UINT8                 EndpointAddr,
  OUT UINT8                 *DataToggle
  )
/*++

  Routine Description:
    Get the datatoggle of a specified endpoint.

  Arguments:
    UsbIo         -     Given Usb Controller device.
    EndpointAddr  -     Given Endpoint address.
    DataToggle    -     The current data toggle of that endpoint

  Returns:
    N/A

--*/
{

  ENDPOINT_DESC_LIST_ENTRY  *EndpointListEntry;

  *DataToggle       = 0;

  EndpointListEntry = FindEndPointListEntry (UsbIo, EndpointAddr);
  if (EndpointListEntry == NULL) {
    return ;
  }

  *DataToggle = (UINT8) (EndpointListEntry->Toggle);
  return ;
}

VOID
SetDataToggleBit (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN UINT8                  EndpointAddr,
  IN UINT8                  DataToggle
  )
/*++

  Routine Description:
    Set the datatoggle of a specified endpoint

  Arguments:
    UsbIo         -     Given Usb Controller device.
    EndpointAddr  -     Given Endpoint address.
    DataToggle    -     The current data toggle of that endpoint to be set

  Returns:
    N/A

--*/
{

  ENDPOINT_DESC_LIST_ENTRY  *EndpointListEntry;

  EndpointListEntry = FindEndPointListEntry (UsbIo, EndpointAddr);
  if (EndpointListEntry == NULL) {
    return ;
  }

  EndpointListEntry->Toggle = DataToggle;
  return ;
}

VOID
GetDeviceEndPointMaxPacketLength (
  IN  EFI_USB_IO_PROTOCOL    *UsbIo,
  IN  UINT8                  EndpointAddr,
  OUT UINTN                  *MaxPacketLength
  )
/*++

  Routine Description:
    Get the Max Packet Length of the speified Endpoint.

  Arguments:
    UsbIo           -     Given Usb Controller device.
    EndpointAddr    -     Given Endpoint address.
    MaxPacketLength -     The max packet length of that endpoint

  Returns:
    N/A

--*/
{

  ENDPOINT_DESC_LIST_ENTRY  *EndpointListEntry;

  *MaxPacketLength  = 0;

  EndpointListEntry = FindEndPointListEntry (UsbIo, EndpointAddr);
  if (EndpointListEntry == NULL) {
    return ;
  }

  *MaxPacketLength = (UINTN) (EndpointListEntry->EndpointDescriptor.MaxPacketSize);

  return ;
}


EFI_STATUS
UsbSetDeviceAddress (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  AddressValue,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Address

Arguments:

  UsbIo         - EFI_USB_IO_PROTOCOL
  AddressValue  - Device address 
  Status        - Transfer status

Returns:

  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 


--*/
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_SET_ADDRESS_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_ADDRESS;
  DevReq.Value        = AddressValue;
 
  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbNoData,
                  TIMEOUT_VALUE,
                  NULL,
                  0,
                  Status
                  );
}

