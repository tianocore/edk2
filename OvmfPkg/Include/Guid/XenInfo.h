/** @file
  XenInfo HOB passed by PEI into DXE.

Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __XEN_INFO_H__
#define __XEN_INFO_H__

#define EFI_XEN_INFO_GUID \
    { 0xd3b46f3b, 0xd441, 0x1244, {0x9a, 0x12, 0x0, 0x12, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct {
  ///
  /// Hypervisor major version.
  ///
  UINT16    VersionMajor;
  ///
  /// Hypervisor minor version.
  ///
  UINT16    VersionMinor;
  ///
  /// Pointer to the RSDP found in the hvm_start_info provided to a PVH guest
  ///
  VOID      *RsdpPvh;
} EFI_XEN_INFO;

extern EFI_GUID  gEfiXenInfoGuid;

#endif
