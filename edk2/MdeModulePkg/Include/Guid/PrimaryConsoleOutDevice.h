/** @file
  This guid is used to specify the primary console out device.
  It will be installed into the virtual device handle for ConOut Splitter.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PRIMARY_CONSOLE_OUT_DEVICE_H__
#define __PRIMARY_CONSOLE_OUT_DEVICE_H__

#define EFI_PRIMARY_CONSOLE_OUT_DEVICE_GUID    \
  { 0x62bdf38a, 0xe3d5, 0x492c, {0x95, 0xc, 0x23, 0xa7, 0xf6, 0x6e, 0x67, 0x2e } }

extern EFI_GUID gEfiPrimaryConsoleOutDeviceGuid;

#endif
