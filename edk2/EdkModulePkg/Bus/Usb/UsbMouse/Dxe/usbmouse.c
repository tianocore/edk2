/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbMouse.c

  Abstract:

--*/

#include "usbmouse.h"
#include "mousehid.h"

EFI_DRIVER_BINDING_PROTOCOL gUsbMouseDriverBinding = {
  USBMouseDriverBindingSupported,
  USBMouseDriverBindingStart,
  USBMouseDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// helper functions
//
STATIC
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  );

STATIC
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_DEV           *UsbMouseDev
  );

STATIC
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Mouse interrupt handler
//
STATIC
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  );

//
// Mouse Protocol
//
STATIC
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  );

STATIC
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

EFI_STATUS
EFIAPI
USBMouseDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle. Any ControllerHandle
    that has UsbHcProtocol installed will be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller         - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS          OpenStatus;
  EFI_USB_IO_PROTOCOL *UsbIo;
  EFI_STATUS          Status;

  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                      (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus) && (OpenStatus != EFI_ALREADY_STARTED)) {
    return EFI_UNSUPPORTED;
  }

  if (OpenStatus == EFI_ALREADY_STARTED) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Use the USB I/O protocol interface to see the Controller is
  // the Mouse controller that can be managed by this driver.
  //
  Status = EFI_SUCCESS;
  if (!IsUsbMouse (UsbIo)) {
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

EFI_STATUS
EFIAPI
USBMouseDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:
    Starting the Usb Bus Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
                          Error
    EFI_OUT_OF_RESOURCES- Can't allocate memory resources
    EFI_ALREADY_STARTED - Thios driver has been started
--*/
{
  EFI_STATUS                  Status;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR *EndpointDesc;
  USB_MOUSE_DEV               *UsbMouseDevice;
  UINT8                       EndpointNumber;
  UINT8                       Index;
  UINT8                       EndpointAddr;
  UINT8                       PollingInterval;
  UINT8                       PacketSize;

  UsbMouseDevice  = NULL;
  Status          = EFI_SUCCESS;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbMouseDevice = AllocateZeroPool (sizeof (USB_MOUSE_DEV));
  if (UsbMouseDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbMouseDevice->UsbIo               = UsbIo;

  UsbMouseDevice->Signature           = USB_MOUSE_DEV_SIGNATURE;

  UsbMouseDevice->InterfaceDescriptor = AllocatePool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));
  if (UsbMouseDevice->InterfaceDescriptor == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  EndpointDesc = AllocatePool (sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));
  if (EndpointDesc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbMouseDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }
  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
          UsbIo,
          UsbMouseDevice->InterfaceDescriptor
          );

  EndpointNumber = UsbMouseDevice->InterfaceDescriptor->NumEndpoints;

  for (Index = 0; Index < EndpointNumber; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            EndpointDesc
            );

    if ((EndpointDesc->Attributes & 0x03) == 0x03) {

      //
      // We only care interrupt endpoint here
      //
      UsbMouseDevice->IntEndpointDescriptor = EndpointDesc;
    }
  }

  if (UsbMouseDevice->IntEndpointDescriptor == NULL) {
    //
    // No interrupt endpoint, then error
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  Status = InitializeUsbMouseDevice (UsbMouseDevice);
  if (EFI_ERROR (Status)) {
    MouseReportStatusCode (
      UsbMouseDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR)
      );

    goto ErrorExit;
  }

  UsbMouseDevice->SimplePointerProtocol.GetState  = GetMouseState;
  UsbMouseDevice->SimplePointerProtocol.Reset     = UsbMouseReset;
  UsbMouseDevice->SimplePointerProtocol.Mode      = &UsbMouseDevice->Mode;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  UsbMouseWaitForInput,
                  UsbMouseDevice,
                  &((UsbMouseDevice->SimplePointerProtocol).WaitForInput)
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiSimplePointerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbMouseDevice->SimplePointerProtocol
                  );

  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto ErrorExit;
  }

  //
  // After Enabling Async Interrupt Transfer on this mouse Device
  // we will be able to get key data from it. Thus this is deemed as
  // the enable action of the mouse
  //

  MouseReportStatusCode (
    UsbMouseDevice->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE)
    );

  //
  // submit async interrupt transfer
  //
  EndpointAddr    = UsbMouseDevice->IntEndpointDescriptor->EndpointAddress;
  PollingInterval = UsbMouseDevice->IntEndpointDescriptor->Interval;
  PacketSize      = (UINT8) (UsbMouseDevice->IntEndpointDescriptor->MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseInterruptComplete,
                    UsbMouseDevice
                    );

  if (!EFI_ERROR (Status)) {

    UsbMouseDevice->ControllerNameTable = NULL;
    AddUnicodeString (
      "eng",
      gUsbMouseComponentName.SupportedLanguages,
      &UsbMouseDevice->ControllerNameTable,
      (CHAR16 *) L"Generic Usb Mouse"
      );

    return EFI_SUCCESS;
  }

  //
  // If submit error, uninstall that interface
  //
  Status = EFI_DEVICE_ERROR;
  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiSimplePointerProtocolGuid,
        &UsbMouseDevice->SimplePointerProtocol
        );

ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseDevice != NULL) {
      if (UsbMouseDevice->InterfaceDescriptor != NULL) {
        gBS->FreePool (UsbMouseDevice->InterfaceDescriptor);
      }

      if (UsbMouseDevice->IntEndpointDescriptor != NULL) {
        gBS->FreePool (UsbMouseDevice->IntEndpointDescriptor);
      }

      if ((UsbMouseDevice->SimplePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseDevice->SimplePointerProtocol).WaitForInput);
      }

      gBS->FreePool (UsbMouseDevice);
      UsbMouseDevice = NULL;
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
USBMouseDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop this driver on ControllerHandle. Support stoping any child handles
    created by this driver.

  Arguments:
    This              - Protocol instance pointer.
    Controller        - Handle of device to stop driver on
    NumberOfChildren  - Number of Children in the ChildHandleBuffer
    ChildHandleBuffer - List of handles for the children we need to stop.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    others

--*/
{
  EFI_STATUS                  Status;
  USB_MOUSE_DEV               *UsbMouseDevice;
  EFI_SIMPLE_POINTER_PROTOCOL *SimplePointerProtocol;
  EFI_USB_IO_PROTOCOL         *UsbIo;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  (VOID **) &SimplePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbMouseDevice = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (SimplePointerProtocol);

  gBS->CloseProtocol (
        Controller,
        &gEfiSimplePointerProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  UsbIo = UsbMouseDevice->UsbIo;

  //
  // Uninstall the Asyn Interrupt Transfer from this device
  // will disable the mouse data input from this device
  //
  MouseReportStatusCode (
    UsbMouseDevice->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE)
    );

  //
  // Delete Mouse Async Interrupt Transfer
  //
  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseDevice->IntEndpointDescriptor->EndpointAddress,
          FALSE,
          UsbMouseDevice->IntEndpointDescriptor->Interval,
          0,
          NULL,
          NULL
          );

  gBS->CloseEvent (UsbMouseDevice->SimplePointerProtocol.WaitForInput);

  if (UsbMouseDevice->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseDevice->DelayedRecoveryEvent);
    UsbMouseDevice->DelayedRecoveryEvent = 0;
  }

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  &UsbMouseDevice->SimplePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  gBS->FreePool (UsbMouseDevice->InterfaceDescriptor);
  gBS->FreePool (UsbMouseDevice->IntEndpointDescriptor);

  if (UsbMouseDevice->ControllerNameTable) {
    FreeUnicodeStringTable (UsbMouseDevice->ControllerNameTable);
  }

  gBS->FreePool (UsbMouseDevice);

  return EFI_SUCCESS;

}

BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  )
/*++

  Routine Description:
    Tell if a Usb Controller is a mouse

  Arguments:
    UsbIo              - Protocol instance pointer.

  Returns:
    TRUE              - It is a mouse
    FALSE             - It is not a mouse
--*/
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the Default interface descriptor, now we only
  // suppose it is interface 1
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((InterfaceDescriptor.InterfaceClass == CLASS_HID) &&
      (InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT) &&
      (InterfaceDescriptor.InterfaceProtocol == PROTOCOL_MOUSE)
      ) {
    return TRUE;
  }

  return FALSE;
}

STATIC
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_DEV           *UsbMouseDev
  )
/*++

  Routine Description:
    Initialize the Usb Mouse Device.

  Arguments:
    UsbMouseDev         - Device instance to be initialized

  Returns:
    EFI_SUCCESS         - Success
    EFI_DEVICE_ERROR    - Init error.
    EFI_OUT_OF_RESOURCES- Can't allocate memory
--*/
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   Protocol;
  EFI_STATUS              Status;
  EFI_USB_HID_DESCRIPTOR  MouseHidDesc;
  UINT8                   *ReportDesc;

  UsbIo = UsbMouseDev->UsbIo;

  //
  // Get HID descriptor
  //
  Status = UsbGetHidDescriptor (
            UsbIo,
            UsbMouseDev->InterfaceDescriptor->InterfaceNumber,
            &MouseHidDesc
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Report descriptor
  //
  if (MouseHidDesc.HidClassDesc[0].DescriptorType != 0x22) {
    return EFI_UNSUPPORTED;
  }

  ReportDesc = AllocateZeroPool (MouseHidDesc.HidClassDesc[0].DescriptorLength);
  if (ReportDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UsbGetReportDescriptor (
            UsbIo,
            UsbMouseDev->InterfaceDescriptor->InterfaceNumber,
            MouseHidDesc.HidClassDesc[0].DescriptorLength,
            ReportDesc
            );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (ReportDesc);
    return Status;
  }

  //
  // Parse report descriptor
  //
  Status = ParseMouseReportDescriptor (
            UsbMouseDev,
            ReportDesc,
            MouseHidDesc.HidClassDesc[0].DescriptorLength
            );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (ReportDesc);
    return Status;
  }

  if (UsbMouseDev->NumberOfButtons >= 1) {
    UsbMouseDev->Mode.LeftButton = TRUE;
  }

  if (UsbMouseDev->NumberOfButtons > 1) {
    UsbMouseDev->Mode.RightButton = TRUE;
  }

  UsbMouseDev->Mode.ResolutionX = 8;
  UsbMouseDev->Mode.ResolutionY = 8;
  UsbMouseDev->Mode.ResolutionZ = 0;
  //
  // Here we just assume interface 0 is the mouse interface
  //
  UsbGetProtocolRequest (
    UsbIo,
    0,
    &Protocol
    );

  if (Protocol != BOOT_PROTOCOL) {
    Status = UsbSetProtocolRequest (
              UsbIo,
              0,
              BOOT_PROTOCOL
              );

    if (EFI_ERROR (Status)) {
      gBS->FreePool (ReportDesc);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Set indefinite Idle rate for USB Mouse
  //
  UsbSetIdleRequest (
    UsbIo,
    0,
    0,
    0
    );

  gBS->FreePool (ReportDesc);

  if (UsbMouseDev->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseDev->DelayedRecoveryEvent);
    UsbMouseDev->DelayedRecoveryEvent = 0;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBMouseRecoveryHandler,
                  UsbMouseDev,
                  &UsbMouseDev->DelayedRecoveryEvent
                  );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  )
/*++

  Routine Description:
    It is called whenever there is data received from async interrupt
    transfer.

  Arguments:
    Data            - Data received.
    DataLength      - Length of Data
    Context         - Passed in context
    Result          - Async Interrupt Transfer result

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  USB_MOUSE_DEV       *UsbMouseDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINT32              UsbResult;

  UsbMouseDevice  = (USB_MOUSE_DEV *) Context;
  UsbIo           = UsbMouseDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    MouseReportStatusCode (
      UsbMouseDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR)
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseDevice->IntEndpointDescriptor->EndpointAddress;

      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &UsbResult
        );
    }

    UsbIo->UsbAsyncInterruptTransfer (
            UsbIo,
            UsbMouseDevice->IntEndpointDescriptor->EndpointAddress,
            FALSE,
            0,
            0,
            NULL,
            NULL
            );

    gBS->SetTimer (
          UsbMouseDevice->DelayedRecoveryEvent,
          TimerRelative,
          EFI_USB_INTERRUPT_DELAY
          );
    return EFI_DEVICE_ERROR;
  }

  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  UsbMouseDevice->StateChanged = TRUE;

  //
  // Check mouse Data
  //
  UsbMouseDevice->State.LeftButton  = (BOOLEAN) (*(UINT8 *) Data & 0x01);
  UsbMouseDevice->State.RightButton = (BOOLEAN) (*(UINT8 *) Data & 0x02);
  UsbMouseDevice->State.RelativeMovementX += *((INT8 *) Data + 1);
  UsbMouseDevice->State.RelativeMovementY += *((INT8 *) Data + 2);

  if (DataLength > 3) {
    UsbMouseDevice->State.RelativeMovementZ += *((INT8 *) Data + 3);
  }

  return EFI_SUCCESS;
}

/*
STATIC VOID
PrintMouseState(
    IN  EFI_MOUSE_STATE *MouseState
    )
{
    Aprint("(%x: %x, %x)\n",
        MouseState->ButtonStates,
        MouseState->dx,
        MouseState->dy
        );
}
*/
STATIC
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  )
/*++

  Routine Description:
    Get the mouse state, see SIMPLE POINTER PROTOCOL.

  Arguments:
    This              - Protocol instance pointer.
    MouseState        - Current mouse state

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_NOT_READY

--*/
{
  USB_MOUSE_DEV *MouseDev;

  if (MouseState == NULL) {
    return EFI_DEVICE_ERROR;
  }

  MouseDev = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!MouseDev->StateChanged) {
    return EFI_NOT_READY;
  }

  CopyMem (
    MouseState,
    &MouseDev->State,
    sizeof (EFI_SIMPLE_POINTER_STATE)
    );

  //
  // Clear previous move state
  //
  MouseDev->State.RelativeMovementX = 0;
  MouseDev->State.RelativeMovementY = 0;
  MouseDev->State.RelativeMovementZ = 0;

  MouseDev->StateChanged            = FALSE;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
/*++

  Routine Description:
    Reset the mouse device, see SIMPLE POINTER PROTOCOL.

  Arguments:
    This                  - Protocol instance pointer.
    ExtendedVerification  - Ignored here/

  Returns:
    EFI_SUCCESS

--*/
{
  USB_MOUSE_DEV       *UsbMouseDevice;

  UsbMouseDevice  = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);

  MouseReportStatusCode (
    UsbMouseDevice->DevicePath,
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET)
    );

  ZeroMem (
    &UsbMouseDevice->State,
    sizeof (EFI_SIMPLE_POINTER_STATE)
    );
  UsbMouseDevice->StateChanged = FALSE;

  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
/*++

Routine Description:

  Event notification function for SIMPLE_POINTER.WaitForInput event
  Signal the event if there is input from mouse

Arguments:
  Event    - Wait Event
  Context  - Passed parameter to event handler
Returns:
  VOID
--*/
{
  USB_MOUSE_DEV *UsbMouseDev;

  UsbMouseDev = (USB_MOUSE_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (UsbMouseDev->StateChanged) {
    gBS->SignalEvent (Event);
  }
}

VOID
EFIAPI
USBMouseRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
/*++

  Routine Description:
    Timer handler for Delayed Recovery timer.

  Arguments:
    Event   -  The Delayed Recovery event.
    Context -  Points to the USB_KB_DEV instance.

  Returns:

--*/
{
  USB_MOUSE_DEV       *UsbMouseDev;
  EFI_USB_IO_PROTOCOL *UsbIo;

  UsbMouseDev = (USB_MOUSE_DEV *) Context;

  UsbIo       = UsbMouseDev->UsbIo;

  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseDev->IntEndpointDescriptor->EndpointAddress,
          TRUE,
          UsbMouseDev->IntEndpointDescriptor->Interval,
          UsbMouseDev->IntEndpointDescriptor->MaxPacketSize,
          OnMouseInterruptComplete,
          UsbMouseDev
          );
}

VOID
MouseReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  )
/*++

  Routine Description:
    Report Status Code in Usb Bot Driver

  Arguments:
    DevicePath  - Use this to get Device Path
    CodeType    - Status Code Type
    CodeValue   - Status Code Value

  Returns:
    None

--*/
{

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    CodeType,
    Value,
    DevicePath
    );
}
