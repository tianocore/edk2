/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

 Module Name:

    UsbDxeLib.c

 Abstract:

   Common Dxe Libarary  for USB

 Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

//
// Get Device Descriptor
//
EFI_STATUS
UsbGetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  OUT VOID                    *Descriptor,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index 
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to contain result
  Status            - Transfer Status
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

  DevReq.RequestType  = USB_DEV_GET_DESCRIPTOR_REQ_TYPE;
  DevReq.Request      = USB_DEV_GET_DESCRIPTOR;
  DevReq.Value        = Value;
  DevReq.Index        = Index;
  DevReq.Length       = DescriptorLength;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  Descriptor,
                  DescriptorLength,
                  Status
                  );
}
//
// Set Device Descriptor
//
EFI_STATUS
UsbSetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  IN  VOID                    *Descriptor,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Descriptor

Arguments:

  UsbIo             - EFI_USB_IO_PROTOCOL
  Value             - Device Request Value
  Index             - Device Request Index 
  DescriptorLength  - Descriptor Length
  Descriptor        - Descriptor buffer to set
  Status            - Transfer Status
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

  DevReq.RequestType  = USB_DEV_SET_DESCRIPTOR_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_DESCRIPTOR;
  DevReq.Value        = Value;
  DevReq.Index        = Index;
  DevReq.Length       = DescriptorLength;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataOut,
                  TIMEOUT_VALUE,
                  Descriptor,
                  DescriptorLength,
                  Status
                  );
}

//
// Get device Interface
//
EFI_STATUS
UsbGetDeviceInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Index,
  OUT UINT8                   *AltSetting,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  Index       - Interface index value
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

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

  DevReq.RequestType  = USB_DEV_GET_INTERFACE_REQ_TYPE;
  DevReq.Request      = USB_DEV_GET_INTERFACE;
  DevReq.Index        = Index;
  DevReq.Length       = 1;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  AltSetting,
                  1,
                  Status
                  );
}
//
// Set device interface
//
EFI_STATUS
UsbSetDeviceInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  InterfaceNo,
  IN  UINT16                  AltSetting,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Interface

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  InterfaceNo - Interface Number
  AltSetting  - Alternate setting
  Status      - Trasnsfer status

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

  DevReq.RequestType  = USB_DEV_SET_INTERFACE_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_INTERFACE;
  DevReq.Value        = AltSetting;
  DevReq.Index        = InterfaceNo;
 

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
//
// Get device configuration
//
EFI_STATUS
UsbGetDeviceConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT8                   *ConfigValue,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Configuration

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  ConfigValue - Config Value
  Status      - Transfer Status

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

  DevReq.RequestType  = USB_DEV_GET_CONFIGURATION_REQ_TYPE;
  DevReq.Request      = USB_DEV_GET_CONFIGURATION;
  DevReq.Length       = 1;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  ConfigValue,
                  1,
                  Status
                  );
}
//
// Set device configuration
//
EFI_STATUS
UsbSetDeviceConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Configuration

Arguments:

  UsbIo   - EFI_USB_IO_PROTOCOL
  Value   - Configuration Value to set
  Status  - Transfer status

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

  DevReq.RequestType  = USB_DEV_SET_CONFIGURATION_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_CONFIGURATION;
  DevReq.Value        = Value;
 
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
//
//  Set Device Feature
//
EFI_STATUS
UsbSetDeviceFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  EFI_USB_RECIPIENT       Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Set Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

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

  switch (Recipient) {

  case EfiUsbDevice:
    DevReq.RequestType = 0x00;
    break;

  case EfiUsbInterface:
    DevReq.RequestType = 0x01;
    break;

  case EfiUsbEndpoint:
    DevReq.RequestType = 0x02;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_DEV_SET_FEATURE;
  DevReq.Value    = Value;
  DevReq.Index    = Target;


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
//
// Clear Device Feature
//
EFI_STATUS
UsbClearDeviceFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  EFI_USB_RECIPIENT       Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Clear Device Feature

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Value     - Request value
  Target    - Request Index
  Status    - Transfer status

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

  switch (Recipient) {

  case EfiUsbDevice:
    DevReq.RequestType = 0x00;
    break;

  case EfiUsbInterface:
    DevReq.RequestType = 0x01;
    break;

  case EfiUsbEndpoint:
    DevReq.RequestType = 0x02;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_DEV_CLEAR_FEATURE;
  DevReq.Value    = Value;
  DevReq.Index    = Target;


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
//
//  Get Device Status
//
EFI_STATUS
UsbGetDeviceStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  EFI_USB_RECIPIENT       Recipient,
  IN  UINT16                  Target,
  OUT UINT16                  *DevStatus,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get Device Status

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  Recipient - Interface/Device/Endpoint
  Target    - Request index
  DevStatus - Device status
  Status    - Transfer status

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

  switch (Recipient) {

  case EfiUsbDevice:
    DevReq.RequestType = 0x80;
    break;

  case EfiUsbInterface:
    DevReq.RequestType = 0x81;
    break;

  case EfiUsbEndpoint:
    DevReq.RequestType = 0x82;
    break;
  }
  //
  // Fill device request, see USB1.1 spec
  //
  DevReq.Request  = USB_DEV_GET_STATUS;
  DevReq.Value    = 0;
  DevReq.Index    = Target;
  DevReq.Length   = 2;

  return UsbIo->UsbControlTransfer (
                  UsbIo,
                  &DevReq,
                  EfiUsbDataIn,
                  TIMEOUT_VALUE,
                  DevStatus,
                  2,
                  Status
                  );
}
//
// Usb Get String
//
EFI_STATUS
UsbGetString (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  LangID,
  IN  UINT8                   Index,
  IN  VOID                    *Buf,
  IN  UINTN                   BufSize,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Usb Get String

Arguments:

  UsbIo     - EFI_USB_IO_PROTOCOL
  LangID    - Language ID
  Index     - Request index
  Buf       - Buffer to store string
  BufSize   - Buffer size
  Status    - Transfer status

Returns:
  
  EFI_INVALID_PARAMETER - Parameter is error
  EFI_SUCCESS           - Success
  EFI_TIMEOUT           - Device has no response 

--*/
{
  UINT16  Value;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill value, see USB1.1 spec
  //
  Value = (UINT16) ((USB_DT_STRING << 8) | Index);

  return UsbGetDescriptor (
          UsbIo,
          Value,
          LangID,
          (UINT16) BufSize,
          Buf,
          Status
          );
}

EFI_STATUS
UsbClearEndpointHalt (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   EndpointNo,
  OUT UINT32                  *Status
  )
/*++

Routine Description:

  Clear endpoint stall

Arguments:

  UsbIo       - EFI_USB_IO_PROTOCOL
  EndpointNo  - Endpoint Number
  Status      - Transfer Status

Returns:

  EFI_NOT_FOUND    - Can't find the Endpoint
  EFI_DEVICE_ERROR - Hardware error
  EFI_SUCCESS      - Success

--*/
{
  EFI_STATUS                    Result;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  UINT8                         Index;

  ZeroMem (&EndpointDescriptor, sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
  //
  // First seach the endpoint descriptor for that endpoint addr
  //
  Result = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );
  if (EFI_ERROR (Result)) {
    return Result;
  }

  for (Index = 0; Index < InterfaceDescriptor.NumEndpoints; Index++) {
    Result = UsbIo->UsbGetEndpointDescriptor (
                      UsbIo,
                      Index,
                      &EndpointDescriptor
                      );
    if (EFI_ERROR (Result)) {
      continue;
    }

    if (EndpointDescriptor.EndpointAddress == EndpointNo) {
      break;
    }
  }

  if (Index == InterfaceDescriptor.NumEndpoints) {
    //
    // No such endpoint
    //
    return EFI_NOT_FOUND;
  }

  Result = UsbClearDeviceFeature (
            UsbIo,
            EfiUsbEndpoint,
            EfiUsbEndpointHalt,
            EndpointDescriptor.EndpointAddress,
            Status
            );

  return Result;
}
