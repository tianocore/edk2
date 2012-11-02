/** @file
  USB Mouse Driver that manages USB mouse and produces Absolute Pointer Protocol.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMouseAbsolutePointer.h"

EFI_DRIVER_BINDING_PROTOCOL gUsbMouseAbsolutePointerDriverBinding = {
  USBMouseAbsolutePointerDriverBindingSupported,
  USBMouseAbsolutePointerDriverBindingStart,
  USBMouseAbsolutePointerDriverBindingStop,
  0x1,
  NULL,
  NULL
};

/**
  Entrypoint of USB Mouse Absolute Pointer Driver.

  This function is the entrypoint of USB Mouse Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbMouseAbsolutePointerDriverBinding,
             ImageHandle,
             &gUsbMouseAbsolutePointerComponentName,
             &gUsbMouseAbsolutePointerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


/**
  Check whether USB Mouse Absolute Pointer Driver supports this device.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;

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
  
  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a mouse device that can be managed by this driver.
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


/**
  Starts the mouse device with this driver.

  This function consumes USB I/O Portocol, intializes USB mouse device,
  installs Absolute Pointer Protocol, and submits Asynchronous Interrupt
  Transfer to manage the USB mouse device.

  @param  This                  The driver binding instance.
  @param  Controller            Handle of device to bind driver to.
  @param  RemainingDevicePath   Optional parameter use to pick a specific child
                                device to start.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
  @retval EFI_OUT_OF_RESOURCES  Can't allocate memory resources.
  @retval EFI_ALREADY_STARTED   This driver has been started.

**/
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_USB_IO_PROTOCOL            *UsbIo;
  USB_MOUSE_ABSOLUTE_POINTER_DEV *UsbMouseAbsolutePointerDevice;
  UINT8                          EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR    EndpointDescriptor;
  UINT8                          Index;
  UINT8                          EndpointAddr;
  UINT8                          PollingInterval;
  UINT8                          PacketSize;
  BOOLEAN                        Found;
  EFI_TPL                        OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  //
  // Open USB I/O Protocol
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
    goto ErrorExit1;
  }
  
  UsbMouseAbsolutePointerDevice = AllocateZeroPool (sizeof (USB_MOUSE_ABSOLUTE_POINTER_DEV));
  ASSERT (UsbMouseAbsolutePointerDevice != NULL);

  UsbMouseAbsolutePointerDevice->UsbIo     = UsbIo;
  UsbMouseAbsolutePointerDevice->Signature = USB_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;

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
  // Report Status Code here since USB mouse will be detected next.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_PRESENCE_DETECT),
    UsbMouseAbsolutePointerDevice->DevicePath
    );

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbMouseAbsolutePointerDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbMouseAbsolutePointerDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find interrupt endpoint
  //
  Found = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
             UsbIo,
             Index,
             &EndpointDescriptor
             );

    if ((EndpointDescriptor.Attributes & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) {
      //
      // We only care interrupt endpoint here
      //
      CopyMem (&UsbMouseAbsolutePointerDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    //
    // Report Status Code to indicate that there is no USB mouse
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_NOT_DETECTED)
      );
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  //
  // Report Status Code here since USB mouse has be detected.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DETECTED),
    UsbMouseAbsolutePointerDevice->DevicePath
    );

  Status = InitializeUsbMouseDevice (UsbMouseAbsolutePointerDevice);
  if (EFI_ERROR (Status)) {
    //
    // Fail to initialize USB mouse device.
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR),
      UsbMouseAbsolutePointerDevice->DevicePath
      );

    goto ErrorExit;
  }

  //
  // Initialize and install EFI Absolute Pointer Protocol.
  //
  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.GetState = GetMouseAbsolutePointerState;
  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.Reset	  = UsbMouseAbsolutePointerReset;
  UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.Mode	  = &UsbMouseAbsolutePointerDevice->Mode;

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
    goto ErrorExit;
  }

  //
  // The next step would be submitting Asynchronous Interrupt Transfer on this mouse device.
  // After that we will be able to get key data from it. Thus this is deemed as
  // the enable action of the mouse, so report status code accordingly.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE),
    UsbMouseAbsolutePointerDevice->DevicePath
    );

  //
  // Submit Asynchronous Interrupt Transfer to manage this device.
  //
  EndpointAddr    = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8) (UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseInterruptComplete,
                    UsbMouseAbsolutePointerDevice
                    );

  if (EFI_ERROR (Status)) {
    //
    // If submit error, uninstall that interface
    //
    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiAbsolutePointerProtocolGuid,
           &UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol
           );
    goto ErrorExit;
  }

  UsbMouseAbsolutePointerDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbMouseAbsolutePointerComponentName.SupportedLanguages,
    &UsbMouseAbsolutePointerDevice->ControllerNameTable,
    L"Generic Usb Mouse Absolute Pointer",
      TRUE
      );
  AddUnicodeString2 (
    "en",
    gUsbMouseAbsolutePointerComponentName2.SupportedLanguages,
    &UsbMouseAbsolutePointerDevice->ControllerNameTable,
    L"Generic Usb Mouse Absolute Pointer",
    FALSE
    );

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

//
// Error handler
//
ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseAbsolutePointerDevice != NULL) {
      if ((UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol).WaitForInput);
      }

      FreePool (UsbMouseAbsolutePointerDevice);
      UsbMouseAbsolutePointerDevice = NULL;
    }
  }

ErrorExit1:
  gBS->RestoreTPL (OldTpl);

  return Status;
}


/**
  Stop the USB mouse device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Absolute Pointer Protocol is not installed on Controller.
  @retval Others                 Fail to uninstall protocols attached on the device.

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
  EFI_STATUS                      Status;
  USB_MOUSE_ABSOLUTE_POINTER_DEV  *UsbMouseAbsolutePointerDevice;
  EFI_ABSOLUTE_POINTER_PROTOCOL   *AbsolutePointerProtocol;
  EFI_USB_IO_PROTOCOL             *UsbIo;

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

  UsbIo = UsbMouseAbsolutePointerDevice->UsbIo;

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE),
    UsbMouseAbsolutePointerDevice->DevicePath
    );

  //
  // Delete the Asynchronous Interrupt Transfer from this device
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.EndpointAddress,
           FALSE,
           UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.Interval,
           0,
           NULL,
           NULL
           );

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

  //
  // Free all resources.
  //
  gBS->CloseEvent (UsbMouseAbsolutePointerDevice->AbsolutePointerProtocol.WaitForInput);
  
  if (UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent);
    UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent = NULL;
  }

  if (UsbMouseAbsolutePointerDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbMouseAbsolutePointerDevice->ControllerNameTable);
  }

  FreePool (UsbMouseAbsolutePointerDevice);

  return EFI_SUCCESS;

}


/**
  Uses USB I/O to check whether the device is a USB mouse device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB mouse device.
  @retval FALSE    Device is a not USB mouse device.

**/
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the default interface descriptor
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
  Initialize the USB mouse device.

  This function retrieves and parses HID report descriptor, and
  initializes state of USB_MOUSE_ABSOLUTE_POINTER_DEV. Then it sets indefinite idle
  rate for the device. Finally it creates event for delayed recovery,
  which deals with device error.

  @param  UsbMouseAbsolutePointerDev   Device instance to be initialized.

  @retval EFI_SUCCESS                  USB mouse device successfully initialized.
  @retval EFI_UNSUPPORTED              HID descriptor type is not report descriptor.
  @retval Other                        USB mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_ABSOLUTE_POINTER_DEV           *UsbMouseAbsolutePointerDev
  )
{
  EFI_USB_IO_PROTOCOL     *UsbIo;
  UINT8                   Protocol;
  EFI_STATUS              Status;
  EFI_USB_HID_DESCRIPTOR  MouseHidDesc;
  UINT8                   *ReportDesc;
  UINT8                   ReportId;
  UINT8                   Duration;

  UsbIo = UsbMouseAbsolutePointerDev->UsbIo;

  //
  // Get HID descriptor
  //
  Status = UsbGetHidDescriptor (
             UsbIo,
             UsbMouseAbsolutePointerDev->InterfaceDescriptor.InterfaceNumber,
             &MouseHidDesc
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get report descriptor
  //
  if (MouseHidDesc.HidClassDesc[0].DescriptorType != USB_DESC_TYPE_REPORT) {
    return EFI_UNSUPPORTED;
  }

  ReportDesc = AllocateZeroPool (MouseHidDesc.HidClassDesc[0].DescriptorLength);
  ASSERT (ReportDesc != NULL);

  Status = UsbGetReportDescriptor (
             UsbIo,
             UsbMouseAbsolutePointerDev->InterfaceDescriptor.InterfaceNumber,
             MouseHidDesc.HidClassDesc[0].DescriptorLength,
             ReportDesc
             );

  if (EFI_ERROR (Status)) {
    FreePool (ReportDesc);
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
    FreePool (ReportDesc);
    return Status;
  }

  UsbMouseAbsolutePointerDev->Mode.AbsoluteMaxX = 1024;
  UsbMouseAbsolutePointerDev->Mode.AbsoluteMaxY = 1024;
  UsbMouseAbsolutePointerDev->Mode.AbsoluteMaxZ = 0;
  UsbMouseAbsolutePointerDev->Mode.AbsoluteMinX = 0;
  UsbMouseAbsolutePointerDev->Mode.AbsoluteMinY = 0;
  UsbMouseAbsolutePointerDev->Mode.AbsoluteMinZ = 0;
  UsbMouseAbsolutePointerDev->Mode.Attributes   = 0x3;
  
  //
  // Set boot protocol for the USB mouse.
  // This driver only supports boot protocol.
  //
  UsbGetProtocolRequest (
    UsbIo,
    UsbMouseAbsolutePointerDev->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  if (Protocol != BOOT_PROTOCOL) {
    Status = UsbSetProtocolRequest (
               UsbIo,
               UsbMouseAbsolutePointerDev->InterfaceDescriptor.InterfaceNumber,
               BOOT_PROTOCOL
               );

    if (EFI_ERROR (Status)) {
      FreePool (ReportDesc);
      return Status;
    }
  }

  //
  // ReportId is zero, which means the idle rate applies to all input reports.
  //
  ReportId = 0;
  //
  // Duration is zero, which means the duration is infinite.
  // so the endpoint will inhibit reporting forever,
  // and only reporting when a change is detected in the report data.
  //
  Duration = 0;
  UsbSetIdleRequest (
    UsbIo,
    UsbMouseAbsolutePointerDev->InterfaceDescriptor.InterfaceNumber,
    ReportId,
    Duration
    );

  FreePool (ReportDesc);

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbMouseAbsolutePointerDev->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseAbsolutePointerDev->DelayedRecoveryEvent);
    UsbMouseAbsolutePointerDev->DelayedRecoveryEvent = 0;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         USBMouseRecoveryHandler,
         UsbMouseAbsolutePointerDev,
         &UsbMouseAbsolutePointerDev->DelayedRecoveryEvent
         );

  return EFI_SUCCESS;
}


/**
  Handler function for USB mouse's asynchronous interrupt transfer.

  This function is the handler function for USB mouse's asynchronous interrupt transfer
  to manage the mouse. It parses data returned from asynchronous interrupt transfer, and
  get button and movement state.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV   *UsbMouseAbsolutePointerDevice;
  EFI_USB_IO_PROTOCOL              *UsbIo;
  UINT8                            EndpointAddr;
  UINT32                           UsbResult;

  UsbMouseAbsolutePointerDevice  = (USB_MOUSE_ABSOLUTE_POINTER_DEV *) Context;
  UsbIo                          = UsbMouseAbsolutePointerDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR),
      UsbMouseAbsolutePointerDevice->DevicePath
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.EndpointAddress;

      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &UsbResult
        );
    }

    //
    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt. 
    //
    UsbIo->UsbAsyncInterruptTransfer (
             UsbIo,
             UsbMouseAbsolutePointerDevice->IntEndpointDescriptor.EndpointAddress,
             FALSE,
             0,
             0,
             NULL,
             NULL
             );
    //
    // EFI_USB_INTERRUPT_DELAY is defined in USB standard for error handling.
    //
    gBS->SetTimer (
           UsbMouseAbsolutePointerDevice->DelayedRecoveryEvent,
           TimerRelative,
           EFI_USB_INTERRUPT_DELAY
           );
    return EFI_DEVICE_ERROR;
  }

  //
  // If no error and no data, just return EFI_SUCCESS.
  //
  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  UsbMouseAbsolutePointerDevice->StateChanged = TRUE;

  //
  // Check mouse Data
  // USB HID Specification specifies following data format:
  // Byte    Bits    Description
  // 0       0       Button 1
  //         1       Button 2
  //         2       Button 3
  //         4 to 7  Device-specific
  // 1       0 to 7  X displacement
  // 2       0 to 7  Y displacement
  // 3 to n  0 to 7  Device specific (optional)
  //
  UsbMouseAbsolutePointerDevice->State.CurrentX += *((INT8 *) Data + 1);
  UsbMouseAbsolutePointerDevice->State.CurrentY += *((INT8 *) Data + 2);
  
  if (DataLength > 3) {
    UsbMouseAbsolutePointerDevice->State.CurrentZ += *((INT8 *) Data + 3);
  }
  UsbMouseAbsolutePointerDevice->State.ActiveButtons = *(UINT8 *) Data & (BIT0 | BIT1);

  return EFI_SUCCESS;
}

/**
  Retrieves the current state of a pointer device.

  @param  This                  A pointer to the EFI_ABSOLUTE_POINTER_PROTOCOL instance.                                   
  @param  MouseState            A pointer to the state information on the pointer device.

  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().                                                           
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.                                                           
  @retval EFI_INVALID_PARAMETER State is NULL.                                                           

**/
EFI_STATUS
EFIAPI
GetMouseAbsolutePointerState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *State
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV *MouseAbsolutePointerDev;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MouseAbsolutePointerDev = USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!MouseAbsolutePointerDev->StateChanged) {
    return EFI_NOT_READY;
  }

  //
  // Retrieve mouse state from USB_MOUSE_ABSOLUTE_POINTER_DEV,
  // which was filled by OnMouseInterruptComplete()
  //
  CopyMem (
    State,
    &MouseAbsolutePointerDev->State,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );

  //
  // Clear previous move state
  //
  MouseAbsolutePointerDev->State.CurrentX      = 0;
  MouseAbsolutePointerDev->State.CurrentY      = 0;
  MouseAbsolutePointerDev->State.CurrentZ      = 0;
  MouseAbsolutePointerDev->State.ActiveButtons = 0;

  MouseAbsolutePointerDev->StateChanged = FALSE;

  return EFI_SUCCESS;
}


/**
  Resets the pointer device hardware.

  @param  This                  A pointer to the EFI_ABSOLUTE_POINTER_PROTOCOL instance.
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
UsbMouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV       *UsbMouseAbsolutePointerDevice;

  UsbMouseAbsolutePointerDevice  = USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET),
    UsbMouseAbsolutePointerDevice->DevicePath
    );

  //
  // Clear mouse state.
  //
  ZeroMem (
    &UsbMouseAbsolutePointerDevice->State,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );
  UsbMouseAbsolutePointerDevice->StateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Event notification function for EFI_ABSOLUTE_POINTER_PROTOCOL.WaitForInput event.

  @param  Event        Event to be signaled when there's input from mouse.
  @param  Context      Points to USB_MOUSE_ABSOLUTE_POINTER_DEV instance.

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
  // If there's input from mouse, signal the event.
  //
  if (UsbMouseAbsolutePointerDev->StateChanged) {
    gBS->SignalEvent (Event);
  }
}

/**
  Handler for Delayed Recovery event.

  This function is the handler for Delayed Recovery event triggered
  by timer.
  After a device error occurs, the event would be triggered
  with interval of EFI_USB_INTERRUPT_DELAY. EFI_USB_INTERRUPT_DELAY
  is defined in USB standard for error handling.

  @param  Event                 The Delayed Recovery event.
  @param  Context               Points to the USB_MOUSE_ABSOLUTE_POINTER_DEV instance.

**/
VOID
EFIAPI
USBMouseRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_MOUSE_ABSOLUTE_POINTER_DEV       *UsbMouseAbsolutePointerDev;
  EFI_USB_IO_PROTOCOL                  *UsbIo;

  UsbMouseAbsolutePointerDev = (USB_MOUSE_ABSOLUTE_POINTER_DEV *) Context;

  UsbIo       = UsbMouseAbsolutePointerDev->UsbIo;

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseAbsolutePointerDev->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbMouseAbsolutePointerDev->IntEndpointDescriptor.Interval,
           UsbMouseAbsolutePointerDev->IntEndpointDescriptor.MaxPacketSize,
           OnMouseInterruptComplete,
           UsbMouseAbsolutePointerDev
           );
}
