/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  timer.c

Abstract:

  EFI Event support

Revision History

--*/


#include <DxeMain.h>

//
// Internal prototypes
//
STATIC
UINT64
CoreCurrentSystemTime (
  VOID
  );

STATIC
VOID
EFIAPI
CoreCheckTimers (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );

STATIC
VOID
CoreInsertEventTimer (
  IN IEVENT       *Event
  );

//
// Internal data
//

static LIST_ENTRY       mEfiTimerList = INITIALIZE_LIST_HEAD_VARIABLE (mEfiTimerList);
static EFI_LOCK         mEfiTimerLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL - 1);
static EFI_EVENT        mEfiCheckTimerEvent;

static EFI_LOCK         mEfiSystemTimeLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);
static UINT64           mEfiSystemTime = 0;

//
// Timer functions
//

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
{
  EFI_STATUS  Status;

  Status = CoreCreateEvent (
              EVT_NOTIFY_SIGNAL,
              TPL_HIGH_LEVEL - 1,
              CoreCheckTimers,
              NULL,
              &mEfiCheckTimerEvent
              );
  ASSERT_EFI_ERROR (Status);
}

STATIC
UINT64
CoreCurrentSystemTime (
  VOID
  )
/*++

Routine Description:

  Returns the current system time

Arguments:

  None

Returns:

  Returns the current system time

--*/
{
  UINT64          SystemTime;

  CoreAcquireLock (&mEfiSystemTimeLock);
  SystemTime = mEfiSystemTime;
  CoreReleaseLock (&mEfiSystemTimeLock);
  return SystemTime;
}

VOID
EFIAPI
CoreTimerTick (
  IN UINT64   Duration
  )
/*++

Routine Description:

  Called by the platform code to process a tick.

Arguments:

  Duration    - The number of 100ns elasped since the last call to TimerTick

Returns:

  None

--*/
{
  IEVENT          *Event;

  //
  // Check runtiem flag in case there are ticks while exiting boot services
  //

  CoreAcquireLock (&mEfiSystemTimeLock);

  //
  // Update the system time
  //

  mEfiSystemTime += Duration;

  //
  // If the head of the list is expired, fire the timer event
  // to process it
  //

  if (!IsListEmpty (&mEfiTimerList)) {
    Event = CR (mEfiTimerList.ForwardLink, IEVENT, u.Timer.Link, EVENT_SIGNATURE);

    if (Event->u.Timer.TriggerTime <= mEfiSystemTime) {
      CoreSignalEvent (mEfiCheckTimerEvent);
    }
  }

  CoreReleaseLock (&mEfiSystemTimeLock);
}

STATIC
VOID
EFIAPI
CoreCheckTimers (
  IN EFI_EVENT            CheckEvent,
  IN VOID                 *Context
  )
/*++

Routine Description:

  Checks the sorted timer list against the current system time.
  Signals any expired event timer.

Arguments:

  CheckEvent  - Not used

  Context     - Not used

Returns:

  None

--*/
{
  UINT64                  SystemTime;
  IEVENT                  *Event;

  //
  // Check the timer database for expired timers
  //

  CoreAcquireLock (&mEfiTimerLock);
  SystemTime = CoreCurrentSystemTime ();

  while (!IsListEmpty (&mEfiTimerList)) {
    Event = CR (mEfiTimerList.ForwardLink, IEVENT, u.Timer.Link, EVENT_SIGNATURE);

    //
    // If this timer is not expired, then we're done
    //

    if (Event->u.Timer.TriggerTime > SystemTime) {
      break;
    }

    //
    // Remove this timer from the timer queue
    //

    RemoveEntryList (&Event->u.Timer.Link);
    Event->u.Timer.Link.ForwardLink = NULL;

    //
    // Signal it
    //
    CoreSignalEvent (Event);

    //
    // If this is a periodic timer, set it
    //
    if (Event->u.Timer.Period) {

      //
      // Compute the timers new trigger time
      //

      Event->u.Timer.TriggerTime = Event->u.Timer.TriggerTime + Event->u.Timer.Period;

      //
      // If that's before now, then reset the timer to start from now
      //
      if (Event->u.Timer.TriggerTime <= SystemTime) {
        Event->u.Timer.TriggerTime = SystemTime;
        CoreSignalEvent (mEfiCheckTimerEvent);
      }

      //
      // Add the timer
      //

      CoreInsertEventTimer (Event);
    }
  }

  CoreReleaseLock (&mEfiTimerLock);
}

STATIC
VOID
CoreInsertEventTimer (
  IN IEVENT   *Event
  )
/*++

Routine Description:

  Inserts the timer event

Arguments:

  Event - Points to the internal structure of timer event to be installed

Returns:

  None

--*/
{
  UINT64          TriggerTime;
  LIST_ENTRY      *Link;
  IEVENT          *Event2;

  ASSERT_LOCKED (&mEfiTimerLock);

  //
  // Get the timer's trigger time
  //

  TriggerTime = Event->u.Timer.TriggerTime;

  //
  // Insert the timer into the timer database in assending sorted order
  //

  for (Link = mEfiTimerList.ForwardLink; Link  != &mEfiTimerList; Link = Link->ForwardLink) {
    Event2 = CR (Link, IEVENT, u.Timer.Link, EVENT_SIGNATURE);

    if (Event2->u.Timer.TriggerTime > TriggerTime) {
      break;
    }
  }

  InsertTailList (Link, &Event->u.Timer.Link);
}



EFI_STATUS
EFIAPI
CoreSetTimer (
  IN EFI_EVENT            UserEvent,
  IN EFI_TIMER_DELAY      Type,
  IN UINT64               TriggerTime
  )
/*++

Routine Description:

  Sets the type of timer and the trigger time for a timer event.

Arguments:

  UserEvent   - The timer event that is to be signaled at the specified time
  Type        - The type of time that is specified in TriggerTime
  TriggerTime - The number of 100ns units until the timer expires

Returns:

  EFI_SUCCESS           - The event has been set to be signaled at the requested time
  EFI_INVALID_PARAMETER - Event or Type is not valid

--*/
{
  IEVENT      *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (Type < 0 || Type > TimerRelative  || !(Event->Type & EVT_TIMER)) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireLock (&mEfiTimerLock);

  //
  // If the timer is queued to the timer database, remove it
  //

  if (Event->u.Timer.Link.ForwardLink != NULL) {
    RemoveEntryList (&Event->u.Timer.Link);
    Event->u.Timer.Link.ForwardLink = NULL;
  }

  Event->u.Timer.TriggerTime = 0;
  Event->u.Timer.Period = 0;

  if (Type != TimerCancel) {

    if (Type == TimerPeriodic) {
      Event->u.Timer.Period = TriggerTime;
    }

    Event->u.Timer.TriggerTime = CoreCurrentSystemTime () + TriggerTime;
    CoreInsertEventTimer (Event);

    if (TriggerTime == 0) {
      CoreSignalEvent (mEfiCheckTimerEvent);
    }
  }

  CoreReleaseLock (&mEfiTimerLock);
  return EFI_SUCCESS;
}
