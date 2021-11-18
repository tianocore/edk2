/** @file
  Recommended GUID to be used in the Vendor Hardware device path nodes that
  identify qemu ramfb devices.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QEMU_RAMFB_H__
#define __QEMU_RAMFB_H__

#define QEMU_RAMFB_GUID \
{0x557423a1, 0x63ab, 0x406c, {0xbe, 0x7e, 0x91, 0xcd, 0xbc, 0x08, 0xc4, 0x57}}

extern EFI_GUID  gQemuRamfbGuid;

#endif
