/** @file
  EDKII SMM Ready To Boot protocol.

  This SMM protocol is to be published by the SMM Foundation code to associate
  with EFI_EVENT_GROUP_READY_TO_BOOT to notify SMM driver that system enter
  ready to boot.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_READY_TO_BOOT_H_
#define _SMM_READY_TO_BOOT_H_

#define EDKII_SMM_READY_TO_BOOT_PROTOCOL_GUID \
  { \
    0x6e057ecf, 0xfa99, 0x4f39, { 0x95, 0xbc, 0x59, 0xf9, 0x92, 0x1d, 0x17, 0xe4 } \
  }

extern EFI_GUID  gEdkiiSmmReadyToBootProtocolGuid;

#endif
