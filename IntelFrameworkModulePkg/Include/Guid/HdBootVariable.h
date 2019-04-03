/** @file
  GUID used as EFI Variable for the device path of Boot file on HardDevice.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HD_DEVICE_PATH_VARIABLE_GUID_H__
#define __HD_DEVICE_PATH_VARIABLE_GUID_H__

///
/// This GUID is used for an EFI Variable that stores the front device pathes
/// for a partial device path that starts with the HD node.
///
#define HD_BOOT_DEVICE_PATH_VARIABLE_GUID \
  { \
  0xfab7e9e1, 0x39dd, 0x4f2b, { 0x84, 0x8, 0xe2, 0xe, 0x90, 0x6c, 0xb6, 0xde } \
  }

#define HD_BOOT_DEVICE_PATH_VARIABLE_NAME L"HDDP"

extern EFI_GUID gHdBootDevicePathVariablGuid;

#endif
