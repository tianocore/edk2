/** @file
  This guid is used to specify the primary StdErr device.
  It will be installed as the protocol guid into the virtual device handle for StdErr Splitter.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PRIMARY_STANDARD_ERROR_DEVICE_H__
#define __PRIMARY_STANDARD_ERROR_DEVICE_H__

#define EFI_PRIMARY_STANDARD_ERROR_DEVICE_GUID    \
  { 0x5a68191b, 0x9b97, 0x4752, {0x99, 0x46, 0xe3, 0x6a, 0x5d, 0xa9, 0x42, 0xb1 } }

extern EFI_GUID gEfiPrimaryStandardErrorDeviceGuid;

#endif
