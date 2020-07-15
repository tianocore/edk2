/** @file
  VirtualKeyboard driver

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VirtualKeyboard.h"

//
// RAM Keyboard Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gVirtualKeyboardDriverBinding = {
  VirtualKeyboardDriverBindingSupported,
  VirtualKeyboardDriverBindingStart,
  VirtualKeyboardDriverBindingStop,
  0x10,
  NULL,
  NULL
};

//
// EFI Driver Binding Protocol Functions
//

/**
  Check whether the driver supports this device.

  @param  This                   The Udriver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  PLATFORM_VIRTUAL_KBD_PROTOCOL   *PlatformVirtual;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gPlatformVirtualKeyboardProtocolGuid,
                  (VOID **) &PlatformVirtual,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  gBS->CloseProtocol (
         Controller,
         &gPlatformVirtualKeyboardProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

/**
  Starts the device with this driver.

  @param  This                   The driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                                Status;
  VIRTUAL_KEYBOARD_DEV                      *VirtualKeyboardPrivate;
  PLATFORM_VIRTUAL_KBD_PROTOCOL             *PlatformVirtual;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gPlatformVirtualKeyboardProtocolGuid,
                  (VOID **) &PlatformVirtual,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate the private device structure
  //
  VirtualKeyboardPrivate = (VIRTUAL_KEYBOARD_DEV *) AllocateZeroPool (sizeof (VIRTUAL_KEYBOARD_DEV));
  if (VirtualKeyboardPrivate == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Initialize the private device structure
  //
  VirtualKeyboardPrivate->Signature                  = VIRTUAL_KEYBOARD_DEV_SIGNATURE;
  VirtualKeyboardPrivate->Handle                     = Controller;
  VirtualKeyboardPrivate->PlatformVirtual            = PlatformVirtual;
  VirtualKeyboardPrivate->Queue.Front                = 0;
  VirtualKeyboardPrivate->Queue.Rear                 = 0;
  VirtualKeyboardPrivate->QueueForNotify.Front       = 0;
  VirtualKeyboardPrivate->QueueForNotify.Rear        = 0;

  VirtualKeyboardPrivate->SimpleTextIn.Reset         = VirtualKeyboardReset;
  VirtualKeyboardPrivate->SimpleTextIn.ReadKeyStroke = VirtualKeyboardReadKeyStroke;

  VirtualKeyboardPrivate->SimpleTextInputEx.Reset               = VirtualKeyboardResetEx;
  VirtualKeyboardPrivate->SimpleTextInputEx.ReadKeyStrokeEx     = VirtualKeyboardReadKeyStrokeEx;
  VirtualKeyboardPrivate->SimpleTextInputEx.SetState            = VirtualKeyboardSetState;

  VirtualKeyboardPrivate->SimpleTextInputEx.RegisterKeyNotify   = VirtualKeyboardRegisterKeyNotify;
  VirtualKeyboardPrivate->SimpleTextInputEx.UnregisterKeyNotify = VirtualKeyboardUnregisterKeyNotify;
  InitializeListHead (&VirtualKeyboardPrivate->NotifyList);

  Status = PlatformVirtual->Register ();
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Report that the keyboard is being enabled
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE
    );

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtualKeyboardWaitForKey,
                  &(VirtualKeyboardPrivate->SimpleTextIn),
                  &((VirtualKeyboardPrivate->SimpleTextIn).WaitForKey)
                  );
  if (EFI_ERROR (Status)) {
    (VirtualKeyboardPrivate->SimpleTextIn).WaitForKey = NULL;
    goto Done;
  }
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  VirtualKeyboardWaitForKeyEx,
                  &(VirtualKeyboardPrivate->SimpleTextInputEx),
                  &(VirtualKeyboardPrivate->SimpleTextInputEx.WaitForKeyEx)
                  );
  if (EFI_ERROR (Status)) {
    VirtualKeyboardPrivate->SimpleTextInputEx.WaitForKeyEx = NULL;
    goto Done;
  }

  //
  // Setup a periodic timer, used for reading keystrokes at a fixed interval
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtualKeyboardTimerHandler,
                  VirtualKeyboardPrivate,
                  &VirtualKeyboardPrivate->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = gBS->SetTimer (
                  VirtualKeyboardPrivate->TimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  KeyNotifyProcessHandler,
                  VirtualKeyboardPrivate,
                  &VirtualKeyboardPrivate->KeyNotifyProcessEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Reset the keyboard device
  //
  Status = VirtualKeyboardPrivate->SimpleTextInputEx.Reset (
                                     &VirtualKeyboardPrivate->SimpleTextInputEx,
                                     FALSE
                                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[KBD]Reset Failed. Status - %r\n", Status));
    goto Done;
  }
  //
  // Install protocol interfaces for the keyboard device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &VirtualKeyboardPrivate->SimpleTextIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &VirtualKeyboardPrivate->SimpleTextInputEx,
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {
    if (VirtualKeyboardPrivate != NULL) {
      if ((VirtualKeyboardPrivate->SimpleTextIn).WaitForKey != NULL) {
        gBS->CloseEvent ((VirtualKeyboardPrivate->SimpleTextIn).WaitForKey);
      }

      if ((VirtualKeyboardPrivate->SimpleTextInputEx).WaitForKeyEx != NULL) {
        gBS->CloseEvent (
               (VirtualKeyboardPrivate->SimpleTextInputEx).WaitForKeyEx
               );
      }

      if (VirtualKeyboardPrivate->KeyNotifyProcessEvent != NULL) {
        gBS->CloseEvent (VirtualKeyboardPrivate->KeyNotifyProcessEvent);
      }

      VirtualKeyboardFreeNotifyList (&VirtualKeyboardPrivate->NotifyList);

      if (VirtualKeyboardPrivate->TimerEvent != NULL) {
        gBS->CloseEvent (VirtualKeyboardPrivate->TimerEvent);
      }
      FreePool (VirtualKeyboardPrivate);
    }
  }

  gBS->CloseProtocol (
         Controller,
         &gPlatformVirtualKeyboardProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Stop the device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a
                                 device error.
  @retval Others                 Fail to uninstall protocols attached on the
                                 device.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  return EFI_SUCCESS;
}


/**
  Enqueue the key.

  @param  Queue                 The queue to be enqueued.
  @param  KeyData               The key data to be enqueued.

  @retval EFI_NOT_READY         The queue is full.
  @retval EFI_SUCCESS           Successfully enqueued the key data.

**/
EFI_STATUS
Enqueue (
  IN SIMPLE_QUEUE         *Queue,
  IN EFI_KEY_DATA         *KeyData
  )
{
  if ((Queue->Rear + 1) % QUEUE_MAX_COUNT == Queue->Front) {
    return EFI_NOT_READY;
  }

  CopyMem (&Queue->Buffer[Queue->Rear], KeyData, sizeof (EFI_KEY_DATA));
  Queue->Rear = (Queue->Rear + 1) % QUEUE_MAX_COUNT;

  return EFI_SUCCESS;
}

/**
  Dequeue the key.

  @param  Queue                 The queue to be dequeued.
  @param  KeyData               The key data to be dequeued.

  @retval EFI_NOT_READY         The queue is empty.
  @retval EFI_SUCCESS           Successfully dequeued the key data.

**/
EFI_STATUS
Dequeue (
  IN SIMPLE_QUEUE         *Queue,
  IN EFI_KEY_DATA         *KeyData
  )
{
  if (Queue->Front == Queue->Rear) {
    return EFI_NOT_READY;
  }

  CopyMem (KeyData, &Queue->Buffer[Queue->Front], sizeof (EFI_KEY_DATA));
  Queue->Front  = (Queue->Front + 1) % QUEUE_MAX_COUNT;

  return EFI_SUCCESS;
}

/**
  Check whether the queue is empty.

  @param  Queue                 The queue to be checked.

  @retval EFI_NOT_READY         The queue is empty.
  @retval EFI_SUCCESS           The queue is not empty.

**/
EFI_STATUS
CheckQueue (
  IN SIMPLE_QUEUE         *Queue
  )
{
  if (Queue->Front == Queue->Rear) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Check key buffer to get the key stroke status.

  @param  This         Pointer of the protocol EFI_SIMPLE_TEXT_IN_PROTOCOL.

  @retval EFI_SUCCESS  A key is being pressed now.
  @retval Other        No key is now pressed.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardCheckForKey (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This
  )
{
  VIRTUAL_KEYBOARD_DEV     *VirtualKeyboardPrivate;

  VirtualKeyboardPrivate = VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  return CheckQueue (&VirtualKeyboardPrivate->Queue);
}

/**
  Free keyboard notify list.

  @param  ListHead   The list head

  @retval EFI_SUCCESS           Free the notify list successfully
  @retval EFI_INVALID_PARAMETER ListHead is invalid.

**/
EFI_STATUS
VirtualKeyboardFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  )
{
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink,
                   VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                   NotifyEntry,
                   VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    gBS->FreePool (NotifyNode);
  }

  return EFI_SUCCESS;
}

/**
  Judge whether is a registed key

  @param RegsiteredData       A pointer to a buffer that is filled in with
                              the keystroke state data for the key that was
                              registered.
  @param InputData            A pointer to a buffer that is filled in with
                              the keystroke state data for the key that was
                              pressed.

  @retval TRUE                Key be pressed matches a registered key.
  @retval FALSE               Match failed.

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
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means
  // these state could be ignored.
  //
  if ((RegsiteredData->KeyState.KeyShiftState != 0) &&
      (RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState)) {
    return FALSE;
  }
  if ((RegsiteredData->KeyState.KeyToggleState != 0) &&
      (RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState)) {
    return FALSE;
  }

  return TRUE;

}

/**
  Event notification function for SIMPLE_TEXT_IN.WaitForKey event
  Signal the event if there is key available

  @param Event    the event object
  @param Context  waiting context

**/
VOID
EFIAPI
VirtualKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
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
  //
  gBS->Stall (1000);
  //
  // Use TimerEvent callback function to check whether there's any key pressed
  //
  VirtualKeyboardTimerHandler (NULL, VIRTUAL_KEYBOARD_DEV_FROM_THIS (Context));

  if (!EFI_ERROR (VirtualKeyboardCheckForKey (Context))) {
    gBS->SignalEvent (Event);
  }
}

/**
  Event notification function for SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx
  event. Signal the event if there is key available

  @param Event    event object
  @param Context  waiting context

**/
VOID
EFIAPI
VirtualKeyboardWaitForKeyEx (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )

{
  VIRTUAL_KEYBOARD_DEV                     *VirtualKeyboardPrivate;

  VirtualKeyboardPrivate = TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS (Context);
  VirtualKeyboardWaitForKey (Event, &VirtualKeyboardPrivate->SimpleTextIn);

}

//
// EFI Simple Text In Protocol Functions
//
/**
  Reset the Keyboard and do BAT test for it, if (ExtendedVerification == TRUE)
  then do some extra keyboard validations.

  @param  This                  Pointer of simple text Protocol.
  @param  ExtendedVerification  Whether perform the extra validation of
                                keyboard. True: perform; FALSE: skip.

  @retval EFI_SUCCESS           The command byte is written successfully.
  @retval EFI_DEVICE_ERROR      Errors occurred during resetting keyboard.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
{
  VIRTUAL_KEYBOARD_DEV *VirtualKeyboardPrivate;
  EFI_STATUS           Status;
  EFI_TPL              OldTpl;

  VirtualKeyboardPrivate = VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  //
  // Raise TPL to avoid mouse operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (VirtualKeyboardPrivate->PlatformVirtual &&
      VirtualKeyboardPrivate->PlatformVirtual->Reset) {
    Status = VirtualKeyboardPrivate->PlatformVirtual->Reset ();
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  //
  // resume priority of task level
  //
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Reset the input device and optionally run diagnostics

  @param  This                  Protocol instance pointer.
  @param  ExtendedVerification  Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning properly and
                                could not be reset.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  VIRTUAL_KEYBOARD_DEV                  *VirtualKeyboardPrivate;
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;

  VirtualKeyboardPrivate = TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  Status = VirtualKeyboardPrivate->SimpleTextIn.Reset (
                                     &VirtualKeyboardPrivate->SimpleTextIn,
                                     ExtendedVerification
                                     );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  VirtualKeyboardPrivate   Virtualkeyboard driver private structure.
  @param  KeyData                  A pointer to a buffer that is filled in
                                   with the keystroke state data for the key
                                   that was pressed.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data available.
  @retval EFI_DEVICE_ERROR         The keystroke information was not returned
                                   due to hardware errors.
  @retval EFI_INVALID_PARAMETER    KeyData is NULL.

**/
EFI_STATUS
KeyboardReadKeyStrokeWorker (
  IN VIRTUAL_KEYBOARD_DEV  *VirtualKeyboardPrivate,
  OUT EFI_KEY_DATA      *KeyData
  )
{
  EFI_STATUS                            Status;
  EFI_TPL                               OldTpl;
  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Use TimerEvent callback function to check whether there's any key pressed
  //

  //
  // Stall 1ms to give a chance to let other driver interrupt this routine for
  // their timer event.
  // e.g. OS loader, other drivers which are driven by timer event will have a
  // bad performance during this period,
  // e.g. usb keyboard driver.
  // Add a stall period can greatly increate other driver performance during
  // the WaitForKey is recursivly invoked. 1ms delay will make little impact
  // to the thunk keyboard driver, and user can not feel the delay at all when
  // input.
  //
  gBS->Stall (1000);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  VirtualKeyboardTimerHandler (NULL, VirtualKeyboardPrivate);
  //
  // If there's no key, just return
  //
  Status = CheckQueue (&VirtualKeyboardPrivate->Queue);
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (OldTpl);
    return EFI_NOT_READY;
  }

  Status = Dequeue (&VirtualKeyboardPrivate->Queue, KeyData);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Read out the scan code of the key that has just been stroked.

  @param  This        Pointer of simple text Protocol.
  @param  Key         Pointer for store the key that read out.

  @retval EFI_SUCCESS The key is read out successfully.
  @retval other       The key reading failed.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  VIRTUAL_KEYBOARD_DEV     *VirtualKeyboardPrivate;
  EFI_STATUS               Status;
  EFI_KEY_DATA             KeyData;

  VirtualKeyboardPrivate = VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  Status = KeyboardReadKeyStrokeWorker (VirtualKeyboardPrivate, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert the Ctrl+[a-z] to Ctrl+[1-26]
  //
  if ((KeyData.KeyState.KeyShiftState & (EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) != 0) {
    if (KeyData.Key.UnicodeChar >= L'a' &&
        KeyData.Key.UnicodeChar <= L'z') {
      KeyData.Key.UnicodeChar = (CHAR16) (KeyData.Key.UnicodeChar - L'a' + 1);
    } else if (KeyData.Key.UnicodeChar >= L'A' &&
               KeyData.Key.UnicodeChar <= L'Z') {
      KeyData.Key.UnicodeChar = (CHAR16) (KeyData.Key.UnicodeChar - L'A' + 1);
    }
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));

  return EFI_SUCCESS;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This         Protocol instance pointer.
  @param  KeyData      A pointer to a buffer that is filled in with the
                       keystroke state data for the key that was pressed.

  @retval  EFI_SUCCESS           The keystroke information was returned.
  @retval  EFI_NOT_READY         There was no keystroke data available.
  @retval  EFI_DEVICE_ERROR      The keystroke information was not returned
                                 due to hardware errors.
  @retval  EFI_INVALID_PARAMETER KeyData is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  VIRTUAL_KEYBOARD_DEV                  *VirtualKeyboardPrivate;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VirtualKeyboardPrivate = TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  return KeyboardReadKeyStrokeWorker (VirtualKeyboardPrivate, KeyData);

}

/**
  Set certain state for the input device.

  @param  This              Protocol instance pointer.
  @param  KeyToggleState    A pointer to the EFI_KEY_TOGGLE_STATE to set the
                            state for the input device.

  @retval EFI_SUCCESS           The device state was set successfully.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and
                                could not have the setting adjusted.
  @retval EFI_UNSUPPORTED       The device does not have the ability to set
                                its state.
  @retval EFI_INVALID_PARAMETER KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Register a notification function for a particular keystroke for the
  input device.

  @param  This                    Protocol instance pointer.
  @param  KeyData                 A pointer to a buffer that is filled in with
                                  the keystroke information data for the key
                                  that was pressed.
  @param  KeyNotificationFunction Points to the function to be called when the
                                  key sequence is typed specified by KeyData.
  @param  NotifyHandle            Points to the unique handle assigned to the
                                  registered notification.


  @retval EFI_SUCCESS             The notification function was registered
                                  successfully.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate resources for necessary
                                  data structures.
  @retval EFI_INVALID_PARAMETER   KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  EFI_STATUS                            Status;
  VIRTUAL_KEYBOARD_DEV                  *VirtualKeyboardPrivate;
  EFI_TPL                               OldTpl;
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *NewNotify;
  LIST_ENTRY                            *Link;
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *CurrentNotify;

  if (KeyData == NULL ||
      NotifyHandle == NULL ||
      KeyNotificationFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  VirtualKeyboardPrivate = TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already
  // registered.
  //
  for (Link = VirtualKeyboardPrivate->NotifyList.ForwardLink;
       Link != &VirtualKeyboardPrivate->NotifyList;
       Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        Status = EFI_SUCCESS;
        goto Exit;
      }
    }
  }

  //
  // Allocate resource to save the notification function
  //

  NewNotify = (VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *) AllocateZeroPool (sizeof (VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  NewNotify->Signature         = VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&VirtualKeyboardPrivate->NotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle                = NewNotify;
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

  @param  This                 Protocol instance pointer.
  @param  NotificationHandle   The handle of the notification function
                               being unregistered.

  @retval EFI_SUCCESS             The notification function was unregistered
                                  successfully.
  @retval EFI_INVALID_PARAMETER   The NotificationHandle is invalid.

**/
EFI_STATUS
EFIAPI
VirtualKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  EFI_STATUS                            Status;
  VIRTUAL_KEYBOARD_DEV                  *VirtualKeyboardPrivate;
  EFI_TPL                               OldTpl;
  LIST_ENTRY                            *Link;
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *CurrentNotify;

  //
  // Check incoming notification handle
  //
  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *) NotificationHandle)->Signature !=
      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  VirtualKeyboardPrivate = TEXT_INPUT_EX_VIRTUAL_KEYBOARD_DEV_FROM_THIS (This);

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  for (Link = VirtualKeyboardPrivate->NotifyList.ForwardLink;
       Link != &VirtualKeyboardPrivate->NotifyList;
       Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (CurrentNotify == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);

      Status = EFI_SUCCESS;
      goto Exit;
    }
  }

  //
  // Can not find the specified Notification Handle
  //
  Status = EFI_INVALID_PARAMETER;

Exit:
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Timer event handler: read a series of scancodes from 8042
  and put them into memory scancode buffer.
  it read as much scancodes to either fill
  the memory buffer or empty the keyboard buffer.
  It is registered as running under TPL_NOTIFY

  @param Event       The timer event
  @param Context     A KEYBOARD_CONSOLE_IN_DEV pointer

**/
VOID
EFIAPI
VirtualKeyboardTimerHandler (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_TPL                                OldTpl;
  LIST_ENTRY                             *Link;
  EFI_KEY_DATA                           KeyData;
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY  *CurrentNotify;
  VIRTUAL_KEYBOARD_DEV                   *VirtualKeyboardPrivate;
  VIRTUAL_KBD_KEY                        VirtualKey;

  VirtualKeyboardPrivate = Context;

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  if (VirtualKeyboardPrivate->PlatformVirtual &&
      VirtualKeyboardPrivate->PlatformVirtual->Query) {
    if (VirtualKeyboardPrivate->PlatformVirtual->Query (&VirtualKey) ==
        FALSE) {
      goto Exit;
    }
    // Found key
    KeyData.Key.ScanCode = VirtualKey.Key.ScanCode;
    KeyData.Key.UnicodeChar = VirtualKey.Key.UnicodeChar;
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
    KeyData.KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
    if (VirtualKeyboardPrivate->PlatformVirtual->Clear) {
      VirtualKeyboardPrivate->PlatformVirtual->Clear (&VirtualKey);
    }
  } else {
    goto Exit;
  }

  //
  // Signal KeyNotify process event if this key pressed matches any key registered.
  //
  for (Link = VirtualKeyboardPrivate->NotifyList.ForwardLink;
       Link != &VirtualKeyboardPrivate->NotifyList;
       Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
      //
      // The key notification function needs to run at TPL_CALLBACK
      // while current TPL is TPL_NOTIFY. It will be invoked in
      // KeyNotifyProcessHandler() which runs at TPL_CALLBACK.
      //
      Enqueue (&VirtualKeyboardPrivate->QueueForNotify, &KeyData);
      gBS->SignalEvent (VirtualKeyboardPrivate->KeyNotifyProcessEvent);
      break;
    }
  }

  Enqueue (&VirtualKeyboardPrivate->Queue, &KeyData);

Exit:
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}

/**
  Process key notify.

  @param  Event                 Indicates the event that invoke this function.
  @param  Context               Indicates the calling context.
**/
VOID
EFIAPI
KeyNotifyProcessHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  EFI_STATUS                            Status;
  VIRTUAL_KEYBOARD_DEV                  *VirtualKeyboardPrivate;
  EFI_KEY_DATA                          KeyData;
  LIST_ENTRY                            *Link;
  LIST_ENTRY                            *NotifyList;
  VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY *CurrentNotify;
  EFI_TPL                               OldTpl;

  VirtualKeyboardPrivate = (VIRTUAL_KEYBOARD_DEV *) Context;

  //
  // Invoke notification functions.
  //
  NotifyList = &VirtualKeyboardPrivate->NotifyList;
  while (TRUE) {
    //
    // Enter critical section
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    Status = Dequeue (&VirtualKeyboardPrivate->QueueForNotify, &KeyData);
    //
    // Leave critical section
    //
    gBS->RestoreTPL (OldTpl);
    if (EFI_ERROR (Status)) {
      break;
    }
    for (Link = GetFirstNode (NotifyList);
         !IsNull (NotifyList, Link);
         Link = GetNextNode (NotifyList, Link)) {
      CurrentNotify = CR (Link,
                        VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                        NotifyEntry,
                        VIRTUAL_KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                        );
      if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
        CurrentNotify->KeyNotificationFn (&KeyData);
      }
    }
  }
}

/**
  The user Entry Point for module VirtualKeyboard. The user code starts with
  this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeVirtualKeyboard(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gVirtualKeyboardDriverBinding,
             ImageHandle,
             &gVirtualKeyboardComponentName,
             &gVirtualKeyboardComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
