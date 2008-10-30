/*++

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ConsoleIn.c

Abstract:

  Console based on Posix APIs.

  This file attaches a SimpleTextIn protocol to a previously open window.

  The constructor for this protocol depends on an open window. Currently
  the SimpleTextOut protocol creates a window when it's constructor is called.
  Thus this code must run after the constructor for the SimpleTextOut
  protocol

--*/

#include "Console.h"
#include <sys/poll.h>

//
// Private worker functions
//
EFI_STATUS
UnixSimpleTextInCheckKey (
  UNIX_SIMPLE_TEXT_PRIVATE_DATA  *Private
  );

EFI_STATUS
EFIAPI
UnixSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = UNIX_SIMPLE_TEXT_IN_PRIVATE_DATA_FROM_THIS (This);
  return EFI_SUCCESS;
}

EFI_STATUS
UnixConvertInputRecordToEfiKey (
  IN  char c,
  OUT EFI_INPUT_KEY   *Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  InputRecord - TODO: add argument description
  Key         - TODO: add argument description

Returns:

  EFI_NOT_READY - TODO: Add description for return value
  EFI_NOT_READY - TODO: Add description for return value
  EFI_NOT_READY - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  Key->ScanCode     = 0;
  if (c == '\n')
    c = '\r';
  Key->UnicodeChar  = c;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UnixSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  OUT EFI_INPUT_KEY                       *Key
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description
  Key   - TODO: add argument description

Returns:

  EFI_DEVICE_ERROR - TODO: Add description for return value
  EFI_NOT_READY - TODO: Add description for return value

--*/
{
  EFI_STATUS                      Status;
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;
  char c;

  Private = UNIX_SIMPLE_TEXT_IN_PRIVATE_DATA_FROM_THIS (This);

  Status  = UnixSimpleTextInCheckKey (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Private->UnixThunk->Read (0, &c, 1) != 1)
    return EFI_NOT_READY;
  Status = UnixConvertInputRecordToEfiKey (c, Key);

  return Status;
}

VOID
EFIAPI
UnixSimpleTextInWaitForKey (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;
  EFI_STATUS                      Status;

  Private = (UNIX_SIMPLE_TEXT_PRIVATE_DATA *) Context;
  Status  = UnixSimpleTextInCheckKey (Private);
  if (!EFI_ERROR (Status)) {
    gBS->SignalEvent (Event);
  }
}

EFI_STATUS
UnixSimpleTextInCheckKey (
  UNIX_SIMPLE_TEXT_PRIVATE_DATA   *Private
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
  struct pollfd pfd;

  pfd.fd = 0;
  pfd.events = POLLIN;
  if (Private->UnixThunk->Poll (&pfd, 1, 0) <= 0) {
    return EFI_NOT_READY;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
UnixSimpleTextInAttachToWindow (
  IN  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private
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

  Private->SimpleTextIn.Reset         = UnixSimpleTextInReset;
  Private->SimpleTextIn.ReadKeyStroke = UnixSimpleTextInReadKeyStroke;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UnixSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextIn.WaitForKey
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
