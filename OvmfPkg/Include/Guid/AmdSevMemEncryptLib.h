/** @file

  AMD Memory Encryption GUID, define a new GUID for defining
  new UEFI environment variables assocaiated with SEV Memory Encryption.

  Copyright (c) 2021, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __AMD_SEV_MEMENCRYPT_LIB_H__
#define __AMD_SEV_MEMENCRYPT_LIB_H__

#define AMD_SEV_MEMENCRYPT_GUID \
{0x0cf29b71, 0x9e51, 0x433a, {0xa3, 0xb7, 0x81, 0xf3, 0xab, 0x16, 0xb8, 0x75}}

extern EFI_GUID  gAmdSevMemEncryptGuid;

#endif
