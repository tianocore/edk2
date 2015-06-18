/** @file
  EDKII SMM Exit Boot Services protocol.

  This SMM protocol is to be published by the SMM Foundation code to associate
  with EFI_EVENT_GROUP_EXIT_BOOT_SERVICES to notify SMM driver that system enter
  exit boot services.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_EXIT_BOOT_SERVICES_H_
#define _SMM_EXIT_BOOT_SERVICES_H_

#define EDKII_SMM_EXIT_BOOT_SERVICES_PROTOCOL_GUID \
  { \
    0x296eb418, 0xc4c8, 0x4e05, { 0xab, 0x59, 0x39, 0xe8, 0xaf, 0x56, 0xf0, 0xa } \
  }

extern EFI_GUID gEdkiiSmmExitBootServicesProtocolGuid;

#endif
