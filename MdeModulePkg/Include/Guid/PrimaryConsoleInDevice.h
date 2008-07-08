/** @file
  This guid is used to specify the primary console in device.
  It will be installed into the virtual device handle for ConIn Splitter.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PRIMARY_CONSOLE_IN_DEVICE_H__
#define __PRIMARY_CONSOLE_IN_DEVICE_H__

#define EFI_PRIMARY_CONSOLE_IN_DEVICE_GUID    \
  { 0xe451dcbe, 0x96a1, 0x4729, {0xa5, 0xcf, 0x6b, 0x9c, 0x2c, 0xff, 0x47, 0xfd } }

extern EFI_GUID gEfiPrimaryConsoleInDeviceGuid;

#endif
