/** @file
  Declare the application prefix string as a GUID, for locating the PK/KEK1
  X509 certificate to enroll, in the "OEM Strings" SMBIOS table.

  Copyright (C) 2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  - https://git.qemu.org/?p=qemu.git;a=commit;h=2d6dcbf93fb0
  - https://libvirt.org/formatdomain.html#elementsSysinfo
  - https://bugs.launchpad.net/qemu/+bug/1826200
  - https://bugzilla.tianocore.org/show_bug.cgi?id=1747
**/

#ifndef OVMF_PK_KEK1_APP_PREFIX_H_
#define OVMF_PK_KEK1_APP_PREFIX_H_

#include <Uefi/UefiBaseType.h>

//
// For the EnrollDefaultKeys application, the hypervisor is expected to add a
// string entry to the "OEM Strings" (Type 11) SMBIOS table, with the following
// format:
//
// 4e32566d-8e9e-4f52-81d3-5bb9715f9727:<Base64 X509 cert for PK and first KEK>
//
// The string representation of the GUID at the front is the "application
// prefix". It is matched by EnrollDefaultKeys case-insensitively.
//
// The base64-encoded blob following the application prefix and the colon (:)
// is an X509 certificate in DER representation; the hypervisor instructs
// EnrollDefaultKeys to enroll this certificate as both Platform Key and first
// Key Exchange Key.
//
#define OVMF_PK_KEK1_APP_PREFIX_GUID                    \
  { 0x4e32566d,                                         \
    0x8e9e,                                             \
    0x4f52,                                             \
    { 0x81, 0xd3, 0x5b, 0xb9, 0x71, 0x5f, 0x97, 0x27 }, \
  }

extern EFI_GUID  gOvmfPkKek1AppPrefixGuid;

#endif /* OVMF_PK_KEK1_APP_PREFIX_H_ */
