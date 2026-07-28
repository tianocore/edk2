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

// -----------------------------------------------------------------------------
// Utility functions
// -----------------------------------------------------------------------------

/**
  Check whether the EFI key buffer is empty.

  @param Queue     Pointer to instance of EFI_KEY_QUEUE.

  @retval TRUE    The EFI key buffer is empty.
  @retval FALSE   The EFI key buffer isn't empty.
**/
STATIC
BOOLEAN
IsEfikeyBufEmpty (
  IN  EFI_KEY_QUEUE  *Queue
  )
{
  return (BOOLEAN)(Queue->Head == Queue->Tail);
}

/**
  Read (but do not remove) one key data entry from the EFI key buffer.

  @param Queue     Pointer to instance of EFI_KEY_QUEUE.
  @param KeyData   Receive the key data.

  @retval EFI_SUCCESS   The key data is popped successfully.
  @retval EFI_NOT_READY There is no key data available.
**/
STATIC
EFI_STATUS
PeekEfikeyBufHead (
  IN  EFI_KEY_QUEUE  *Queue,
  OUT EFI_KEY_DATA   *KeyData OPTIONAL
  )
{
  if (IsEfikeyBufEmpty (Queue)) {
    return EFI_NOT_READY;
  }

  if (KeyData != NULL) {
    CopyMem (KeyData, &Queue->Buffer[Queue->Head], sizeof (EFI_KEY_DATA));
  }

  return EFI_SUCCESS;
}

/**
  Read & remove one key data from the EFI key buffer.

  @param Queue     Pointer to instance of EFI_KEY_QUEUE.
  @param KeyData   Receive the key data.

  @retval EFI_SUCCESS   The key data is popped successfully.
  @retval EFI_NOT_READY There is no key data available.
**/
STATIC
EFI_STATUS
PopEfikeyBufHead (
  IN  EFI_KEY_QUEUE  *Queue,
  OUT EFI_KEY_DATA   *KeyData OPTIONAL
  )
{
  if (IsEfikeyBufEmpty (Queue)) {
    return EFI_NOT_READY;
  }

  //
  // Retrieve and remove the values
  //
  if (KeyData != NULL) {
    CopyMem (KeyData, &Queue->Buffer[Queue->Head], sizeof (EFI_KEY_DATA));
  }

  ZeroMem (&Queue->Buffer[Queue->Head], sizeof (EFI_KEY_DATA));
  Queue->Head = (Queue->Head + 1) % KEYBOARD_EFI_KEY_MAX_COUNT;
  return EFI_SUCCESS;
}

/**
  Push one key data to the EFI key buffer.

  @param Queue     Pointer to instance of EFI_KEY_QUEUE.
  @param KeyData   The key data to push.
**/
STATIC
VOID
PushEfikeyBufTail (
  IN  EFI_KEY_QUEUE  *Queue,
  IN  EFI_KEY_DATA   *KeyData
  )
{
  if ((Queue->Tail + 1) % KEYBOARD_EFI_KEY_MAX_COUNT == Queue->Head) {
    //
    // If Queue is full, pop the one from head.
    //
    PopEfikeyBufHead (Queue, NULL);
  }

  CopyMem (&Queue->Buffer[Queue->Tail], KeyData, sizeof (EFI_KEY_DATA));
  Queue->Tail = (Queue->Tail + 1) % KEYBOARD_EFI_KEY_MAX_COUNT;
}

/**
  Clear the EFI key queue.

  @param Queue   Pointer to instance of EFI_KEY_QUEUE.
 */
STATIC
VOID
ClearEfikeyBuf (
  IN  EFI_KEY_QUEUE  *Queue
  )
{
  Queue->Head = 0;
  Queue->Tail = 0;
  ZeroMem (Queue->Buffer, sizeof (Queue->Buffer));
}

// -----------------------------------------------------------------------------
// End utility functions
// -----------------------------------------------------------------------------

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
  OUT EFI_KEY_DATA         *KeyData
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

  EFI_INPUT_KEY  *Key = &KeyData->Key;

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
        Key->UnicodeChar = MapShift[Code];
      } else {
        Key->UnicodeChar = Map[Code];
      }

      // If this key cannot be mapped to either a scancode or a printable character, return
      if (Key->UnicodeChar == CHAR_NULL) {
        return;
      }

      // If we are processing a shiftable character, clear the explicit shift state (if any)
      // because it is now reflected in the character representation itself.
      // "if a class of printable characters that are normally adjusted by shift modifiers
      // (e.g. Shift Key + “f” key) would be presented solely as a KeyData.Key.UnicodeChar
      // without the associated shift state."
      if (MapShift[Code] != Map[Code]) {
        KeyData->KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
      }

      if (Dev->CapsLock) {
        // On CapsLock, invert capitalization of alphabetic characters
        // NB1: this must run in series with Shift handling (CapsLock XORs with Shift)
        // NB2: this must run after Ctrl handling (CapsLock must not affect control codes)
        if (((Key->UnicodeChar >= 'a') && (Key->UnicodeChar <= 'z')) ||
            ((Key->UnicodeChar >= 'A') && (Key->UnicodeChar <= 'Z')))
        {
          Key->UnicodeChar ^= 0x20;
        }
      }

      break;
  }
}

// -----------------------------------------------------------------------------
// Function populating modifier and toggle state of an UEFI key event
STATIC
VOID
VirtioKeyboardInitializeKeyState (
  IN  VIRTIO_INPUT_DEV  *Dev,
  OUT EFI_KEY_STATE     *KeyState
  )
{
  KeyState->KeyShiftState = (
                             (Dev->KeyActive[KEY_RIGHTSHIFT] ? EFI_RIGHT_SHIFT_PRESSED : 0) |
                             (Dev->KeyActive[KEY_LEFTSHIFT] ? EFI_LEFT_SHIFT_PRESSED : 0) |
                             (Dev->KeyActive[KEY_RIGHTCTRL] ? EFI_RIGHT_CONTROL_PRESSED : 0) |
                             (Dev->KeyActive[KEY_LEFTCTRL] ? EFI_LEFT_CONTROL_PRESSED : 0) |
                             (Dev->KeyActive[KEY_RIGHTALT] ? EFI_RIGHT_ALT_PRESSED : 0) |
                             (Dev->KeyActive[KEY_LEFTALT] ? EFI_LEFT_ALT_PRESSED : 0) |
                             (Dev->KeyActive[KEY_RIGHTMETA] ? EFI_RIGHT_LOGO_PRESSED : 0) |
                             (Dev->KeyActive[KEY_LEFTMETA] ? EFI_LEFT_LOGO_PRESSED : 0) |
                             (Dev->KeyActive[KEY_COMPOSE] ? EFI_MENU_KEY_PRESSED : 0) |
                             (Dev->KeyActive[KEY_SYSRQ] ? EFI_SYS_REQ_PRESSED : 0) |
                             EFI_SHIFT_STATE_VALID
                             );

  KeyState->KeyToggleState = (
                              (Dev->NumLock ? EFI_NUM_LOCK_ACTIVE : 0) |
                              (Dev->CapsLock ? EFI_CAPS_LOCK_ACTIVE : 0) |
                              (Dev->ScrollLock ? EFI_SCROLL_LOCK_ACTIVE : 0) |
                              (Dev->SupportPartialKeys ? EFI_KEY_STATE_EXPOSED : 0) |
                              EFI_TOGGLE_STATE_VALID
                              );
}

// -----------------------------------------------------------------------------
// Function processing a single VirtIO keyboard event into a complete UEFI event
STATIC
EFI_STATUS
VirtioKeyboardProcessEvent (
  IN  VIRTIO_INPUT_DEV  *Dev,
  IN  UINT16            KeyCode,
  OUT EFI_KEY_DATA      *KeyData
  )
{
  //
  // Initialize the key data structure with current keyboard and toggle state
  //
  VirtioKeyboardInitializeKeyState (Dev, &KeyData->KeyState);

  //
  // Translate the virtio scancode into EFI scancode and Unicode char
  //
  VirtioKeyboardConvertKeyCode (Dev, KeyCode, KeyData);

  //
  // Keys with no Unicode representation and no EFI scancode are not valid,
  // unless we were requested to process partial keys
  //
  if ((KeyData->Key.UnicodeChar == CHAR_NULL) && (KeyData->Key.ScanCode == SCAN_NULL)) {
    if (!Dev->SupportPartialKeys) {
      return EFI_NOT_READY;
    }
  }

  return EFI_SUCCESS;
}

// -----------------------------------------------------------------------------
// Function handling VirtIO keyboard events
VOID
VirtioKeyboardHandleEvent (
  IN OUT VIRTIO_INPUT_DEV  *Dev,
  IN VIRTIO_INPUT_EVENT    *Event
  )
{
  EFI_STATUS    Status;
  EFI_KEY_DATA  KeyData;

  if (Event->Value != KEY_RELEASED) {
    // Key pressed event received
    Dev->KeyActive[(UINT8)Event->Code] = TRUE;

    //
    // Update toggle state
    // This must happen before EFI_KEY_DATA is populated, since the toggle state
    // is used both to initialize ->KeyState and to modify key code behavior.
    // NB: only explicitly handle KEY_PRESSED to ignore autorepeat events
    //
    if (Event->Value == KEY_PRESSED) {
      switch (Event->Code) {
        case KEY_NUMLOCK:
          Dev->NumLock = (BOOLEAN) !Dev->NumLock;
          break;
        case KEY_CAPSLOCK:
          Dev->CapsLock = (BOOLEAN) !Dev->CapsLock;
          break;
        case KEY_SCROLLLOCK:
          Dev->ScrollLock = (BOOLEAN) !Dev->ScrollLock;
          break;
      }
    }

    // Evaluate key
    Status = VirtioKeyboardProcessEvent (Dev, Event->Code, &KeyData);
    if (EFI_ERROR (Status)) {
      return;
    }

    // Submit this key
    PushEfikeyBufTail (&Dev->KeyQueue, &KeyData);
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

  ZeroMem (Dev->KeyActive, sizeof (Dev->KeyActive));
  ClearEfikeyBuf (&Dev->KeyQueue);

  Dev->NumLock            = FALSE;
  Dev->CapsLock           = FALSE;
  Dev->ScrollLock         = FALSE;
  Dev->SupportPartialKeys = FALSE;

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
  EFI_KEY_DATA      KeyData;
  EFI_STATUS        Status;

  if (Key == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // ReadKeyStroke of SIMPLE_TEXT_INPUT must omit partial keystrokes
  //
  while (TRUE) {
    Status = PopEfikeyBufHead (&Dev->KeyQueue, &KeyData);
    if (EFI_ERROR (Status)) {
      break;
    }

    if ((KeyData.Key.ScanCode == SCAN_NULL) && (KeyData.Key.UnicodeChar == CHAR_NULL)) {
      continue;
    }

    // Since ReadKeyStroke does not allow to convey modifier state, we have to
    // fold Ctrl into the printable character to produce a C0 control code
    // (i.e. apply the caret notation).
    // NB1: this is not actually in the SIMPLE_TEXT_INPUT(_EX) spec; we do it
    //     for compatibility with other keyboard drivers and TerminalDxe.
    // NB2: We only apply a subset of caret notation limited to alphabetic
    //      characters, as these are idempotent wrt. Shift and CapsLock
    //      (which are already applied at this point).
    if (KeyData.KeyState.KeyShiftState & (EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) {
      if (((KeyData.Key.UnicodeChar >= 'A') && (KeyData.Key.UnicodeChar <= 'Z')) ||
          ((KeyData.Key.UnicodeChar >= 'a') && (KeyData.Key.UnicodeChar <= 'z')))
      {
        KeyData.Key.UnicodeChar &= 0x1F;
      }
    }

    *Key = KeyData.Key;
    break;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
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
  EFI_TPL           OldTpl;
  EFI_KEY_DATA      KeyData;
  EFI_STATUS        Status;

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

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // If there is a new key ready - send signal
  // WaitForKey(Ex) must omit partial keystrokes
  //
  while (TRUE) {
    Status = PeekEfikeyBufHead (&Dev->KeyQueue, &KeyData);
    if (EFI_ERROR (Status)) {
      break;
    }

    if ((KeyData.Key.ScanCode == SCAN_NULL) && (KeyData.Key.UnicodeChar == CHAR_NULL)) {
      (void)PopEfikeyBufHead (&Dev->KeyQueue, NULL);
      continue;
    }

    gBS->SignalEvent (Event);
    break;
  }

  gBS->RestoreTPL (OldTpl);
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
  EFI_TPL           OldTpl;
  EFI_STATUS        Status;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = PopEfikeyBufHead (&Dev->KeyQueue, KeyData);
  // "There was no keystroke data available. Current KeyData.KeyState values are exposed."
  if (Status == EFI_NOT_READY) {
    ZeroMem (&KeyData->Key, sizeof (KeyData->Key));
    VirtioKeyboardInitializeKeyState (Dev, &KeyData->KeyState);
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
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
  VIRTIO_INPUT_DEV  *Dev;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_INPUT_EX_FROM_THIS (This);

  if (!(*KeyToggleState & EFI_TOGGLE_STATE_VALID)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Update effective toggle states
  // TODO: updating (virtio) hardware LEDs is not implemented
  //       (needs EV_LED write on status virtqueue; this driver does not initialize it)
  //
  Dev->NumLock            = (BOOLEAN)((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) != 0);
  Dev->CapsLock           = (BOOLEAN)((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) != 0);
  Dev->ScrollLock         = (BOOLEAN)((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) != 0);
  Dev->SupportPartialKeys = (BOOLEAN)((*KeyToggleState & EFI_KEY_STATE_EXPOSED) != 0);

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
