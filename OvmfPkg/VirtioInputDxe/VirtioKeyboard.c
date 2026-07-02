/** @file

  EFI_SIMPLE_TEXT_INPUT_PROTOCOL and EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL
  implementation for virtio keyboard.

  Copyright (C) 2026, Advanced Micro Devices, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include <IndustryStandard/VirtioInput.h>

#include "VirtioInput.h"
#include "VirtioKeyCodes.h"

BOOLEAN
VirtioKeyboardProbe (
  IN VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;
  UINT8       Size;
  UINT8       Bitmap;
  UINTN       Index;

  Status = VirtioInputConfigQuerySize (Dev, VirtioInputCfgEvBits, EV_KEY, &Size);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  // Keyboard keys are 0 ~ 255, so if any of them is supported, we have a keyboard
  Size = MIN (Size, (MAX_KEYBOARD_CODE / 8) + 1);
  for (Index = 0; Index < Size; Index++) {
    Status = Dev->VirtIo->ReadDevice (Dev->VirtIo, OFFSET_OF_VINPUT (Data) + Index, 1, 1, &Bitmap);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    if (Bitmap) {
      return TRUE;
    }
  }

  return FALSE;
}

// -----------------------------------------------------------------------------
// Function converting VirtIO key codes to UEFI key codes
STATIC
VOID
VirtioKeyboardConvertKeyCode (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN UINT16                Code,
  OUT EFI_INPUT_KEY        *Key
  )
{
  // Key mapping in between Linux and UEFI
  // https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
  // https://dox.ipxe.org/SimpleTextIn_8h_source.html#l00048
  // https://uefi.org/specs/UEFI/2.10/Apx_B_Console.html

  static const UINT16  Map[] = {
    [KEY_1]             = '1',  '2',  '3', '4', '5', '6', '7', '8', '9', '0',
    [KEY_MINUS]         = '-',  '=',
    [KEY_Q]             = 'q',  'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    [KEY_LEFTBRACE]     = '[',  ']',
    [KEY_A]             = 'a',  's',  'd', 'f', 'g', 'h', 'j', 'k', 'l',
    [KEY_SEMICOLON]     = ';',  '\'', '`',
    [KEY_BACKSLASH]     = '\\',
    [KEY_Z]             = 'z',  'x',  'c', 'v', 'b', 'n', 'm',
    [KEY_COMMA]         = ',',  '.',  '/',
    [KEY_SPACE]         = ' ',
    [MAX_KEYBOARD_CODE] = 0x00
  };

  static const UINT16  MapShift[] = {
    [KEY_1]             = '!', '@',  '#', '$', '%', '^', '&', '*', '(', ')',
    [KEY_MINUS]         = '_', '+',
    [KEY_Q]             = 'Q', 'W',  'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    [KEY_LEFTBRACE]     = '{', '}',
    [KEY_A]             = 'A', 'S',  'D', 'F', 'G', 'H', 'J', 'K', 'L',
    [KEY_SEMICOLON]     = ':', '\"', '~',
    [KEY_BACKSLASH]     = '|',
    [KEY_Z]             = 'Z', 'X',  'C', 'V', 'B', 'N', 'M',
    [KEY_COMMA]         = '<', '>',  '?',
    [KEY_SPACE]         = ' ',
    [MAX_KEYBOARD_CODE] = 0x00
  };

  // Set default readings
  Key->ScanCode    = SCAN_NULL;
  Key->UnicodeChar = CHAR_NULL;

  // Check if key code is not out of the keyboard mapping boundaries
  if (Code >= MAX_KEYBOARD_CODE) {
    DEBUG ((DEBUG_INFO, "%a: Key code out of range \n", __func__));
    return;
  }

  // Handle F1 - F10 keys
  if ((Code >= KEY_F1) && (Code <= KEY_F10)) {
    Key->ScanCode = SCAN_F1 + (Code - KEY_F1);
    return;
  }

  switch (Code) {
    case KEY_PAGEUP:
      Key->ScanCode = SCAN_PAGE_UP;
      break;

    case KEY_PAGEDOWN:
      Key->ScanCode = SCAN_PAGE_DOWN;
      break;

    case KEY_HOME:
      Key->ScanCode = SCAN_HOME;
      break;

    case KEY_END:
      Key->ScanCode = SCAN_END;
      break;

    case KEY_DELETE:
      Key->ScanCode = SCAN_DELETE;
      break;

    case KEY_INSERT:
      Key->ScanCode = SCAN_INSERT;
      break;

    case KEY_UP:
      Key->ScanCode = SCAN_UP;
      break;

    case KEY_LEFT:
      Key->ScanCode = SCAN_LEFT;
      break;

    case KEY_RIGHT:
      Key->ScanCode = SCAN_RIGHT;
      break;

    case KEY_DOWN:
      Key->ScanCode = SCAN_DOWN;
      break;

    case KEY_BACKSPACE:
      Key->UnicodeChar = CHAR_BACKSPACE;
      break;

    case KEY_TAB:
      Key->UnicodeChar = CHAR_TAB;
      break;

    case KEY_ENTER:
      Key->UnicodeChar = CHAR_CARRIAGE_RETURN;
      break;

    case KEY_ESC:
      Key->ScanCode = SCAN_ESC;
      break;

    default:
      if (Dev->KeyActive[KEY_LEFTSHIFT] || Dev->KeyActive[KEY_RIGHTSHIFT]) {
        Key->ScanCode    = MapShift[Code];
        Key->UnicodeChar = MapShift[Code];
      } else {
        Key->ScanCode    = Map[Code];
        Key->UnicodeChar = Map[Code];
      }

      if (Dev->KeyActive[KEY_LEFTCTRL] || Dev->KeyActive[KEY_RIGHTCTRL]) {
        // Convert Ctrl+[a-z] and Ctrl+[A-Z] into [1-26] ASCII table entries
        Key->UnicodeChar &= 0x1F;
      }

      break;
  }
}

// -----------------------------------------------------------------------------
// Function handling VirtIO keyboard events
VOID
VirtioKeyboardHandleEvent (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN VIRTIO_INPUT_EVENT    *Event
  )
{
  if (Event->Value == KEY_PRESSED) {
    // Key pressed event received
    Dev->KeyActive[(UINT8)Event->Code] = TRUE;

    // Evaluate key
    VirtioKeyboardConvertKeyCode (Dev, Event->Code, &Dev->LastKey);

    // Flag that printable character is ready to be send
    Dev->KeyReady = TRUE;
  } else {
    // Key released event received
    Dev->KeyActive[(UINT8)Event->Code] = FALSE;
  }
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN BOOLEAN                         ExtendedVerification
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  Dev = VIRTIO_INPUT_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Dev->KeyReady            = FALSE;
  Dev->LastKey.ScanCode    = SCAN_NULL;
  Dev->LastKey.UnicodeChar = CHAR_NULL;
  ZeroMem (Dev->KeyActive, sizeof (Dev->KeyActive));

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_TPL           OldTpl;

  if (Key == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (Dev->KeyReady) {
    // Get last key from the buffer
    *Key = Dev->LastKey;

    // Mark key as consumed
    Dev->KeyReady = FALSE;

    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
  }

  gBS->RestoreTPL (OldTpl);

  return EFI_NOT_READY;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_PROTOCOL API
STATIC
VOID
EFIAPI
VirtioKeyboardWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VIRTIO_INPUT_DEV  *Dev = (VIRTIO_INPUT_DEV *)Context;

  //
  // Stall 1ms to give a chance to let other driver interrupt this routine
  // for their timer event.
  // e.g. UI setup or Shell, other drivers which are driven by timer event
  // will have a bad performance during this period,
  // e.g. usb keyboard driver.
  // Add a stall period can greatly increate other driver performance during
  // the WaitForKey is recursivly invoked. 1ms delay will make little impact
  // to the thunk keyboard driver, and user can not feel the delay at all when
  // input.
  gBS->Stall (1000);

  // Use TimerEvent callback function to check whether there's any key pressed
  VirtioInputTimer (NULL, Dev);

  // If there is a new key ready - send signal
  if (Dev->KeyReady) {
    gBS->SignalEvent (Event);
  }
}

/// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_STATUS        Status;

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  // Call the reset function from SIMPLE_TEXT_INPUT protocol
  Status = Dev->Txt.Reset (
                      &Dev->Txt,
                      ExtendedVerification
                      );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  VIRTIO_INPUT_DEV  *Dev;
  EFI_STATUS        Status;
  EFI_INPUT_KEY     Key;
  EFI_KEY_STATE     KeyState;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  // Get the last pressed key
  Status = Dev->Txt.ReadKeyStroke (&Dev->Txt, &Key);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  // Add key state informations
  KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  // Shift key modifier
  if (Dev->KeyActive[KEY_LEFTSHIFT]) {
    KeyState.KeyShiftState |= EFI_LEFT_SHIFT_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTSHIFT]) {
    KeyState.KeyShiftState |= EFI_RIGHT_SHIFT_PRESSED;
  }

  // Ctrl key modifier
  if (Dev->KeyActive[KEY_LEFTCTRL]) {
    KeyState.KeyShiftState |= EFI_LEFT_CONTROL_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTCTRL]) {
    KeyState.KeyShiftState |= EFI_RIGHT_CONTROL_PRESSED;
  }

  // ALt key modifier
  if (Dev->KeyActive[KEY_LEFTALT]) {
    KeyState.KeyShiftState |= EFI_LEFT_ALT_PRESSED;
  }

  if (Dev->KeyActive[KEY_RIGHTALT]) {
    KeyState.KeyShiftState |= EFI_RIGHT_ALT_PRESSED;
  }

  // Return value only when there is no failure
  KeyData->Key      = Key;
  KeyData->KeyState = KeyState;

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardSetStateEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegisteredData,
  IN EFI_KEY_DATA  *InputData
  )

{
  ASSERT (RegisteredData != NULL && InputData != NULL);

  if ((RegisteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegisteredData->Key.UnicodeChar != InputData->Key.UnicodeChar))
  {
    return FALSE;
  }

  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means
  // these state could be ignored.
  //
  if ((RegisteredData->KeyState.KeyShiftState != 0) &&
      (RegisteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState))
  {
    return FALSE;
  }

  if ((RegisteredData->KeyState.KeyToggleState != 0) &&
      (RegisteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState))
  {
    return FALSE;
  }

  return TRUE;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardRegisterKeyNotifyEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  EFI_STATUS                 Status;
  VIRTIO_INPUT_DEV           *Dev;
  EFI_TPL                    OldTpl;
  LIST_ENTRY                 *Link;
  VIRTIO_INPUT_IN_EX_NOTIFY  *NewNotify;
  VIRTIO_INPUT_IN_EX_NOTIFY  *CurrentNotify;

  if ((KeyData == NULL) ||
      (NotifyHandle == NULL) ||
      (KeyNotificationFunction == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  // Check if the (KeyData, NotificationFunction) pair is already registered.
  for (Link = Dev->KeyNotifyList.ForwardLink;
       Link != &Dev->KeyNotifyList;
       Link = Link->ForwardLink)
  {
    CurrentNotify = CR (
                      Link,
                      VIRTIO_INPUT_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTIO_INPUT_SIG
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        Status        = EFI_SUCCESS;
        goto Exit;
      }
    }
  }

  NewNotify = (VIRTIO_INPUT_IN_EX_NOTIFY *)AllocateZeroPool (sizeof (VIRTIO_INPUT_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  NewNotify->Signature         = VIRTIO_INPUT_SIG;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&Dev->KeyNotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle = NewNotify;
  Status        = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

// -----------------------------------------------------------------------------
// EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL API
STATIC
EFI_STATUS
EFIAPI
VirtioKeyboardUnregisterKeyNotifyEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  EFI_STATUS                 Status;
  VIRTIO_INPUT_DEV           *Dev;
  EFI_TPL                    OldTpl;
  LIST_ENTRY                 *Link;
  VIRTIO_INPUT_IN_EX_NOTIFY  *CurrentNotify;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((VIRTIO_INPUT_IN_EX_NOTIFY *)NotificationHandle)->Signature != VIRTIO_INPUT_SIG) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  for (Link = Dev->KeyNotifyList.ForwardLink;
       Link != &Dev->KeyNotifyList;
       Link = Link->ForwardLink)
  {
    CurrentNotify = CR (
                      Link,
                      VIRTIO_INPUT_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTIO_INPUT_SIG
                      );
    if (CurrentNotify == NotificationHandle) {
      RemoveEntryList (&CurrentNotify->NotifyEntry);

      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  // Notification has not been found
  Status = EFI_INVALID_PARAMETER;

Exit:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
VirtioKeyboardInit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  EFI_STATUS  Status;

  InitializeListHead (&Dev->KeyNotifyList);

  Dev->Txt.Reset         = VirtioKeyboardReset;
  Dev->Txt.ReadKeyStroke = VirtioKeyboardReadKeyStroke;

  Dev->TxtEx.Reset               = VirtioKeyboardResetEx;
  Dev->TxtEx.ReadKeyStrokeEx     = VirtioKeyboardReadKeyStrokeEx;
  Dev->TxtEx.SetState            = VirtioKeyboardSetStateEx;
  Dev->TxtEx.RegisterKeyNotify   = VirtioKeyboardRegisterKeyNotifyEx;
  Dev->TxtEx.UnregisterKeyNotify = VirtioKeyboardUnregisterKeyNotifyEx;

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioKeyboardWaitForKey,
                  Dev,
                  &(Dev->Txt.WaitForKey)
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Setup the WaitForKeyEx event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtioKeyboardWaitForKey,
                  Dev,
                  &(Dev->TxtEx.WaitForKeyEx)
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

VOID
VirtioKeyboardUninit (
  IN OUT VIRTIO_INPUT_DEV  *Dev
  )
{
  gBS->CloseEvent (Dev->Txt.WaitForKey);
  gBS->CloseEvent (Dev->TxtEx.WaitForKeyEx);
}
