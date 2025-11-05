/** @file
  This GUID is installed to the device handler to specify that the device is a StdErr device.


Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STANDARD_ERROR_DEVICE_H__
#define __STANDARD_ERROR_DEVICE_H__

#define EFI_STANDARD_ERROR_DEVICE_GUID    \
    { 0xd3b36f2d, 0xd551, 0x11d4, {0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

extern EFI_GUID  gEfiStandardErrorDeviceGuid;

#endif
