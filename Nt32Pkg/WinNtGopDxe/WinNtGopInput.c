/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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


#include "WinNtGop.h"


/**
  TODO: Add function description

  @param  Private               TODO: add argument description

  @retval EFI_SUCCESS           TODO: Add description for return value

**/
EFI_STATUS
GopPrivateCreateQ (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  GOP_QUEUE_FIXED     *Queue
  )
{
  Private->WinNtThunk->InitializeCriticalSection (&Queue->Cs);
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
  IN  GOP_PRIVATE_DATA    *Private,
  IN  GOP_QUEUE_FIXED     *Queue
  )
{
  Queue->Front = 0;
  Queue->Rear  = 0;
  Private->WinNtThunk->DeleteCriticalSection (&Queue->Cs);
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
  IN GOP_PRIVATE_DATA     *Private,
  IN GOP_QUEUE_FIXED      *Queue,
  IN EFI_KEY_DATA         *KeyData
  )
{
  Private->WinNtThunk->EnterCriticalSection (&Queue->Cs);

  if ((Queue->Rear + 1) % MAX_Q == Queue->Front) {
    Private->WinNtThunk->LeaveCriticalSection (&Queue->Cs);
    return EFI_NOT_READY;
  }

  CopyMem (&Queue->Q[Queue->Rear], KeyData, sizeof (EFI_KEY_DATA));
  Queue->Rear           = (Queue->Rear + 1) % MAX_Q;

  Private->WinNtThunk->LeaveCriticalSection (&Queue->Cs);
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
  IN  GOP_QUEUE_FIXED     *Queue,
  OUT EFI_KEY_DATA        *Key
  )
{
  Private->WinNtThunk->EnterCriticalSection (&Queue->Cs);

  if (Queue->Front == Queue->Rear) {
    Private->WinNtThunk->LeaveCriticalSection (&Queue->Cs);
    return EFI_NOT_READY;
  }

  CopyMem (Key, &Queue->Q[Queue->Front], sizeof (EFI_KEY_DATA));
  Queue->Front  = (Queue->Front + 1) % MAX_Q;

  Private->WinNtThunk->LeaveCriticalSection (&Queue->Cs);
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

BOOLEAN
GopPrivateIsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )
/*++

Routine Description:

Arguments:

  RegsiteredData    - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was registered.
  InputData         - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was pressed.

Returns:
  TRUE              - Key be pressed matches a registered key.
  FLASE             - Match failed. 
  
--*/
{
  ASSERT (RegsiteredData != NULL && InputData != NULL);
  
  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar)) {
    return FALSE;  
  }      
  
  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means these state could be ignored.
  //
  if (RegsiteredData->KeyState.KeyShiftState != 0 &&
      RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState) {
    return FALSE;    
  }   
  if (RegsiteredData->KeyState.KeyToggleState != 0 &&
      RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState) {
    return FALSE;    
  }     
  
  return TRUE;

}


VOID
GopPrivateInvokeRegisteredFunction (
  IN GOP_PRIVATE_DATA                     *Private,
  IN EFI_KEY_DATA                         *KeyData
  )
/*++

Routine Description:

  This function updates the status light of NumLock, ScrollLock and CapsLock.

Arguments:

  Private       - The private structure of WinNt Gop device.
  KeyData       - A pointer to a buffer that is filled in with the keystroke 
                  state data for the key that was pressed.

Returns:

  EFI_SUCCESS   - The status light is updated successfully.

--*/  
{ 
  LIST_ENTRY                          *Link;
  WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY  *CurrentNotify;
  
  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );
    if (GopPrivateIsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      CurrentNotify->KeyNotificationFn (KeyData);
    }
  }    
}

VOID
WinNtGopSimpleTextInTimerHandler (
  IN EFI_EVENT          Event,
  IN GOP_PRIVATE_DATA   *Private
  )
{
  EFI_KEY_DATA          KeyData;

  while (GopPrivateDeleteQ (Private, &Private->QueueForNotify, &KeyData) == EFI_SUCCESS) {
    GopPrivateInvokeRegisteredFunction (Private, &KeyData);
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
  IN  GOP_PRIVATE_DATA    *Private,
  IN  EFI_INPUT_KEY       Key
  )
{
  EFI_KEY_DATA            KeyData;

  KeyData.Key = Key;

  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  KeyData.KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  //
  // Record Key shift state and toggle state
  //
  if (Private->LeftCtrl) {
    KeyData.KeyState.KeyShiftState  |= EFI_LEFT_CONTROL_PRESSED;
  }
  if (Private->RightCtrl) {
    KeyData.KeyState.KeyShiftState  |= EFI_RIGHT_CONTROL_PRESSED;
  }
  if (Private->LeftAlt) {
    KeyData.KeyState.KeyShiftState  |= EFI_LEFT_ALT_PRESSED;
  }
  if (Private->RightAlt) {
    KeyData.KeyState.KeyShiftState  |= EFI_RIGHT_ALT_PRESSED;
  }
  if (Private->LeftShift) {
    KeyData.KeyState.KeyShiftState  |= EFI_LEFT_SHIFT_PRESSED;
  }                                    
  if (Private->RightShift) {
    KeyData.KeyState.KeyShiftState  |= EFI_RIGHT_SHIFT_PRESSED;
  }
  if (Private->LeftLogo) {
    KeyData.KeyState.KeyShiftState  |= EFI_LEFT_LOGO_PRESSED;
  }
  if (Private->RightLogo) {
    KeyData.KeyState.KeyShiftState  |= EFI_RIGHT_LOGO_PRESSED;
  }
  if (Private->Menu) {
    KeyData.KeyState.KeyShiftState  |= EFI_MENU_KEY_PRESSED;
  }
  if (Private->SysReq) {
    KeyData.KeyState.KeyShiftState  |= EFI_SYS_REQ_PRESSED;
  }  
  if (Private->CapsLock) {
    KeyData.KeyState.KeyToggleState |= EFI_CAPS_LOCK_ACTIVE;
  }
  if (Private->NumLock) {
    KeyData.KeyState.KeyToggleState |= EFI_NUM_LOCK_ACTIVE;
  }
  if (Private->ScrollLock) {
    KeyData.KeyState.KeyToggleState |= EFI_SCROLL_LOCK_ACTIVE;
  }
  
  //
  // Convert Ctrl+[1-26] to Ctrl+[A-Z]
  //
  if ((Private->LeftCtrl || Private->RightCtrl) && 
      (KeyData.Key.UnicodeChar >= 1) && (KeyData.Key.UnicodeChar <= 26)
     ) {
    if ((Private->LeftShift || Private->RightShift) == Private->CapsLock) {
      KeyData.Key.UnicodeChar = KeyData.Key.UnicodeChar + L'a' - 1;
    } else {
      KeyData.Key.UnicodeChar = KeyData.Key.UnicodeChar + L'A' - 1;
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

  GopPrivateAddQ (Private, &Private->QueueForNotify, &KeyData);

  //
  // Convert Ctrl+[A-Z] to Ctrl+[1-26]
  //
  if (Private->LeftCtrl || Private->RightCtrl) {
    if ((KeyData.Key.UnicodeChar >= L'a') && (KeyData.Key.UnicodeChar <= L'z')) {
      KeyData.Key.UnicodeChar = KeyData.Key.UnicodeChar - L'a' + 1;
    } else if ((KeyData.Key.UnicodeChar >= L'A') && (KeyData.Key.UnicodeChar <= L'Z')) {
      KeyData.Key.UnicodeChar = KeyData.Key.UnicodeChar - L'A' + 1;
    }
  }
  GopPrivateAddQ (Private, &Private->QueueForRead, &KeyData);

  return EFI_SUCCESS;
}

EFI_STATUS
GopPrivateUpdateStatusLight (
  IN GOP_PRIVATE_DATA                     *Private
  )
/*++

Routine Description:

  This function updates the status light of NumLock, ScrollLock and CapsLock.

Arguments:

  Private       - The private structure of WinNt console In/Out.

Returns:

  EFI_SUCCESS   - The status light is updated successfully.

--*/  
{ 
  //
  // BUGBUG:Only SendInput/keybd_event function can toggle 
  // NumLock, CapsLock and ScrollLock keys.
  // Neither of these functions is included in EFI_WIN_NT_THUNK_PROTOCOL.
  // Thus, return immediately without operation.
  //
  return EFI_SUCCESS;
  
}


EFI_STATUS
GopPrivateResetWorker (
  IN GOP_PRIVATE_DATA                     *Private
  )
/*++

Routine Description:

  This function is a worker function for SimpleTextIn/SimpleTextInEx.Reset().

Arguments:

  Private     - WinNT GOP private structure

Returns:

  EFI_SUCCESS - Reset successfully

--*/
{
  EFI_KEY_DATA      KeyData;
  EFI_TPL           OldTpl;

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // A reset is draining the Queue
  //
  while (GopPrivateDeleteQ (Private, &Private->QueueForRead, &KeyData) == EFI_SUCCESS)
    ;
  while (GopPrivateDeleteQ (Private, &Private->QueueForNotify, &KeyData) == EFI_SUCCESS)
    ;

  Private->LeftShift               = FALSE;
  Private->RightShift              = FALSE;
  Private->LeftAlt                 = FALSE;
  Private->RightAlt                = FALSE;
  Private->LeftCtrl                = FALSE;
  Private->RightCtrl               = FALSE;
  Private->LeftLogo                = FALSE;
  Private->RightLogo               = FALSE;
  Private->Menu                    = FALSE;
  Private->SysReq                  = FALSE;
  
  Private->CapsLock                = FALSE;
  Private->NumLock                 = FALSE;
  Private->ScrollLock              = FALSE;
 
  Private->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  Private->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

EFI_STATUS
GopPrivateReadKeyStrokeWorker (
  IN GOP_PRIVATE_DATA                   *Private,
  OUT EFI_KEY_DATA                      *KeyData
  )
/*++

  Routine Description:
    Reads the next keystroke from the input device. The WaitForKey Event can 
    be used to test for existance of a keystroke via WaitForEvent () call.

  Arguments:
    Private    - The private structure of WinNt Gop device.
    KeyData    - A pointer to a buffer that is filled in with the keystroke 
                 state data for the key that was pressed.

  Returns:
    EFI_SUCCESS           - The keystroke information was returned.
    EFI_NOT_READY         - There was no keystroke data availiable.
    EFI_DEVICE_ERROR      - The keystroke information was not returned due to 
                            hardware errors.
    EFI_INVALID_PARAMETER - KeyData is NULL.                        

--*/
{
  EFI_STATUS                      Status;
  EFI_TPL                         OldTpl;  

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Call hot key callback before telling caller there is a key available
  //
  WinNtGopSimpleTextInTimerHandler (NULL, Private);

  Status  = GopPrivateCheckQ (&Private->QueueForRead);
  if (!EFI_ERROR (Status)) {
    //
    // If a Key press exists try and read it.
    //
    Status = GopPrivateDeleteQ (Private, &Private->QueueForRead, KeyData);
    if (!EFI_ERROR (Status)) {
      //
      // Leave critical section and return
      //
      gBS->RestoreTPL (OldTpl);
      return EFI_SUCCESS;
    }
  }

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;

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

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  return GopPrivateResetWorker (Private);
}


/**
  TODO: Add function description

  @param  This                  TODO: add argument description
  @param  Key                   TODO: add argument description

  @return TODO: add return values

**/
EFI_STATUS
EFIAPI
WinNtGopSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL          *This,
  OUT EFI_INPUT_KEY                       *Key
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_KEY_DATA      KeyData;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);

  Status = GopPrivateReadKeyStrokeWorker (Private, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));
  
  return EFI_SUCCESS;  
}


/**
  TODO: Add function description

  @param  Event                 TODO: add argument description
  @param  Context               TODO: add argument description

  @return TODO: add return values

**/
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
  
  //
  // Call hot key callback before telling caller there is a key available
  //
  WinNtGopSimpleTextInTimerHandler (NULL, Private);

  Status  = GopPrivateCheckQ (&Private->QueueForRead);
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

//
// Simple Text Input Ex protocol functions
//

EFI_STATUS
EFIAPI
WinNtGopSimpleTextInExResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
/*++

  Routine Description:
    Reset the input device and optionaly run diagnostics

  Arguments:
    This                 - Protocol instance pointer.
    ExtendedVerification - Driver may perform diagnostics on reset.

  Returns:
    EFI_SUCCESS           - The device was reset.

--*/
{
  GOP_PRIVATE_DATA *Private;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);
  
  return GopPrivateResetWorker (Private);
}

EFI_STATUS
EFIAPI
WinNtGopSimpleTextInExReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
/*++

  Routine Description:
    Reads the next keystroke from the input device. The WaitForKey Event can 
    be used to test for existance of a keystroke via WaitForEvent () call.

  Arguments:
    This       - Protocol instance pointer.
    KeyData    - A pointer to a buffer that is filled in with the keystroke 
                 state data for the key that was pressed.

  Returns:
    EFI_SUCCESS           - The keystroke information was returned.
    EFI_NOT_READY         - There was no keystroke data availiable.
    EFI_DEVICE_ERROR      - The keystroke information was not returned due to 
                            hardware errors.
    EFI_INVALID_PARAMETER - KeyData is NULL.                        

--*/
{
  GOP_PRIVATE_DATA *Private;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);
  
  return GopPrivateReadKeyStrokeWorker (Private, KeyData);

}

EFI_STATUS
EFIAPI
WinNtGopSimpleTextInExSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
/*++

  Routine Description:
    Set certain state for the input device.

  Arguments:
    This                  - Protocol instance pointer.
    KeyToggleState        - A pointer to the EFI_KEY_TOGGLE_STATE to set the 
                            state for the input device.
                          
  Returns:                
    EFI_SUCCESS           - The device state was set successfully.
    EFI_DEVICE_ERROR      - The device is not functioning correctly and could 
                            not have the setting adjusted.
    EFI_UNSUPPORTED       - The device does not have the ability to set its state.
    EFI_INVALID_PARAMETER - KeyToggleState is NULL.                       

--*/   
{
  EFI_STATUS                      Status;
  GOP_PRIVATE_DATA                *Private;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);

  if (((Private->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) ||
      ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID)) {
    return EFI_UNSUPPORTED;  
  }

  Private->ScrollLock = FALSE;
  Private->NumLock    = FALSE;
  Private->CapsLock   = FALSE;

  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    Private->ScrollLock = TRUE;
  } 
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    Private->NumLock = TRUE;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    Private->CapsLock = TRUE;
  }

  Status = GopPrivateUpdateStatusLight (Private);  
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private->KeyState.KeyToggleState = *KeyToggleState;
  return EFI_SUCCESS;
  
}

EFI_STATUS
EFIAPI
WinNtGopSimpleTextInExRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                        *NotifyHandle
  )
/*++

  Routine Description:
    Register a notification function for a particular keystroke for the input device.

  Arguments:
    This                    - Protocol instance pointer.
    KeyData                 - A pointer to a buffer that is filled in with the keystroke 
                              information data for the key that was pressed.
    KeyNotificationFunction - Points to the function to be called when the key 
                              sequence is typed specified by KeyData.                        
    NotifyHandle            - Points to the unique handle assigned to the registered notification.                          

  Returns:
    EFI_SUCCESS             - The notification function was registered successfully.
    EFI_OUT_OF_RESOURCES    - Unable to allocate resources for necesssary data structures.
    EFI_INVALID_PARAMETER   - KeyData or NotifyHandle is NULL.                       
                              
--*/   
{
  GOP_PRIVATE_DATA                   *Private;
  WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *CurrentNotify;
  LIST_ENTRY                         *Link;
  WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *NewNotify;      

  if (KeyData == NULL || KeyNotificationFunction == NULL || NotifyHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );
    if (GopPrivateIsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify->NotifyHandle;
        return EFI_SUCCESS;
      }
    }
  }    
  
  //
  // Allocate resource to save the notification function
  //  
  NewNotify = (WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *) AllocateZeroPool (sizeof (WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE;     
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  NewNotify->NotifyHandle      = (EFI_HANDLE) NewNotify;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&Private->NotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle = NewNotify->NotifyHandle;  
  
  return EFI_SUCCESS;
  
}

EFI_STATUS
EFIAPI
WinNtGopSimpleTextInExUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  )
/*++

  Routine Description:
    Remove a registered notification function from a particular keystroke.

  Arguments:
    This                    - Protocol instance pointer.    
    NotificationHandle      - The handle of the notification function being unregistered.

  Returns:
    EFI_SUCCESS             - The notification function was unregistered successfully.
    EFI_INVALID_PARAMETER   - The NotificationHandle is invalid.
                              
--*/   
{
  GOP_PRIVATE_DATA                   *Private;
  LIST_ENTRY                         *Link;
  WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *CurrentNotify;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  } 

  if (((WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *) NotificationHandle)->Signature != WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  } 

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);

  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );       
    if (CurrentNotify->NotifyHandle == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);      

      gBS->FreePool (CurrentNotify);            
      return EFI_SUCCESS;
    }
  }

  //
  // Can not find the specified Notification Handle
  //
  return EFI_INVALID_PARAMETER;
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

  GopPrivateCreateQ (Private, &Private->QueueForRead);
  GopPrivateCreateQ (Private, &Private->QueueForNotify);

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

  
  Private->SimpleTextInEx.Reset               = WinNtGopSimpleTextInExResetEx;
  Private->SimpleTextInEx.ReadKeyStrokeEx     = WinNtGopSimpleTextInExReadKeyStrokeEx;
  Private->SimpleTextInEx.SetState            = WinNtGopSimpleTextInExSetState;
  Private->SimpleTextInEx.RegisterKeyNotify   = WinNtGopSimpleTextInExRegisterKeyNotify;
  Private->SimpleTextInEx.UnregisterKeyNotify = WinNtGopSimpleTextInExUnregisterKeyNotify;

  Private->SimpleTextInEx.Reset (&Private->SimpleTextInEx, FALSE);
  
  InitializeListHead (&Private->NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  WinNtGopSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextInEx.WaitForKeyEx
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create the Timer to trigger hot key notifications
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  WinNtGopSimpleTextInTimerHandler,
                  Private,
                  &Private->TimerEvent
                  );
  ASSERT_EFI_ERROR (Status);
  
  Status = gBS->SetTimer (
                  Private->TimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  ASSERT_EFI_ERROR (Status);

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
  gBS->CloseEvent (Private->TimerEvent);

  GopPrivateDestroyQ (Private, &Private->QueueForRead);
  GopPrivateDestroyQ (Private, &Private->QueueForNotify);

  return EFI_SUCCESS;
}
