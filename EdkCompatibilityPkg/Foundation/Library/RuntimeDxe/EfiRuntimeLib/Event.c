/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Event.c

Abstract:

  Support for Event lib fucntions.

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"

EFI_EVENT
RtEfiLibCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration
  )
/*++

Routine Description:

  Create a protocol notification event and return it.

Arguments:

  ProtocolGuid    - Protocol to register notification event on.

  NotifyTpl       - Maximum TPL to single the NotifyFunction.

  NotifyFunction  - EFI notification routine.

  NotifyContext   - Context passed into Event when it is created.

  Registration    - Registration key returned from RegisterProtocolNotify().

Returns:

  The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
  is added to the system.

--*/
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  //
  // Create the event
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  NotifyTpl,
                  NotifyFunction,
                  NotifyContext,
                  &Event
                  );
  ASSERT (!EFI_ERROR (Status));

  //
  // Register for protocol notifactions on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  ProtocolGuid,
                  Event,
                  Registration
                  );

  ASSERT (!EFI_ERROR (Status));

  //
  // Kick the event so we will perform an initial pass of
  // current installed drivers
  //
  gBS->SignalEvent (Event);
  return Event;
}

EFI_STATUS
EfiLibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
/*++

Routine Description:

  Return the EFI 1.0 System Tabl entry with TableGuid

Arguments:

  TableGuid - Name of entry to return in the system table
  Table     - Pointer in EFI system table associated with TableGuid

Returns:

  EFI_SUCCESS - Table returned;
  EFI_NOT_FOUND - TableGuid not in EFI system table

--*/
{
  UINTN Index;

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (EfiCompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EfiConvertList (
  IN UINTN                DebugDisposition,
  IN OUT EFI_LIST_ENTRY   *ListHead
  )
/*++

Routine Description:

  Conver the standard Lib double linked list to a virtual mapping.

Arguments:

  DebugDisposition - Argument to EfiConvertPointer (EFI 1.0 API)

  ListHead         - Head of linked list to convert

Returns:

  EFI_SUCCESS

--*/
{
  EFI_LIST_ENTRY  *Link;
  EFI_LIST_ENTRY  *NextLink;

  //
  // Convert all the ForwardLink & BackLink pointers in the list
  //
  Link = ListHead;
  do {
    NextLink = Link->ForwardLink;

    EfiConvertPointer (
      Link->ForwardLink == ListHead ? DebugDisposition : 0,
      (VOID **) &Link->ForwardLink
      );

    EfiConvertPointer (
      Link->BackLink == ListHead ? DebugDisposition : 0,
      (VOID **) &Link->BackLink
      );

    Link = NextLink;
  } while (Link != ListHead);
  return EFI_SUCCESS;
}

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

STATIC
VOID
EFIAPI
EventNotifySignalAllNullEvent (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // This null event is a size efficent way to enusre that 
  // EFI_EVENT_NOTIFY_SIGNAL_ALL is error checked correctly.
  // EFI_EVENT_NOTIFY_SIGNAL_ALL is now mapped into 
  // CreateEventEx() and this function is used to make the
  // old error checking in CreateEvent() for Tiano extensions
  // function.
  //
  return;
}

#endif

EFI_STATUS
EFIAPI
RtEfiCreateEventLegacyBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *LegacyBootEvent
  )
/*++

Routine Description:
  Create a Legacy Boot Event.  
  Tiano extended the CreateEvent Type enum to add a legacy boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification by 
  declaring a GUID for the legacy boot event class. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  LegacyBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Returns:
  EFI_SUCCESS   Event was created.
  Other         Event was not created.

--*/
{
  EFI_STATUS        Status;
  UINT32            EventType;
  EFI_EVENT_NOTIFY  WorkerNotifyFunction;

#if (EFI_SPECIFICATION_VERSION < 0x00020000)

  if (NotifyFunction == NULL) {
    EventType = EFI_EVENT_SIGNAL_LEGACY_BOOT | EFI_EVENT_NOTIFY_SIGNAL_ALL;
  } else {
    EventType = EFI_EVENT_SIGNAL_LEGACY_BOOT;
  }
  WorkerNotifyFunction = NotifyFunction;

  //
  // prior to UEFI 2.0 use Tiano extension to EFI
  //
  Status = gBS->CreateEvent (
                  EventType,
                  NotifyTpl,
                  WorkerNotifyFunction,
                  NotifyContext,
                  LegacyBootEvent
                  );
#else

  EventType = EFI_EVENT_NOTIFY_SIGNAL;
  if (NotifyFunction == NULL) {
    //
    // CreatEventEx will check NotifyFunction is NULL or not
    //
    WorkerNotifyFunction = EventNotifySignalAllNullEvent;
  } else {
    WorkerNotifyFunction = NotifyFunction;
  }

  //
  // For UEFI 2.0 and the future use an Event Group
  //
  Status = gBS->CreateEventEx (
                  EventType,
                  NotifyTpl,
                  WorkerNotifyFunction,
                  NotifyContext,
                  &gEfiEventLegacyBootGuid,
                  LegacyBootEvent
                  );
#endif
  return Status;
}

EFI_STATUS
EFIAPI
RtEfiCreateEventReadyToBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *ReadyToBootEvent
  )
/*++

Routine Description:
  Create a Read to Boot Event.  
  
  Tiano extended the CreateEvent Type enum to add a ready to boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification and use 
  the ready to boot event class defined in UEFI 2.0. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  ReadyToBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Return:
  EFI_SUCCESS   - Event was created.
  Other         - Event was not created.

--*/
{
  EFI_STATUS        Status;
  UINT32            EventType;
  EFI_EVENT_NOTIFY  WorkerNotifyFunction;

#if (EFI_SPECIFICATION_VERSION < 0x00020000)

  if (NotifyFunction == NULL) {
    EventType = EFI_EVENT_SIGNAL_READY_TO_BOOT | EFI_EVENT_NOTIFY_SIGNAL_ALL;
  } else {
    EventType = EFI_EVENT_SIGNAL_READY_TO_BOOT;
  }
  WorkerNotifyFunction = NotifyFunction;

  //
  // prior to UEFI 2.0 use Tiano extension to EFI
  //
  Status = gBS->CreateEvent (
                  EventType,
                  NotifyTpl,
                  WorkerNotifyFunction,
                  NotifyContext,
                  ReadyToBootEvent
                  );
#else

  EventType = EFI_EVENT_NOTIFY_SIGNAL;
  if (NotifyFunction == NULL) {
    //
    // CreatEventEx will check NotifyFunction is NULL or not
    //
    WorkerNotifyFunction = EventNotifySignalAllNullEvent;
  } else {
    WorkerNotifyFunction = NotifyFunction;
  }

  //
  // For UEFI 2.0 and the future use an Event Group
  //
  Status = gBS->CreateEventEx (
                  EventType,
                  NotifyTpl,
                  WorkerNotifyFunction,
                  NotifyContext,
                  &gEfiEventReadyToBootGuid,
                  ReadyToBootEvent
                  );
#endif
  return Status;
}
