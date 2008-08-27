/** @file
  DXE Core library services.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"

UINTN mErrorLevel = DEBUG_ERROR | DEBUG_LOAD;

EFI_DXE_DEVICE_HANDLE_EXTENDED_DATA mStatusCodeData = {
  {
    sizeof (EFI_STATUS_CODE_DATA),
    0,
    EFI_STATUS_CODE_DXE_CORE_GUID
  },
  NULL
};


/**
  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiCallerIdGuid,
  with a handle as additional information.

  @param  Value              Describes the class/subclass/operation of the
                             hardware or software entity that the Status Code
                             relates to.
  @param  Handle             Additional information.

**/
VOID
CoreReportProgressCodeSpecific (
  IN  EFI_STATUS_CODE_VALUE   Value,
  IN  EFI_HANDLE              Handle
  )
{
  mStatusCodeData.DataHeader.Size = sizeof (EFI_DXE_DEVICE_HANDLE_EXTENDED_DATA) - sizeof (EFI_STATUS_CODE_DATA);
  mStatusCodeData.Handle          = Handle;

  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiCallerIdGuid,
      (EFI_STATUS_CODE_DATA *) &mStatusCodeData
      );
  }
}


/**
  Report status code of type EFI_PROGRESS_CODE by caller ID gEfiCallerIdGuid.

  @param  Value              Describes the class/subclass/operation of the
                             hardware or software entity that the Status Code
                             relates to.

**/
VOID
CoreReportProgressCode (
  IN  EFI_STATUS_CODE_VALUE   Value
  )
{
  if ((gStatusCode != NULL) && (gStatusCode->ReportStatusCode != NULL) ) {
    gStatusCode->ReportStatusCode (
      EFI_PROGRESS_CODE,
      Value,
      0,
      &gEfiCallerIdGuid,
      NULL
      );
  }
}


//
// Lock Stuff
//
/**
  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

  @param  Lock               The EFI_LOCK structure to initialize

  @retval EFI_SUCCESS        Lock Owned.
  @retval EFI_ACCESS_DENIED  Reentrant Lock Acquisition, Lock not Owned.

**/
EFI_STATUS
CoreAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock != EfiLockUninitialized);

  if (Lock->Lock == EfiLockAcquired) {
    //
    // Lock is already owned, so bail out
    //
    return EFI_ACCESS_DENIED;
  }

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);

  Lock->Lock = EfiLockAcquired;
  return EFI_SUCCESS;
}



/**
  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.

  @param  Lock               The lock to acquire

  @return Lock owned

**/
VOID
CoreAcquireLock (
  IN EFI_LOCK  *Lock
  )
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockReleased);

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);
  Lock->Lock     = EfiLockAcquired;
}



/**
  Releases ownership of the mutual exclusion lock, and
  restores the previous task priority level.

  @param  Lock               The lock to release

  @return Lock unowned

**/
VOID
CoreReleaseLock (
  IN EFI_LOCK  *Lock
  )
{
  EFI_TPL Tpl;

  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock == EfiLockAcquired);

  Tpl = Lock->OwnerTpl;

  Lock->Lock = EfiLockReleased;

  CoreRestoreTpl (Tpl);
}


/**
  Create a protocol notification event and return it.

  @param  ProtocolGuid       Protocol to register notification event on.
  @param  NotifyTpl          Maximum TPL to signal the NotifyFunction.
  @param  NotifyFunction     EFI notification routine.
  @param  NotifyContext      Context passed into Event when it is created.
  @param  Registration       Registration key returned from
                             RegisterProtocolNotify().
  @param  SignalFlag         Boolean value to decide whether kick the event after
                             register or not.

  @return The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
          is added to the system.

**/
EFI_EVENT
CoreCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration,
  IN  BOOLEAN             SignalFlag
  )
{
  EFI_STATUS              Status;
  EFI_EVENT               Event;

  //
  // Create the event
  //
  Status = CoreCreateEvent (
            EVT_NOTIFY_SIGNAL,
            NotifyTpl,
            NotifyFunction,
            NotifyContext,
            &Event
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifactions on this event
  //
  Status = CoreRegisterProtocolNotify (
             ProtocolGuid,
             Event,
             Registration
             );
  ASSERT_EFI_ERROR (Status);

  if (SignalFlag) {
    //
    // Kick the event so we will perform an initial pass of
    // current installed drivers
    //
    CoreSignalEvent (Event);
  }

  return Event;
}


