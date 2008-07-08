/** @file
  This guid is used to specifiy the device is the hot plug device.
  If the device is the hot plug device, this guid as the protocol guid
  will be installed into this device handle.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __HOT_PLUG_DEVICE_H__
#define __HOT_PLUG_DEVICE_H__

#define HOT_PLUG_DEVICE_GUID    \
    { 0x220ac432, 0x1d43, 0x49e5, {0xa7, 0x4f, 0x4c, 0x9d, 0xa6, 0x7a, 0xd2, 0x3b } }

extern EFI_GUID gEfiHotPlugDeviceGuid;
#endif
