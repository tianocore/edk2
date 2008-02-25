/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include <UefiUsbLibInternal.h>


/**
  Get Hid Descriptor

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  InterfaceNum      Hid interface number
  @param  HidDescriptor     Caller allocated buffer to store Usb hid descriptor if
                            successfully returned.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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

  Request.RequestType = 0x81;
  Request.Request     = 0x06;
  Request.Value       = (UINT16) (0x21 << 8);
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
//
// Function to get Report Class descriptor
//

/**
  get Report Class descriptor

  @param  UsbIo             EFI_USB_IO_PROTOCOL.
  @param  InterfaceNum      Report interface number.
  @param  DescriptorSize    Length of DescriptorBuffer.
  @param  DescriptorBuffer  Caller allocated buffer to store Usb report descriptor
                            if successfully returned.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0x81;
  Request.Request     = 0x06;
  Request.Value       = (UINT16) (0x22 << 8);
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
//
// Following are HID class request
//

/**
  Get Hid Protocol Request

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to get protocol
  @param  Protocol          Protocol value returned.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0xa1;
  //
  // 10100001b;
  //
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
  Set Hid Protocol Request

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to set protocol
  @param  Protocol          Protocol value the caller wants to set.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0x21;
  //
  // 00100001b;
  //
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

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  Duration          Idle rate the caller wants to set.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0x21;
  //
  // 00100001b;
  //
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

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to get.
  @param  ReportId          Which report the caller wants to get.
  @param  Duration          Idle rate the caller wants to get.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0xa1;
  //
  // 10100001b;
  //
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

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  ReportType        Type of report.
  @param  ReportLen         Length of report descriptor.
  @param  Report            Report Descriptor buffer.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0x21;
  //
  // 00100001b;
  //
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

  @param  UsbIo             EFI_USB_IO_PROTOCOL
  @param  Interface         Which interface the caller wants to set.
  @param  ReportId          Which report the caller wants to set.
  @param  ReportType        Type of report.
  @param  ReportLen         Length of report descriptor.
  @param  Report            Caller allocated buffer to store Report Descriptor.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_TIMEOUT

**/
EFI_STATUS
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
  Request.RequestType = 0xa1;
  //
  // 10100001b;
  //
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
