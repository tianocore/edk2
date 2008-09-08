/** @file

  The library provides USB HID Class standard and specific requests defined
  in USB HID Firmware Specification 7 section : Requests.
  
  Copyright (c) 2004, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <UefiUsbLibInternal.h>

//  
//  Hid RequestType Bits specifying characteristics of request.
//  Valid values are 10100001b (0xa1) or 00100001b (0x21).
//  The following description:
//    7 Data transfer direction
//        0 = Host to device
//        1 = Device to host
//    6..5 Type
//        1 = Class
//    4..0 Recipient
//        1 = Interface
//

/**
  Get Hid Descriptor.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  InterfaceNum      Hid interface number.
  @param  HidDescriptor     Caller allocated buffer to store Usb hid descriptor if
                            successfully returned.

  @return Status of getting HID descriptor through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbGetHidDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINT8                      InterfaceNum,
  OUT EFI_USB_HID_DESCRIPTOR     *HidDescriptor
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;
  
  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Request.RequestType = USB_HID_GET_DESCRIPTOR_REQ_TYPE;
  Request.Request     = USB_REQ_GET_DESCRIPTOR;
  Request.Value       = (UINT16) (USB_DESC_TYPE_HID << 8);
  Request.Index       = InterfaceNum;
  Request.Length      = sizeof (EFI_USB_HID_DESCRIPTOR);

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataIn,
                    TIMEOUT_VALUE,
                    HidDescriptor,
                    sizeof (EFI_USB_HID_DESCRIPTOR),
                    &Status
                    );

  return Result;

}

/**
  Get Report Class descriptor.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  InterfaceNum      Report interface number.
  @param  DescriptorSize    Length of DescriptorBuffer.
  @param  DescriptorBuffer  Caller allocated buffer to store Usb report descriptor
                            if successfully returned.

  @return Status of getting Report Class descriptor through USB
          I/O protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbGetReportDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   InterfaceNum,
  IN  UINT16                  DescriptorSize,
  OUT UINT8                   *DescriptorBuffer
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_GET_DESCRIPTOR_REQ_TYPE;
  Request.Request     = USB_REQ_GET_DESCRIPTOR;
  Request.Value       = (UINT16) (USB_DESC_TYPE_REPORT << 8);
  Request.Index       = InterfaceNum;
  Request.Length      = DescriptorSize;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataIn,
                    TIMEOUT_VALUE,
                    DescriptorBuffer,
                    DescriptorSize,
                    &Status
                    );

  return Result;

}

/**
  Get Hid Protocol Request

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to get protocol
  @param  Protocol          Protocol value returned.

  @return Status of getting Protocol Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbGetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   *Protocol
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_GET_REQ_TYPE;
  Request.Request = EFI_USB_GET_PROTOCOL_REQUEST;
  Request.Value   = 0;
  Request.Index   = Interface;
  Request.Length  = 1;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataIn,
                    TIMEOUT_VALUE,
                    Protocol,
                    sizeof (UINT8),
                    &Status
                    );

  return Result;
}



/**
  Set Hid Protocol Request.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to
                            set protocol.
  @param  Protocol          Protocol value the caller wants to set.

  @return Status of setting Protocol Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbSetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   Protocol
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_SET_REQ_TYPE;
  Request.Request = EFI_USB_SET_PROTOCOL_REQUEST;
  Request.Value   = Protocol;
  Request.Index   = Interface;
  Request.Length  = 0;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbNoData,
                    TIMEOUT_VALUE,
                    NULL,
                    0,
                    &Status
                    );
  return Result;
}


/**
  Set Idel request.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  Duration          Idle rate the caller wants to set.

  @return Status of setting IDLE Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbSetIdleRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   Duration
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_SET_REQ_TYPE;
  Request.Request = EFI_USB_SET_IDLE_REQUEST;
  Request.Value   = (UINT16) ((Duration << 8) | ReportId);
  Request.Index   = Interface;
  Request.Length  = 0;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbNoData,
                    TIMEOUT_VALUE,
                    NULL,
                    0,
                    &Status
                    );
  return Result;
}


/**
  Get Idel request.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to get.
  @param  ReportId          Which report the caller wants to get.
  @param  Duration          Idle rate the caller wants to get.

  @return Status of getting IDLE Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbGetIdleRequest (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Interface,
  IN  UINT8                   ReportId,
  OUT UINT8                   *Duration
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;
  
  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_GET_REQ_TYPE;
  Request.Request = EFI_USB_GET_IDLE_REQUEST;
  Request.Value   = ReportId;
  Request.Index   = Interface;
  Request.Length  = 1;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataIn,
                    TIMEOUT_VALUE,
                    Duration,
                    1,
                    &Status
                    );

  return Result;
}



/**
  Hid Set Report request.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  ReportType        Type of report.
  @param  ReportLen         Length of report descriptor.
  @param  Report            Report Descriptor buffer.

  @return Status of setting Report Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbSetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_SET_REQ_TYPE;
  Request.Request = EFI_USB_SET_REPORT_REQUEST;
  Request.Value   = (UINT16) ((ReportType << 8) | ReportId);
  Request.Index   = Interface;
  Request.Length  = ReportLen;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataOut,
                    TIMEOUT_VALUE,
                    Report,
                    ReportLen,
                    &Status
                    );

  return Result;
}


/**
  Hid Set Report request.

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  ReportType        Type of report.
  @param  ReportLen         Length of report descriptor.
  @param  Report            Caller allocated buffer to store Report Descriptor.

  @return Status of getting Report Request through USB I/O
          protocol's UsbControlTransfer().

**/
EFI_STATUS
EFIAPI
UsbGetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  )
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

  if (UsbIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Fill Device request packet
  //
  Request.RequestType = USB_HID_CLASS_GET_REQ_TYPE;
  Request.Request = EFI_USB_GET_REPORT_REQUEST;
  Request.Value   = (UINT16) ((ReportType << 8) | ReportId);
  Request.Index   = Interface;
  Request.Length  = ReportLen;

  Result = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Request,
                    EfiUsbDataIn,
                    TIMEOUT_VALUE,
                    Report,
                    ReportLen,
                    &Status
                    );

  return Result;
}
