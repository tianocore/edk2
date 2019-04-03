/** @file
Capsule on Fat Usb Disk GUID.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


This is the contract between the recovery module and device recovery module
in order to convey the name of a given recovery module type

**/

#ifndef _PEI_CAPSULE_ON_FAT_USB_DISK_H
#define _PEI_CAPSULE_ON_FAT_USB_DISK_H

#define PEI_CAPSULE_ON_FAT_USB_DISK_GUID \
  { \
  0x0ffbce19, 0x324c, 0x4690, {0xa0, 0x09, 0x98, 0xc6, 0xae, 0x2e, 0xb1, 0x86 } \
  };

extern EFI_GUID gPeiCapsuleOnFatUsbDiskGuid;

#endif
