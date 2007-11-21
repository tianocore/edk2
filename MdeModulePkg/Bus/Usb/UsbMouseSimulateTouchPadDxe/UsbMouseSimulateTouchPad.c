/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbMouseSimulateTouchPad.c

  Abstract:


**/

#include "UsbMouseSimulateTouchPad.h"

#include <Library/DebugLib.h>
#include <IndustryStandard/Usb.h>

#include "mousehid.h"

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );

EFI_GUID  gEfiUsbMouseSimulateTouchPadDriverGuid = {
  0xa579f729, 0xa71d, 0x4b45, { 0xbe, 0xd7, 0xd, 0xb0, 0xa8, 0x7c, 0x3e, 0x8d }
};

EFI_DRIVER_BINDING_PROTOCOL gUsbMouseSimulateTouchPadDriverBinding = {
  USBMouseSimulateTouchPadDriverBindingSupported,
  USBMouseSimulateTouchPadDriverBindingStart,
  USBMouseSimulateTouchPadDriverBindingStop,
  0x1,
  NULL,
  NULL
};

//
// helper functions
//
STATIC
BOOLEAN
IsUsbMouseSimulateTouchPad (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  );

STATIC
EFI_STATUS
InitializeUsbMouseSimulateTouchPadDevice (
  IN  USB_MOUSE_SIMULATE_TOUCHPAD_DEV           *UsbMouseSimulateTouchPadDev
  );

STATIC
VOID
EFIAPI
UsbMouseSimulateTouchPadWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Mouse interrupt handler
//
STATIC
EFI_STATUS
EFIAPI
OnMouseSimulateTouchPadInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  );

//
// Mouse simulate TouchPad, Using AbsolutePointer Protocol
//
STATIC
EFI_STATUS
EFIAPI
GetMouseSimulateTouchPadState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *MouseSimulateTouchPadState
  );

STATIC
EFI_STATUS
EFIAPI
UsbMouseSimulateTouchPadReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

//
// Driver start here
//
EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

  Routine Description:
    Entry point for EFI drivers.

  Arguments:
   ImageHandle - EFI_HANDLE
   SystemTable - EFI_SYSTEM_TABLE
  Returns:
    EFI_SUCCESS
    others

--*/
{
	return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gUsbMouseSimulateTouchPadDriverBinding,
           ImageHandle,
           &gUsbMouseSimulateTouchPadComponentName,
           &gUsbMouseSimulateTouchPadComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  that has UsbHcProtocol installed will be supported.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to test
  @param  RemainingDevicePath   Not used

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.

**/
EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
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
  if (!IsUsbMouseSimulateTouchPad (UsbIo)) {
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
  Starting the Usb Bus Driver

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to test
  @param  RemainingDevicePath   Not used

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error
                                EFI_OUT_OF_RESOURCES- Can't allocate memory
                                resources
  @retval EFI_ALREADY_STARTED   Thios driver has been started

**/
EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR *EndpointDesc;
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV               *UsbMouseSimulateTouchPadDevice;
  UINT8                       EndpointNumber;
  UINT8                       Index;
  UINT8                       EndpointAddr;
  UINT8                       PollingInterval;
  UINT8                       PacketSize;

  UsbMouseSimulateTouchPadDevice  = NULL;
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
  
  UsbMouseSimulateTouchPadDevice = AllocateZeroPool (sizeof (USB_MOUSE_SIMULATE_TOUCHPAD_DEV));
  if (UsbMouseSimulateTouchPadDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbMouseSimulateTouchPadDevice->UsbIo               = UsbIo;

  UsbMouseSimulateTouchPadDevice->Signature           = USB_MOUSE_SIMULATE_TOUCHPAD_DEV_SIGNATURE;

  UsbMouseSimulateTouchPadDevice->InterfaceDescriptor = AllocatePool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));

  if (UsbMouseSimulateTouchPadDevice->InterfaceDescriptor == NULL) {
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
                  (VOID **) &UsbMouseSimulateTouchPadDevice->DevicePath,
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
          UsbMouseSimulateTouchPadDevice->InterfaceDescriptor
          );

  EndpointNumber = UsbMouseSimulateTouchPadDevice->InterfaceDescriptor->NumEndpoints;

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
      UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor = EndpointDesc;
    }
  }

  if (UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor == NULL) {
    //
    // No interrupt endpoint, then error
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  Status = InitializeUsbMouseSimulateTouchPadDevice (UsbMouseSimulateTouchPadDevice);
  if (EFI_ERROR (Status)) {
    MouseSimulateTouchPadReportStatusCode (
      UsbMouseSimulateTouchPadDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueMouseInterfaceError)
      );

    goto ErrorExit;
  }

  UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol.GetState = GetMouseSimulateTouchPadState;
  UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol.Reset	   = UsbMouseSimulateTouchPadReset;
  UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol.Mode	   = &UsbMouseSimulateTouchPadDevice->AbsolutePointerMode;

  Status = gBS->CreateEvent (
          EVT_NOTIFY_WAIT,
          TPL_NOTIFY,
          UsbMouseSimulateTouchPadWaitForInput,
          UsbMouseSimulateTouchPadDevice,
          &((UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol).WaitForInput)
          );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallProtocolInterface (
          &Controller,
          &gEfiAbsolutePointerProtocolGuid,
          EFI_NATIVE_INTERFACE,
          &UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol
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

  MouseSimulateTouchPadReportStatusCode (
    UsbMouseSimulateTouchPadDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseEnable)
    );

  //
  // submit async interrupt transfer
  //
  EndpointAddr    = UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->EndpointAddress;
  PollingInterval = UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->Interval;
  PacketSize      = (UINT8) (UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseSimulateTouchPadInterruptComplete,
                    UsbMouseSimulateTouchPadDevice
                    );

  if (!EFI_ERROR (Status)) {

    UsbMouseSimulateTouchPadDevice->ControllerNameTable = NULL;
    AddUnicodeString2 (
      "eng",
      gUsbMouseSimulateTouchPadComponentName.SupportedLanguages,
      &UsbMouseSimulateTouchPadDevice->ControllerNameTable,
      L"Generic Usb Mouse Simulate TouchPad",
      TRUE
      );
    AddUnicodeString2 (
      "en",
      gUsbMouseSimulateTouchPadComponentName2.SupportedLanguages,
      &UsbMouseSimulateTouchPadDevice->ControllerNameTable,
      L"Generic Usb Mouse Simulate TouchPad2",
      FALSE
      );

    return EFI_SUCCESS;
  }

  //
  // If submit error, uninstall that interface
  //
  Status = EFI_DEVICE_ERROR;

  gBS->UninstallProtocolInterface (
        Controller,
        &gEfiAbsolutePointerProtocolGuid,
        &UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol
  );

ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseSimulateTouchPadDevice != NULL) {
      if (UsbMouseSimulateTouchPadDevice->InterfaceDescriptor != NULL) {
        gBS->FreePool (UsbMouseSimulateTouchPadDevice->InterfaceDescriptor);
      }

      if (UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor != NULL) {
        gBS->FreePool (UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor);
      }
  
      if ((UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol).WaitForInput);
      }

      gBS->FreePool (UsbMouseSimulateTouchPadDevice);
      UsbMouseSimulateTouchPadDevice = NULL;
    }
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This                  Protocol instance pointer.
  @param  Controller            Handle of device to stop driver on
  @param  NumberOfChildren      Number of Children in the ChildHandleBuffer
  @param  ChildHandleBuffer     List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return others

**/
EFI_STATUS
EFIAPI
USBMouseSimulateTouchPadDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV               *UsbMouseSimulateTouchPadDevice;
  EFI_ABSOLUTE_POINTER_PROTOCOL *AbsolutePointerProtocol;
  EFI_USB_IO_PROTOCOL         *UsbIo;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
      Controller,
      &gEfiAbsolutePointerProtocolGuid,
      (VOID **) &AbsolutePointerProtocol,
      This->DriverBindingHandle,
      Controller,
      EFI_OPEN_PROTOCOL_GET_PROTOCOL
  );

  if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
  }
  UsbMouseSimulateTouchPadDevice = USB_MOUSE_SIMULATE_TOUCHPAD_DEV_FROM_MOUSE_PROTOCOL (AbsolutePointerProtocol);

  gBS->CloseProtocol (
        Controller,
        &gEfiAbsolutePointerProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  UsbIo = UsbMouseSimulateTouchPadDevice->UsbIo;

  //
  // Uninstall the Asyn Interrupt Transfer from this device
  // will disable the mouse data input from this device
  //
  MouseSimulateTouchPadReportStatusCode (
    UsbMouseSimulateTouchPadDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseDisable)
    );

  //
  // Delete Mouse Async Interrupt Transfer
  //
  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->EndpointAddress,
          FALSE,
          UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->Interval,
          0,
          NULL,
          NULL
          );

  gBS->CloseEvent (UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol.WaitForInput);
  
  if (UsbMouseSimulateTouchPadDevice->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseSimulateTouchPadDevice->DelayedRecoveryEvent);
    UsbMouseSimulateTouchPadDevice->DelayedRecoveryEvent = 0;
  }

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  &UsbMouseSimulateTouchPadDevice->AbsolutePointerProtocol
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

  gBS->FreePool (UsbMouseSimulateTouchPadDevice->InterfaceDescriptor);
  gBS->FreePool (UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor);

  if (UsbMouseSimulateTouchPadDevice->ControllerNameTable) {
    FreeUnicodeStringTable (UsbMouseSimulateTouchPadDevice->ControllerNameTable);
  }

  gBS->FreePool (UsbMouseSimulateTouchPadDevice);

  return EFI_SUCCESS;

}


/**
  Tell if a Usb Controller is a mouse

  @param  UsbIo                 Protocol instance pointer.

  @retval TRUE                  It is a mouse
  @retval FALSE                 It is not a mouse

**/
BOOLEAN
IsUsbMouseSimulateTouchPad (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  )
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


/**
  Initialize the Usb Mouse Simulate TouchPad Device.

  @param  UsbMouseSimulateTouchPadDev           Device instance to be initialized

  @retval EFI_SUCCESS           Success
  @retval EFI_DEVICE_ERROR      Init error. EFI_OUT_OF_RESOURCES- Can't allocate
                                memory

**/
STATIC
EFI_STATUS
InitializeUsbMouseSimulateTouchPadDevice (
  IN  USB_MOUSE_SIMULATE_TOUCHPAD_DEV           *UsbMouseSimulateTouchPadDev
  )
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   Protocol;
  EFI_STATUS              Status;
  EFI_USB_HID_DESCRIPTOR  MouseHidDesc;
  UINT8                   *ReportDesc;

  UsbIo = UsbMouseSimulateTouchPadDev->UsbIo;

  //
  // Get HID descriptor
  //
  Status = UsbGetHidDescriptor (
            UsbIo,
            UsbMouseSimulateTouchPadDev->InterfaceDescriptor->InterfaceNumber,
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
            UsbMouseSimulateTouchPadDev->InterfaceDescriptor->InterfaceNumber,
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
            UsbMouseSimulateTouchPadDev,
            ReportDesc,
            MouseHidDesc.HidClassDesc[0].DescriptorLength
            );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (ReportDesc);
    return Status;
  }

  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMaxX = 1024;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMaxY = 1024;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMaxZ = 0;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMinX = 0;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMinY = 0;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.AbsoluteMinZ = 0;
  UsbMouseSimulateTouchPadDev->AbsolutePointerMode.Attributes   = 0x3;
  
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

  if (UsbMouseSimulateTouchPadDev->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseSimulateTouchPadDev->DelayedRecoveryEvent);
    UsbMouseSimulateTouchPadDev->DelayedRecoveryEvent = 0;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBMouseSimulateTouchPadRecoveryHandler,
                  UsbMouseSimulateTouchPadDev,
                  &UsbMouseSimulateTouchPadDev->DelayedRecoveryEvent
                  );

  return EFI_SUCCESS;
}


/**
  It is called whenever there is data received from async interrupt
  transfer.

  @param  Data                  Data received.
  @param  DataLength            Length of Data
  @param  Context               Passed in context
  @param  Result                Async Interrupt Transfer result

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR

**/
STATIC
EFI_STATUS
EFIAPI
OnMouseSimulateTouchPadInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  )
{
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV       *UsbMouseSimulateTouchPadDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINT32              UsbResult;

  UsbMouseSimulateTouchPadDevice  = (USB_MOUSE_SIMULATE_TOUCHPAD_DEV *) Context;
  UsbIo           = UsbMouseSimulateTouchPadDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    MouseSimulateTouchPadReportStatusCode (
      UsbMouseSimulateTouchPadDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueMouseInputError)
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->EndpointAddress;

      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &UsbResult
        );
    }

    UsbIo->UsbAsyncInterruptTransfer (
            UsbIo,
            UsbMouseSimulateTouchPadDevice->IntEndpointDescriptor->EndpointAddress,
            FALSE,
            0,
            0,
            NULL,
            NULL
            );

    gBS->SetTimer (
          UsbMouseSimulateTouchPadDevice->DelayedRecoveryEvent,
          TimerRelative,
          EFI_USB_INTERRUPT_DELAY
          );
    return EFI_DEVICE_ERROR;
  }

  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  //
  //Check mouse Data
  //
  UsbMouseSimulateTouchPadDevice->AbsolutePointerStateChanged = TRUE;
  UsbMouseSimulateTouchPadDevice->AbsolutePointerState.CurrentX += *((INT8 *) Data + 1);
  UsbMouseSimulateTouchPadDevice->AbsolutePointerState.CurrentY += *((INT8 *) Data + 2);
  if (DataLength > 3) {
    UsbMouseSimulateTouchPadDevice->AbsolutePointerState.CurrentZ += *((INT8 *) Data + 3);
  }
  UsbMouseSimulateTouchPadDevice->AbsolutePointerState.ActiveButtons = *(UINT8 *)Data & 0x3;

  return EFI_SUCCESS;
}

/**
  Get the mouse state, see ABSOLUTE POINTER PROTOCOL.

  @param  This                  Protocol instance pointer.
  @param  MouseState            Current mouse state

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR
  @return EFI_NOT_READY

**/
STATIC
EFI_STATUS
EFIAPI
GetMouseSimulateTouchPadState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *MouseSimulateTouchPadState
  )
{
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV *MouseSimulateTouchPadDev;

  if (MouseSimulateTouchPadState == NULL) {
    return EFI_DEVICE_ERROR;
  }

  MouseSimulateTouchPadDev = USB_MOUSE_SIMULATE_TOUCHPAD_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!MouseSimulateTouchPadDev->AbsolutePointerStateChanged) {
    return EFI_NOT_READY;
  }

  CopyMem (
    MouseSimulateTouchPadState,
    &MouseSimulateTouchPadDev->AbsolutePointerState,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );

  //
  // Clear previous move state
  //
  MouseSimulateTouchPadDev->AbsolutePointerState.CurrentX = 0;
  MouseSimulateTouchPadDev->AbsolutePointerState.CurrentY = 0;
  MouseSimulateTouchPadDev->AbsolutePointerState.CurrentZ = 0;
  MouseSimulateTouchPadDev->AbsolutePointerState.ActiveButtons = 0;

  MouseSimulateTouchPadDev->AbsolutePointerStateChanged            = FALSE;

  return EFI_SUCCESS;
}


/**
  Reset the mouse device, see ABSOLUTE POINTER PROTOCOL.

  @param  This                  Protocol instance pointer.
  @param  ExtendedVerification  Ignored here/

  @return EFI_SUCCESS

**/
STATIC
EFI_STATUS
EFIAPI
UsbMouseSimulateTouchPadReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV       *UsbMouseSimulateTouchPadDevice;

  UsbMouseSimulateTouchPadDevice  = USB_MOUSE_SIMULATE_TOUCHPAD_DEV_FROM_MOUSE_PROTOCOL (This);

  MouseSimulateTouchPadReportStatusCode (
    UsbMouseSimulateTouchPadDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseReset)
    );

  ZeroMem (
    &UsbMouseSimulateTouchPadDevice->AbsolutePointerState,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );
  UsbMouseSimulateTouchPadDevice->AbsolutePointerStateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Event notification function for ABSOLUTE_POINTER.WaitForInput event
  Signal the event if there is input from mouse

  @param  Event                 Wait Event
  @param  Context               Passed parameter to event handler
 VOID

**/
STATIC
VOID
EFIAPI
UsbMouseSimulateTouchPadWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV *UsbMouseSimulateTouchPadDev;

  UsbMouseSimulateTouchPadDev = (USB_MOUSE_SIMULATE_TOUCHPAD_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (UsbMouseSimulateTouchPadDev->AbsolutePointerStateChanged) {
    gBS->SignalEvent (Event);
  }
}

/**
  Timer handler for Delayed Recovery timer.

  @param  Event                 The Delayed Recovery event.
  @param  Context               Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBMouseSimulateTouchPadRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_MOUSE_SIMULATE_TOUCHPAD_DEV       *UsbMouseSimulateTouchPadDev;
  EFI_USB_IO_PROTOCOL *UsbIo;

  UsbMouseSimulateTouchPadDev = (USB_MOUSE_SIMULATE_TOUCHPAD_DEV *) Context;

  UsbIo       = UsbMouseSimulateTouchPadDev->UsbIo;

  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseSimulateTouchPadDev->IntEndpointDescriptor->EndpointAddress,
          TRUE,
          UsbMouseSimulateTouchPadDev->IntEndpointDescriptor->Interval,
          UsbMouseSimulateTouchPadDev->IntEndpointDescriptor->MaxPacketSize,
          OnMouseSimulateTouchPadInterruptComplete,
          UsbMouseSimulateTouchPadDev
          );
}


/**
  Report Status Code in Usb Bot Driver

  @param  DevicePath            Use this to get Device Path
  @param  CodeType              Status Code Type
  @param  CodeValue             Status Code Value

  @return None

**/
VOID
MouseSimulateTouchPadReportStatusCode (
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
