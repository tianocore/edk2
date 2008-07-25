/** @file

  The interface of HID data structure.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_MOUSE_HID_H_
#define _EFI_MOUSE_HID_H_

#include "UsbMouse.h"

//
// HID Item general structure
//
typedef struct _hid_item {
  UINT16  Format;
  UINT8   Size;
  UINT8   Type;
  UINT8   Tag;
  union {
    UINT8   U8;
    UINT16  U16;
    UINT32  U32;
    INT8    I8;
    INT16   I16;
    INT32   I32;
    UINT8   *LongData;
  } Data;
} HID_ITEM;

typedef struct {
  UINT16  UsagePage;
  INT32   LogicMin;
  INT32   LogicMax;
  INT32   PhysicalMin;
  INT32   PhysicalMax;
  UINT16  UnitExp;
  UINT16 UINT;
  UINT16 ReportId;
  UINT16 ReportSize;
  UINT16 ReportCount;
} HID_GLOBAL;

typedef struct {
  UINT16  Usage[16];  /* usage array */
  UINT16  UsageIndex;
  UINT16  UsageMin;
} HID_LOCAL;

typedef struct {
  UINT16  Type;
  UINT16  Usage;
} HID_COLLECTION;

typedef struct {
  HID_GLOBAL      Global;
  HID_GLOBAL      GlobalStack[8];
  UINT32          GlobalStackPtr;
  HID_LOCAL       Local;
  HID_COLLECTION  CollectionStack[8];
  UINT32          CollectionStackPtr;
} HID_PARSER;

/**
  Parse Mouse Report Descriptor.

  @param  UsbMouse          The instance of USB_MOUSE_DEV
  @param  ReportDescriptor  Report descriptor to parse
  @param  ReportSize        Report descriptor size

  @retval EFI_DEVICE_ERROR  Report descriptor error
  @retval EFI_SUCCESS       Parse descriptor success

**/
EFI_STATUS
ParseMouseReportDescriptor (
  IN  USB_MOUSE_DEV   *UsbMouse,
  IN  UINT8           *ReportDescriptor,
  IN  UINTN           ReportSize
  );

#endif
