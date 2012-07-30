/*++

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


--*/

#include "UnixGop.h"


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
EFIAPI
GopPrivateInvokeRegisteredFunction (
  IN VOID           *Context,
  IN EFI_KEY_DATA   *KeyData
  )
{ 
  LIST_ENTRY                        *Link;
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY  *CurrentNotify;
  GOP_PRIVATE_DATA                  *Private = (GOP_PRIVATE_DATA *)Context;
  
  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );
    if (GopPrivateIsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      // We could be called at a high TPL so signal an event to call the registered function 
      // at a lower TPL.
      gBS->SignalEvent (CurrentNotify->Event);
    }
  }    
}



//
// Simple Text In implementation.
//

/**
  Reset the input device and optionally run diagnostics

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL       *This,
  IN BOOLEAN                              ExtendedVerification
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_KEY_DATA      KeyData;
  EFI_TPL           OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // A reset is draining the Queue
  //
  while (Private->UgaIo->UgaGetKey (Private->UgaIo, &KeyData) == EFI_SUCCESS)
    ;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This  Protocol instance pointer.
  @param  Key   A pointer to a buffer that is filled in with the keystroke
                information for the key that was pressed.

  @retval EFI_SUCCESS      The keystroke information was returned.
  @retval EFI_NOT_READY    There was no keystroke data available.
  @retval EFI_DEVICE_ERROR The keystroke information was not returned due to
                           hardware errors.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInReadKeyStroke (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL       *This,
  OUT EFI_INPUT_KEY                       *Key
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;
  EFI_KEY_DATA      KeyData;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaGetKey(Private->UgaIo, &KeyData);
  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}



/**
  SimpleTextIn and SimpleTextInEx Notify Wait Event 

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               Pointer to GOP_PRIVATE_DATA.

**/
VOID
EFIAPI
UnixGopSimpleTextInWaitForKey (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = (GOP_PRIVATE_DATA *) Context;
  if (Private->UgaIo == NULL) {
    return;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaCheckKey (Private->UgaIo);
  if (!EFI_ERROR (Status)) {
    //
    // If a there is a key in the queue signal our event.
    //
    gBS->SignalEvent (Event);
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}


//
// Simple Text Input Ex protocol functions
//


/**
  The Reset() function resets the input device hardware. As part
  of initialization process, the firmware/device will make a quick
  but reasonable attempt to verify that the device is functioning.
  If the ExtendedVerification flag is TRUE the firmware may take
  an extended amount of time to verify the device is operating on
  reset. Otherwise the reset operation is to occur as quickly as
  possible. The hardware verification process is not defined by
  this specification and is left up to the platform firmware or
  driver to implement.

  @param This                 A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param ExtendedVerification Indicates that the driver may
                              perform a more exhaustive
                              verification operation of the
                              device during reset.


  @retval EFI_SUCCESS       The device was reset.
  
  @retval EFI_DEVICE_ERROR  The device is not functioning
                            correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInExResetEx (
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
  
  return EFI_SUCCESS;
}



/**
  The function reads the next keystroke from the input device. If
  there is no pending keystroke the function returns
  EFI_NOT_READY. If there is a pending keystroke, then
  KeyData.Key.ScanCode is the EFI scan code defined in Error!
  Reference source not found. The KeyData.Key.UnicodeChar is the
  actual printable character or is zero if the key does not
  represent a printable character (control key, function key,
  etc.). The KeyData.KeyState is shift state for the character
  reflected in KeyData.Key.UnicodeChar or KeyData.Key.ScanCode .
  When interpreting the data from this function, it should be
  noted that if a class of printable characters that are
  normally adjusted by shift modifiers (e.g. Shift Key + "f"
  key) would be presented solely as a KeyData.Key.UnicodeChar
  without the associated shift state. So in the previous example
  of a Shift Key + "f" key being pressed, the only pertinent
  data returned would be KeyData.Key.UnicodeChar with the value
  of "F". This of course would not typically be the case for
  non-printable characters such as the pressing of the Right
  Shift Key + F10 key since the corresponding returned data
  would be reflected both in the KeyData.KeyState.KeyShiftState
  and KeyData.Key.ScanCode values. UEFI drivers which implement
  the EFI_SIMPLE_TEXT_INPUT_EX protocol are required to return
  KeyData.Key and KeyData.KeyState values. These drivers must
  always return the most current state of
  KeyData.KeyState.KeyShiftState and
  KeyData.KeyState.KeyToggleState. It should also be noted that
  certain input devices may not be able to produce shift or toggle
  state information, and in those cases the high order bit in the
  respective Toggle and Shift state fields should not be active.

  
  @param This     A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param KeyData  A pointer to a buffer that is filled in with
                  the keystroke state data for the key that was
                  pressed.

  
  @retval EFI_SUCCESS     The keystroke information was
                          returned.
  
  @retval EFI_NOT_READY   There was no keystroke data available.
                          EFI_DEVICE_ERROR The keystroke
                          information was not returned due to
                          hardware errors.


**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInExReadKeyStrokeEx (
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
  EFI_STATUS        Status;
  GOP_PRIVATE_DATA  *Private;
  EFI_TPL           OldTpl;


  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaGetKey(Private->UgaIo, KeyData);

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}



/**
  The SetState() function allows the input device hardware to
  have state settings adjusted.
  
  @param This           A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  
  @param KeyToggleState Pointer to the EFI_KEY_TOGGLE_STATE to
                        set the state for the input device.
  
  
  @retval EFI_SUCCESS       The device state was set appropriately.

  @retval EFI_DEVICE_ERROR  The device is not functioning
                            correctly and could not have the
                            setting adjusted.

  @retval EFI_UNSUPPORTED   The device does not support the
                            ability to have its state set.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInExSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaKeySetState (Private->UgaIo, KeyToggleState);
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  SimpleTextIn and SimpleTextInEx Notify Wait Event 

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               Pointer to GOP_PRIVATE_DATA.

**/
VOID
EFIAPI
UnixGopRegisterKeyCallback (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *ExNotify = (UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *)Context;
  
  ExNotify->KeyNotificationFn (&ExNotify->KeyData);
}



/**
  The RegisterKeystrokeNotify() function registers a function
  which will be called when a specified keystroke will occur.
  
  @param This                     A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  
  @param KeyData                  A pointer to a buffer that is filled in with
                                  the keystroke information for the key that was
                                  pressed.
  
  @param KeyNotificationFunction  Points to the function to be
                                  called when the key sequence
                                  is typed specified by KeyData.
  
  
  @param NotifyHandle             Points to the unique handle assigned to
                                  the registered notification.
  
  @retval EFI_SUCCESS           The device state was set
                                appropriately.

  @retval EFI_OUT_OF_RESOURCES  Unable to allocate necessary
                                data structures.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInExRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  EFI_STATUS                          Status;
  GOP_PRIVATE_DATA                    *Private;
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY    *CurrentNotify;
  LIST_ENTRY                          *Link;
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY    *NewNotify;

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
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );
    if (GopPrivateIsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        return EFI_SUCCESS;
      }
    }
  }    
  
  //
  // Allocate resource to save the notification function
  //  
  NewNotify = (UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *) AllocateZeroPool (sizeof (UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE;     
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (KeyData));
  InsertTailList (&Private->NotifyList, &NewNotify->NotifyEntry);
  
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UnixGopRegisterKeyCallback,
                  NewNotify,
                  NewNotify->Event
                  );
  ASSERT_EFI_ERROR (Status);


  *NotifyHandle = NewNotify;  
  
  return EFI_SUCCESS;
  
}


/**
  The UnregisterKeystrokeNotify() function removes the
  notification which was previously registered.
  
  @param This               A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.
  
  @param NotificationHandle The handle of the notification
                            function being unregistered.
  
  @retval EFI_SUCCESS The device state was set appropriately.
  
  @retval EFI_INVALID_PARAMETER The NotificationHandle is
                                invalid.

**/
EFI_STATUS
EFIAPI
UnixGopSimpleTextInExUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
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
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *CurrentNotify;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  } 

  if (((UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *) NotificationHandle)->Signature != UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  } 

  Private = GOP_PRIVATE_DATA_FROM_TEXT_IN_EX_THIS (This);

  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                      NotifyEntry, 
                      UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                      );       
    if (CurrentNotify == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);    
      
      gBS->CloseEvent (CurrentNotify->Event);

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
  Initialize SimplelTextIn and SimpleTextInEx protocols in the Private
  context structure.

  @param  Private               Context structure to fill in. 

  @return EFI_SUCCESS           Initialization was a success

**/
EFI_STATUS
UnixGopInitializeSimpleTextInForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS  Status;

  //
  // Initialize Simple Text In protoocol
  //
  Private->SimpleTextIn.Reset         = UnixGopSimpleTextInReset;
  Private->SimpleTextIn.ReadKeyStroke = UnixGopSimpleTextInReadKeyStroke;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UnixGopSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextIn.WaitForKey
                  );
  ASSERT_EFI_ERROR (Status);
  
  
  //
  // Initialize Simple Text In Ex
  //
  
  Private->SimpleTextInEx.Reset               = UnixGopSimpleTextInExResetEx;
  Private->SimpleTextInEx.ReadKeyStrokeEx     = UnixGopSimpleTextInExReadKeyStrokeEx;
  Private->SimpleTextInEx.SetState            = UnixGopSimpleTextInExSetState;
  Private->SimpleTextInEx.RegisterKeyNotify   = UnixGopSimpleTextInExRegisterKeyNotify;
  Private->SimpleTextInEx.UnregisterKeyNotify = UnixGopSimpleTextInExUnregisterKeyNotify;

  Private->SimpleTextInEx.Reset (&Private->SimpleTextInEx, FALSE);
  
  InitializeListHead (&Private->NotifyList);

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UnixGopSimpleTextInWaitForKey,
                  Private,
                  &Private->SimpleTextInEx.WaitForKeyEx
                  );
  ASSERT_EFI_ERROR (Status);


  return Status;
}







//
// Simple Pointer implementation.
//


/**                                                                 
  Resets the pointer device hardware.
    
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL
                                instance.                                   
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.                                       
                                
  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.  
                                   
**/
EFI_STATUS
EFIAPI
UnixGopSimplePointerReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL          *This,
  IN BOOLEAN                              ExtendedVerification
  )
{
  GOP_PRIVATE_DATA             *Private;
  EFI_SIMPLE_POINTER_STATE     State;
  EFI_TPL                      OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_POINTER_MODE_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // A reset is draining the Queue
  //
  while (Private->UgaIo->UgaGetPointerState (Private->UgaIo, &State) == EFI_SUCCESS)
    ;

  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}


/**                                                                 
  Retrieves the current state of a pointer device.
    
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL
                                instance.                                   
  @param  State                 A pointer to the state information on the pointer device.
                                
  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().                                                           
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.                                                           
                                   
**/
EFI_STATUS
EFIAPI
UnixGopSimplePointerGetState (
  IN EFI_SIMPLE_POINTER_PROTOCOL          *This,
  IN OUT EFI_SIMPLE_POINTER_STATE         *State
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = GOP_PRIVATE_DATA_FROM_POINTER_MODE_THIS (This);
  if (Private->UgaIo == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaGetPointerState (Private->UgaIo, State);
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  SimplePointer Notify Wait Event 

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               Pointer to GOP_PRIVATE_DATA.

**/
VOID
EFIAPI
UnixGopSimplePointerWaitForInput (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  GOP_PRIVATE_DATA  *Private;
  EFI_STATUS        Status;
  EFI_TPL           OldTpl;

  Private = (GOP_PRIVATE_DATA *) Context;
  if (Private->UgaIo == NULL) {
    return;
  }

  //
  // Enter critical section
  //
  OldTpl  = gBS->RaiseTPL (TPL_NOTIFY);

  Status  = Private->UgaIo->UgaCheckPointer (Private->UgaIo);
  if (!EFI_ERROR (Status)) {
    //
    // If the pointer state has changed, signal our event.
    //
    gBS->SignalEvent (Event);
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}


/**
  SimplePointer constructor 

  @param  Private Context structure to fill in.      

  @retval EFI_SUCCESS Constructor had success        

**/
EFI_STATUS
UnixGopInitializeSimplePointerForWindow (
  IN  GOP_PRIVATE_DATA    *Private
  )
{
  EFI_STATUS  Status;

  //
  // Initialize Simple Pointer protoocol
  //
  Private->PointerMode.ResolutionX = 1;
  Private->PointerMode.ResolutionY = 1;
  Private->PointerMode.ResolutionZ = 1;
  Private->PointerMode.LeftButton  = TRUE;
  Private->PointerMode.RightButton = TRUE;

  Private->SimplePointer.Reset     = UnixGopSimplePointerReset;
  Private->SimplePointer.GetState  = UnixGopSimplePointerGetState;
  Private->SimplePointer.Mode      = &Private->PointerMode;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UnixGopSimplePointerWaitForInput,
                  Private,
                  &Private->SimplePointer.WaitForInput
                  );

  return Status;
}
