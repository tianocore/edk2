/** @file
  DXE SMM Ready To Lock protocol as defined in the PI 1.2 specification.

  This UEFI protocol indicates that SMM is about to be locked.
  This protocol is a mandatory protocol published by a DXE driver prior to invoking the 
  EFI_SMM_ACCESS2_PROTOCOL.Lock() function to lock SMM. 

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DXE_SMM_READY_TO_LOCK_H_
#define _DXE_SMM_READY_TO_LOCK_H_

#define EFI_DXE_SMM_READY_TO_LOCK_PROTOCOL_GUID \
  { \
    0x60ff8964, 0xe906, 0x41d0, { 0xaf, 0xed, 0xf2, 0x41, 0xe9, 0x74, 0xe0, 0x8e } \
  }

extern EFI_GUID gEfiDxeSmmReadyToLockProtocolGuid;

#endif
