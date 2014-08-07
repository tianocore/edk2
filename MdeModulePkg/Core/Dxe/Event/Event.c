/** @file
  UEFI Event support functions implemented in this file.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "DxeMain.h"
#include "Event.h"

///
/// gEfiCurrentTpl - Current Task priority level
///
EFI_TPL  gEfiCurrentTpl = TPL_APPLICATION;

///
/// gEventQueueLock - Protects the event queues
///
EFI_LOCK gEventQueueLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_HIGH_LEVEL);

///
/// gEventQueue - A list of event's to notify for each priority level
///
LIST_ENTRY      gEventQueue[TPL_HIGH_LEVEL + 1];

///
/// gEventPending - A bitmask of the EventQueues that are pending
///
UINTN           gEventPending = 0;

///
/// gEventSignalQueue - A list of events to signal based on EventGroup type
///
LIST_ENTRY      gEventSignalQueue = INITIALIZE_LIST_HEAD_VARIABLE (gEventSignalQueue);

///
/// Enumerate the valid types
///
UINT32 mEventTable[] = {
  ///
  /// 0x80000200       Timer event with a notification function that is
  /// queue when the event is signaled with SignalEvent()
  ///
  EVT_TIMER | EVT_NOTIFY_SIGNAL,
  ///
  /// 0x80000000       Timer event without a notification function. It can be
  /// signaled with SignalEvent() and checked with CheckEvent() or WaitForEvent().
  ///
  EVT_TIMER,
  ///
  /// 0x00000100       Generic event with a notification function that
  /// can be waited on with CheckEvent() or WaitForEvent()
  ///
  EVT_NOTIFY_WAIT,
  ///
  /// 0x00000200       Generic event with a notification function that
  /// is queue when the event is signaled with SignalEvent()
  ///
  EVT_NOTIFY_SIGNAL,
  ///
  /// 0x00000201       ExitBootServicesEvent.
  ///
  EVT_SIGNAL_EXIT_BOOT_SERVICES,
  ///
  /// 0x60000202       SetVirtualAddressMapEvent.
  ///
  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,

  ///
  /// 0x00000000       Generic event without a notification function.
  /// It can be signaled with SignalEvent() and checked with CheckEvent()
  /// or WaitForEvent().
  ///
  0x00000000,
  ///
  /// 0x80000100       Timer event with a notification function that can be
  /// waited on with CheckEvent() or WaitForEvent()
  ///
  EVT_TIMER | EVT_NOTIFY_WAIT,
};

///
/// gIdleLoopEvent - Event which is signalled when the core is idle
///
EFI_EVENT       gIdleLoopEvent = NULL;


/**
  Enter critical section by acquiring the lock on gEventQueueLock.

**/
VOID
CoreAcquireEventLock (
  VOID
  )
{
  CoreAcquireLock (&gEventQueueLock);
}


/**
  Exit critical section by releasing the lock on gEventQueueLock.

**/
VOID
CoreReleaseEventLock (
  VOID
  )
{
  CoreReleaseLock (&gEventQueueLock);
}



/**
  Initializes "event" support.

  @retval EFI_SUCCESS            Always return success

**/
EFI_STATUS
CoreInitializeEventServices (
  VOID
  )
{
  UINTN        Index;

  for (Index=0; Index <= TPL_HIGH_LEVEL; Index++) {
    InitializeListHead (&gEventQueue[Index]);
  }

  CoreInitializeTimer ();

  CoreCreateEventEx (
    EVT_NOTIFY_SIGNAL,
    TPL_NOTIFY,
    CoreEmptyCallbackFunction,
    NULL,
    &gIdleLoopEventGuid,
    &gIdleLoopEvent
    );

  return EFI_SUCCESS;
}



/**
  Dispatches all pending events.

  @param  Priority               The task priority level of event notifications
                                 to dispatch

**/
VOID
CoreDispatchEventNotifies (
  IN EFI_TPL      Priority
  )
{
  IEVENT          *Event;
  LIST_ENTRY      *Head;

  CoreAcquireEventLock ();
  ASSERT (gEventQueueLock.OwnerTpl == Priority);
  Head = &gEventQueue[Priority];

  //
  // Dispatch all the pending notifications
  //
  while (!IsListEmpty (Head)) {

    Event = CR (Head->ForwardLink, IEVENT, NotifyLink, EVENT_SIGNATURE);
    RemoveEntryList (&Event->NotifyLink);

    Event->NotifyLink.ForwardLink = NULL;

    //
    // Only clear the SIGNAL status if it is a SIGNAL type event.
    // WAIT type events are only cleared in CheckEvent()
    //
    if ((Event->Type & EVT_NOTIFY_SIGNAL) != 0) {
      Event->SignalCount = 0;
    }

    CoreReleaseEventLock ();

    //
    // Notify this event
    //
    ASSERT (Event->NotifyFunction != NULL);
    Event->NotifyFunction (Event, Event->NotifyContext);

    //
    // Check for next pending event
    //
    CoreAcquireEventLock ();
  }

  gEventPending &= ~(UINTN)(1 << Priority);
  CoreReleaseEventLock ();
}



/**
  Queues the event's notification function to fire.

  @param  Event                  The Event to notify

**/
VOID
CoreNotifyEvent (
  IN  IEVENT      *Event
  )
{

  //
  // Event database must be locked
  //
  ASSERT_LOCKED (&gEventQueueLock);

  //
  // If the event is queued somewhere, remove it
  //

  if (Event->NotifyLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->NotifyLink);
    Event->NotifyLink.ForwardLink = NULL;
  }

  //
  // Queue the event to the pending notification list
  //

  InsertTailList (&gEventQueue[Event->NotifyTpl], &Event->NotifyLink);
  gEventPending |= (UINTN)(1 << Event->NotifyTpl);
}




/**
  Signals all events in the EventGroup.

  @param  EventGroup             The list to signal

**/
VOID
CoreNotifySignalList (
  IN EFI_GUID     *EventGroup
  )
{
  LIST_ENTRY              *Link;
  LIST_ENTRY              *Head;
  IEVENT                  *Event;

  CoreAcquireEventLock ();

  Head = &gEventSignalQueue;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    Event = CR (Link, IEVENT, SignalLink, EVENT_SIGNATURE);
    if (CompareGuid (&Event->EventGroup, EventGroup)) {
      CoreNotifyEvent (Event);
    }
  }

  CoreReleaseEventLock ();
}


/**
  Creates an event.

  @param  Type                   The type of event to create and its mode and
                                 attributes
  @param  NotifyTpl              The task priority level of event notifications
  @param  NotifyFunction         Pointer to the events notification function
  @param  NotifyContext          Pointer to the notification functions context;
                                 corresponds to parameter "Context" in the
                                 notification function
  @param  Event                  Pointer to the newly created event if the call
                                 succeeds; undefined otherwise

  @retval EFI_SUCCESS            The event structure was created
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  @retval EFI_OUT_OF_RESOURCES   The event could not be allocated

**/
EFI_STATUS
EFIAPI
CoreCreateEvent (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction, OPTIONAL
  IN VOID                     *NotifyContext, OPTIONAL
  OUT EFI_EVENT               *Event
  )
{
  return CoreCreateEventEx (Type, NotifyTpl, NotifyFunction, NotifyContext, NULL, Event);
}



/**
  Creates an event in a group.

  @param  Type                   The type of event to create and its mode and
                                 attributes
  @param  NotifyTpl              The task priority level of event notifications
  @param  NotifyFunction         Pointer to the events notification function
  @param  NotifyContext          Pointer to the notification functions context;
                                 corresponds to parameter "Context" in the
                                 notification function
  @param  EventGroup             GUID for EventGroup if NULL act the same as
                                 gBS->CreateEvent().
  @param  Event                  Pointer to the newly created event if the call
                                 succeeds; undefined otherwise

  @retval EFI_SUCCESS            The event structure was created
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  @retval EFI_OUT_OF_RESOURCES   The event could not be allocated

**/
EFI_STATUS
EFIAPI
CoreCreateEventEx (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction, OPTIONAL
  IN CONST VOID               *NotifyContext, OPTIONAL
  IN CONST EFI_GUID           *EventGroup,    OPTIONAL
  OUT EFI_EVENT               *Event
  )
{
  //
  // If it's a notify type of event, check for invalid NotifyTpl
  //
  if ((Type & (EVT_NOTIFY_WAIT | EVT_NOTIFY_SIGNAL)) != 0) {
    if (NotifyTpl != TPL_APPLICATION &&
        NotifyTpl != TPL_CALLBACK &&
        NotifyTpl != TPL_NOTIFY) {
      return EFI_INVALID_PARAMETER;
    }
  }

  return CoreCreateEventInternal (Type, NotifyTpl, NotifyFunction, NotifyContext, EventGroup, Event);
}

/**
  Creates a general-purpose event structure

  @param  Type                   The type of event to create and its mode and
                                 attributes
  @param  NotifyTpl              The task priority level of event notifications
  @param  NotifyFunction         Pointer to the events notification function
  @param  NotifyContext          Pointer to the notification functions context;
                                 corresponds to parameter "Context" in the
                                 notification function
  @param  EventGroup             GUID for EventGroup if NULL act the same as
                                 gBS->CreateEvent().
  @param  Event                  Pointer to the newly created event if the call
                                 succeeds; undefined otherwise

  @retval EFI_SUCCESS            The event structure was created
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  @retval EFI_OUT_OF_RESOURCES   The event could not be allocated

**/
EFI_STATUS
EFIAPI
CoreCreateEventInternal (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction, OPTIONAL
  IN CONST VOID               *NotifyContext, OPTIONAL
  IN CONST EFI_GUID           *EventGroup,    OPTIONAL
  OUT EFI_EVENT               *Event
  )
{
  EFI_STATUS      Status;
  IEVENT          *IEvent;
  INTN            Index;


  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to make sure no reserved flags are set
  //
  Status = EFI_INVALID_PARAMETER;
  for (Index = 0; Index < (sizeof (mEventTable) / sizeof (UINT32)); Index++) {
     if (Type == mEventTable[Index]) {
       Status = EFI_SUCCESS;
       break;
     }
  }
  if(EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert Event type for pre-defined Event groups
  //
  if (EventGroup != NULL) {
    //
    // For event group, type EVT_SIGNAL_EXIT_BOOT_SERVICES and EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
    // are not valid
    //
    if ((Type == EVT_SIGNAL_EXIT_BOOT_SERVICES) || (Type == EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE)) {
      return EFI_INVALID_PARAMETER;
    }
    if (CompareGuid (EventGroup, &gEfiEventExitBootServicesGuid)) {
      Type = EVT_SIGNAL_EXIT_BOOT_SERVICES;
    } else if (CompareGuid (EventGroup, &gEfiEventVirtualAddressChangeGuid)) {
      Type = EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
    }
  } else {
    //
    // Convert EFI 1.10 Events to their UEFI 2.0 CreateEventEx mapping
    //
    if (Type == EVT_SIGNAL_EXIT_BOOT_SERVICES) {
      EventGroup = &gEfiEventExitBootServicesGuid;
    } else if (Type == EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE) {
      EventGroup = &gEfiEventVirtualAddressChangeGuid;
    }
  }

  //
  // If it's a notify type of event, check its parameters
  //
  if ((Type & (EVT_NOTIFY_WAIT | EVT_NOTIFY_SIGNAL)) != 0) {
    //
    // Check for an invalid NotifyFunction or NotifyTpl
    //
    if ((NotifyFunction == NULL) ||
        (NotifyTpl <= TPL_APPLICATION) ||
       (NotifyTpl >= TPL_HIGH_LEVEL)) {
      return EFI_INVALID_PARAMETER;
    }

  } else {
    //
    // No notification needed, zero ignored values
    //
    NotifyTpl = 0;
    NotifyFunction = NULL;
    NotifyContext = NULL;
  }

  //
  // Allocate and initialize a new event structure.
  //
  if ((Type & EVT_RUNTIME) != 0) {
    IEvent = AllocateRuntimeZeroPool (sizeof (IEVENT));
  } else {
    IEvent = AllocateZeroPool (sizeof (IEVENT));
  }
  if (IEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IEvent->Signature = EVENT_SIGNATURE;
  IEvent->Type = Type;

  IEvent->NotifyTpl      = NotifyTpl;
  IEvent->NotifyFunction = NotifyFunction;
  IEvent->NotifyContext  = (VOID *)NotifyContext;
  if (EventGroup != NULL) {
    CopyGuid (&IEvent->EventGroup, EventGroup);
    IEvent->ExFlag = TRUE;
  }

  *Event = IEvent;

  if ((Type & EVT_RUNTIME) != 0) {
    //
    // Keep a list of all RT events so we can tell the RT AP.
    //
    IEvent->RuntimeData.Type           = Type;
    IEvent->RuntimeData.NotifyTpl      = NotifyTpl;
    IEvent->RuntimeData.NotifyFunction = NotifyFunction;
    IEvent->RuntimeData.NotifyContext  = (VOID *) NotifyContext;
    IEvent->RuntimeData.Event          = (EFI_EVENT *) IEvent;
    InsertTailList (&gRuntime->EventHead, &IEvent->RuntimeData.Link);
  }

  CoreAcquireEventLock ();

  if ((Type & EVT_NOTIFY_SIGNAL) != 0x00000000) {
    //
    // The Event's NotifyFunction must be queued whenever the event is signaled
    //
    InsertHeadList (&gEventSignalQueue, &IEvent->SignalLink);
  }

  CoreReleaseEventLock ();

  //
  // Done
  //
  return EFI_SUCCESS;
}




/**
  Signals the event.  Queues the event to be notified if needed.

  @param  UserEvent              The event to signal .

  @retval EFI_INVALID_PARAMETER  Parameters are not valid.
  @retval EFI_SUCCESS            The event was signaled.

**/
EFI_STATUS
EFIAPI
CoreSignalEvent (
  IN EFI_EVENT    UserEvent
  )
{
  IEVENT          *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireEventLock ();

  //
  // If the event is not already signalled, do so
  //

  if (Event->SignalCount == 0x00000000) {
    Event->SignalCount++;

    //
    // If signalling type is a notify function, queue it
    //
    if ((Event->Type & EVT_NOTIFY_SIGNAL) != 0) {
      if (Event->ExFlag) {
        //
        // The CreateEventEx() style requires all members of the Event Group
        //  to be signaled.
        //
        CoreReleaseEventLock ();
        CoreNotifySignalList (&Event->EventGroup);
        CoreAcquireEventLock ();
       } else {
        CoreNotifyEvent (Event);
      }
    }
  }

  CoreReleaseEventLock ();
  return EFI_SUCCESS;
}



/**
  Check the status of an event.

  @param  UserEvent              The event to check

  @retval EFI_SUCCESS            The event is in the signaled state
  @retval EFI_NOT_READY          The event is not in the signaled state
  @retval EFI_INVALID_PARAMETER  Event is of type EVT_NOTIFY_SIGNAL

**/
EFI_STATUS
EFIAPI
CoreCheckEvent (
  IN EFI_EVENT        UserEvent
  )
{
  IEVENT      *Event;
  EFI_STATUS  Status;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Event->Type & EVT_NOTIFY_SIGNAL) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_READY;

  if ((Event->SignalCount == 0) && ((Event->Type & EVT_NOTIFY_WAIT) != 0)) {

    //
    // Queue the wait notify function
    //
    CoreAcquireEventLock ();
    if (Event->SignalCount == 0) {
      CoreNotifyEvent (Event);
    }
    CoreReleaseEventLock ();
  }

  //
  // If the even looks signalled, get the lock and clear it
  //

  if (Event->SignalCount != 0) {
    CoreAcquireEventLock ();

    if (Event->SignalCount != 0) {
      Event->SignalCount = 0;
      Status = EFI_SUCCESS;
    }

    CoreReleaseEventLock ();
  }

  return Status;
}



/**
  Stops execution until an event is signaled.

  @param  NumberOfEvents         The number of events in the UserEvents array
  @param  UserEvents             An array of EFI_EVENT
  @param  UserIndex              Pointer to the index of the event which
                                 satisfied the wait condition

  @retval EFI_SUCCESS            The event indicated by Index was signaled.
  @retval EFI_INVALID_PARAMETER  The event indicated by Index has a notification
                                 function or Event was not a valid type
  @retval EFI_UNSUPPORTED        The current TPL is not TPL_APPLICATION

**/
EFI_STATUS
EFIAPI
CoreWaitForEvent (
  IN UINTN        NumberOfEvents,
  IN EFI_EVENT    *UserEvents,
  OUT UINTN       *UserIndex
  )
{
  EFI_STATUS      Status;
  UINTN           Index;

  //
  // Can only WaitForEvent at TPL_APPLICATION
  //
  if (gEfiCurrentTpl != TPL_APPLICATION) {
    return EFI_UNSUPPORTED;
  }

  if (NumberOfEvents == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (UserEvents == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for(;;) {

    for(Index = 0; Index < NumberOfEvents; Index++) {

      Status = CoreCheckEvent (UserEvents[Index]);

      //
      // provide index of event that caused problem
      //
      if (Status != EFI_NOT_READY) {
        if (UserIndex != NULL) {
          *UserIndex = Index;
        }
        return Status;
      }
    }

    //
    // Signal the Idle event
    //
    CoreSignalEvent (gIdleLoopEvent);
  }
}


/**
  Closes an event and frees the event structure.

  @param  UserEvent              Event to close

  @retval EFI_INVALID_PARAMETER  Parameters are not valid.
  @retval EFI_SUCCESS            The event has been closed

**/
EFI_STATUS
EFIAPI
CoreCloseEvent (
  IN EFI_EVENT    UserEvent
  )
{
  EFI_STATUS  Status;
  IEVENT      *Event;

  Event = UserEvent;

  if (Event == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event->Signature != EVENT_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If it's a timer event, make sure it's not pending
  //
  if ((Event->Type & EVT_TIMER) != 0) {
    CoreSetTimer (Event, TimerCancel, 0);
  }

  CoreAcquireEventLock ();

  //
  // If the event is queued somewhere, remove it
  //

  if (Event->RuntimeData.Link.ForwardLink != NULL) {
    RemoveEntryList (&Event->RuntimeData.Link);
  }

  if (Event->NotifyLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->NotifyLink);
  }

  if (Event->SignalLink.ForwardLink != NULL) {
    RemoveEntryList (&Event->SignalLink);
  }

  CoreReleaseEventLock ();

  //
  // If the event is registered on a protocol notify, then remove it from the protocol database
  //
  CoreUnregisterProtocolNotify (Event);

  Status = CoreFreePool (Event);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

