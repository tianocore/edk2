/** @file
  Common Dxe Libarary  for USB

  Copyright (c) 2006, Intel Corporation<BR>
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

EFI_STATUS
EFIAPI
UsbGetHidDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINT8                      InterfaceNum,
  OUT EFI_USB_HID_DESCRIPTOR     *HidDescriptor
  );

EFI_STATUS
EFIAPI
UsbGetReportDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   InterfaceNum,
  IN  UINT16                  DescriptorSize,
  OUT UINT8                   *DescriptorBuffer
  );

EFI_STATUS
EFIAPI
UsbGetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   *Protocol
  );

EFI_STATUS
EFIAPI
UsbSetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   Protocol
  );

EFI_STATUS
EFIAPI
UsbSetIdleRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   Duration
  );

EFI_STATUS
EFIAPI
UsbGetIdleRequest (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Interface,
  IN  UINT8                   ReportId,
  OUT UINT8                   *Duration
  );

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

typedef enum {
  EfiUsbEndpointHalt,
  EfiUsbDeviceRemoteWakeup
} EFI_USB_STANDARD_FEATURE_SELECTOR;

EFI_STATUS
UsbGetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  OUT VOID                    *Descriptor,
  OUT UINT32                  *Status
  );

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

EFI_STATUS
EFIAPI
UsbGetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Index,
  OUT UINT8                   *AltSetting,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbSetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  InterfaceNo,
  IN  UINT16                  AltSetting,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbGetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT8                   *ConfigValue,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbSetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbSetFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbClearFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbGetStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Target,
  OUT UINT16                  *DevStatus,
  OUT UINT32                  *Status
  );

EFI_STATUS
EFIAPI
UsbGetHubDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  OUT VOID                    *Descriptor,
  OUT UINT32                  *Status
  );

//
// Clear endpoint stall
//
EFI_STATUS
EFIAPI
UsbClearEndpointHalt (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   EndpointNo,
  OUT UINT32                  *Status
  );

#endif
