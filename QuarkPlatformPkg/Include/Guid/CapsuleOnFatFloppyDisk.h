/** @file
Capsule on Fat Floppy Disk GUID.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

This is the contract between the recovery module and device recovery module
in order to convey the name of a given recovery module type

**/

#ifndef _CAPSULE_ON_FAT_FLOPPY_DISK_H
#define _CAPSULE_ON_FAT_FLOPPY_DISK_H

#define PEI_CAPSULE_ON_FAT_FLOPPY_DISK_GUID \
  { \
  0x2e3d2e75, 0x9b2e, 0x412d, {0xb4, 0xb1, 0x70, 0x41, 0x6b, 0x87, 0x0, 0xff }\
  };

extern EFI_GUID gPeiCapsuleOnFatFloppyDiskGuid;

#endif
