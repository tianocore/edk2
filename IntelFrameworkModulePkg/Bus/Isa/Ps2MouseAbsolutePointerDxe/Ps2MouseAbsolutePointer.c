/** @file
  A faked PS/2 Absolute Pointer driver. Routines that interacts with callers,
  conforming to EFI driver model
  
Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ps2MouseAbsolutePointer.h"
#include "CommPs2.h"

//
// DriverBinding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gPS2MouseAbsolutePointerDriver = {
  PS2MouseAbsolutePointerDriverSupported,
  PS2MouseAbsolutePointerDriverStart,
  PS2MouseAbsolutePointerDriverStop,
  0x1,
  NULL,
  NULL
};

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a IsaIo protocol can be supported.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PS2MouseAbsolutePointerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  Status = EFI_SUCCESS;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Use the ISA I/O Protocol to see if Controller is the Mouse controller
  //
  switch (IsaIo->ResourceList->Device.HID) {
  case EISA_PNP_ID (0xF03):
  //
  // Microsoft PS/2 style mouse
  //
  case EISA_PNP_ID (0xF13):
    //
    // PS/2 Port for PS/2-style Mice
    //
    break;

  case EISA_PNP_ID (0x303):
    //
    // IBM Enhanced (101/102-key, PS/2 mouse support)
    //
    if (IsaIo->ResourceList->Device.UID == 1) {
      break;
    }

  default:
    Status = EFI_UNSUPPORTED;
    break;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Start this driver on ControllerHandle by opening a IsaIo protocol, creating
  PS2_MOUSE_ABSOLUTE_POINTER_DEV device and install gEfiAbsolutePointerProtocolGuid
  finally.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PS2MouseAbsolutePointerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_STATUS                          EmptyStatus;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  PS2_MOUSE_ABSOLUTE_POINTER_DEV     *MouseAbsolutePointerDev;
  UINT8                               Data;
  EFI_TPL                             OldTpl;
  EFI_STATUS_CODE_VALUE               StatusCode;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;

  StatusCode  = 0;
  MouseAbsolutePointerDev    = NULL;
  IsaIo       = NULL;

  //
  // Open the device path protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Report that the keyboard is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE,
    ParentDevicePath
    );

  //
  // Get the ISA I/O Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_INVALID_PARAMETER;
  }
  //
  // Raise TPL to avoid keyboard operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Allocate private data
  //
  MouseAbsolutePointerDev = AllocateZeroPool (sizeof (PS2_MOUSE_ABSOLUTE_POINTER_DEV));
  if (MouseAbsolutePointerDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Setup the device instance
  //
  MouseAbsolutePointerDev->Signature       = PS2_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  MouseAbsolutePointerDev->Handle          = Controller;
  MouseAbsolutePointerDev->SampleRate      = SampleRate20;
  MouseAbsolutePointerDev->Resolution      = MouseResolution4;
  MouseAbsolutePointerDev->Scaling         = Scaling1;
  MouseAbsolutePointerDev->DataPackageSize = 3;
  MouseAbsolutePointerDev->IsaIo           = IsaIo;
  MouseAbsolutePointerDev->DevicePath      = ParentDevicePath;

  //
  // Resolution = 4 counts/mm
  //
  MouseAbsolutePointerDev->Mode.AbsoluteMaxX               = 1024;
  MouseAbsolutePointerDev->Mode.AbsoluteMinX               = 0;
  MouseAbsolutePointerDev->Mode.AbsoluteMaxY               = 798;
  MouseAbsolutePointerDev->Mode.AbsoluteMinY               = 0;
  MouseAbsolutePointerDev->Mode.AbsoluteMaxZ               = 0;
  MouseAbsolutePointerDev->Mode.AbsoluteMinZ               = 0;
  MouseAbsolutePointerDev->Mode.Attributes                 = 0x03;

  MouseAbsolutePointerDev->AbsolutePointerProtocol.Reset     = MouseAbsolutePointerReset;
  MouseAbsolutePointerDev->AbsolutePointerProtocol.GetState  = MouseAbsolutePointerGetState;
  MouseAbsolutePointerDev->AbsolutePointerProtocol.Mode      = &(MouseAbsolutePointerDev->Mode);

  //
  // Initialize keyboard controller if necessary
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (  
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_MOUSE_PC_SELF_TEST,
    ParentDevicePath
    );

  IsaIo->Io.Read (IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);
  if ((Data & KBC_SYSF) != KBC_SYSF) {
    Status = KbcSelfTest (IsaIo);
    if (EFI_ERROR (Status)) {
      StatusCode = EFI_PERIPHERAL_MOUSE | EFI_P_EC_CONTROLLER_ERROR;
      goto ErrorExit;
    }
  }

  KbcEnableAux (IsaIo);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_PRESENCE_DETECT,
    ParentDevicePath
    );

  //
  // Reset the mouse
  //
  Status = MouseAbsolutePointerDev->AbsolutePointerProtocol.Reset (
                     &MouseAbsolutePointerDev->AbsolutePointerProtocol,
                     FeaturePcdGet (PcdPs2MouseExtendedVerification)
                     );
  if (EFI_ERROR (Status)) {
    //
    // mouse not connected
    //
    Status      = EFI_SUCCESS;
    StatusCode  = EFI_PERIPHERAL_MOUSE | EFI_P_EC_NOT_DETECTED;
    goto ErrorExit;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_DETECTED,
    ParentDevicePath
    );

  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  MouseAbsolutePointerWaitForInput,
                  MouseAbsolutePointerDev,
                  &((MouseAbsolutePointerDev->AbsolutePointerProtocol).WaitForInput)
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Setup a periodic timer, used to poll mouse state
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  PollMouseAbsolutePointer,
                  MouseAbsolutePointerDev,
                  &MouseAbsolutePointerDev->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Start timer to poll mouse (100 samples per second)
  //
  Status = gBS->SetTimer (MouseAbsolutePointerDev->TimerEvent, TimerPeriodic, 100000);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  MouseAbsolutePointerDev->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gPs2MouseAbsolutePointerComponentName.SupportedLanguages,
    &MouseAbsolutePointerDev->ControllerNameTable,
    L"Faked PS/2 Touchpad Device",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gPs2MouseAbsolutePointerComponentName2.SupportedLanguages,
    &MouseAbsolutePointerDev->ControllerNameTable,
    L"Faked PS/2 Touchpad Device",
    FALSE
    );


  //
  // Install protocol interfaces for the mouse device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  &MouseAbsolutePointerDev->AbsolutePointerProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;

ErrorExit:

  KbcDisableAux (IsaIo);

  if (StatusCode != 0) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      StatusCode,
      ParentDevicePath
      );
  }

  if ((MouseAbsolutePointerDev != NULL) && (MouseAbsolutePointerDev->AbsolutePointerProtocol.WaitForInput != NULL)) {
    gBS->CloseEvent (MouseAbsolutePointerDev->AbsolutePointerProtocol.WaitForInput);
  }

  if ((MouseAbsolutePointerDev != NULL) && (MouseAbsolutePointerDev->TimerEvent != NULL)) {
    gBS->CloseEvent (MouseAbsolutePointerDev->TimerEvent);
  }

  if ((MouseAbsolutePointerDev != NULL) && (MouseAbsolutePointerDev->ControllerNameTable != NULL)) {
    FreeUnicodeStringTable (MouseAbsolutePointerDev->ControllerNameTable);
  }
  //
  // Since there will be no timer handler for mouse input any more,
  // exhaust input data just in case there is still mouse data left
  //
  EmptyStatus = EFI_SUCCESS;
  while (!EFI_ERROR (EmptyStatus)) {
    EmptyStatus = In8042Data (IsaIo, &Data);
  }

  if (MouseAbsolutePointerDev != NULL) {
    FreePool (MouseAbsolutePointerDev);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->RestoreTPL (OldTpl);

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
PS2MouseAbsolutePointerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                            Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL         *AbsolutePointerProtocol;
  PS2_MOUSE_ABSOLUTE_POINTER_DEV       *MouseAbsolutePointerDev;
  UINT8                                 Data;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  (VOID **) &AbsolutePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  MouseAbsolutePointerDev = PS2_MOUSE_ABSOLUTE_POINTER_DEV_FROM_THIS (AbsolutePointerProtocol);

  //
  // Report that the keyboard is being disabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE,
    MouseAbsolutePointerDev->DevicePath
    );

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  &MouseAbsolutePointerDev->AbsolutePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Cancel mouse data polling timer, close timer event
  //
  gBS->SetTimer (MouseAbsolutePointerDev->TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (MouseAbsolutePointerDev->TimerEvent);

  //
  // Since there will be no timer handler for mouse input any more,
  // exhaust input data just in case there is still mouse data left
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseAbsolutePointerDev->IsaIo, &Data);
  }

  gBS->CloseEvent (MouseAbsolutePointerDev->AbsolutePointerProtocol.WaitForInput);
  FreeUnicodeStringTable (MouseAbsolutePointerDev->ControllerNameTable);
  FreePool (MouseAbsolutePointerDev);

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiIsaIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

/**
  Reset the Mouse and do BAT test for it, if ExtendedVerification is TRUE and there is a mouse device connected to system.

  @param This                 - Pointer of simple pointer Protocol.
  @param ExtendedVerification - Whether configure mouse parameters. True: do; FALSE: skip.


  @retval EFI_SUCCESS         - The command byte is written successfully.
  @retval EFI_DEVICE_ERROR    - Errors occurred during resetting keyboard.

**/
EFI_STATUS
EFIAPI
MouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS                       Status;
  PS2_MOUSE_ABSOLUTE_POINTER_DEV  *MouseAbsolutePointerDev;
  EFI_TPL                          OldTpl;
  BOOLEAN                          KeyboardEnable;
  UINT8                            Data;

  MouseAbsolutePointerDev = PS2_MOUSE_ABSOLUTE_POINTER_DEV_FROM_THIS (This);

  //
  // Report reset progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET,
    MouseAbsolutePointerDev->DevicePath
    );

  KeyboardEnable = FALSE;

  //
  // Raise TPL to avoid keyboard operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  ZeroMem (&MouseAbsolutePointerDev->State, sizeof (EFI_ABSOLUTE_POINTER_STATE));
  MouseAbsolutePointerDev->StateChanged = FALSE;

  //
  // Exhaust input data
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseAbsolutePointerDev->IsaIo, &Data);
  }

  CheckKbStatus (MouseAbsolutePointerDev->IsaIo, &KeyboardEnable);

  KbcDisableKb (MouseAbsolutePointerDev->IsaIo);

  MouseAbsolutePointerDev->IsaIo->Io.Read (MouseAbsolutePointerDev->IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

  //
  // if there's data block on KBC data port, read it out
  //
  if ((Data & KBC_OUTB) == KBC_OUTB) {
    MouseAbsolutePointerDev->IsaIo->Io.Read (MouseAbsolutePointerDev->IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Data);
  }

  Status = EFI_SUCCESS;
  //
  // The PS2 mouse driver reset behavior is always successfully return no matter wheater or not there is mouse connected to system.
  // This behavior is needed by performance speed. The following mouse command only succeessfully finish when mouse device is
  // connected to system, so if PS2 mouse device not connect to system or user not ask for, we skip the mouse configuration and enabling
  //
  if (ExtendedVerification && CheckMouseAbsolutePointerConnect (MouseAbsolutePointerDev)) {
    //
    // Send mouse reset command and set mouse default configure
    //
    Status = PS2MouseReset (MouseAbsolutePointerDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetSampleRate (MouseAbsolutePointerDev->IsaIo, MouseAbsolutePointerDev->SampleRate);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetResolution (MouseAbsolutePointerDev->IsaIo, MouseAbsolutePointerDev->Resolution);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetScaling (MouseAbsolutePointerDev->IsaIo, MouseAbsolutePointerDev->Scaling);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseEnable (MouseAbsolutePointerDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }
Exit:
  gBS->RestoreTPL (OldTpl);

  if (KeyboardEnable) {
    KbcEnableKb (MouseAbsolutePointerDev->IsaIo);
  }

  return Status;
}

/**
  Check whether there is Ps/2 mouse device in system

  @param MouseAbsolutePointerDev - Absolute Pointer Device Private Data Structure

  @retval TRUE                - Keyboard in System.
  @retval FALSE               - Keyboard not in System.

**/
BOOLEAN
CheckMouseAbsolutePointerConnect (
  IN  PS2_MOUSE_ABSOLUTE_POINTER_DEV     *MouseAbsolutePointerDev
  )

{
  EFI_STATUS     Status;

  Status = PS2MouseEnable (MouseAbsolutePointerDev->IsaIo);
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Get and Clear mouse status.
  
  @param This                 - Pointer of simple pointer Protocol.
  @param State                - Output buffer holding status.

  @retval EFI_INVALID_PARAMETER Output buffer is invalid.
  @retval EFI_NOT_READY         Mouse is not changed status yet.
  @retval EFI_SUCCESS           Mouse status is changed and get successful.
**/
EFI_STATUS
EFIAPI
MouseAbsolutePointerGetState (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL     *This,
  IN OUT EFI_ABSOLUTE_POINTER_STATE    *State
  )
{
  PS2_MOUSE_ABSOLUTE_POINTER_DEV *MouseAbsolutePointerDev;
  EFI_TPL       OldTpl;

  MouseAbsolutePointerDev = PS2_MOUSE_ABSOLUTE_POINTER_DEV_FROM_THIS (This);

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!MouseAbsolutePointerDev->StateChanged) {
    return EFI_NOT_READY;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  CopyMem (State, &(MouseAbsolutePointerDev->State), sizeof (EFI_ABSOLUTE_POINTER_STATE));

  //
  // clear mouse state
  //
  MouseAbsolutePointerDev->State.CurrentX = 0;
  MouseAbsolutePointerDev->State.CurrentY = 0;
  MouseAbsolutePointerDev->State.CurrentZ = 0;
  MouseAbsolutePointerDev->State.ActiveButtons = 0x0;
  MouseAbsolutePointerDev->StateChanged            = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**

  Event notification function for SIMPLE_POINTER.WaitForInput event.
  Signal the event if there is input from mouse.

  @param Event    event object
  @param Context  event context

**/
VOID
EFIAPI
MouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  PS2_MOUSE_ABSOLUTE_POINTER_DEV *MouseAbsolutePointerDev;

  MouseAbsolutePointerDev = (PS2_MOUSE_ABSOLUTE_POINTER_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (MouseAbsolutePointerDev->StateChanged) {
    gBS->SignalEvent (Event);
  }

}

/**
  Event notification function for TimerEvent event.
  If mouse device is connected to system, try to get the mouse packet data.

  @param Event      -  TimerEvent in PS2_MOUSE_DEV
  @param Context    -  Pointer to PS2_MOUSE_DEV structure

**/
VOID
EFIAPI
PollMouseAbsolutePointer(
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )

{
  PS2_MOUSE_ABSOLUTE_POINTER_DEV *MouseAbsolutePointerDev;

  MouseAbsolutePointerDev = (PS2_MOUSE_ABSOLUTE_POINTER_DEV *) Context;

  //
  // Polling mouse packet data
  //
  PS2MouseGetPacket (MouseAbsolutePointerDev);
}

/**
  The user Entry Point for module Ps2MouseAbsolutePointer. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializePs2MouseAbsolutePointer(
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
             &gPS2MouseAbsolutePointerDriver,
             ImageHandle,
             &gPs2MouseAbsolutePointerComponentName,
             &gPs2MouseAbsolutePointerComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

