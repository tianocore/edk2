/** @file
  USB Mouse Driver that manages USB mouse and produces Simple Pointer Protocol.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbMouse.h"

EFI_DRIVER_BINDING_PROTOCOL  gUsbMouseDriverBinding = {
  USBMouseDriverBindingSupported,
  USBMouseDriverBindingStart,
  USBMouseDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  Entrypoint of USB Mouse Driver.

  This function is the entrypoint of USB Mouse Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbMouseDriverBinding,
             ImageHandle,
             &gUsbMouseComponentName,
             &gUsbMouseComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether USB mouse driver supports this device.

  @param  This                   The USB mouse driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
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

  This function consumes USB I/O Protocol, initializes USB mouse device,
  installs Simple Pointer Protocol, and submits Asynchronous Interrupt
  Transfer to manage the USB mouse device.

  @param  This                  The USB mouse driver binding instance.
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
USBMouseDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_USB_IO_PROTOCOL          *UsbIo;
  USB_MOUSE_DEV                *UsbMouseDevice;
  UINT8                        EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR  EndpointDescriptor;
  UINT8                        Index;
  UINT8                        EndpointAddr;
  UINT8                        PollingInterval;
  UINT8                        PacketSize;
  BOOLEAN                      Found;
  EFI_TPL                      OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  //
  // Open USB I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit1;
  }

  UsbMouseDevice = AllocateZeroPool (sizeof (USB_MOUSE_DEV));
  ASSERT (UsbMouseDevice != NULL);

  UsbMouseDevice->UsbIo     = UsbIo;
  UsbMouseDevice->Signature = USB_MOUSE_DEV_SIGNATURE;

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbMouseDevice->DevicePath,
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
    UsbMouseDevice->DevicePath
    );

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbMouseDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbMouseDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find interrupt endpoint IN
  //
  Found = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
             UsbIo,
             Index,
             &EndpointDescriptor
             );

    if (((EndpointDescriptor.Attributes & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) &&
        ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN) != 0))
    {
      //
      // We only care interrupt endpoint here
      //
      CopyMem (&UsbMouseDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof (EndpointDescriptor));
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
    UsbMouseDevice->DevicePath
    );

  Status = InitializeUsbMouseDevice (UsbMouseDevice);
  if (EFI_ERROR (Status)) {
    //
    // Fail to initialize USB mouse device.
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR),
      UsbMouseDevice->DevicePath
      );

    goto ErrorExit;
  }

  //
  // Initialize and install EFI Simple Pointer Protocol.
  //
  UsbMouseDevice->SimplePointerProtocol.GetState = GetMouseState;
  UsbMouseDevice->SimplePointerProtocol.Reset    = UsbMouseReset;
  UsbMouseDevice->SimplePointerProtocol.Mode     = &UsbMouseDevice->Mode;

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
    UsbMouseDevice->DevicePath
    );

  //
  // Submit Asynchronous Interrupt Transfer to manage this device.
  //
  EndpointAddr    = UsbMouseDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbMouseDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8)(UsbMouseDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseInterruptComplete,
                    UsbMouseDevice
                    );

  if (EFI_ERROR (Status)) {
    //
    // If submit error, uninstall that interface
    //
    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiSimplePointerProtocolGuid,
           &UsbMouseDevice->SimplePointerProtocol
           );
    goto ErrorExit;
  }

  UsbMouseDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbMouseComponentName.SupportedLanguages,
    &UsbMouseDevice->ControllerNameTable,
    L"Generic Usb Mouse",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUsbMouseComponentName2.SupportedLanguages,
    &UsbMouseDevice->ControllerNameTable,
    L"Generic Usb Mouse",
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

    if (UsbMouseDevice != NULL) {
      if ((UsbMouseDevice->SimplePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((UsbMouseDevice->SimplePointerProtocol).WaitForInput);
      }

      FreePool (UsbMouseDevice);
      UsbMouseDevice = NULL;
    }
  }

ErrorExit1:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Stop the USB mouse device handled by this driver.

  @param  This                   The USB mouse driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Pointer Protocol is not installed on Controller.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  USB_MOUSE_DEV                *UsbMouseDevice;
  EFI_SIMPLE_POINTER_PROTOCOL  *SimplePointerProtocol;
  EFI_USB_IO_PROTOCOL          *UsbIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimplePointerProtocolGuid,
                  (VOID **)&SimplePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbMouseDevice = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (SimplePointerProtocol);

  UsbIo = UsbMouseDevice->UsbIo;

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE),
    UsbMouseDevice->DevicePath
    );

  //
  // Delete the Asynchronous Interrupt Transfer from this device
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseDevice->IntEndpointDescriptor.EndpointAddress,
           FALSE,
           UsbMouseDevice->IntEndpointDescriptor.Interval,
           0,
           NULL,
           NULL
           );

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

  //
  // Free all resources.
  //
  gBS->CloseEvent (UsbMouseDevice->SimplePointerProtocol.WaitForInput);

  if (UsbMouseDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseDevice->DelayedRecoveryEvent);
    UsbMouseDevice->DelayedRecoveryEvent = NULL;
  }

  if (UsbMouseDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbMouseDevice->ControllerNameTable);
  }

  FreePool (UsbMouseDevice);

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
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
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
      )
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Initialize the USB mouse device.

  This function retrieves and parses HID report descriptor, and
  initializes state of USB_MOUSE_DEV. Then it sets indefinite idle
  rate for the device. Finally it creates event for delayed recovery,
  which deals with device error.

  @param  UsbMouseDev           Device instance to be initialized.

  @retval EFI_SUCCESS           USB mouse device successfully initialized..
  @retval EFI_UNSUPPORTED       HID descriptor type is not report descriptor.
  @retval Other                 USB mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeUsbMouseDevice (
  IN OUT USB_MOUSE_DEV  *UsbMouseDev
  )
{
  EFI_USB_IO_PROTOCOL        *UsbIo;
  UINT8                      Protocol;
  EFI_STATUS                 Status;
  EFI_USB_HID_DESCRIPTOR     *MouseHidDesc;
  UINT8                      *ReportDesc;
  EFI_USB_CONFIG_DESCRIPTOR  ConfigDesc;
  VOID                       *Buf;
  UINT32                     TransferResult;
  UINT16                     Total;
  USB_DESC_HEAD              *Head;
  BOOLEAN                    Start;

  UsbIo = UsbMouseDev->UsbIo;

  //
  // Get the current configuration descriptor. Note that it doesn't include other descriptors.
  //
  Status = UsbIo->UsbGetConfigDescriptor (
                    UsbIo,
                    &ConfigDesc
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // By issuing Get_Descriptor(Configuration) request with total length, we get the Configuration descriptor,
  // all Interface descriptors, all Endpoint descriptors, and the HID descriptor for each interface.
  //
  Buf = AllocateZeroPool (ConfigDesc.TotalLength);
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UsbGetDescriptor (
             UsbIo,
             (UINT16)((USB_DESC_TYPE_CONFIG << 8) | (ConfigDesc.ConfigurationValue - 1)),
             0,
             ConfigDesc.TotalLength,
             Buf,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    FreePool (Buf);
    return Status;
  }

  Total        = 0;
  Start        = FALSE;
  Head         = (USB_DESC_HEAD *)Buf;
  MouseHidDesc = NULL;

  //
  // Get HID descriptor from the receipt of Get_Descriptor(Configuration) request.
  // This algorithm is based on the fact that the HID descriptor shall be interleaved
  // between the interface and endpoint descriptors for HID interfaces.
  //
  while (Total < ConfigDesc.TotalLength) {
    if (Head->Type == USB_DESC_TYPE_INTERFACE) {
      if ((((USB_INTERFACE_DESCRIPTOR *)Head)->InterfaceNumber == UsbMouseDev->InterfaceDescriptor.InterfaceNumber) &&
          (((USB_INTERFACE_DESCRIPTOR *)Head)->AlternateSetting == UsbMouseDev->InterfaceDescriptor.AlternateSetting))
      {
        Start = TRUE;
      }
    }

    if (Start && (Head->Type == USB_DESC_TYPE_ENDPOINT)) {
      break;
    }

    if (Start && (Head->Type == USB_DESC_TYPE_HID)) {
      MouseHidDesc = (EFI_USB_HID_DESCRIPTOR *)Head;
      break;
    }

    Total = Total + (UINT16)Head->Len;
    Head  = (USB_DESC_HEAD *)((UINT8 *)Buf + Total);
  }

  if (MouseHidDesc == NULL) {
    FreePool (Buf);
    return EFI_UNSUPPORTED;
  }

  //
  // Get report descriptor
  //
  if (MouseHidDesc->HidClassDesc[0].DescriptorType != USB_DESC_TYPE_REPORT) {
    FreePool (Buf);
    return EFI_UNSUPPORTED;
  }

  ReportDesc = AllocateZeroPool (MouseHidDesc->HidClassDesc[0].DescriptorLength);
  ASSERT (ReportDesc != NULL);

  Status = UsbGetReportDescriptor (
             UsbIo,
             UsbMouseDev->InterfaceDescriptor.InterfaceNumber,
             MouseHidDesc->HidClassDesc[0].DescriptorLength,
             ReportDesc
             );

  if (EFI_ERROR (Status)) {
    FreePool (Buf);
    FreePool (ReportDesc);
    return Status;
  }

  //
  // Parse report descriptor
  //
  Status = ParseMouseReportDescriptor (
             UsbMouseDev,
             ReportDesc,
             MouseHidDesc->HidClassDesc[0].DescriptorLength
             );

  if (EFI_ERROR (Status)) {
    FreePool (Buf);
    FreePool (ReportDesc);
    return Status;
  }

  //
  // Check the presence of left and right buttons,
  // and initialize fields of EFI_SIMPLE_POINTER_MODE.
  //
  if (UsbMouseDev->NumberOfButtons >= 1) {
    UsbMouseDev->Mode.LeftButton = TRUE;
  }

  if (UsbMouseDev->NumberOfButtons > 1) {
    UsbMouseDev->Mode.RightButton = TRUE;
  }

  UsbMouseDev->Mode.ResolutionX = 8;
  UsbMouseDev->Mode.ResolutionY = 8;
  UsbMouseDev->Mode.ResolutionZ = 8;

  //
  // Set boot protocol for the USB mouse.
  // This driver only supports boot protocol.
  //
  UsbGetProtocolRequest (
    UsbIo,
    UsbMouseDev->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  if (Protocol != BOOT_PROTOCOL) {
    Status = UsbSetProtocolRequest (
               UsbIo,
               UsbMouseDev->InterfaceDescriptor.InterfaceNumber,
               BOOT_PROTOCOL
               );

    if (EFI_ERROR (Status)) {
      FreePool (Buf);
      FreePool (ReportDesc);
      return Status;
    }
  }

  FreePool (Buf);
  FreePool (ReportDesc);

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbMouseDev->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseDev->DelayedRecoveryEvent);
    UsbMouseDev->DelayedRecoveryEvent = 0;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         USBMouseRecoveryHandler,
         UsbMouseDev,
         &UsbMouseDev->DelayedRecoveryEvent
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
  @param  Context          Pointing to USB_MOUSE_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Result
  )
{
  USB_MOUSE_DEV        *UsbMouseDevice;
  EFI_USB_IO_PROTOCOL  *UsbIo;
  UINT8                EndpointAddr;
  UINT32               UsbResult;

  UsbMouseDevice = (USB_MOUSE_DEV *)Context;
  UsbIo          = UsbMouseDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR),
      UsbMouseDevice->DevicePath
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseDevice->IntEndpointDescriptor.EndpointAddress;

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
             UsbMouseDevice->IntEndpointDescriptor.EndpointAddress,
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
           UsbMouseDevice->DelayedRecoveryEvent,
           TimerRelative,
           EFI_USB_INTERRUPT_DELAY
           );
    return EFI_DEVICE_ERROR;
  }

  //
  // If no error and no data, just return EFI_SUCCESS.
  //
  if ((DataLength == 0) || (Data == NULL)) {
    return EFI_SUCCESS;
  }

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
  if (DataLength < 3) {
    return EFI_DEVICE_ERROR;
  }

  UsbMouseDevice->StateChanged = TRUE;

  UsbMouseDevice->State.LeftButton         = (BOOLEAN)((*(UINT8 *)Data & BIT0) != 0);
  UsbMouseDevice->State.RightButton        = (BOOLEAN)((*(UINT8 *)Data & BIT1) != 0);
  UsbMouseDevice->State.RelativeMovementX += *((INT8 *)Data + 1);
  UsbMouseDevice->State.RelativeMovementY += *((INT8 *)Data + 2);

  if (DataLength > 3) {
    UsbMouseDevice->State.RelativeMovementZ += *((INT8 *)Data + 3);
  }

  return EFI_SUCCESS;
}

/**
  Retrieves the current state of a pointer device.

  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL instance.
  @param  MouseState            A pointer to the state information on the pointer device.

  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.
  @retval EFI_INVALID_PARAMETER MouseState is NULL.

**/
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  )
{
  USB_MOUSE_DEV  *MouseDev;

  if (MouseState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MouseDev = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!MouseDev->StateChanged) {
    return EFI_NOT_READY;
  }

  //
  // Retrieve mouse state from USB_MOUSE_DEV, which was filled by OnMouseInterruptComplete()
  //
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

  MouseDev->StateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Resets the pointer device hardware.

  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL instance.
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  )
{
  USB_MOUSE_DEV  *UsbMouseDevice;

  UsbMouseDevice = USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET),
    UsbMouseDevice->DevicePath
    );

  //
  // Clear mouse state.
  //
  ZeroMem (
    &UsbMouseDevice->State,
    sizeof (EFI_SIMPLE_POINTER_STATE)
    );
  UsbMouseDevice->StateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Event notification function for EFI_SIMPLE_POINTER_PROTOCOL.WaitForInput event.

  @param  Event        Event to be signaled when there's input from mouse.
  @param  Context      Points to USB_MOUSE_DEV instance.

**/
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  USB_MOUSE_DEV  *UsbMouseDev;

  UsbMouseDev = (USB_MOUSE_DEV *)Context;

  //
  // If there's input from mouse, signal the event.
  //
  if (UsbMouseDev->StateChanged) {
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

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_MOUSE_DEV instance.

**/
VOID
EFIAPI
USBMouseRecoveryHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  )
{
  USB_MOUSE_DEV        *UsbMouseDev;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  UsbMouseDev = (USB_MOUSE_DEV *)Context;

  UsbIo = UsbMouseDev->UsbIo;

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseDev->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbMouseDev->IntEndpointDescriptor.Interval,
           UsbMouseDev->IntEndpointDescriptor.MaxPacketSize,
           OnMouseInterruptComplete,
           UsbMouseDev
           );
}
