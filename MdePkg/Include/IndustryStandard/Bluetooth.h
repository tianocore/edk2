/** @file
  This file contains the Bluetooth definitions that are consumed by drivers.
  These definitions are from Bluetooth Core Specification Version 4.0 June, 2010

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#pragma pack(1)

///
/// BLUETOOTH_ADDRESS
///
typedef struct {
  ///
  /// 48bit Bluetooth device address.
  ///
  UINT8      Address[6];
} BLUETOOTH_ADDRESS;

///
/// BLUETOOTH_CLASS_OF_DEVICE. See Bluetooth specification for detail.
///
typedef struct {
  UINT8      FormatType:2;
  UINT8      MinorDeviceClass: 6;
  UINT16     MajorDeviceClass: 5;
  UINT16     MajorServiceClass:11;
} BLUETOOTH_CLASS_OF_DEVICE;

///
/// BLUETOOTH_LE_ADDRESS
///
typedef struct {
  ///
  /// 48-bit Bluetooth device address
  ///
  UINT8      Address[6];
  ///
  /// 0x00 - Public Device Address
  /// 0x01 - Random Device Address
  ///
  UINT8      Type;
} BLUETOOTH_LE_ADDRESS;

#pragma pack()

#define BLUETOOTH_HCI_COMMAND_LOCAL_READABLE_NAME_MAX_SIZE    248

#define BLUETOOTH_HCI_LINK_KEY_SIZE                           16

#endif
