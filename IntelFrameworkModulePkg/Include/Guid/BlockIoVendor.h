/** @file
  Guid for unrecognized EDD 3.0 device.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BLOCKIO_VENDOR_H__
#define __BLOCKIO_VENDOR_H__

//
// Guid is to specifiy the unrecognized EDD 3.0 device.
//
#define BLOCKIO_VENDOR_GUID \
  { 0xcf31fac5, 0xc24e, 0x11d2,  {0x85, 0xf3, 0x0, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}  }

typedef struct {
  VENDOR_DEVICE_PATH              DevicePath;
  UINT8                           LegacyDriveLetter;
} BLOCKIO_VENDOR_DEVICE_PATH;

extern GUID gBlockIoVendorGuid;

#endif
