/** @file
  This GUID can be installed to the device handle to specify that the device is the console-out device.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CONSOLE_OUT_DEVICE_H__
#define __CONSOLE_OUT_DEVICE_H__

#define EFI_CONSOLE_OUT_DEVICE_GUID    \
    { 0xd3b36f2c, 0xd551, 0x11d4, {0x9a, 0x46, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

extern EFI_GUID gEfiConsoleOutDeviceGuid;

#endif
