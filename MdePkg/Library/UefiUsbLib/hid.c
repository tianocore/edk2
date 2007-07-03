/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    hid.c

  Abstract:

    HID class request

  Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

//
// Function to get HID descriptor
//
EFI_STATUS
UsbGetHidDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINT8                      InterfaceNum,
  OUT EFI_USB_HID_DESCRIPTOR     *HidDescriptor
  )
/*++

  Routine Description:
    Get Hid Descriptor

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    InterfaceNum      -   Hid interface number
    HidDescriptor     -   Caller allocated buffer to store Usb hid descriptor
                          if successfully returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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
EFI_STATUS
UsbGetReportDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   InterfaceNum,
  IN  UINT16                  DescriptorSize,
  OUT UINT8                   *DescriptorBuffer
  )
/*++

  Routine Description:
    get Report Class descriptor

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL.
    InterfaceNum      -   Report interface number.
    DescriptorSize    -   Length of DescriptorBuffer.
    DescriptorBuffer  -   Caller allocated buffer to store Usb report descriptor
                          if successfully returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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
EFI_STATUS
UsbGetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   *Protocol
  )
/*++

  Routine Description:
    Get Hid Protocol Request

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to get protocol
    Protocol          -   Protocol value returned.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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


EFI_STATUS
UsbSetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   Protocol
  )
/*++

  Routine Description:
    Set Hid Protocol Request

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set protocol
    Protocol          -   Protocol value the caller wants to set.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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


EFI_STATUS
UsbSetIdleRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   Duration
  )
/*++

  Routine Description:
    Set Idel request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    Duration          -   Idle rate the caller wants to set.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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

EFI_STATUS
UsbGetIdleRequest (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Interface,
  IN  UINT8                   ReportId,
  OUT UINT8                   *Duration
  )
/*++

  Routine Description:
    Get Idel request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to get.
    ReportId          -   Which report the caller wants to get.
    Duration          -   Idle rate the caller wants to get.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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


EFI_STATUS
UsbSetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  )
/*++

  Routine Description:
    Hid Set Report request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    ReportType        -   Type of report.
    ReportLen         -   Length of report descriptor.
    Report            -   Report Descriptor buffer.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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

EFI_STATUS
UsbGetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  )
/*++

  Routine Description:
    Hid Set Report request.

  Arguments:
    UsbIo             -   EFI_USB_IO_PROTOCOL
    Interface         -   Which interface the caller wants to set.
    ReportId          -   Which report the caller wants to set.
    ReportType        -   Type of report.
    ReportLen         -   Length of report descriptor.
    Report            -   Caller allocated buffer to store Report Descriptor.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_TIMEOUT

--*/
{
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_DEVICE_REQUEST  Request;

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
