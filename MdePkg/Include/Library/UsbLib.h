/** @file
  Common Dxe Libarary  for USB.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __USB_DXE_LIB_H__
#define __USB_DXE_LIB_H__

#include <Protocol/UsbIo.h>

//
// define the timeout time as 3ms
//
#define TIMEOUT_VALUE 3 * 1000

/**
  Get Hid Descriptor.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );


/**
  get Report Class descriptor.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );

/**
  Get Hid Protocol Request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to get protocol.
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
  );

/**
  Set Hid Protocol Request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
  @param  Interface         Which interface the caller wants to set protocol.
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
  );

/**
  Set Idel request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );

/**
  Get Idel request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );

/**
  Hid Set Report request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );

/**
  Hid Set Report request.

  @param  UsbIo             A pointer to EFI_USB_IO_PROTOCOL.
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
  );

/**
  Usb Get Descriptor.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Value                  Device Request Value.
  @param  Index                  Device Request Index.
  @param  DescriptorLength       Descriptor Length.
  @param  Descriptor             Descriptor buffer to contain result.
  @param  Status                 Transfer Status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbGetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  OUT VOID                    *Descriptor,
  OUT UINT32                  *Status
  );

/**
  Usb Set Descriptor.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Value                  Device Request Value.
  @param  Index                  Device Request Index.
  @param  DescriptorLength       Descriptor Length.
  @param  Descriptor             Descriptor buffer to set.
  @param  Status                 Transfer Status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbSetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  IN  VOID                    *Descriptor,
  OUT UINT32                  *Status
  );

/**
  Usb Get Device Interface.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Index                  Interface index value.
  @param  AltSetting             Alternate setting.
  @param  Status                 Trasnsfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbGetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Index,
  OUT UINT8                   *AltSetting,
  OUT UINT32                  *Status
  );

/**
  Usb Set Device Interface.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  InterfaceNo            Interface Number.
  @param  AltSetting             Alternate setting.
  @param  Status                 Trasnsfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbSetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  InterfaceNo,
  IN  UINT16                  AltSetting,
  OUT UINT32                  *Status
  );

/**
  Usb Get Device Configuration.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  ConfigValue            Config Value.
  @param  Status                 Transfer Status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbGetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT8                   *ConfigValue,
  OUT UINT32                  *Status
  );

/**
  Usb Set Device Configuration.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Value                  Configuration Value to set.
  @param  Status                 Transfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbSetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  OUT UINT32                  *Status
  );

/**
  Usb Set Device Feature.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Recipient              Interface/Device/Endpoint.
  @param  Value                  Request value.
  @param  Target                 Request Index.
  @param  Status                 Transfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbSetFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

/**
  Usb Clear Device Feature.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Recipient              Interface/Device/Endpoint.
  @param  Value                  Request value.
  @param  Target                 Request Index.
  @param  Status                 Transfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbClearFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

/**
  Usb Get Device Status.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  Recipient              Interface/Device/Endpoint.
  @param  Target                 Request index.
  @param  DevStatus              Device status.
  @param  Status                 Transfer status.

  @retval EFI_INVALID_PARAMETER  Parameter is error.
  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Device has no response.

**/
EFI_STATUS
EFIAPI
UsbGetStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Target,
  OUT UINT16                  *DevStatus,
  OUT UINT32                  *Status
  );

/**
  Clear endpoint stall.

  @param  UsbIo                  A pointer to EFI_USB_IO_PROTOCOL.
  @param  EndpointNo             Endpoint Number.
  @param  Status                 Transfer Status.

  @retval EFI_NOT_FOUND          Can't find the Endpoint.
  @retval EFI_DEVICE_ERROR       Hardware error.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
EFIAPI
UsbClearEndpointHalt (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   EndpointNo,
  OUT UINT32                  *Status
  );

#endif
