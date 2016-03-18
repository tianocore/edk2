/** @file
  GUID used as EFI Variable for the device path of Boot file on HardDevice.
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
