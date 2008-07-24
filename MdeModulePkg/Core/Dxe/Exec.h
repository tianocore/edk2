/** @file
  UEFI Event support functions and structure.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EXEC_H_
#define _EXEC_H_

#define VALID_TPL(a)            ((a) <= TPL_HIGH_LEVEL)

//
// EFI_EVENT
//



#define EVENT_SIGNATURE         EFI_SIGNATURE_32('e','v','n','t')
typedef struct {
  UINTN                   Signature;
  UINT32                  Type;
  UINT32                  SignalCount;

  //
  // Entry if the event is registered to be signalled
  //

  LIST_ENTRY              SignalLink;

  //
  // Notification information for this event
  //

  EFI_TPL                 NotifyTpl;
  EFI_EVENT_NOTIFY        NotifyFunction;
  VOID                    *NotifyContext;
  EFI_GUID                EventGroup;
  LIST_ENTRY              NotifyLink;
  BOOLEAN                 ExFlag;

  //
  // A list of all runtime events
  //
  EFI_RUNTIME_EVENT_ENTRY   RuntimeData;

  //
  // Information by event type
  //

  union {
    //
    // For timer events
    //
    struct {
      LIST_ENTRY      Link;
      UINT64          TriggerTime;
      UINT64          Period;
    } Timer;
  } u;

} IEVENT;

//
// Internal prototypes
//


/**
  Dispatches all pending events.

  @param  Priority               The task priority level of event notifications
                                 to dispatch

**/
VOID
CoreDispatchEventNotifies (
  IN EFI_TPL      Priority
  );



/**
  Return the highest set bit.

  @param  Number  The value to check

  @return Bit position of the highest set bit

**/
UINTN
CoreHighestSetBit (
  IN UINTN     Number
  );



/**
  Disables CPU interrupts.

  @retval EFI_SUCCESS            If interrupts were disabled in the CPU.
  @retval EFI_INVALID_PARAMETER  State is NULL.

**/
BOOLEAN
GetInterruptState (
  VOID
  );

//
// Exported functions
//


/**
  A function out of date, should be removed.

**/
VOID
CoreEventVirtualAddressFixup (
  VOID
  );



/**
  Initializes timer support.

**/
VOID
CoreInitializeTimer (
  VOID
  );

//
// extern data declarations
//

extern EFI_LOCK       gEventQueueLock;
extern UINTN          gEventPending;
extern LIST_ENTRY     gEventQueue[];
extern LIST_ENTRY     gEventSignalQueue;

#endif
