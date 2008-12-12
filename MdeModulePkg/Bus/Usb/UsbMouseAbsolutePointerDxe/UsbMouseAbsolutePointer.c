/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbMouseAbsolutePointer.c

  Abstract:


**/

#include "UsbMouseAbsolutePointer.h"

#include <Library/DebugLib.h>
#include <IndustryStandard/Usb.h>

#include "mousehid.h"


EFI_GUID  gEfiUsbMouseAbsolutePointerDriverGuid = {
  0xa579f729, 0xa71d, 0x4b45, { 0xbe, 0xd7, 0xd, 0xb0, 0xa8, 0x7c, 0x3e, 0x8d }
};

EFI_DRIVER_BINDING_PROTOCOL gUsbMouseAbsolutePointerDriverBinding = {
  USBMouseAbsolutePointerDriverBindingSupported,
  USBMouseAbsolutePointerDriverBindingStart,
  USBMouseAbsolutePointerDriverBindingStop,
  0x1,
  NULL,
  NULL
};

//
// helper functions
//
BOOLEAN
IsUsbMouseAbsolutePointer (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  );

EFI_STATUS
InitializeUsbMouseAbsolutePointerDevice (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV           *UsbMouseAbsolutePointerDev
  );

VOID
EFIAPI
UsbMouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Mouse interrupt handler
//
EFI_STATUS
EFIAPI
OnMouseAbsolutePointerInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  );

//
// Mouse simulate TouchPad, Using AbsolutePointer Protocol
//
EFI_STATUS
EFIAPI
GetMouseAbsolutePointerState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *MouseAbsolutePointerState
  );

EFI_STATUS
EFIAPI
UsbMouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

//
// Driver start here
//
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingEntryPoint (
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
           &gUsbMouseAbsolutePointerDriverBinding,
           ImageHandle,
           &gUsbMouseAbsolutePointerComponentName,
           &gUsbMouseAbsolutePointerComponentName2
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
USBMouseAbsolutePointerDriverBindingSupported (
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
  if (!IsUsbMouseAbsolutePointer (UsbIo)) {
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
  @retval EFI_ALREADY_STARTED   This driver has been started

**/
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR *EndpointDesc;
  USB_MOUSE_ABSOLUTE_POINTER_DEV               *UsbMouseAbsolutePointerDevice;
  UINT8                       EndpointNumber;
  UINT8                       Index;
  UINT8                       EndpointAddr;
  UINT8                       PollingInterval;
  UINT8                       PacketSize;

  UsbMouseAbsolutePointerDevice  = NULL;
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
  
  UsbMouseAbsolutePointerDevice = AllocateZeroPool (sizeof (USB_MOUSE_ABSOLUTE_POINTER_DEV));
  if (UsbMouseAbsolutePointerDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  UsbMouseAbsolutePointerDevice->UsbIo               = UsbIo;

  UsbMouseAbsolutePointerDevice->Signature           = USB_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;

  UsbMouseAbsolutePointerDevice->InterfaceDescriptor = AllocatePool (sizeof (EFI_USB_INTERFACE_DESCRIPTOR));

  if (UsbMouseAbsolutePointerDevice->InterfaceDescriptor == NULL) {
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
                  (VOID **) &UsbMouseAbsolutePointerDevice->DevicePath,
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
          UsbMouseAbsolutePointerDevice->InterfaceDescriptor
          );

  EndpointNumber = UsbMouseAbsolutePointerDevice->InterfaceDescriptor->NumEndpoints;

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
      UsbMouseAbsolutePointerDevice->IntEndpointDescriptor = EndpointDesc;
    }
  }

  if (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor == NULL) {
    //
    // No interrupt endpoint, then error
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  Status = InitializeUsbMouseAbsolutePointerDevice (UsbMouseAbsolutePointerDevice);
  if (EFI_ERROR (Status)) {
    MouseAbsolutePointerReportStatusCode (
      UsbMouseAbsolutePointerDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueMouseInterfaceError)
      );

    goto ErrorExit;
  }

  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.GetState = GetMouseAbsolutePointerState;
  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.Reset	   = UsbMouseAbsolutePointerReset;
  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.Mode	   = &UsbMouseAbsolutePointerDevice->AbsolutePointerMode;

  Status = gBS->CreateEvent (
          EVT_NOTIFY_WAIT,
          TPL_NOTIFY,
          UsbMouseAbsolutePointerWaitForInput,
          UsbMouseAbsolutePointerDevice,
          &((UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol).WaitForInput)
          );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->InstallProtocolInterface (
          &Controller,
          &gEfiAbsolutePointerProtocolGuid,
          EFI_NATIVE_INTERFACE,
          &UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol
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

  MouseAbsolutePointerReportStatusCode (
    UsbMouseAbsolutePointerDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseEnable)
    );

  //
  // submit async interrupt transfer
  //
  EndpointAddr    = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->EndpointAddress;
  PollingInterval = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->Interval;
  PacketSize      = (UINT8) (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseAbsolutePointerInterruptComplete,
                    UsbMouseAbsolutePointerDevice
                    );

  if (!EFI_ERROR (Status)) {

    UsbMouseAbsolutePointerDevice->ControllerNameTable = NULL;
    AddUnicodeString2 (
      "eng",
      gUsbMouseAbsolutePointerComponentName.SupportedLanguages,
      &UsbMouseAbsolutePointerDevice->ControllerNameTable,
      L"Generic Usb Mouse Simulate TouchPad",
      TRUE
      );
    AddUnicodeString2 (
      "en",
      gUsbMouseAbsolutePointerComponentName2.SupportedLanguages,
      &UsbMouseAbsolutePointerDevice->ControllerNameTable,
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
        &UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol
  );

ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseAbsolutePointerDevice != NULL) {
      if (UsbMouseAbsolutePointerDevice->InterfaceDescriptor != NULL) {
        gBS->FreePool (UsbMouseAbsolutePointerDevice->InterfaceDescriptor);
      }

      if (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor != NULL) {
        gBS->FreePool (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor);
      }
  
      if ((UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol).WaitForInput);
      }

      gBS->FreePool (UsbMouseAbsolutePointerDevice);
      UsbMouseAbsolutePointerDevice = NULL;
    }
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle. Support stopping any child handles
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
USBMouseAbsolutePointerDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  USB_MOUSE_ABSOLUTE_POINTER_DEV               *UsbMouseAbsolutePointerDevice;
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
  UsbMouseAbsolutePointerDevice = USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (AbsolutePointerProtocol);

  gBS->CloseProtocol (
        Controller,
        &gEfiAbsolutePointerProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  UsbIo = UsbMouseAbsolutePointerDevice->UsbIo;

  //
  // Uninstall the Asyn Interrupt Transfer from this device
  // will disable the mouse data input from this device
  //
  MouseAbsolutePointerReportStatusCode (
    UsbMouseAbsolutePointerDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseDisable)
    );

  //
  // Delete Mouse Async Interrupt Transfer
  //
  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->EndpointAddress,
          FALSE,
          UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->Interval,
          0,
          NULL,
          NULL
          );

  gBS->CloseEvent (UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.WaitForInput);
  
  if (UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent);
    UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent = 0;
  }

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  &UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol
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

  gBS->FreePool (UsbMouseAbsolutePointerDevice->InterfaceDescriptor);
  gBS->FreePool (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor);

  if (UsbMouseAbsolutePointerDevice->ControllerNameTable) {
    FreeUnicodeStringTable (UsbMouseAbsolutePointerDevice->ControllerNameTable);
  }

  gBS->FreePool (UsbMouseAbsolutePointerDevice);

  return EFI_SUCCESS;

}


/**
  Tell if a Usb Controller is a mouse

  @param  UsbIo                 Protocol instance pointer.

  @retval TRUE                  It is a mouse
  @retval FALSE                 It is not a mouse

**/
BOOLEAN
IsUsbMouseAbsolutePointer (
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

  @param  UsbMouseAbsolutePointerDev           Device instance to be initialized

  @retval EFI_SUCCESS           Success
  @retval EFI_DEVICE_ERROR      Init error. EFI_OUT_OF_RESOURCES- Can't allocate
                                memory

**/
EFI_STATUS
InitializeUsbMouseAbsolutePointerDevice (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV           *UsbMouseAbsolutePointerDev
  )
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   Protocol;
  EFI_STATUS              Status;
  EFI_USB_HID_DESCRIPTOR  MouseHidDesc;
  UINT8                   *ReportDesc;

  UsbIo = UsbMouseAbsolutePointerDev->UsbIo;

  //
  // Get HID descriptor
  //
  Status = UsbGetHidDescriptor (
            UsbIo,
            UsbMouseAbsolutePointerDev->InterfaceDescriptor->InterfaceNumber,
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
            UsbMouseAbsolutePointerDev->InterfaceDescriptor->InterfaceNumber,
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
            UsbMouseAbsolutePointerDev,
            ReportDesc,
            MouseHidDesc.HidClassDesc[0].DescriptorLength
            );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (ReportDesc);
    return Status;
  }

  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMaxX = 1024;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMaxY = 1024;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMaxZ = 0;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMinX = 0;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMinY = 0;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.AbsoluteMinZ = 0;
  UsbMouseAbsolutePointerDev->AbsolutePointerMode.Attributes   = 0x3;
  
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

  if (UsbMouseAbsolutePointerDev->DelayedRecoveryEvent) {
    gBS->CloseEvent (UsbMouseAbsolutePointerDev->DelayedRecoveryEvent);
    UsbMouseAbsolutePointerDev->DelayedRecoveryEvent = 0;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBMouseAbsolutePointerRecoveryHandler,
                  UsbMouseAbsolutePointerDev,
                  &UsbMouseAbsolutePointerDev->DelayedRecoveryEvent
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
EFI_STATUS
EFIAPI
OnMouseAbsolutePointerInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV       *UsbMouseAbsolutePointerDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               EndpointAddr;
  UINT32              UsbResult;

  UsbMouseAbsolutePointerDevice  = (USB_MOUSE_ABSOLUTE_POINTER_DEV *) Context;
  UsbIo           = UsbMouseAbsolutePointerDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    MouseAbsolutePointerReportStatusCode (
      UsbMouseAbsolutePointerDevice->DevicePath,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      PcdGet32 (PcdStatusCodeValueMouseInputError)
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->EndpointAddress;

      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &UsbResult
        );
    }

    UsbIo->UsbAsyncInterruptTransfer (
            UsbIo,
            UsbMouseAbsolutePointerDevice->IntEndpointDescriptor->EndpointAddress,
            FALSE,
            0,
            0,
            NULL,
            NULL
            );

    gBS->SetTimer (
          UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent,
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
  UsbMouseAbsolutePointerDevice->AbsolutePointerStateChanged = TRUE;
  UsbMouseAbsolutePointerDevice->AbsolutePointerState.CurrentX += *((INT8 *) Data + 1);
  UsbMouseAbsolutePointerDevice->AbsolutePointerState.CurrentY += *((INT8 *) Data + 2);
  if (DataLength > 3) {
    UsbMouseAbsolutePointerDevice->AbsolutePointerState.CurrentZ += *((INT8 *) Data + 3);
  }
  UsbMouseAbsolutePointerDevice->AbsolutePointerState.ActiveButtons = *(UINT8 *)Data & 0x3;

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
EFI_STATUS
EFIAPI
GetMouseAbsolutePointerState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *MouseAbsolutePointerState
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV *MouseAbsolutePointerDev;

  if (MouseAbsolutePointerState == NULL) {
    return EFI_DEVICE_ERROR;
  }

  MouseAbsolutePointerDev = USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!MouseAbsolutePointerDev->AbsolutePointerStateChanged) {
    return EFI_NOT_READY;
  }

  CopyMem (
    MouseAbsolutePointerState,
    &MouseAbsolutePointerDev->AbsolutePointerState,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );

  //
  // Clear previous move state
  //
  MouseAbsolutePointerDev->AbsolutePointerState.CurrentX = 0;
  MouseAbsolutePointerDev->AbsolutePointerState.CurrentY = 0;
  MouseAbsolutePointerDev->AbsolutePointerState.CurrentZ = 0;
  MouseAbsolutePointerDev->AbsolutePointerState.ActiveButtons = 0;

  MouseAbsolutePointerDev->AbsolutePointerStateChanged            = FALSE;

  return EFI_SUCCESS;
}


/**
  Reset the mouse device, see ABSOLUTE POINTER PROTOCOL.

  @param  This                  Protocol instance pointer.
  @param  ExtendedVerification  Ignored here/

  @return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
UsbMouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV       *UsbMouseAbsolutePointerDevice;

  UsbMouseAbsolutePointerDevice  = USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  MouseAbsolutePointerReportStatusCode (
    UsbMouseAbsolutePointerDevice->DevicePath,
    EFI_PROGRESS_CODE,
    PcdGet32 (PcdStatusCodeValueMouseReset)
    );

  ZeroMem (
    &UsbMouseAbsolutePointerDevice->AbsolutePointerState,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );
  UsbMouseAbsolutePointerDevice->AbsolutePointerStateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Event notification function for ABSOLUTE_POINTER.WaitForInput event
  Signal the event if there is input from mouse

  @param  Event                 Wait Event
  @param  Context               Passed parameter to event handler
 VOID

**/
VOID
EFIAPI
UsbMouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV *UsbMouseAbsolutePointerDev;

  UsbMouseAbsolutePointerDev = (USB_MOUSE_ABSOLUTE_POINTER_DEV *) Context;

  //
  // Someone is waiting on the mouse event, if there's
  // input from mouse, signal the event
  //
  if (UsbMouseAbsolutePointerDev->AbsolutePointerStateChanged) {
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
USBMouseAbsolutePointerRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV       *UsbMouseAbsolutePointerDev;
  EFI_USB_IO_PROTOCOL *UsbIo;

  UsbMouseAbsolutePointerDev = (USB_MOUSE_ABSOLUTE_POINTER_DEV *) Context;

  UsbIo       = UsbMouseAbsolutePointerDev->UsbIo;

  UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbMouseAbsolutePointerDev->IntEndpointDescriptor->EndpointAddress,
          TRUE,
          UsbMouseAbsolutePointerDev->IntEndpointDescriptor->Interval,
          UsbMouseAbsolutePointerDev->IntEndpointDescriptor->MaxPacketSize,
          OnMouseAbsolutePointerInterruptComplete,
          UsbMouseAbsolutePointerDev
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
MouseAbsolutePointerReportStatusCode (
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
