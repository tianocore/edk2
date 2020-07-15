/** @file
  EDKII SMM Legacy Boot protocol.

  This SMM protocol is to be published by the SMM Foundation code to associate
  with EFI_EVENT_LEGACY_BOOT_GUID to notify SMM driver that system enter legacy boot.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_LEGACY_BOOT_H_
#define _SMM_LEGACY_BOOT_H_

#define EDKII_SMM_LEGACY_BOOT_PROTOCOL_GUID \
  { \
    0x85a8ab57, 0x644, 0x4110, { 0x85, 0xf, 0x98, 0x13, 0x22, 0x4, 0x70, 0x70 } \
  }

extern EFI_GUID gEdkiiSmmLegacyBootProtocolGuid;

#endif
