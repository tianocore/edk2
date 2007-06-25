/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtUgaInput.c

Abstract:

  This file produces the Simple Text In for an Uga window.

  This stuff is linked at the hip to the Window, since the window
  processing is done in a thread kicked off in WinNtUgaImplementation.c

  Since the window information is processed in an other thread we need
  a keyboard Queue to pass data about. The Simple Text In code just
  takes data off the Queue. The WinProc message loop takes keyboard input
  and places it in the Queue.

--*/

#include "WinNtUga.h"

EFI_STATUS
UgaPrivateCreateQ (
  IN  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  Private->WinNtThunk->InitializeCriticalSection (&Private->QCriticalSection);

  Private->Queue.Front  = 0;
  Private->Queue.Rear   = MAX_Q - 1;
  Private->Queue.Count  = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
UgaPrivateDestroyQ (
  IN  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  Private->Queue.Count = 0;
  Private->WinNtThunk->DeleteCriticalSection (&Private->QCriticalSection);
  return EFI_SUCCESS;
}

EFI_STATUS
UgaPrivateAddQ (
  IN  UGA_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description
  Key     - TODO: add argument description

Returns:

  EFI_NOT_READY - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
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

EFI_STATUS
UgaPrivateDeleteQ (
  IN  UGA_PRIVATE_DATA    *Private,
  OUT EFI_INPUT_KEY       *Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description
  Key     - TODO: add argument description

Returns:

  EFI_NOT_READY - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
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

EFI_STATUS
UgaPrivateCheckQ (
  IN  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  EFI_NOT_READY - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  if (Private->Queue.Count == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

//
// Simple Text In implementation.
//

EFI_STATUS
EFIAPI
WinNtUgaSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_IN_PROTOCOL          *This,
  IN BOOLEAN                              ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UGA_PRIVATE_DATA  *Private;
  EFI_INPUT_KEY     Key;
  EFI_TPL           OldTpl;

  Private = UGA_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // A reset is draining the Queue
  //
  while (UgaPrivateDeleteQ (Private, &Key) == EFI_SUCCESS)
    ;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtUgaSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_IN_PROTOCOL          *This,
  OUT EFI_INPUT_KEY                       *Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description
  Key   - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  UGA_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = UGA_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = UgaPrivateCheckQ (Private);
  if (!EFI_ERROR (Status)) {
    //
    // If a Key press exists try and read it.
    //
    Status = UgaPrivateDeleteQ (Private, Key);
  }

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}

STATIC
VOID
EFIAPI
WinNtUgaSimpleTextInWaitForKey (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Event   - TODO: add argument description
  Context - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  UGA_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = (UGA_PRIVATE_DATA *) Context;

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = UgaPrivateCheckQ (Private);
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

EFI_STATUS
WinNtUgaInitializeSimpleTextInForWindow (
  IN  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_STATUS  Status;

  UgaPrivateCreateQ (Private);

  //
  // Initialize Simple Text In protoocol
  //
  Private->SimpleTextIn.Reset         = WinNtUgaSimpleTextInReset;
  Private->SimpleTextIn.ReadKeyStroke = WinNtUgaSimpleTextInReadKeyStroke;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  WinNtUgaSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextIn.WaitForKey
                  );

  return Status;
}

EFI_STATUS
WinNtUgaDestroySimpleTextInForWindow (
  IN  UGA_PRIVATE_DATA    *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  UgaPrivateDestroyQ (Private);
  return EFI_SUCCESS;
}
