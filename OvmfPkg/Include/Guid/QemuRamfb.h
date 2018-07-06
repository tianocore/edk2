/** @file
  Recommended GUID to be used in the Vendor Hardware device path nodes that
  identify qemu ramfb devices.

  Copyright (C) 2018, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __QEMU_RAMFB_H__
#define __QEMU_RAMFB_H__

#define QEMU_RAMFB_GUID \
{0x557423a1, 0x63ab, 0x406c, {0xbe, 0x7e, 0x91, 0xcd, 0xbc, 0x08, 0xc4, 0x57}}

extern EFI_GUID gQemuRamfbGuid;

#endif
