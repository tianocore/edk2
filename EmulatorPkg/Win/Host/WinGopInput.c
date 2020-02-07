/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  WinGopInput.c

Abstract:

  This file produces the Simple Text In for an Gop window.

  This stuff is linked at the hip to the Window, since the window
  processing is done in a thread kicked off in WinNtGopImplementation.c

  Since the window information is processed in an other thread we need
  a keyboard Queue to pass data about. The Simple Text In code just
  takes data off the Queue. The WinProc message loop takes keyboard input
  and places it in the Queue.


**/


#include "WinGop.h"


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateCreateQ (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN GOP_QUEUE_FIXED           *Queue
  )
{
  InitializeCriticalSection (&Queue->Cs);
  Queue->Front = 0;
  Queue->Rear  = 0;
  return EFI_SUCCESS;
}


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateDestroyQ (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN GOP_QUEUE_FIXED           *Queue
  )
{
  Queue->Front = 0;
  Queue->Rear  = 0;
  DeleteCriticalSection (&Queue->Cs);
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
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN GOP_QUEUE_FIXED           *Queue,
  IN EFI_KEY_DATA              *KeyData
  )
{
  EnterCriticalSection (&Queue->Cs);

  if ((Queue->Rear + 1) % MAX_Q == Queue->Front) {
    LeaveCriticalSection (&Queue->Cs);
    return EFI_NOT_READY;
  }

  CopyMem (&Queue->Q[Queue->Rear], KeyData, sizeof (EFI_KEY_DATA));
  Queue->Rear           = (Queue->Rear + 1) % MAX_Q;

  LeaveCriticalSection (&Queue->Cs);
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
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN  GOP_QUEUE_FIXED          *Queue,
  OUT EFI_KEY_DATA             *Key
  )
{
  EnterCriticalSection (&Queue->Cs);

  if (Queue->Front == Queue->Rear) {
    LeaveCriticalSection (&Queue->Cs);
    return EFI_NOT_READY;
  }

  CopyMem (Key, &Queue->Q[Queue->Front], sizeof (EFI_KEY_DATA));
  Queue->Front  = (Queue->Front + 1) % MAX_Q;

  if (Key->Key.ScanCode == SCAN_NULL && Key->Key.UnicodeChar == CHAR_NULL) {
    if (!Private->IsPartialKeySupport) {
      //
      // If partial keystrok is not enabled, don't return the partial keystroke.
      //
      LeaveCriticalSection (&Queue->Cs);
      ZeroMem (Key, sizeof (EFI_KEY_DATA));
      return EFI_NOT_READY;
    }
  }
  LeaveCriticalSection (&Queue->Cs);
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
  IN  GOP_QUEUE_FIXED     *Queue
  )
{
  if (Queue->Front == Queue->Rear) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the key state.

  @param  Private               The GOP_PRIVATE_DATA instance.
  @param  KeyState              A pointer to receive the key state information.
**/
VOID
InitializeKeyState (
  IN  GRAPHICS_PRIVATE_DATA    *Private,
  IN  EFI_KEY_STATE            *KeyState
  )
{
  KeyState->KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyState->KeyToggleState = EFI_TOGGLE_STATE_VALID;

  //
  // Record Key shift state and toggle state
  //
  if (Private->LeftCtrl) {
    KeyState->KeyShiftState  |= EFI_LEFT_CONTROL_PRESSED;
  }
  if (Private->RightCtrl) {
    KeyState->KeyShiftState  |= EFI_RIGHT_CONTROL_PRESSED;
  }
  if (Private->LeftAlt) {
    KeyState->KeyShiftState  |= EFI_LEFT_ALT_PRESSED;
  }
  if (Private->RightAlt) {
    KeyState->KeyShiftState  |= EFI_RIGHT_ALT_PRESSED;
  }
  if (Private->LeftShift) {
    KeyState->KeyShiftState  |= EFI_LEFT_SHIFT_PRESSED;
  }
  if (Private->RightShift) {
    KeyState->KeyShiftState  |= EFI_RIGHT_SHIFT_PRESSED;
  }
  if (Private->LeftLogo) {
    KeyState->KeyShiftState  |= EFI_LEFT_LOGO_PRESSED;
  }
  if (Private->RightLogo) {
    KeyState->KeyShiftState  |= EFI_RIGHT_LOGO_PRESSED;
  }
  if (Private->Menu) {
    KeyState->KeyShiftState  |= EFI_MENU_KEY_PRESSED;
  }
  if (Private->SysReq) {
    KeyState->KeyShiftState  |= EFI_SYS_REQ_PRESSED;
  }
  if (Private->CapsLock) {
    KeyState->KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }
  if (Private->NumLock) {
    KeyState->KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }
  if (Private->ScrollLock) {
    KeyState->KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }
  if (Private->IsPartialKeySupport) {
    KeyState->KeyToggleState |= EFI_KEY_STATE_EXPOSED;
  }
}

/**
  TODO: Add function description

  @param  Private               TODO: add argument description
  @param  Key                   TODO: add argument description

  @retval EFI_NOT_READY         TODO: Add description for return value
  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateAddKey (
  IN  GRAPHICS_PRIVATE_DATA  *Private,
  IN  EFI_INPUT_KEY          Key
  )
{
  EFI_KEY_DATA            KeyData;

  KeyData.Key = Key;
  InitializeKeyState (Private, &KeyData.KeyState);

  //
  // Convert Ctrl+[1-26] to Ctrl+[A-Z]
  //
  if ((Private->LeftCtrl || Private->RightCtrl) &&
      (KeyData.Key.UnicodeChar >= 1) && (KeyData.Key.UnicodeChar <= 26)
     ) {
    if ((Private->LeftShift || Private->RightShift) == Private->CapsLock) {
      KeyData.Key.UnicodeChar = (CHAR16)(KeyData.Key.UnicodeChar + L'a' - 1);
    } else {
      KeyData.Key.UnicodeChar = (CHAR16)(KeyData.Key.UnicodeChar + L'A' - 1);
    }
  }

  //
  // Unmask the Shift bit for printable char
  //
  if (((KeyData.Key.UnicodeChar >= L'a') && (KeyData.Key.UnicodeChar <= L'z')) ||
      ((KeyData.Key.UnicodeChar >= L'A') && (KeyData.Key.UnicodeChar <= L'Z'))
     ) {
    KeyData.KeyState.KeyShiftState &= ~(EFI_LEFT_SHIFT_PRESSED | EFI_RIGHT_SHIFT_PRESSED);
  }

  GopPrivateAddQ (Private, &Private->QueueForRead, &KeyData);
  if (Private->MakeRegisterdKeyCallback != NULL) {
    Private->MakeRegisterdKeyCallback (Private->RegisterdKeyCallbackContext, &KeyData);
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
WinNtWndCheckKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo
  )
{
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  return GopPrivateCheckQ (&Private->QueueForRead);

}
EFI_STATUS
EFIAPI
WinNtWndGetKey (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_KEY_DATA                  *KeyData
  )
/*++

  Routine Description:
    Reads the next keystroke from the input device. The WaitForKey Event can
    be used to test for existence of a keystroke via WaitForEvent () call.

  Arguments:
    Private    - The private structure of WinNt Gop device.
    KeyData    - A pointer to a buffer that is filled in with the keystroke
                 state data for the key that was pressed.

  Returns:
    EFI_SUCCESS           - The keystroke information was returned.
    EFI_NOT_READY         - There was no keystroke data available.
    EFI_DEVICE_ERROR      - The keystroke information was not returned due to
                            hardware errors.
    EFI_INVALID_PARAMETER - KeyData is NULL.

--*/
{
  EFI_STATUS                      Status;
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  ZeroMem (&KeyData->Key, sizeof (KeyData->Key));
  InitializeKeyState (Private, &KeyData->KeyState);

  Status  = GopPrivateCheckQ (&Private->QueueForRead);
  if (!EFI_ERROR (Status)) {
    //
    // If a Key press exists try and read it.
    //
    Status = GopPrivateDeleteQ (Private, &Private->QueueForRead, KeyData);
    if (!EFI_ERROR (Status)) {
      //
      // If partial keystroke is not enabled, check whether it is value key. If not return
      // EFI_NOT_READY.
      //
      if (!Private->IsPartialKeySupport) {
        if (KeyData->Key.ScanCode == SCAN_NULL && KeyData->Key.UnicodeChar == CHAR_NULL) {
          Status = EFI_NOT_READY;
        }
      }
    }
  }

  return Status;

}

EFI_STATUS
EFIAPI
WinNtWndKeySetState (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL   *GraphicsIo,
  IN EFI_KEY_TOGGLE_STATE           *KeyToggleState
  )
{
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);
  Private->ScrollLock = FALSE;
  Private->NumLock = FALSE;
  Private->CapsLock = FALSE;
  Private->IsPartialKeySupport = FALSE;

  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    Private->ScrollLock = TRUE;
  }
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    Private->NumLock = TRUE;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    Private->CapsLock = TRUE;
  }
  if ((*KeyToggleState & EFI_KEY_STATE_EXPOSED) == EFI_KEY_STATE_EXPOSED) {
    Private->IsPartialKeySupport = TRUE;
  }
  Private->KeyState.KeyToggleState = *KeyToggleState;
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
WinNtWndRegisterKeyNotify (
  IN EMU_GRAPHICS_WINDOW_PROTOCOL                        *GraphicsIo,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    MakeCallBack,
  IN EMU_GRAPHICS_WINDOW_REGISTER_KEY_NOTIFY_CALLBACK    BreakCallBack,
  IN VOID                                                *Context
  )
{
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  Private->MakeRegisterdKeyCallback    = MakeCallBack;
  Private->BreakRegisterdKeyCallback   = BreakCallBack;
  Private->RegisterdKeyCallbackContext = Context;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtWndCheckPointer (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL *GraphicsIo
  )
{
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  if (!Private->PointerStateChanged) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
WinNtWndGetPointerState (
  IN  EMU_GRAPHICS_WINDOW_PROTOCOL  *GraphicsIo,
  IN  EFI_SIMPLE_POINTER_STATE      *State
  )
{
  GRAPHICS_PRIVATE_DATA           *Private;

  Private = GRAPHICS_PRIVATE_DATA_FROM_THIS (GraphicsIo);

  if (!Private->PointerStateChanged) {
    return EFI_NOT_READY;
  }

  State->RelativeMovementX = Private->PointerState.RelativeMovementX;
  State->RelativeMovementY = Private->PointerState.RelativeMovementY;
  State->RelativeMovementZ = Private->PointerState.RelativeMovementZ;
  State->LeftButton        = Private->PointerState.LeftButton;
  State->RightButton       = Private->PointerState.RightButton;

  Private->PointerState.RelativeMovementX = 0;
  Private->PointerState.RelativeMovementY = 0;
  Private->PointerState.RelativeMovementZ = 0;

  Private->PointerStateChanged = FALSE;

  return EFI_SUCCESS;
}
