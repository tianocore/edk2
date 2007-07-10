/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Ps2KbdTextIn.c

Abstract:

  PS/2 Keyboard  driver
  Routines that support SIMPLE_TEXT_IN protocol

Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "Ps2Keyboard.h"

//
// function declarations
//
EFI_STATUS
EFIAPI
KeyboardEfiReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  );

EFI_STATUS
EFIAPI
KeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  );

VOID
EFIAPI
KeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

EFI_STATUS
KeyboardCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
KeyboardEfiReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
/*++

Routine Description:

  Implement SIMPLE_TEXT_IN.Reset()
  Perform 8042 controller and keyboard initialization

Arguments:

Returns:

--*/
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    ExtendedVerification - add argument and description to function comment
// GC_TODO:    EFI_DEVICE_ERROR - add return value to function comment
// GC_TODO:    EFI_DEVICE_ERROR - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS              Status;
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;
  EFI_TPL                 OldTpl;

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);
  if (ConsoleIn->KeyboardErr) {
    return EFI_DEVICE_ERROR;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET,
    ConsoleIn->DevicePath
    );

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Call InitKeyboard to initialize the keyboard
  //
  Status = InitKeyboard (ConsoleIn, ExtendedVerification);
  if (EFI_ERROR (Status)) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);
    return EFI_DEVICE_ERROR;
  }
  //
  // Clear the status of ConsoleIn.Key
  //
  ConsoleIn->Key.ScanCode     = SCAN_NULL;
  ConsoleIn->Key.UnicodeChar  = 0x0000;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  //
  // Report the status If a stuck key was detected
  //
  if (KeyReadStatusRegister (ConsoleIn) & 0x01) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_EC_STUCK_KEY,
      ConsoleIn->DevicePath
      );
  }
  //
  // Report the status If keyboard is locked
  //
  if (!(KeyReadStatusRegister (ConsoleIn) & 0x10)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_EC_LOCKED,
      ConsoleIn->DevicePath
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
KeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
/*++

Routine Description:

  Implement SIMPLE_TEXT_IN.ReadKeyStroke().
  Retrieve key values for driver user.

Arguments:

Returns:

--*/
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    Key - add argument and description to function comment
// GC_TODO:    EFI_DEVICE_ERROR - add return value to function comment
// GC_TODO:    EFI_NOT_READY - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS              Status;
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;
  EFI_TPL                 OldTpl;

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (ConsoleIn->KeyboardErr) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);

    return EFI_DEVICE_ERROR;
  }
  //
  // If there's no key, just return
  //
  Status = KeyboardCheckForKey (This);
  if (EFI_ERROR (Status)) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_READY;
  }

  Key->ScanCode               = ConsoleIn->Key.ScanCode;
  Key->UnicodeChar            = ConsoleIn->Key.UnicodeChar;

  ConsoleIn->Key.ScanCode     = SCAN_NULL;
  ConsoleIn->Key.UnicodeChar  = 0x0000;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

VOID
EFIAPI
KeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  Event notification function for SIMPLE_TEXT_IN.WaitForKey event
  Signal the event if there is key available

Arguments:

Returns:

--*/
// GC_TODO:    Event - add argument and description to function comment
// GC_TODO:    Context - add argument and description to function comment
{
  EFI_TPL                 OldTpl;
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (Context);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (ConsoleIn->KeyboardErr) {
    //
    // Leave critical section and return
    //
    gBS->RestoreTPL (OldTpl);
    return ;
  }
  //
  // Someone is waiting on the keyboard event, if there's
  // a key pending, signal the event
  //
  if (!EFI_ERROR (KeyboardCheckForKey (Context))) {
    gBS->SignalEvent (Event);
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return ;
}

EFI_STATUS
KeyboardCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This  - GC_TODO: add argument description

Returns:

  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);

  //
  // If ready to read next key, check it
  //
  if (ConsoleIn->Key.ScanCode == SCAN_NULL && ConsoleIn->Key.UnicodeChar == 0x00) {
    return KeyGetchar (ConsoleIn);
  }

  return EFI_SUCCESS;
}
