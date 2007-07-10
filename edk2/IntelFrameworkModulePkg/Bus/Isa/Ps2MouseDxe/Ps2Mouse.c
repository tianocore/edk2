/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Ps2Mouse.c

Abstract:

  PS/2 Mouse driver. Routines that interacts with callers,
  conforming to EFI driver model

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "Ps2Mouse.h"
#include "CommPs2.h"

//
// DriverBinding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gPS2MouseDriver = {
  PS2MouseDriverSupported,
  PS2MouseDriverStart,
  PS2MouseDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
PS2MouseDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

Routine Description:

  ControllerDriver Protocol Method

Arguments:

Returns:

--*/
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    Controller - add argument and description to function comment
// GC_TODO:    RemainingDevicePath - add argument and description to function comment
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
  // Use the ISA I/O Protocol to see if Controller is the Keyboard controller
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

EFI_STATUS
EFIAPI
PS2MouseDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

Routine Description:
    Start protocol interfaces for the mouse device handles.

Arguments:
    This                               - Protocol instance pointer.
    Controller                      - Handle of device to bind driver to.
    RemainingDevicePath  - Not used.

Returns:
    EFI_SUCCESS             - This driver is added to DeviceHandle.
    other                               - Errors occurred.

--*/
{
  EFI_STATUS                          Status;
  EFI_STATUS                          EmptyStatus;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  PS2_MOUSE_DEV                       *MouseDev;
  UINT8                               Data;
  EFI_TPL                             OldTpl;
  EFI_STATUS_CODE_VALUE               StatusCode;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;

  StatusCode  = 0;
  MouseDev    = NULL;
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
  MouseDev = AllocateZeroPool (sizeof (PS2_MOUSE_DEV));
  if (MouseDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Setup the device instance
  //
  MouseDev->Signature       = PS2_MOUSE_DEV_SIGNATURE;
  MouseDev->Handle          = Controller;
  MouseDev->SampleRate      = SSR_20;
  MouseDev->Resolution      = CMR4;
  MouseDev->Scaling         = SF1;
  MouseDev->DataPackageSize = 3;
  MouseDev->IsaIo           = IsaIo;
  MouseDev->DevicePath      = ParentDevicePath;

  //
  // Resolution = 4 counts/mm
  //
  MouseDev->Mode.ResolutionX                = 4;
  MouseDev->Mode.ResolutionY                = 4;
  MouseDev->Mode.LeftButton                 = TRUE;
  MouseDev->Mode.RightButton                = TRUE;

  MouseDev->SimplePointerProtocol.Reset     = MouseReset;
  MouseDev->SimplePointerProtocol.GetState  = MouseGetState;
  MouseDev->SimplePointerProtocol.Mode      = &(MouseDev->Mode);

  //
  // Initialize keyboard controller if necessary
  //
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
  Status = MouseDev->SimplePointerProtocol.Reset (&MouseDev->SimplePointerProtocol, TRUE);
  if (EFI_ERROR (Status)) {
    //
    // mouse not connected
    //
    Status      = EFI_SUCCESS;
    StatusCode  = EFI_PERIPHERAL_MOUSE | EFI_P_EC_NOT_DETECTED;
    goto ErrorExit;
  }
  //
  // Setup the WaitForKey event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  MouseWaitForInput,
                  MouseDev,
                  &((MouseDev->SimplePointerProtocol).WaitForInput)
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
                  PollMouse,
                  MouseDev,
                  &MouseDev->TimerEvent
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Start timer to poll mouse (100 samples per second)
  //
  Status = gBS->SetTimer (MouseDev->TimerEvent, TimerPeriodic, 100000);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  MouseDev->ControllerNameTable = NULL;
  AddUnicodeString (
    "eng",
    gPs2MouseComponentName.SupportedLanguages,
    &MouseDev->ControllerNameTable,
    L"PS/2 Mouse Device"
    );

  //
  // Install protocol interfaces for the mouse device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &MouseDev->SimplePointerProtocol,
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

  if ((MouseDev != NULL) && (MouseDev->SimplePointerProtocol.WaitForInput != NULL)) {
    gBS->CloseEvent (MouseDev->SimplePointerProtocol.WaitForInput);
  }

  if ((MouseDev != NULL) && (MouseDev->TimerEvent != NULL)) {
    gBS->CloseEvent (MouseDev->TimerEvent);
  }

  if ((MouseDev != NULL) && (MouseDev->ControllerNameTable != NULL)) {
    FreeUnicodeStringTable (MouseDev->ControllerNameTable);
  }
  //
  // Since there will be no timer handler for mouse input any more,
  // exhaust input data just in case there is still mouse data left
  //
  EmptyStatus = EFI_SUCCESS;
  while (!EFI_ERROR (EmptyStatus)) {
    EmptyStatus = In8042Data (IsaIo, &Data);
  }

  if (MouseDev != NULL) {
    gBS->FreePool (MouseDev);
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

EFI_STATUS
EFIAPI
PS2MouseDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN UINTN                          NumberOfChildren,
  IN EFI_HANDLE                     *ChildHandleBuffer
  )
/*++

  Routine Description:

  Arguments:

  Returns:

--*/
// GC_TODO:    This - add argument and description to function comment
// GC_TODO:    Controller - add argument and description to function comment
// GC_TODO:    NumberOfChildren - add argument and description to function comment
// GC_TODO:    ChildHandleBuffer - add argument and description to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
// GC_TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS                  Status;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointerProtocol;
  PS2_MOUSE_DEV               *MouseDev;
  UINT8                       Data;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  (VOID **) &SimplePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (SimplePointerProtocol);

  //
  // Report that the keyboard is being disabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE,
    MouseDev->DevicePath
    );

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &MouseDev->SimplePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Disable mouse on keyboard controller
  //
  KbcDisableAux (MouseDev->IsaIo);

  //
  // Cancel mouse data polling timer, close timer event
  //
  gBS->SetTimer (MouseDev->TimerEvent, TimerCancel, 0);
  gBS->CloseEvent (MouseDev->TimerEvent);

  //
  // Since there will be no timer handler for mouse input any more,
  // exhaust input data just in case there is still mouse data left
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseDev->IsaIo, &Data);
  }

  gBS->CloseEvent (MouseDev->SimplePointerProtocol.WaitForInput);
  FreeUnicodeStringTable (MouseDev->ControllerNameTable);
  gBS->FreePool (MouseDev);

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

EFI_STATUS
EFIAPI
MouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
/*++

Routine Description:

  Reset the Mouse and do BAT test for it, if ExtendedVerification isTRUE and there is a mouse device connectted to system

Arguments:

  This                 - Pointer of simple pointer Protocol.
  ExtendedVerification - Whether configure mouse parameters. True: do; FALSE: skip.

Returns:

 EFI_SUCCESS         - The command byte is written successfully.
 EFI_DEVICE_ERROR    - Errors occurred during reseting keyboard.

--*/
{
  EFI_STATUS    Status;
  PS2_MOUSE_DEV *MouseDev;
  EFI_TPL       OldTpl;
  BOOLEAN       KeyboardEnable;
  UINT8         Data;

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (This);

  //
  // Report reset progress code
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET,
    MouseDev->DevicePath
    );

  KeyboardEnable = FALSE;

  //
  // Raise TPL to avoid keyboard operation impact
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  ZeroMem (&MouseDev->State, sizeof (EFI_SIMPLE_POINTER_STATE));
  MouseDev->StateChanged = FALSE;

  //
  // Exhaust input data
  //
  Status = EFI_SUCCESS;
  while (!EFI_ERROR (Status)) {
    Status = In8042Data (MouseDev->IsaIo, &Data);
  }

  CheckKbStatus (MouseDev->IsaIo, &KeyboardEnable);

  KbcDisableKb (MouseDev->IsaIo);

  MouseDev->IsaIo->Io.Read (MouseDev->IsaIo, EfiIsaIoWidthUint8, KBC_CMD_STS_PORT, 1, &Data);

  //
  // if there's data block on KBC data port, read it out
  //
  if ((Data & KBC_OUTB) == KBC_OUTB) {
    MouseDev->IsaIo->Io.Read (MouseDev->IsaIo, EfiIsaIoWidthUint8, KBC_DATA_PORT, 1, &Data);
  }

  Status = EFI_SUCCESS;
  //
  // The PS2 mouse driver reset behavior is always successfully return no matter wheater or not there is mouse connected to system.
  // This behavior is needed by performance speed. The following mouse command only succeessfully finish when mouse device is
  // connected to system, so if PS2 mouse device not connect to system or user not ask for, we skip the mouse configuration and enabling
  //
  if (ExtendedVerification && CheckMouseConnect (MouseDev)) {
    //
    // Send mouse reset command and set mouse default configure
    //
    Status = PS2MouseReset (MouseDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetSampleRate (MouseDev->IsaIo, MouseDev->SampleRate);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetResolution (MouseDev->IsaIo, MouseDev->Resolution);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseSetScaling (MouseDev->IsaIo, MouseDev->Scaling);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Status = PS2MouseEnable (MouseDev->IsaIo);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }
  }
Exit:
  gBS->RestoreTPL (OldTpl);

  if (KeyboardEnable) {
    KbcEnableKb (MouseDev->IsaIo);
  }

  return Status;
}

BOOLEAN
CheckMouseConnect (
  IN  PS2_MOUSE_DEV     *MouseDev
  )
/*++

Routine Description:

  Check whether there is Ps/2 mouse device in system

Arguments:

  PS2_MOUSE_DEV - Mouse Private Data Structure

Returns:

  TRUE                - Keyboard in System.
  FALSE               - Keyboard not in System.

--*/
{
  EFI_STATUS     Status;

  Status = PS2MouseEnable (MouseDev->IsaIo);
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
EFIAPI
MouseGetState (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN OUT EFI_SIMPLE_POINTER_STATE   *State
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  This  - GC_TODO: add argument description
  State - GC_TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - GC_TODO: Add description for return value
  EFI_NOT_READY - GC_TODO: Add description for return value
  EFI_SUCCESS - GC_TODO: Add description for return value

--*/
{
  PS2_MOUSE_DEV *MouseDev;
  EFI_TPL       OldTpl;

  MouseDev = PS2_MOUSE_DEV_FROM_THIS (This);

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!MouseDev->StateChanged) {
    return EFI_NOT_READY;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  CopyMem (State, &(MouseDev->State), sizeof (EFI_SIMPLE_POINTER_STATE));

  //
  // clear mouse state
  //
  MouseDev->State.RelativeMovementX = 0;
  MouseDev->State.RelativeMovementY = 0;
  MouseDev->State.RelativeMovementZ = 0;
  MouseDev->StateChanged            = FALSE;
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

VOID
EFIAPI
MouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  Event notification function for SIMPLE_POINTER.WaitForInput event
  Signal the event if there is input from mouse

Arguments:

Returns:

--*/
// GC_TODO:    Event - add argument and description to function comment
// GC_TODO:    Context - add argument and description to function comment
{
  PS2_MOUSE_DEV *MouseDev;

  MouseDev = (PS2_MOUSE_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (MouseDev->StateChanged) {
    gBS->SignalEvent (Event);
  }

}

VOID
EFIAPI
PollMouse (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
/*++

Routine Description:

  Event notification function for TimerEvent event
  If mouse device is connected to system, try to get the mouse packet data

Arguments:

  Event      -  TimerEvent in PS2_MOUSE_DEV
  Context  -  Pointer to PS2_MOUSE_DEV structure

Returns:

  None

--*/
{
  PS2_MOUSE_DEV *MouseDev;

  MouseDev = (PS2_MOUSE_DEV *) Context;

  //
  // Polling mouse packet data
  //
  PS2MouseGetPacket (MouseDev);
}
