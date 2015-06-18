/** @file
  SMM Ready To Lock protocol as defined in the PI 1.2 specification.

  This SMM protocol indicates that SMM is about to be locked.
  This protocol is a mandatory protocol published by the SMM Foundation code when the system is 
  preparing to lock SMM. This protocol should be installed immediately after
  EFI_END_OF_DXE_EVENT_GROUP_GUID with no intervening modules dispatched.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMM_READY_TO_LOCK_H_
#define _SMM_READY_TO_LOCK_H_

#define EFI_SMM_READY_TO_LOCK_PROTOCOL_GUID \
  { \
    0x47b7fa8c, 0xf4bd, 0x4af6, { 0x82, 0x00, 0x33, 0x30, 0x86, 0xf0, 0xd2, 0xc8 } \
  }

extern EFI_GUID gEfiSmmReadyToLockProtocolGuid;

#endif
