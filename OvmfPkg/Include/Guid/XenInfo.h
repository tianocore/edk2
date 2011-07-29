/** @file
  XenInfo HOB passed by PEI into DXE.

Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __XEN_INFO_H__
#define __XEN_INFO_H__

#define EFI_XEN_INFO_GUID \
    { 0xd3b46f3b, 0xd441, 0x1244, {0x9a, 0x12, 0x0, 0x12, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct {
  ///
  /// Beginning of the hypercall page.
  ///
  VOID *HyperPages;
  ///
  /// Location of the hvm_info page.
  ///
  VOID *HvmInfo;
  ///
  /// Hypervisor major version.
  ///
  UINT16 VersionMajor;
  ///
  /// Hypervisor minor version.
  ///
  UINT16 VersionMinor;
} EFI_XEN_INFO;

extern EFI_GUID gEfiXenInfoGuid;

#endif
