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
// Standard device request and request type
// By [Spec-USB20/Chapter-9.4]
//
#define USB_DEV_GET_STATUS                  0x00
#define USB_DEV_GET_STATUS_REQ_TYPE_D       0x80 // Receiver : Device
#define USB_DEV_GET_STATUS_REQ_TYPE_I       0x81 // Receiver : Interface
#define USB_DEV_GET_STATUS_REQ_TYPE_E       0x82 // Receiver : Endpoint

#define USB_DEV_CLEAR_FEATURE               0x01
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_D    0x00 // Receiver : Device
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_I    0x01 // Receiver : Interface
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_E    0x02 // Receiver : Endpoint

#define USB_DEV_SET_FEATURE                 0x03
#define USB_DEV_SET_FEATURE_REQ_TYPE_D      0x00 // Receiver : Device
#define USB_DEV_SET_FEATURE_REQ_TYPE_I      0x01 // Receiver : Interface
#define USB_DEV_SET_FEATURE_REQ_TYPE_E      0x02 // Receiver : Endpoint

#define USB_DEV_SET_ADDRESS                 0x05
#define USB_DEV_SET_ADDRESS_REQ_TYPE        0x00

#define USB_DEV_GET_DESCRIPTOR              0x06
#define USB_DEV_GET_DESCRIPTOR_REQ_TYPE     0x80

#define USB_DEV_SET_DESCRIPTOR              0x07
#define USB_DEV_SET_DESCRIPTOR_REQ_TYPE     0x00

#define USB_DEV_GET_CONFIGURATION           0x08
#define USB_DEV_GET_CONFIGURATION_REQ_TYPE  0x80

#define USB_DEV_SET_CONFIGURATION           0x09
#define USB_DEV_SET_CONFIGURATION_REQ_TYPE  0x00

#define USB_DEV_GET_INTERFACE               0x0A
#define USB_DEV_GET_INTERFACE_REQ_TYPE      0x81

#define USB_DEV_SET_INTERFACE               0x0B
#define USB_DEV_SET_INTERFACE_REQ_TYPE      0x01

#define USB_DEV_SYNCH_FRAME                 0x0C
#define USB_DEV_SYNCH_FRAME_REQ_TYPE        0x82

//
// define the timeout time as 3ms
//
#define TIMEOUT_VALUE 3 * 1000

//
// HID constants definition, see HID rev1.0
//
//
// HID report item format
//
#define HID_ITEM_FORMAT_SHORT 0
#define HID_ITEM_FORMAT_LONG  1

//
// Special tag indicating long items
//
#define HID_ITEM_TAG_LONG 15

//
// HID report descriptor item type (prefix bit 2,3)
//
#define HID_ITEM_TYPE_MAIN      0
#define HID_ITEM_TYPE_GLOBAL    1
#define HID_ITEM_TYPE_LOCAL     2
#define HID_ITEM_TYPE_RESERVED  3

//
// HID report descriptor main item tags
//
#define HID_MAIN_ITEM_TAG_INPUT             8
#define HID_MAIN_ITEM_TAG_OUTPUT            9
#define HID_MAIN_ITEM_TAG_FEATURE           11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION  10
#define HID_MAIN_ITEM_TAG_END_COLLECTION    12

//
// HID report descriptor main item contents
//
#define HID_MAIN_ITEM_CONSTANT      0x001
#define HID_MAIN_ITEM_VARIABLE      0x002
#define HID_MAIN_ITEM_RELATIVE      0x004
#define HID_MAIN_ITEM_WRAP          0x008
#define HID_MAIN_ITEM_NONLINEAR     0x010
#define HID_MAIN_ITEM_NO_PREFERRED  0x020
#define HID_MAIN_ITEM_NULL_STATE    0x040
#define HID_MAIN_ITEM_VOLATILE      0x080
#define HID_MAIN_ITEM_BUFFERED_BYTE 0x100

//
// HID report descriptor collection item types
//
#define HID_COLLECTION_PHYSICAL     0
#define HID_COLLECTION_APPLICATION  1
#define HID_COLLECTION_LOGICAL      2

//
// HID report descriptor global item tags
//
#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE        0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM   1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM   2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM  3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM  4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT     5
#define HID_GLOBAL_ITEM_TAG_UNIT              6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE       7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID         8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT      9
#define HID_GLOBAL_ITEM_TAG_PUSH              10
#define HID_GLOBAL_ITEM_TAG_POP               11

//
// HID report descriptor local item tags
//
#define HID_LOCAL_ITEM_TAG_USAGE              0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM      1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM      2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX   3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM 4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM 5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX       7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM     8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM     9
#define HID_LOCAL_ITEM_TAG_DELIMITER          10

//
// HID usage tables
//
#define HID_USAGE_PAGE    0xffff0000

#define HID_UP_GENDESK    0x00010000
#define HID_UP_KEYBOARD   0x00070000
#define HID_UP_LED        0x00080000
#define HID_UP_BUTTON     0x00090000
#define HID_UP_CONSUMER   0x000c0000
#define HID_UP_DIGITIZER  0x000d0000
#define HID_UP_PID        0x000f0000

#define HID_USAGE         0x0000ffff

#define HID_GD_POINTER    0x00010001
#define HID_GD_MOUSE      0x00010002
#define HID_GD_JOYSTICK   0x00010004
#define HID_GD_GAMEPAD    0x00010005
#define HID_GD_HATSWITCH  0x00010039

//
// HID report types
//
#define HID_INPUT_REPORT    1
#define HID_OUTPUT_REPORT   2
#define HID_FEATURE_REPORT  3

//
// HID device quirks.
//
#define HID_QUIRK_INVERT  0x01
#define HID_QUIRK_NOTOUCH 0x02

//
// HID class protocol request
//
#define EFI_USB_GET_REPORT_REQUEST    0x01
#define EFI_USB_GET_IDLE_REQUEST      0x02
#define EFI_USB_GET_PROTOCOL_REQUEST  0x03
#define EFI_USB_SET_REPORT_REQUEST    0x09
#define EFI_USB_SET_IDLE_REQUEST      0x0a
#define EFI_USB_SET_PROTOCOL_REQUEST  0x0b

#pragma pack(1)
//
// Descriptor header for Report/Physical Descriptors
//
typedef struct hid_class_descriptor {
  UINT8   DescriptorType;
  UINT16  DescriptorLength;
} EFI_USB_HID_CLASS_DESCRIPTOR;

typedef struct hid_descriptor {
  UINT8                         Length;
  UINT8                         DescriptorType;
  UINT16                        BcdHID;
  UINT8                         CountryCode;
  UINT8                         NumDescriptors;
  EFI_USB_HID_CLASS_DESCRIPTOR  HidClassDesc[1];
} EFI_USB_HID_DESCRIPTOR;

#pragma pack()

EFI_STATUS
UsbGetHidDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINT8                      InterfaceNum,
  OUT EFI_USB_HID_DESCRIPTOR     *HidDescriptor
  );

EFI_STATUS
UsbGetReportDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   InterfaceNum,
  IN  UINT16                  DescriptorSize,
  OUT UINT8                   *DescriptorBuffer
  );

EFI_STATUS
UsbGetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   *Protocol
  );

EFI_STATUS
UsbSetProtocolRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   Protocol
  );

EFI_STATUS
UsbSetIdleRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   Duration
  );

EFI_STATUS
UsbGetIdleRequest (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Interface,
  IN  UINT8                   ReportId,
  OUT UINT8                   *Duration
  );

EFI_STATUS
UsbSetReportRequest (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Interface,
  IN UINT8                   ReportId,
  IN UINT8                   ReportType,
  IN UINT16                  ReportLen,
  IN UINT8                   *Report
  );

EFI_STATUS
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
UsbSetDescriptor (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  IN  UINT16                  Index,
  IN  UINT16                  DescriptorLength,
  IN  VOID                    *Descriptor,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbGetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Index,
  OUT UINT8                   *AltSetting,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbSetInterface (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  InterfaceNo,
  IN  UINT16                  AltSetting,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbGetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT8                   *ConfigValue,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbSetConfiguration (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT16                  Value,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbSetFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbClearFeature (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Value,
  IN  UINT16                  Target,
  OUT UINT32                  *Status
  );

EFI_STATUS
UsbGetStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINTN                   Recipient,
  IN  UINT16                  Target,
  OUT UINT16                  *DevStatus,
  OUT UINT32                  *Status
  );

EFI_STATUS
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
UsbClearEndpointHalt (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   EndpointNo,
  OUT UINT32                  *Status
  );

#endif
