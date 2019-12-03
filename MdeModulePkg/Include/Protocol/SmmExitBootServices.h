/** @file
  EDKII SMM Exit Boot Services protocol.

  This SMM protocol is to be published by the SMM Foundation code to associate
  with EFI_EVENT_GROUP_EXIT_BOOT_SERVICES to notify SMM driver that system enter
  exit boot services.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_EXIT_BOOT_SERVICES_H_
#define _SMM_EXIT_BOOT_SERVICES_H_

#define EDKII_SMM_EXIT_BOOT_SERVICES_PROTOCOL_GUID \
  { \
    0x296eb418, 0xc4c8, 0x4e05, { 0xab, 0x59, 0x39, 0xe8, 0xaf, 0x56, 0xf0, 0xa } \
  }

extern EFI_GUID gEdkiiSmmExitBootServicesProtocolGuid;

#endif
