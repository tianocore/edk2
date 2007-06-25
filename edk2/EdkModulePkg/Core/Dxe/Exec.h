/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  exec.h

Abstract:
    
  EFI Event support

--*/

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

VOID
CoreDispatchEventNotifies (
  IN EFI_TPL      Priority
  )
/*++

Routine Description:

  Dispatches all pending events. 

Arguments:

  Priority - The task priority level of event notifications to dispatch
    
Returns:

  None

--*/
;


UINTN
CoreHighestSetBit (
  IN UINTN         Number
  )
/*++

Routine Description:
  
  Return the highest set bit
  
Arguments:
  
  Number - The value to check
  
Returns:
  
  Bit position of the highest set bit

--*/
;


BOOLEAN
GetInterruptState (
  VOID               
  )
/*++

Routine Description:

  Disables CPU interrupts.

Arguments:

  This                - Protocol instance structure

  State               - Pointer to the CPU's current interrupt state

Returns: 

  EFI_SUCCESS           - If interrupts were disabled in the CPU.

  EFI_INVALID_PARAMETER - State is NULL.
  
--*/
;

//
// Exported functions
//

VOID
CoreEventVirtualAddressFixup (
  VOID
  )
/*++

Routine Description:

  A function out of date, should be removed.

Arguments:

  None
    
Returns:

  None

--*/
;


VOID
CoreInitializeTimer (
  VOID
  )
/*++

Routine Description:

  Initializes timer support

Arguments:

  None
    
Returns:

  None

--*/
;

//
// extern data declarations
//

extern EFI_LOCK       gEventQueueLock;
extern UINTN          gEventPending;
extern LIST_ENTRY     gEventQueue[];
extern LIST_ENTRY     gEventSignalQueue;
extern UINT8          gHSB[];

#endif
