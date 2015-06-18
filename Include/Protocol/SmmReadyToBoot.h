/** @file
  EDKII SMM Ready To Boot protocol.

  This SMM protocol is to be published by the SMM Foundation code to associate
  with EFI_EVENT_GROUP_READY_TO_BOOT to notify SMM driver that system enter
  ready to boot.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_READY_TO_BOOT_H_
#define _SMM_READY_TO_BOOT_H_

#define EDKII_SMM_READY_TO_BOOT_PROTOCOL_GUID \
  { \
    0x6e057ecf, 0xfa99, 0x4f39, { 0x95, 0xbc, 0x59, 0xf9, 0x92, 0x1d, 0x17, 0xe4 } \
  }

extern EFI_GUID gEdkiiSmmReadyToBootProtocolGuid;

#endif
