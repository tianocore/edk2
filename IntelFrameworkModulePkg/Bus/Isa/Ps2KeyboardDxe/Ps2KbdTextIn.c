/**@file
  PS/2 Keyboard  driver
  Routines that support SIMPLE_TEXT_IN protocol

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


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

/**
  @param RegsiteredData    - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was registered.
  @param InputData         - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was pressed.

  @retval TRUE              - Key be pressed matches a registered key.
  @retval FALSE             - Match failed. 
  
**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );

/**
    Reads the next keystroke from the input device. The WaitForKey Event can 
    be used to test for existance of a keystroke via WaitForEvent () call.

  
    @param ConsoleInDev          - Ps2 Keyboard private structure
    @param KeyData               - A pointer to a buffer that is filled in with the keystroke 
                            state data for the key that was pressed.

  
    @retval EFI_SUCCESS           - The keystroke information was returned.
    @retval EFI_NOT_READY         - There was no keystroke data availiable.
    @retval EFI_DEVICE_ERROR      - The keystroke information was not returned due to 
                            hardware errors.
    @retval EFI_INVALID_PARAMETER - KeyData is NULL.                        

**/
EFI_STATUS
KeyboardReadKeyStrokeWorker (
  IN  KEYBOARD_CONSOLE_IN_DEV           *ConsoleInDev,
  OUT EFI_KEY_DATA                      *KeyData
  )

{
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;
  LIST_ENTRY                            *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY         *CurrentNotify;
  EFI_KEY_DATA                          OriginalKeyData;
  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (ConsoleInDev->KeyboardErr) {
    gBS->RestoreTPL (OldTpl);
    return EFI_DEVICE_ERROR;
  }
  //
  // If there's no key, just return
  //
  Status = KeyboardCheckForKey (&ConsoleInDev->ConIn);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_READY;
  }
  CopyMem (&KeyData->Key, &ConsoleInDev->Key, sizeof (EFI_INPUT_KEY));

  ConsoleInDev->Key.ScanCode    = SCAN_NULL;          
  ConsoleInDev->Key.UnicodeChar = 0x0000;     
  CopyMem (&KeyData->KeyState, &ConsoleInDev->KeyState, sizeof (EFI_KEY_STATE));
                                          
  ConsoleInDev->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  ConsoleInDev->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
  gBS->RestoreTPL (OldTpl);
  //
  //Switch the control value to their original characters. In KeyGetchar() the  CTRL-Alpha characters have been switched to 
  // their corresponding control value (ctrl-a = 0x0001 through ctrl-Z = 0x001A), here switch them back for notification function.
  //
  CopyMem (&OriginalKeyData, KeyData, sizeof (EFI_KEY_DATA));
  if (ConsoleInDev->Ctrled) {
    if (OriginalKeyData.Key.UnicodeChar >= 0x01 && OriginalKeyData.Key.UnicodeChar <= 0x1A) {
      if (ConsoleInDev->CapsLock) {
        OriginalKeyData.Key.UnicodeChar = (CHAR16)(OriginalKeyData.Key.UnicodeChar + 'A' - 1);
      } else {
        OriginalKeyData.Key.UnicodeChar = (CHAR16)(OriginalKeyData.Key.UnicodeChar + 'a' - 1);
      } 
    }
  }
  //
  // Invoke notification functions if exist
  //
  for (Link = ConsoleInDev->NotifyList.ForwardLink; Link != &ConsoleInDev->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, &OriginalKeyData)) { 
      CurrentNotify->KeyNotificationFn (&OriginalKeyData);
    }
  }

  return EFI_SUCCESS;
}

/**
  logic reset keyboard
  Implement SIMPLE_TEXT_IN.Reset()
  Perform 8042 controller and keyboard initialization

  @param This    Pointer to instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  @param ExtendedVerification Indicate that the driver may perform a more 
                              exhaustive verification operation of the device during 
                              reset, now this par is ignored in this driver    

**/
EFI_STATUS
EFIAPI
KeyboardEfiReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
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

/**
  Implement SIMPLE_TEXT_IN.ReadKeyStroke().
  Retrieve key values for driver user.

  @param This    Pointer to instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  @param Key     The output buffer for key value 

  @retval EFI_SUCCESS success to read key stroke
**/
EFI_STATUS
EFIAPI
KeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  EFI_STATUS              Status;
  KEYBOARD_CONSOLE_IN_DEV *ConsoleIn;
  EFI_KEY_DATA            KeyData;

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);
  Status = KeyboardReadKeyStrokeWorker (ConsoleIn, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));
  return EFI_SUCCESS;
  
}

/**
  Event notification function for SIMPLE_TEXT_IN.WaitForKey event
  Signal the event if there is key available

  @param Event    the event object
  @param Context  waitting context

**/
VOID
EFIAPI
KeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
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

/**
  Check keyboard for given key value
  
  @param  This  Point to instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL
  
  @retval EFI_SUCCESS success check keyboard value
**/
EFI_STATUS
KeyboardCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This
  )
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

/**
  Judge whether is a registed key

  @param RegsiteredData    - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was registered.
  @param InputData         - A pointer to a buffer that is filled in with the keystroke 
                      state data for the key that was pressed.

  @retval TRUE              - Key be pressed matches a registered key.
  @retval FLASE             - Match failed. 
  
**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )

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

/**
  Event notification function for SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx event
  Signal the event if there is key available

  @param Event    event object
  @param Context  waiting context

**/
VOID
EFIAPI
KeyboardWaitForKeyEx (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )

{
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;

  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (Context); 
  KeyboardWaitForKey (Event, &ConsoleInDev->ConIn);
  
}

/**
  Reset the input device and optionaly run diagnostics

  @param This                 - Protocol instance pointer.
  @param ExtendedVerification - Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS           - The device was reset.
  @retval EFI_DEVICE_ERROR      - The device is not functioning properly and could 
                                  not be reset.

**/
EFI_STATUS
EFIAPI
KeyboardEfiResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )

{
  EFI_STATUS                            Status;
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;
  EFI_TPL                               OldTpl;

  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This); 
  if (ConsoleInDev->KeyboardErr) {
    return EFI_DEVICE_ERROR;
  }

  Status = ConsoleInDev->ConIn.Reset (
                                 &ConsoleInDev->ConIn, 
                                 ExtendedVerification
                                 );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  ConsoleInDev->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  ConsoleInDev->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  gBS->RestoreTPL (OldTpl);  
  
  return EFI_SUCCESS;
}

/**
    Reads the next keystroke from the input device. The WaitForKey Event can 
    be used to test for existance of a keystroke via WaitForEvent () call.


    @param This       - Protocol instance pointer.
    @param KeyData    - A pointer to a buffer that is filled in with the keystroke 
                 state data for the key that was pressed.

    @retval EFI_SUCCESS           - The keystroke information was returned.
    @retval EFI_NOT_READY         - There was no keystroke data availiable.
    @retval EFI_DEVICE_ERROR      - The keystroke information was not returned due to 
                            hardware errors.
    @retval EFI_INVALID_PARAMETER - KeyData is NULL.                        

**/
EFI_STATUS
EFIAPI
KeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )

{
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);
  return KeyboardReadKeyStrokeWorker (ConsoleInDev, KeyData);
  
}

/**
  Set certain state for the input device.

  @param This              - Protocol instance pointer.
  @param KeyToggleState    - A pointer to the EFI_KEY_TOGGLE_STATE to set the 
                        state for the input device.

  @retval EFI_SUCCESS           - The device state was set successfully.
  @retval EFI_DEVICE_ERROR      - The device is not functioning correctly and could 
                            not have the setting adjusted.
  @retval EFI_UNSUPPORTED       - The device does not have the ability to set its state.
  @retval EFI_INVALID_PARAMETER - KeyToggleState is NULL.                       

**/   
EFI_STATUS
EFIAPI
KeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )

{
  EFI_STATUS                            Status;
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;
  EFI_TPL                               OldTpl;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (ConsoleInDev->KeyboardErr) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (((ConsoleInDev->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) ||
      ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID)) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }
  
  //
  // Update the status light
  //
  ConsoleInDev->ScrollLock = FALSE;
  ConsoleInDev->NumLock    = FALSE;
  ConsoleInDev->CapsLock   = FALSE;

  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    ConsoleInDev->ScrollLock = TRUE;
  } 
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    ConsoleInDev->NumLock = TRUE;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    ConsoleInDev->CapsLock = TRUE;
  }

  Status = UpdateStatusLights (ConsoleInDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;    
  }

  ConsoleInDev->KeyState.KeyToggleState = *KeyToggleState;
  
Exit:   
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;

}

/**
    Register a notification function for a particular keystroke for the input device.

    @param This                    - Protocol instance pointer.
    @param KeyData                 - A pointer to a buffer that is filled in with the keystroke 
                              information data for the key that was pressed.
    @param KeyNotificationFunction - Points to the function to be called when the key 
                              sequence is typed specified by KeyData.                        
    @param NotifyHandle            - Points to the unique handle assigned to the registered notification.                          

    @retval EFI_SUCCESS             - The notification function was registered successfully.
    @retval EFI_OUT_OF_RESOURCES    - Unable to allocate resources for necesssary data structures.
    @retval EFI_INVALID_PARAMETER   - KeyData or NotifyHandle is NULL.                       
                              
**/   
EFI_STATUS
EFIAPI
KeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                        *NotifyHandle
  )
{
  EFI_STATUS                            Status;
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;
  EFI_TPL                               OldTpl;
  LIST_ENTRY                            *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY         *CurrentNotify;  
  KEYBOARD_CONSOLE_IN_EX_NOTIFY         *NewNotify;

  if (KeyData == NULL || NotifyHandle == NULL || KeyNotificationFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  for (Link = ConsoleInDev->NotifyList.ForwardLink; Link != &ConsoleInDev->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify->NotifyHandle;        
        Status = EFI_SUCCESS;
        goto Exit;
      }
    }
  }    
  
  //
  // Allocate resource to save the notification function
  //  
  NewNotify = (KEYBOARD_CONSOLE_IN_EX_NOTIFY *) AllocateZeroPool (sizeof (KEYBOARD_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  NewNotify->Signature         = KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE;     
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&ConsoleInDev->NotifyList, &NewNotify->NotifyEntry);

  //
  // Use gSimpleTextInExNotifyGuid to get a valid EFI_HANDLE
  //  
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewNotify->NotifyHandle,
                  &gSimpleTextInExNotifyGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  *NotifyHandle                = NewNotify->NotifyHandle;  
  Status                       = EFI_SUCCESS;

Exit:
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return Status;  

}

/**
    Remove a registered notification function from a particular keystroke.

    @param This                    - Protocol instance pointer.    
    @param NotificationHandle      - The handle of the notification function being unregistered.

  
    @retval EFI_SUCCESS             - The notification function was unregistered successfully.
    @retval EFI_INVALID_PARAMETER   - The NotificationHandle is invalid.
    @retval EFI_NOT_FOUND           - Can not find the matching entry in database.  
                              
**/   
EFI_STATUS
EFIAPI
KeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  )
{
  EFI_STATUS                            Status;
  KEYBOARD_CONSOLE_IN_DEV               *ConsoleInDev;
  EFI_TPL                               OldTpl;
  LIST_ENTRY                            *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY         *CurrentNotify;    

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  } 
  
  Status = gBS->OpenProtocol (
                  NotificationHandle,
                  &gSimpleTextInExNotifyGuid,
                  NULL,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  ConsoleInDev = TEXT_INPUT_EX_KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (This);
  
  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);  

  for (Link = ConsoleInDev->NotifyList.ForwardLink; Link != &ConsoleInDev->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );       
    if (CurrentNotify->NotifyHandle == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);      
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      CurrentNotify->NotifyHandle,
                      &gSimpleTextInExNotifyGuid,
                      NULL,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
      gBS->FreePool (CurrentNotify);            
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  //
  // Can not find the specified Notification Handle
  //
  Status = EFI_NOT_FOUND;
Exit:
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return Status;
}

