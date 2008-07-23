/** @file

  USB Keyboard Driver that includes the implementation of interface.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EfiKey.h"
#include "KeyBoard.h"

/**
  The Usb Keyboard Driver Entry Point.

  @param  ImageHandle       The driver image handle.
  @param  SystemTable       The system table.

  @return EFI_SUCCESS      The component name protocol is installed.
  @return Others           Failed to install.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

/**
  Check whether USB keyboard driver support this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.
**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Start running driver on the controller.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @return Other                  The keyboard driver doesn't support this controller.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop handle the controller by this USB keyboard driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

/**
  Reset Usb Keyboard.

  @param  This                  The protocol instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL.
  @param  ExtendedVerification  Whether completely reset keyboard or not.

  @retval EFI_SUCCESS           Reset keyboard successfully.
  @retval EFI_DEVICE_ERROR      Reset keyboard failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                      ExtendedVerification
  );

/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.ReadKeyStroke() function.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          Read key stroke successfully.
  @retval Other                Read key stroke failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                 *Key
  );

/**
  Handler function for WaitForKey event.

  @param  Event        Event to be signaled when a key is pressed.
  @param  Context      Points to USB_KB_DEV instance.

  @return None.
**/
VOID
EFIAPI
USBKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

/**
  Check whether there is key pending.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

  @retval EFI_SUCCESS          Have key pending to read.
  @retval Other                Parse key failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardCheckForKey (
  IN  USB_KB_DEV      *UsbKeyboardDevice
  );

EFI_GUID  gEfiUsbKeyboardDriverGuid = {
  0xa05f5f78, 0xfb3, 0x4d10, {0x90, 0x90, 0xac, 0x4, 0x6e, 0xeb, 0x7c, 0x3c}
};

/**
  Free keyboard notify list.

  @param  ListHead                The list head.

  @retval EFI_SUCCESS             Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER   ListHead is invalid.

**/
EFI_STATUS
EFIAPI
KbdFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  );  

/**
  Whether the pressed key matches a registered key or not.

  @param  RegsiteredData    A pointer to a buffer that is filled in with the keystroke
                            state data for the key that was registered.
  @param  InputData         A pointer to a buffer that is filled in with the keystroke
                            state data for the key that was pressed.

  @retval TRUE              Key pressed matches a registered key.
  @retval FLASE             Match failed.

**/
BOOLEAN
EFIAPI
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  );


//
// USB Keyboard Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gUsbKeyboardDriverBinding = {
  USBKeyboardDriverBindingSupported,
  USBKeyboardDriverBindingStart,
  USBKeyboardDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The Usb Keyboard Driver Entry Point.

  @param  ImageHandle       The driver image handle.
  @param  SystemTable       The system table.

  @return EFI_SUCCESS      The component name protocol is installed.
  @return Others           Failed to install.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gUsbKeyboardDriverBinding,
           ImageHandle,
           &gUsbKeyboardComponentName,
           &gUsbKeyboardComponentName2
           );
}

/**
  Check whether USB keyboard driver support this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.
**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          OpenStatus;
  EFI_USB_IO_PROTOCOL *UsbIo;
  EFI_STATUS          Status;

  //
  // Check if USB_IO protocol is attached on the controller handle.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                      (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  //
  // Use the USB I/O protocol interface to check whether the Controller is
  // the Keyboard controller that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUSBKeyboard (UsbIo)) {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}

/**
  Start running driver on the controller.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @return Other                  The keyboard driver doesn't support this controller.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_KB_DEV                    *UsbKeyboardDevice;
  UINT8                         EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  UINT8                         Index;
  UINT8                         EndpointAddr;
  UINT8                         PollingInterval;
  UINT8                         PacketSize;
  BOOLEAN                       Found;

  UsbKeyboardDevice = NULL;
  Found             = FALSE;

  //
  // Open USB_IO Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UsbKeyboardDevice = AllocateZeroPool (sizeof (USB_KB_DEV));
  if (UsbKeyboardDevice == NULL) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbKeyboardDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }
  //
  // Report that the usb keyboard is being enabled
  //
  KbdReportStatusCode (
    UsbKeyboardDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueKeyboardEnable)
    );

  //
  // This is pretty close to keyboard detection, so log progress
  //
  KbdReportStatusCode (
    UsbKeyboardDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueKeyboardPresenceDetect)
    );

  //
  // Initialize UsbKeyboardDevice
  //
  UsbKeyboardDevice->UsbIo = UsbIo;

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
          UsbIo,
          &UsbKeyboardDevice->InterfaceDescriptor
          );

  EndpointNumber = UsbKeyboardDevice->InterfaceDescriptor.NumEndpoints;

  for (Index = 0; Index < EndpointNumber; Index++) {

    UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            &EndpointDescriptor
            );

    if ((EndpointDescriptor.Attributes & 0x03) == 0x03) {
      //
      // We only care interrupt endpoint here
      //
      CopyMem(&UsbKeyboardDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
    }
  }

  if (!Found) {
    //
    // No interrupt endpoint found, then return unsupported.
    //
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return EFI_UNSUPPORTED;
  }

  UsbKeyboardDevice->Signature                  = USB_KB_DEV_SIGNATURE;
  UsbKeyboardDevice->SimpleInput.Reset          = USBKeyboardReset;
  UsbKeyboardDevice->SimpleInput.ReadKeyStroke  = USBKeyboardReadKeyStroke;

  UsbKeyboardDevice->SimpleInputEx.Reset               = USBKeyboardResetEx;
  UsbKeyboardDevice->SimpleInputEx.ReadKeyStrokeEx     = USBKeyboardReadKeyStrokeEx;
  UsbKeyboardDevice->SimpleInputEx.SetState            = USBKeyboardSetState;
  UsbKeyboardDevice->SimpleInputEx.RegisterKeyNotify   = USBKeyboardRegisterKeyNotify;
  UsbKeyboardDevice->SimpleInputEx.UnregisterKeyNotify = USBKeyboardUnregisterKeyNotify; 
  
  InitializeListHead (&UsbKeyboardDevice->NotifyList);
  
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx)
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = InitKeyboardLayout (UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInput.WaitForKey)
                  );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  //
  // Install simple txt in protocol interface
  // for the usb keyboard device.
  // Usb keyboard is a hot plug device, and expected to work immediately
  // when plugging into system, so a HotPlugDeviceGuid is installed onto
  // the usb keyboard device handle, to distinguish it from other conventional
  // console devices.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &UsbKeyboardDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &UsbKeyboardDevice->SimpleInputEx,
                  &gEfiHotPlugDeviceGuid,
                  NULL,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  //
  // Reset USB Keyboard Device
  //
  Status = UsbKeyboardDevice->SimpleInput.Reset (
                                            &UsbKeyboardDevice->SimpleInput,
                                            TRUE
                                            );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiSimpleTextInProtocolGuid,
           &UsbKeyboardDevice->SimpleInput,
           &gEfiSimpleTextInputExProtocolGuid,
           &UsbKeyboardDevice->SimpleInputEx,
           &gEfiHotPlugDeviceGuid,
           NULL,
           NULL
           );
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }
  //
  // submit async interrupt transfer
  //
  EndpointAddr    = UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbKeyboardDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    KeyboardHandler,
                    UsbKeyboardDevice
                    );

  if (EFI_ERROR (Status)) {

    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiSimpleTextInProtocolGuid,
           &UsbKeyboardDevice->SimpleInput,
           &gEfiSimpleTextInputExProtocolGuid,
           &UsbKeyboardDevice->SimpleInputEx,
           &gEfiHotPlugDeviceGuid,
           NULL,
           NULL
           );
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    gBS->FreePool (UsbKeyboardDevice);
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    return Status;
  }

  UsbKeyboardDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbKeyboardComponentName.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUsbKeyboardComponentName2.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    FALSE
    );


  return EFI_SUCCESS;

ErrorExit:
  if (UsbKeyboardDevice != NULL) {
    if (UsbKeyboardDevice->SimpleInput.WaitForKey != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    }
    if (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);
    }
    KbdFreeNotifyList (&UsbKeyboardDevice->NotifyList);    
    gBS->FreePool (UsbKeyboardDevice);
    UsbKeyboardDevice = NULL;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;

}


/**
  Stop handle the controller by this USB keyboard driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleInput;
  USB_KB_DEV                  *UsbKeyboardDevice;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **) &SimpleInput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Get USB_KB_DEV instance.
  //
  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (SimpleInput);

  gBS->CloseProtocol (
        Controller,
        &gEfiSimpleTextInProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  //
  // Uninstall the Asyn Interrupt Transfer from this device
  // will disable the key data input from this device
  //
  KbdReportStatusCode (
    UsbKeyboardDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueKeyboardDisable)
    );

  //
  // Destroy asynchronous interrupt transfer
  //
  UsbKeyboardDevice->UsbIo->UsbAsyncInterruptTransfer (
                              UsbKeyboardDevice->UsbIo,
                              UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                              FALSE,
                              UsbKeyboardDevice->IntEndpointDescriptor.Interval,
                              0,
                              NULL,
                              NULL
                              );

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &UsbKeyboardDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &UsbKeyboardDevice->SimpleInputEx,
                  &gEfiHotPlugDeviceGuid,
                  NULL,
                  NULL
                  );
  //
  // free all the resources.
  //
  gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
  gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
  gBS->CloseEvent ((UsbKeyboardDevice->SimpleInput).WaitForKey);
  gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);  
  KbdFreeNotifyList (&UsbKeyboardDevice->NotifyList);    

  ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
  gBS->CloseEvent (UsbKeyboardDevice->KeyboardLayoutEvent);

  if (UsbKeyboardDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbKeyboardDevice->ControllerNameTable);
  }

  gBS->FreePool (UsbKeyboardDevice);

  return Status;

}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

  @param  UsbKeyboardDevice       Usb keyboard's private structure.
  @param  KeyData                 A pointer to a buffer that is filled in with the keystroke
                                  state data for the key that was pressed.

  @return EFI_SUCCESS             The keystroke information was returned.
  @return EFI_NOT_READY           There was no keystroke data availiable.
  @return EFI_DEVICE_ERROR        The keystroke information was not returned due to
                                  hardware errors.
  @return EFI_INVALID_PARAMETER   KeyData is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStrokeWorker (
  IN  USB_KB_DEV                        *UsbKeyboardDevice,
  OUT EFI_KEY_DATA                      *KeyData
  )
{

  EFI_STATUS                        Status;
  UINT8                             KeyChar;  
  LIST_ENTRY                        *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;  
  EFI_KEY_DATA                      OriginalKeyData;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if there is no saved ASCII byte, fetch it
  // by calling USBKeyboardCheckForKey().
  //
  if (UsbKeyboardDevice->CurKeyChar == 0) {
    Status = USBKeyboardCheckForKey (UsbKeyboardDevice);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  KeyData->Key.UnicodeChar = 0;
  KeyData->Key.ScanCode    = SCAN_NULL;

  KeyChar = UsbKeyboardDevice->CurKeyChar;

  UsbKeyboardDevice->CurKeyChar = 0;

  //
  // Translate saved ASCII byte into EFI_INPUT_KEY
  //
  Status = USBKeyCodeToEFIScanCode (UsbKeyboardDevice, KeyChar, &KeyData->Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (&KeyData->KeyState, &UsbKeyboardDevice->KeyState, sizeof (KeyData->KeyState));
  
  UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  //
  //Switch the control value to their original characters. In USBKeyCodeToEFIScanCode() the  CTRL-Alpha characters have been switched to 
  // their corresponding control value (ctrl-a = 0x0001 through ctrl-Z = 0x001A), here switch them back for notification function.
  //
  CopyMem (&OriginalKeyData, KeyData, sizeof (EFI_KEY_DATA));
  if (UsbKeyboardDevice->CtrlOn != 0) {
    if (OriginalKeyData.Key.UnicodeChar >= 0x01 && OriginalKeyData.Key.UnicodeChar <= 0x1A) {
      if (UsbKeyboardDevice->CapsOn != 0) {
        OriginalKeyData.Key.UnicodeChar = (CHAR16)(OriginalKeyData.Key.UnicodeChar + 'A' - 1);
      } else {
        OriginalKeyData.Key.UnicodeChar = (CHAR16)(OriginalKeyData.Key.UnicodeChar + 'a' - 1);
      } 
    }
  }
  
  //
  // Invoke notification functions if exist
  //
  for (Link = UsbKeyboardDevice->NotifyList.ForwardLink; Link != &UsbKeyboardDevice->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, &OriginalKeyData)) { 
      CurrentNotify->KeyNotificationFn (&OriginalKeyData);
    }
  }

  return EFI_SUCCESS;
  
}

/**
  Reset Usb Keyboard.

  @param  This                  The protocol instance of EFI_SIMPLE_TEXT_INPUT_PROTOCOL.
  @param  ExtendedVerification  Whether completely reset keyboard or not.

  @retval EFI_SUCCESS           Reset keyboard successfully.
  @retval EFI_DEVICE_ERROR      Reset keyboard failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                       ExtendedVerification
  )
{
  EFI_STATUS          Status;
  USB_KB_DEV          *UsbKeyboardDevice;

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (This);

  KbdReportStatusCode (
    UsbKeyboardDevice->DevicePath,
    EFI_PROGRESS_CODE,
        PcdGet32 (PcdStatusCodeValueKeyboardReset)
    );

  //
  // Non Exhaustive reset:
  // only reset private data structures.
  //
  if (!ExtendedVerification) {
    //
    // Clear the key buffer of this Usb keyboard
    //
    KbdReportStatusCode (
      UsbKeyboardDevice->DevicePath,
      EFI_PROGRESS_CODE,
      PcdGet32 (PcdStatusCodeValueKeyboardClearBuffer)
      );

    InitUSBKeyBuffer (&(UsbKeyboardDevice->KeyboardBuffer));
    UsbKeyboardDevice->CurKeyChar = 0;
    return EFI_SUCCESS;
  }

  //
  // Exhaustive reset
  //
  Status                        = InitUSBKeyboard (UsbKeyboardDevice);
  UsbKeyboardDevice->CurKeyChar = 0;
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Implements EFI_SIMPLE_TEXT_INPUT_PROTOCOL.ReadKeyStroke() function.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          Read key stroke successfully.
  @retval Other                Read key stroke failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                 *Key
  )
{
  USB_KB_DEV   *UsbKeyboardDevice;
  EFI_STATUS   Status;
  EFI_KEY_DATA KeyData;

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (This);

  Status = USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, &KeyData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));

  return EFI_SUCCESS;

}


/**
  Handler function for WaitForKey event.

  @param  Event        Event to be signaled when a key is pressed.
  @param  Context      Points to USB_KB_DEV instance.

  @return None.
**/
VOID
EFIAPI
USBKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  if (UsbKeyboardDevice->CurKeyChar == 0) {

    if (EFI_ERROR (USBKeyboardCheckForKey (UsbKeyboardDevice))) {
      return ;
    }
  }
  //
  // If has key pending, signal the event.
  //
  gBS->SignalEvent (Event);
}


/**
  Check whether there is key pending.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

  @retval EFI_SUCCESS          Have key pending to read.
  @retval Other                Parse key failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardCheckForKey (
  IN  USB_KB_DEV    *UsbKeyboardDevice
  )
{
  EFI_STATUS  Status;
  UINT8       KeyChar;

  //
  // Fetch raw data from the USB keyboard input,
  // and translate it into ASCII data.
  //
  Status = USBParseKey (UsbKeyboardDevice, &KeyChar);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UsbKeyboardDevice->CurKeyChar = KeyChar;
  return EFI_SUCCESS;
}

/**
  Report Status Code in Usb Keyboard Driver.

  @param  DevicePath            Use this to get Device Path.
  @param  CodeType              Status Code Type.
  @param  CodeValue             Status Code Value.

  @return None.

**/
VOID
EFIAPI
KbdReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  )
{

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    CodeType,
    Value,
    DevicePath
    );
}

/**
  Free keyboard notify list.

  @param  ListHead                The list head.

  @retval EFI_SUCCESS             Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER   ListHead is invalid.

**/
EFI_STATUS
EFIAPI
KbdFreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  )
{
  KEYBOARD_CONSOLE_IN_EX_NOTIFY *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink, 
                   KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                   NotifyEntry, 
                   USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    gBS->FreePool (NotifyNode);
  }
  
  return EFI_SUCCESS;
}

/**
  Whether the pressed key matches a registered key or not.

  @param  RegsiteredData    A pointer to a buffer that is filled in with the keystroke
                            state data for the key that was registered.
  @param  InputData         A pointer to a buffer that is filled in with the keystroke
                            state data for the key that was pressed.

  @retval TRUE              Key pressed matches a registered key.
  @retval FLASE             Match failed.

**/
BOOLEAN
EFIAPI
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

//
// Simple Text Input Ex protocol functions 
//
/**
  The extension routine to reset the input device.

  @param This                     Protocol instance pointer.
  @param ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS             The device was reset.
  @retval EFI_DEVICE_ERROR        The device is not functioning properly and could
                                  not be reset.

**/
EFI_STATUS
EFIAPI
USBKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  EFI_STATUS                Status;
  USB_KB_DEV                *UsbKeyboardDevice;
  EFI_TPL                   OldTpl;
  

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  Status = UsbKeyboardDevice->SimpleInput.Reset (&UsbKeyboardDevice->SimpleInput, ExtendedVerification);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

  @param  This                    Protocol instance pointer.
  @param  KeyData                 A pointer to a buffer that is filled in with the keystroke
                                  state data for the key that was pressed.

  @return EFI_SUCCESS             The keystroke information was returned successfully.
  @retval EFI_INVALID_PARAMETER   KeyData is NULL.
  @retval Other                   Read key stroke information failed.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  return USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, KeyData);
  
}

/**
  Set certain state for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyToggleState          A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                  state for the input device.

  @retval EFI_SUCCESS             The device state was set successfully.
  @retval EFI_UNSUPPORTED         The device does not have the ability to set its state.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  if (((UsbKeyboardDevice->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) ||
      ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Update the status light
  //

  UsbKeyboardDevice->ScrollOn   = 0;
  UsbKeyboardDevice->NumLockOn  = 0;
  UsbKeyboardDevice->CapsOn     = 0;
 
  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    UsbKeyboardDevice->ScrollOn = 1;
  }
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    UsbKeyboardDevice->NumLockOn = 1;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    UsbKeyboardDevice->CapsOn = 1;
  }

  SetKeyLED (UsbKeyboardDevice);

  UsbKeyboardDevice->KeyState.KeyToggleState = *KeyToggleState;

  return EFI_SUCCESS;
  
}

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with the keystroke
                                      information data for the key that was pressed.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necesssary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT EFI_HANDLE                         *NotifyHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  EFI_STATUS                        Status;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *NewNotify;
  LIST_ENTRY                        *Link;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;  

  if (KeyData == NULL || NotifyHandle == NULL || KeyNotificationFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  for (Link = UsbKeyboardDevice->NotifyList.ForwardLink; Link != &UsbKeyboardDevice->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) { 
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify->NotifyHandle;        
        return EFI_SUCCESS;
      }
    }
  }
  
  //
  // Allocate resource to save the notification function
  //  
  NewNotify = (KEYBOARD_CONSOLE_IN_EX_NOTIFY *) AllocateZeroPool (sizeof (KEYBOARD_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE;     
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&UsbKeyboardDevice->NotifyList, &NewNotify->NotifyEntry);

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
  
  *NotifyHandle = NewNotify->NotifyHandle;  
  
  return EFI_SUCCESS;
  
}

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid or opening gSimpleTextInExNotifyGuid
                                   on NotificationHandle fails.
  @retval EFI_NOT_FOUND            Can not find the matching entry in database.

**/
EFI_STATUS
EFIAPI
USBKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_HANDLE                         NotificationHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  EFI_STATUS                        Status;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;
  LIST_ENTRY                        *Link;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }  
  
  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);
  
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

  for (Link = UsbKeyboardDevice->NotifyList.ForwardLink; Link != &UsbKeyboardDevice->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = CR (
                      Link, 
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY, 
                      NotifyEntry, 
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
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
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;  
}

