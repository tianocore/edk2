/** @file
Capsule on Fat Ide Disk GUID.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


This is the contract between the recovery module and device recovery module
in order to convey the name of a given recovery module type

**/

#ifndef _CAPSULE_ON_FAT_IDE_DISK_H
#define _CAPSULE_ON_FAT_IDE_DISK_H

#define PEI_CAPSULE_ON_FAT_IDE_DISK_GUID \
  { \
  0xb38573b6, 0x6200, 0x4ac5, {0xb5, 0x1d, 0x82, 0xe6, 0x59, 0x38, 0xd7, 0x83 }\
  };

extern EFI_GUID gPeiCapsuleOnFatIdeDiskGuid;

#endif
