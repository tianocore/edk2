/** @file

  PS/2 Keyboard driver. Routines that interacts with callers,
  conforming to EFI driver model

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ps2Keyboard.h"

//
// Function prototypes
//

/**
  Test controller is a keyboard Controller.

  @param This                 Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller           driver's controller
  @param RemainingDevicePath  children device path

  @retval EFI_UNSUPPORTED controller is not floppy disk
  @retval EFI_SUCCESS     controller is floppy disk
**/
EFI_STATUS
EFIAPI
KbdControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Create KEYBOARD_CONSOLE_IN_DEV instance on controller.

  @param This         Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller   driver controller handle
  @param RemainingDevicePath Children's device path

  @retval whether success to create floppy control instance.
**/
EFI_STATUS
EFIAPI
KbdControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
KbdControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Free the waiting key notify list.

  @param ListHead  Pointer to list head

  @retval EFI_INVALID_PARAMETER  ListHead is NULL
  @retval EFI_SUCCESS            Success to free NotifyList
**/
EFI_STATUS
KbdFreeNotifyList (
  IN OUT LIST_ENTRY  *ListHead
  );

//
// DriverBinding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gKeyboardControllerDriver = {
  KbdControllerDriverSupported,
  KbdControllerDriverStart,
  KbdControllerDriverStop,
  0xa,
  NULL,
  NULL
};

/**
  Test controller is a keyboard Controller.

  @param This                 Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller           driver's controller
  @param RemainingDevicePath  children device path

  @retval EFI_UNSUPPORTED controller is not floppy disk
  @retval EFI_SUCCESS     controller is floppy disk
**/
EFI_STATUS
EFIAPI
KbdControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_SIO_PROTOCOL          *Sio;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  ACPI_HID_DEVICE_PATH      *Acpi;

  //
  // Check whether the controller is keyboard.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  do {
    Acpi       = (ACPI_HID_DEVICE_PATH *)DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } while (!IsDevicePathEnd (DevicePath));

  if ((DevicePathType (Acpi) != ACPI_DEVICE_PATH) ||
      ((DevicePathSubType (Acpi) != ACPI_DP) && (DevicePathSubType (Acpi) != ACPI_EXTENDED_DP)))
  {
    return EFI_UNSUPPORTED;
  }

  if ((Acpi->HID != EISA_PNP_ID (0x303)) || (Acpi->UID != 0)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSioProtocolGuid,
                  (VOID **)&Sio,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiSioProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Create KEYBOARD_CONSOLE_IN_DEV instance on controller.

  @param This         Pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param Controller   driver controller handle
  @param RemainingDevicePath Children's device path

  @retval whether success to create floppy control instance.
**/
EFI_STATUS
EFIAPI
KbdControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                Status1;
  EFI_SIO_PROTOCOL          *Sio;
  KEYBOARD_CONSOLE_IN_DEV   *ConsoleIn;
  UINT8                     Data;
  EFI_STATUS_CODE_VALUE     StatusCode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  StatusCode = 0;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Report that the keyboard is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE,
    DevicePath
    );

  //
  // Get the ISA I/O Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSioProtocolGuid,
                  (VOID **)&Sio,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate private data
  //
  ConsoleIn = AllocateZeroPool (sizeof (KEYBOARD_CONSOLE_IN_DEV));
  if (ConsoleIn == NULL) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  //
  // Setup the device instance
  //
  ConsoleIn->Signature              = KEYBOARD_CONSOLE_IN_DEV_SIGNATURE;
  ConsoleIn->Handle                 = Controller;
  (ConsoleIn->ConIn).Reset          = KeyboardEfiReset;
  (ConsoleIn->ConIn).ReadKeyStroke  = KeyboardReadKeyStroke;
  ConsoleIn->DataRegisterAddress    = KEYBOARD_8042_DATA_REGISTER;
  ConsoleIn->StatusRegisterAddress  = KEYBOARD_8042_STATUS_REGISTER;
  ConsoleIn->CommandRegisterAddress = KEYBOARD_8042_COMMAND_REGISTER;
  ConsoleIn->DevicePath             = DevicePath;

  ConsoleIn->ConInEx.Reset               = KeyboardEfiResetEx;
  ConsoleIn->ConInEx.ReadKeyStrokeEx     = KeyboardReadKeyStrokeEx;
  ConsoleIn->ConInEx.SetState            = KeyboardSetState;
  ConsoleIn->ConInEx.RegisterKeyNotify   = KeyboardRegisterKeyNotify;
  ConsoleIn->ConInEx.UnregisterKeyNotify = KeyboardUnregisterKeyNotify;

  InitializeListHead (&ConsoleIn->NotifyList);

  //
  // Fix for random hangs in System waiting for the Key if no KBC is present in BIOS.
  // When KBC decode (IO port 0x60/0x64 decode) is not enabled,
  // KeyboardRead will read back as 0xFF and return status is EFI_SUCCESS.
  // So instead we read status register to detect after read if KBC decode is enabled.
  //

  //
  // Return code is ignored on purpose.
  //
  if (!PcdGetBool (PcdFastPS2Detection)) {
    KeyboardRead (ConsoleIn, &Data);
    if ((KeyReadStatusRegister (ConsoleIn) & (KBC_PARE | KBC_TIM)) == (KBC_PARE | KBC_TIM)) {
      //
      // If nobody decodes KBC I/O port, it will read back as 0xFF.
      // Check the Time-Out and Parity bit to see if it has an active KBC in system
      //
      Status     = EFI_DEVICE_ERROR;
      StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED;
      goto ErrorExit;
    }
  }

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  KeyboardWaitForKey,
                  ConsoleIn,
                  &((ConsoleIn->ConIn).WaitForKey)
                  );
  if (EFI_ERROR (Status)) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  //
  // Setup the WaitForKeyEx event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  KeyboardWaitForKeyEx,
                  ConsoleIn,
                  &(ConsoleIn->ConInEx.WaitForKeyEx)
                  );
  if (EFI_ERROR (Status)) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  // Setup a periodic timer, used for reading keystrokes at a fixed interval
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  KeyboardTimerHandler,
                  ConsoleIn,
                  &ConsoleIn->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  Status = gBS->SetTimer (
                  ConsoleIn->TimerEvent,
                  TimerPeriodic,
                  KEYBOARD_TIMER_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  KeyNotifyProcessHandler,
                  ConsoleIn,
                  &ConsoleIn->KeyNotifyProcessEvent
                  );
  if (EFI_ERROR (Status)) {
    Status     = EFI_OUT_OF_RESOURCES;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT,
    DevicePath
    );

  //
  // Reset the keyboard device
  //
  Status = ConsoleIn->ConInEx.Reset (&ConsoleIn->ConInEx, FeaturePcdGet (PcdPs2KbdExtendedVerification));
  if (EFI_ERROR (Status)) {
    Status     = EFI_DEVICE_ERROR;
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED;
    goto ErrorExit;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DETECTED,
    DevicePath
    );

  ConsoleIn->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gPs2KeyboardComponentName.SupportedLanguages,
    &ConsoleIn->ControllerNameTable,
    L"PS/2 Keyboard Device",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gPs2KeyboardComponentName2.SupportedLanguages,
    &ConsoleIn->ControllerNameTable,
    L"PS/2 Keyboard Device",
    FALSE
    );

  //
  // Install protocol interfaces for the keyboard device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &ConsoleIn->ConIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &ConsoleIn->ConInEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    StatusCode = EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR;
    goto ErrorExit;
  }

  return Status;

ErrorExit:
  //
  // Report error code
  //
  if (StatusCode != 0) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      StatusCode,
      DevicePath
      );
  }

  if ((ConsoleIn != NULL) && (ConsoleIn->ConIn.WaitForKey != NULL)) {
    gBS->CloseEvent (ConsoleIn->ConIn.WaitForKey);
  }

  if ((ConsoleIn != NULL) && (ConsoleIn->TimerEvent != NULL)) {
    gBS->CloseEvent (ConsoleIn->TimerEvent);
  }

  if ((ConsoleIn != NULL) && (ConsoleIn->ConInEx.WaitForKeyEx != NULL)) {
    gBS->CloseEvent (ConsoleIn->ConInEx.WaitForKeyEx);
  }

  if ((ConsoleIn != NULL) && (ConsoleIn->KeyNotifyProcessEvent != NULL)) {
    gBS->CloseEvent (ConsoleIn->KeyNotifyProcessEvent);
  }

  KbdFreeNotifyList (&ConsoleIn->NotifyList);
  if ((ConsoleIn != NULL) && (ConsoleIn->ControllerNameTable != NULL)) {
    FreeUnicodeStringTable (ConsoleIn->ControllerNameTable);
  }

  //
  // Since there will be no timer handler for keyboard input any more,
  // exhaust input data just in case there is still keyboard data left
  //
  if (ConsoleIn != NULL) {
    Status1 = EFI_SUCCESS;
    while (!EFI_ERROR (Status1) && (Status != EFI_DEVICE_ERROR)) {
      Status1 = KeyboardRead (ConsoleIn, &Data);
    }
  }

  if (ConsoleIn != NULL) {
    gBS->FreePool (ConsoleIn);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiSioProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
KbdControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ConIn;
  KEYBOARD_CONSOLE_IN_DEV         *ConsoleIn;
  UINT8                           Data;

  //
  // Disable Keyboard
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&ConIn,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
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
    return Status;
  }

  ConsoleIn = KEYBOARD_CONSOLE_IN_DEV_FROM_THIS (ConIn);

  //
  // Report that the keyboard is being disabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE,
    ConsoleIn->DevicePath
    );

  if (ConsoleIn->TimerEvent != NULL) {
    gBS->CloseEvent (ConsoleIn->TimerEvent);
    ConsoleIn->TimerEvent = NULL;
  }

  //
  // Since there will be no timer handler for keyboard input any more,
  // exhaust input data just in case there is still keyboard data left
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = KeyboardRead (ConsoleIn, &Data);
  }

  //
  // Uninstall the SimpleTextIn and SimpleTextInEx protocols
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &ConsoleIn->ConIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &ConsoleIn->ConInEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiSioProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Free other resources
  //
  if ((ConsoleIn->ConIn).WaitForKey != NULL) {
    gBS->CloseEvent ((ConsoleIn->ConIn).WaitForKey);
    (ConsoleIn->ConIn).WaitForKey = NULL;
  }

  if (ConsoleIn->ConInEx.WaitForKeyEx != NULL) {
    gBS->CloseEvent (ConsoleIn->ConInEx.WaitForKeyEx);
    ConsoleIn->ConInEx.WaitForKeyEx = NULL;
  }

  if (ConsoleIn->KeyNotifyProcessEvent != NULL) {
    gBS->CloseEvent (ConsoleIn->KeyNotifyProcessEvent);
    ConsoleIn->KeyNotifyProcessEvent = NULL;
  }

  KbdFreeNotifyList (&ConsoleIn->NotifyList);
  FreeUnicodeStringTable (ConsoleIn->ControllerNameTable);
  gBS->FreePool (ConsoleIn);

  return EFI_SUCCESS;
}

/**
  Free the waiting key notify list.

  @param ListHead  Pointer to list head

  @retval EFI_INVALID_PARAMETER  ListHead is NULL
  @retval EFI_SUCCESS            Success to free NotifyList
**/
EFI_STATUS
KbdFreeNotifyList (
  IN OUT LIST_ENTRY  *ListHead
  )
{
  KEYBOARD_CONSOLE_IN_EX_NOTIFY  *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink,
                   KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                   NotifyEntry,
                   KEYBOARD_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    gBS->FreePool (NotifyNode);
  }

  return EFI_SUCCESS;
}

/**
  The module Entry Point for module Ps2Keyboard.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePs2Keyboard (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gKeyboardControllerDriver,
             ImageHandle,
             &gPs2KeyboardComponentName,
             &gPs2KeyboardComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
