/** @file

  Copyright (c) 2008-2009 Apple Inc. All rights reserved.<BR>

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TIMERDEBUGSUPPORTPROTOCOL_H__
#define __TIMERDEBUGSUPPORTPROTOCOL_H__

//
// Protocol GUID
//
#define TIMER_DEBUG_PROTOCOL_GUID { 0x68300561, 0x0197, 0x465d, { 0xb5, 0xa1, 0x28, 0xeb, 0xa1, 0x98, 0xdd, 0x0b } }



//
// Protocol interface structure
//
typedef struct _TIMER_DEBUG_SUPPORT_PROTOCOL  TIMER_DEBUG_SUPPORT_PROTOCOL;


typedef
EFI_STATUS
(EFIAPI *TIMER_DEBUG_SUPPORT_REGISTER_PERIODIC_CALLBACK) (
  IN  TIMER_DEBUG_SUPPORT_PROTOCOL  *This,
  IN  EFI_PERIODIC_CALLBACK         PeriodicCallback
  )
/*++

Routine Description:
  Register a periodic callback for debug support.

Arguments:
  This              - pointer to protocol
  PeriodicCallback  - callback to be registered
  
Returns:
  EFI_SUCCESS - callback registered

--*/
;

struct _TIMER_DEBUG_SUPPORT_PROTOCOL {
  TIMER_DEBUG_SUPPORT_REGISTER_PERIODIC_CALLBACK  RegisterPeriodicCallback;
};

extern EFI_GUID gTimerDebugSupportProtocolGuid;

#endif	// __TIMERDEBUGSUPPORTPROTOCOL_H__

