/** @file
  Core Timer Services

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Event.h"

//
// Internal data
//

LIST_ENTRY  mEfiTimerList       = INITIALIZE_LIST_HEAD_VARIABLE (mEfiTimerList);
EFI_LOCK    mEfiTimerLock       = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL - 1);
EFI_EVENT   mEfiCheckTimerEvent = NULL;

EFI_LOCK  mEfiSystemTimeLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);
UINT64    mEfiSystemTime     = 0;

//
// Timer functions
//

/**
  Inserts the timer event.

  @param  Event                  Points to the internal structure of timer event
                                 to be installed

**/
VOID
CoreInsertEventTimer (
  IN IEVENT  *Event
  )
{
  UINT64      TriggerTime;
  LIST_ENTRY  *Link;
  IEVENT      *Event2;

  ASSERT_LOCKED (&mEfiTimerLock);

  //
  // Get the timer's trigger time
  //
  TriggerTime = Event->Timer.TriggerTime;

  //
  // Insert the timer into the timer database in assending sorted order
  //
  for (Link = mEfiTimerList.ForwardLink; Link != &mEfiTimerList; Link = Link->ForwardLink) {
    Event2 = CR (Link, IEVENT, Timer.Link, EVENT_SIGNATURE);

    if (Event2->Timer.TriggerTime > TriggerTime) {
      break;
    }
  }

  InsertTailList (Link, &Event->Timer.Link);
}

/**
  Returns the current system time.

  @return The current system time

**/
UINT64
CoreCurrentSystemTime (
  VOID
  )
{
  UINT64  SystemTime;

  CoreAcquireLock (&mEfiSystemTimeLock);
  SystemTime = mEfiSystemTime;
  CoreReleaseLock (&mEfiSystemTimeLock);

  return SystemTime;
}

/**
  Checks the sorted timer list against the current system time.
  Signals any expired event timer.

  @param  CheckEvent             Not used
  @param  Context                Not used

**/
VOID
EFIAPI
CoreCheckTimers (
  IN EFI_EVENT  CheckEvent,
  IN VOID       *Context
  )
{
  UINT64  SystemTime;
  IEVENT  *Event;

  //
  // Check the timer database for expired timers
  //
  CoreAcquireLock (&mEfiTimerLock);
  SystemTime = CoreCurrentSystemTime ();

  while (!IsListEmpty (&mEfiTimerList)) {
    Event = CR (mEfiTimerList.ForwardLink, IEVENT, Timer.Link, EVENT_SIGNATURE);

    //
    // If this timer is not expired, then we're done
    //
    if (Event->Timer.TriggerTime > SystemTime) {
      break;
    }

    //
    // Remove this timer from the timer queue
    //

    RemoveEntryList (&Event->Timer.Link);
    Event->Timer.Link.ForwardLink = NULL;

    //
    // Signal it
    //
    CoreSignalEvent (Event);

    //
    // If this is a periodic timer, set it
    //
    if (Event->Timer.Period != 0) {
      //
      // Compute the timers new trigger time
      //
      Event->Timer.TriggerTime = Event->Timer.TriggerTime + Event->Timer.Period;

      //
      // If that's before now, then reset the timer to start from now
      //
      if (Event->Timer.TriggerTime <= SystemTime) {
        Event->Timer.TriggerTime = SystemTime;
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

/**
  Initializes timer support.

**/
VOID
CoreInitializeTimer (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = CoreCreateEventInternal (
             EVT_NOTIFY_SIGNAL,
             TPL_HIGH_LEVEL - 1,
             CoreCheckTimers,
             NULL,
             NULL,
             &mEfiCheckTimerEvent
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Called by the platform code to process a tick.

  @param  Duration               The number of 100ns elapsed since the last call
                                 to TimerTick

**/
STATIC
VOID
EFIAPI
CoreTimerTick (
  IN UINT64  Duration
  )
{
  EFI_TPL  OriginalTPL;
  IEVENT   *Event;

  DEBUG_CODE_BEGIN ();
  if (gCpu != NULL) {
    BOOLEAN  State;

    ASSERT_EFI_ERROR (gCpu->GetInterruptState (gCpu, &State));
    ASSERT (!State);
  }

  DEBUG_CODE_END ();

  OriginalTPL = CoreRaiseTpl (TPL_HIGH_LEVEL);

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
    Event = CR (mEfiTimerList.ForwardLink, IEVENT, Timer.Link, EVENT_SIGNATURE);

    if (Event->Timer.TriggerTime <= mEfiSystemTime) {
      CoreSignalEvent (mEfiCheckTimerEvent);
    }
  }

  CoreReleaseLock (&mEfiSystemTimeLock);

  //
  // Restore the original TPL but without re-enabling interrupts. This is the
  // responsibility of the caller, which will do so implicitly by returning
  // from the timer ISR in an architecture-specific manner (IRET, ERET, etc).
  //
  // Re-enabling interrupts before that leaves a window where the timer
  // interrupt, which has been re-armed at this point, may fire again
  // immediately, resulting in unbounded recursion if the interrupts are
  // arriving at a higher rate than they can be serviced.
  //
  CoreRestoreTplWithInterruptsMasked (OriginalTPL);
}

/**
  Sets the type of timer and the trigger time for a timer event.

  @param  UserEvent              The timer event that is to be signaled at the
                                 specified time
  @param  Type                   The type of time that is specified in
                                 TriggerTime
  @param  TriggerTime            The number of 100ns units until the timer
                                 expires

  @retval EFI_SUCCESS            The event has been set to be signaled at the
                                 requested time
  @retval EFI_INVALID_PARAMETER  Event or Type is not valid

**/
EFI_STATUS
EFIAPI
CoreSetTimer (
  IN EFI_EVENT        UserEvent,
  IN EFI_TIMER_DELAY  Type,
  IN UINT64           TriggerTime
  )
{
  IEVENT  *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (((UINT32)Type > TimerRelative) || ((Event->Type & EVT_TIMER) == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireLock (&mEfiTimerLock);

  //
  // If the timer is queued to the timer database, remove it
  //
  if (Event->Timer.Link.ForwardLink != NULL) {
    RemoveEntryList (&Event->Timer.Link);
    Event->Timer.Link.ForwardLink = NULL;
  }

  Event->Timer.TriggerTime = 0;
  Event->Timer.Period      = 0;

  if (Type != TimerCancel) {
    if (Type == TimerPeriodic) {
      if (TriggerTime == 0) {
        gTimer->GetTimerPeriod (gTimer, &TriggerTime);
      }

      Event->Timer.Period = TriggerTime;
    }

    Event->Timer.TriggerTime = CoreCurrentSystemTime () + TriggerTime;
    CoreInsertEventTimer (Event);

    if (TriggerTime == 0) {
      CoreSignalEvent (mEfiCheckTimerEvent);
    }
  }

  CoreReleaseLock (&mEfiTimerLock);

  return EFI_SUCCESS;
}

/**
  Register the DXE core timer tick handler with the timer driver.

  @retval EFI_SUCCESS           The timer handler was registered.
  @retval EFI_UNSUPPORTED       The platform does not support timer interrupts.
  @retval EFI_ALREADY_STARTED   A handler is already registered.
  @retval EFI_DEVICE_ERROR      The timer handler could not be registered.

**/
EFI_STATUS
EFIAPI
CoreRegisterTimerHandler (
  VOID
  )
{
  return gTimer->RegisterHandler (gTimer, CoreTimerTick);
}
