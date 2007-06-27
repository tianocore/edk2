/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtGopInput.c

Abstract:

  This file produces the Simple Text In for an Gop window.

  This stuff is linked at the hip to the Window, since the window
  processing is done in a thread kicked off in WinNtGopImplementation.c

  Since the window information is processed in an other thread we need
  a keyboard Queue to pass data about. The Simple Text In code just
  takes data off the Queue. The WinProc message loop takes keyboard input
  and places it in the Queue.


**/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/EventGroup.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GraphicsOutput.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "WinNtGop.h"


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateCreateQ (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  Private->WinNtThunk->InitializeCriticalSection (&Private->QCriticalSection);

  Private->Queue.Front  = 0;
  Private->Queue.Rear   = MAX_Q - 1;
  Private->Queue.Count  = 0;
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateDestroyQ (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  Private->Queue.Count = 0;
  Private->WinNtThunk->DeleteCriticalSection (&Private->QCriticalSection);
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description
  @param  Key                   TODO: add argument description

  @retval EFI_NOT_READY         TODO: Add description for return value
  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateAddQ (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  )
{
  Private->WinNtThunk->EnterCriticalSection (&Private->QCriticalSection);

  if (Private->Queue.Count == MAX_Q) {
    Private->WinNtThunk->LeaveCriticalSection (&Private->QCriticalSection);
    return EFI_NOT_READY;
  }

  Private->Queue.Rear                   = (Private->Queue.Rear + 1) % MAX_Q;
  Private->Queue.Q[Private->Queue.Rear] = Key;
  Private->Queue.Count++;

  Private->WinNtThunk->LeaveCriticalSection (&Private->QCriticalSection);
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description
  @param  Key                   TODO: add argument description

  @retval EFI_NOT_READY         TODO: Add description for return value
  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateDeleteQ (
  IN  GOP_PRIVATE_DATA    *Private,
  OUT EFI_INPUT_KEY       *Key
  )
{
  Private->WinNtThunk->EnterCriticalSection (&Private->QCriticalSection);

  if (Private->Queue.Count == 0) {
    Private->WinNtThunk->LeaveCriticalSection (&Private->QCriticalSection);
    return EFI_NOT_READY;
  }

  *Key                  = Private->Queue.Q[Private->Queue.Front];
  Private->Queue.Front  = (Private->Queue.Front + 1) % MAX_Q;
  Private->Queue.Count--;

  Private->WinNtThunk->LeaveCriticalSection (&Private->QCriticalSection);
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_NOT_READY         TODO: Add description for return value
  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateCheckQ (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  if (Private->Queue.Count == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

//
// Simple Text In implementation.
//


/**
  TODO: Add function description

  @param  This                  TODO: add argument description
  @param  ExtendedVerification  TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
EFIAPI
WinNtGopSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  IN BOOLEAN                              ExtendedVerification
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_INPUT_KEY     Key;
  EFI_TPL           OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // A reset is draining the Queue
  //
  while (GopPrivateDeleteQ (Private, &Key) == EFI_SUCCESS)
    ;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  This                  TODO: add argument description
  @param  Key                   TODO: add argument description

  @return TODO: add return values

**/
STATIC
EFI_STATUS
EFIAPI
WinNtGopSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  OUT EFI_INPUT_KEY                       *Key
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = GopPrivateCheckQ (Private);
  if (!EFI_ERROR (Status)) {
    //
    // If a Key press exists try and read it.
    //
    Status = GopPrivateDeleteQ (Private, Key);
  }

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  TODO: Add function description

  @param  Event                 TODO: add argument description
  @param  Context               TODO: add argument description

  @return TODO: add return values

**/
STATIC
VOID
EFIAPI
WinNtGopSimpleTextInWaitForKey (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = (GOP_PRIVATE_DATA *) Context;

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = GopPrivateCheckQ (Private);
  if (!EFI_ERROR (Status)) {
    //
    // If a there is a key in the queue signal our event.
    //
    gBS->SignalEvent (Event);
  } else {
    //
    // We need to sleep or NT will schedule this thread with such high
    // priority that WinProc thread will never run and we will not see
    // keyboard input. This Sleep makes the syste run 10x faster, so don't
    // remove it.
    //
    Private->WinNtThunk->Sleep (1);
  }

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
WinNtGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS  Status;

  GopPrivateCreateQ (Private);

  //
  // Initialize Simple Text In protoocol
  //
  Private->SimpleTextIn.Reset         = WinNtGopSimpleTextInReset;
  Private->SimpleTextIn.ReadKeyStroke = WinNtGopSimpleTextInReadKeyStroke;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  WinNtGopSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextIn.WaitForKey
                  );

  return Status;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
WinNtGopDestroySimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  GopPrivateDestroyQ (Private);
  return EFI_SUCCESS;
}
