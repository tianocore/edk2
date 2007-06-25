/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Ehci.c

Abstract:


Revision History
--*/


#include "Ehci.h"


GLOBAL_REMOVE_IF_UNREFERENCED UINTN    gEHCDebugLevel  = EFI_D_ERROR;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN    gEHCErrorLevel  = EFI_D_ERROR;


//
// Ehci Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gEhciDriverBinding = {
  EhciDriverBindingSupported,
  EhciDriverBindingStart,
  EhciDriverBindingStop,
  0xa,
  NULL,
  NULL
};


EFI_STATUS
EFIAPI
EhciDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
/*++

  Routine Description:

    Test to see if this driver supports ControllerHandle. Any ControllerHandle
    that has Usb2HcProtocol installed will be supported.

  Arguments:

    This                - Protocol instance pointer.
    Controlle           - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS       This driver supports this device.
    EFI_UNSUPPORTED   This driver does not support this device.

--*/
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  USB_CLASSC          UsbClassCReg;


  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto exit;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        CLASSC,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiPciIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    Status = EFI_UNSUPPORTED;
    goto exit;
  }

  //
  // Test whether the controller belongs to Ehci type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.PI != PCI_CLASSC_PI_EHCI)
      ) {

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    Status = EFI_UNSUPPORTED;
    goto exit;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
/*++

  Routine Description:

    Starting the Usb EHCI Driver

  Arguments:

    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:

    EFI_SUCCESS           supports this device.
    EFI_UNSUPPORTED       do not support this device.
    EFI_DEVICE_ERROR      cannot be started due to device Error
    EFI_OUT_OF_RESOURCES  cannot allocate resources

--*/
{
  EFI_STATUS            Status;
  USB2_HC_DEV           *HcDev;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT8                 MaxSpeed;
  UINT8                 PortNumber;
  UINT8                 Is64BitCapable;
  UINT64                Supports;

  //
  // Open the PciIo Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  // Enable the USB Host Controller
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto close_pciio_protocol;
  }

  //
  // Allocate memory for EHC private data structure
  //
  HcDev = AllocateZeroPool (sizeof (USB2_HC_DEV));
  if (NULL == HcDev) {
    Status = EFI_OUT_OF_RESOURCES;
    goto close_pciio_protocol;
  }

  //
  // Init EFI_USB2_HC_PROTOCOL interface and private data structure
  //
  HcDev->Usb2Hc.GetCapability             = EhciGetCapability;
  HcDev->Usb2Hc.Reset                     = EhciReset;
  HcDev->Usb2Hc.GetState                  = EhciGetState;
  HcDev->Usb2Hc.SetState                  = EhciSetState;
  HcDev->Usb2Hc.ControlTransfer           = EhciControlTransfer;
  HcDev->Usb2Hc.BulkTransfer              = EhciBulkTransfer;
  HcDev->Usb2Hc.AsyncInterruptTransfer    = EhciAsyncInterruptTransfer;
  HcDev->Usb2Hc.SyncInterruptTransfer     = EhciSyncInterruptTransfer;
  HcDev->Usb2Hc.IsochronousTransfer       = EhciIsochronousTransfer;
  HcDev->Usb2Hc.AsyncIsochronousTransfer  = EhciAsyncIsochronousTransfer;
  HcDev->Usb2Hc.GetRootHubPortStatus      = EhciGetRootHubPortStatus;
  HcDev->Usb2Hc.SetRootHubPortFeature     = EhciSetRootHubPortFeature;
  HcDev->Usb2Hc.ClearRootHubPortFeature   = EhciClearRootHubPortFeature;
  HcDev->Usb2Hc.MajorRevision             = 0x1;
  HcDev->Usb2Hc.MinorRevision             = 0x1;

  HcDev->AsyncRequestList                 = NULL;
  HcDev->ControllerNameTable              = NULL;
  HcDev->Signature                        = USB2_HC_DEV_SIGNATURE;
  HcDev->PciIo = PciIo;

  //
  // Install USB2_HC_PROTOCOL
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HcDev->Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto free_pool;
  }

  //
  // Get Capability Register Length
  //
  Status = GetCapabilityLen (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto uninstall_usb2hc_protocol;
  }

  ClearLegacySupport (HcDev);
  HostReset (HcDev);

  DEBUG_CODE (
   DumpEHCIPortsStatus (HcDev);
  );

  //
  // Create and Init Perodic Frame List
  //
  Status = EhciGetCapability (
             &HcDev->Usb2Hc,
             &MaxSpeed,
             &PortNumber,
             &Is64BitCapable
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto uninstall_usb2hc_protocol;
  }
  HcDev->Is64BitCapable = Is64BitCapable;

  //
  // Create and Init Perodic Frame List
  //
  Status = InitialPeriodicFrameList (
             HcDev,
             EHCI_MAX_FRAME_LIST_LENGTH
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto uninstall_usb2hc_protocol;
  }

  //
  // Init memory pool management
  //
  Status = InitialMemoryManagement (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto deinit_perodic_frame_list;
  }

  Status = CreateNULLQH (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto deinit_perodic_frame_list;
  }
  //
  // Create AsyncRequest Polling Timer
  //
  Status = CreatePollingTimer (HcDev, (EFI_EVENT_NOTIFY) AsyncRequestMoniter);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto deinit_null_qh;
  }

  //
  // Default Maxximum Interrupt Interval is 8,
  // it means that 8 micro frame = 1ms
  //

  //
  // Start the Host Controller
  //
  if (IsEhcHalted (HcDev)) {
    Status = StartScheduleExecution (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto deinit_timer;
    }
  }

  //
  // Set all ports routing to EHC
  //
  Status = SetPortRoutingEhc (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto deinit_timer;
  }

  //
  // Component name protocol
  //
  Status = AddUnicodeString (
             "eng",
             gEhciComponentName.SupportedLanguages,
             &HcDev->ControllerNameTable,
             L"Usb Enhanced Host Controller"
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto deinit_timer;
  }

  goto exit;

  //
  // Error handle process
  //
deinit_timer:
  DestoryPollingTimer (HcDev);
deinit_null_qh:
  DestroyNULLQH(HcDev);
  DeinitialMemoryManagement (HcDev);
deinit_perodic_frame_list:
  DeinitialPeriodicFrameList (HcDev);
uninstall_usb2hc_protocol:
  gBS->UninstallProtocolInterface (
         Controller,
         &gEfiUsb2HcProtocolGuid,
         &HcDev->Usb2Hc
         );
free_pool:
  gBS->FreePool (HcDev);
close_pciio_protocol:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
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

    EFI_SUCCESS         Success
    EFI_DEVICE_ERROR    Fail
--*/
{
  EFI_STATUS            Status;
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  USB2_HC_DEV           *HcDev;
  UINT64                Supports;

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  HcDev = USB2_HC_DEV_FROM_THIS (Usb2Hc);

  //
  // free all the controller related memory and uninstall UHCI Protocol.
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Set Host Controller state as halt
  //
  Status = Usb2Hc->SetState (
                     Usb2Hc,
                     EfiUsbHcStateHalt
                     );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Stop AsyncRequest Polling Timer
  //
  Status = StopPollingTimer (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Destroy Asynchronous Request Event
  //
  DestoryPollingTimer (HcDev);

  //
  // Destroy Perodic Frame List
  //
  DeinitialPeriodicFrameList (HcDev);

  //
  // Destroy NULLQH
  //
  DestroyNULLQH (HcDev);

  //
  // Deinit Ehci pool memory management
  //
  DeinitialMemoryManagement (HcDev);

  //
  // Denint Unicode String Table
  //
  FreeUnicodeStringTable (HcDev->ControllerNameTable);

  //
  // Disable the USB Host Controller
  //
  Status = HcDev->PciIo->Attributes (
                           HcDev->PciIo,
                           EfiPciIoAttributeOperationSupported,
                           0,
                           &Supports
                           );
  if (!EFI_ERROR (Status)) {
    Supports &= EFI_PCI_DEVICE_ENABLE;
    Status = HcDev->PciIo->Attributes (
                             HcDev->PciIo,
                             EfiPciIoAttributeOperationDisable,
                             Supports,
                             NULL
                             );
  }
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  gBS->FreePool (HcDev);

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciGetCapability (
  IN  EFI_USB2_HC_PROTOCOL *This,
  OUT UINT8                *MaxSpeed,
  OUT UINT8                *PortNumber,
  OUT UINT8                *Is64BitCapable
  )
/*++

  Routine Description:

    Retrieves the capablility of root hub ports.

  Arguments:

    This            - A pointer to the EFI_USB_HC_PROTOCOL instance.
    MaxSpeed        - A pointer to the number of the host controller.
    PortNumber      - A pointer to the number of the root hub ports.
    Is64BitCapable  - A pointer to the flag for whether controller supports
                      64-bit memory addressing.

  Returns:

    EFI_SUCCESS            host controller capability were retrieved successfully.
    EFI_INVALID_PARAMETER  MaxSpeed or PortNumber or Is64BitCapable is NULL.
    EFI_DEVICE_ERROR       An error was encountered while attempting to retrieve the capabilities.

--*/
{
  EFI_STATUS  Status;
  USB2_HC_DEV *HcDev;
  UINT32      HcStructParamsAddr;
  UINT32      HcStructParamsReg;
  UINT32      HcCapParamsAddr;
  UINT32      HcCapParamsReg;

  if (MaxSpeed == NULL || PortNumber == NULL || Is64BitCapable == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  HcStructParamsAddr  = HCSPARAMS;
  HcCapParamsAddr     = HCCPARAMS;
  HcDev               = USB2_HC_DEV_FROM_THIS (This);

  Status = ReadEhcCapabiltiyReg (
             HcDev,
             HcStructParamsAddr,
             &HcStructParamsReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = ReadEhcCapabiltiyReg (
             HcDev,
             HcCapParamsAddr,
             &HcCapParamsReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  *MaxSpeed       = EFI_USB_SPEED_HIGH;
  *PortNumber     = (UINT8) (HcStructParamsReg & HCSP_NPORTS);
  *Is64BitCapable = (UINT8) (HcCapParamsReg & HCCP_64BIT);

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciReset (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN UINT16               Attributes
  )
/*++

  Routine Description:

    Provides software reset for the USB host controller.

  Arguments:

  This        - A pointer to the EFI_USB2_HC_PROTOCOL instance.
  Attributes  - A bit mask of the reset operation to perform.
                See below for a list of the supported bit mask values.

  #define EFI_USB_HC_RESET_GLOBAL  0x0001
  #define EFI_USB_HC_RESET_HOST_CONTROLLER  0x0002
  #define EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG  0x0004
  #define EFI_USB_HC_RESET_HOST_WITH_DEBUG  0x0008

  EFI_USB_HC_RESET_GLOBAL
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host
        controller hardware and all the devices attached on the USB bus.
  EFI_USB_HC_RESET_HOST_CONTROLLER
        If this bit is set, the USB host controller hardware will be reset.
        No reset signal will be sent to the USB bus.
  EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host
        controller hardware and all the devices attached on the USB bus.
        If this is an EHCI controller and the debug port has configured, then
        this is will still reset the host controller.
  EFI_USB_HC_RESET_HOST_WITH_DEBUG
        If this bit is set, the USB host controller hardware will be reset.
        If this is an EHCI controller and the debug port has been configured,
        then this will still reset the host controller.

  Returns:

    EFI_SUCCESS
        The reset operation succeeded.
    EFI_INVALID_PARAMETER
        Attributes is not valid.
    EFI_UNSUPPOURTED
        The type of reset specified by Attributes is not currently supported by
        the host controller hardware.
    EFI_ACCESS_DENIED
        Reset operation is rejected due to the debug port being configured and
        active; only EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG or
        EFI_USB_HC_RESET_HOST_WITH_DEBUG reset Atrributes can be used to
        perform reset operation for this host controller.
    EFI_DEVICE_ERROR
        An error was encountered while attempting to perform
        the reset operation.

--*/
{
  EFI_STATUS        Status;
  USB2_HC_DEV       *HcDev;
  UINTN             FrameIndex;
  FRAME_LIST_ENTRY  *FrameEntryPtr;

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  switch (Attributes) {

  case EFI_USB_HC_RESET_GLOBAL:

    //
    // Same behavior as Host Controller Reset
    //

  case EFI_USB_HC_RESET_HOST_CONTROLLER:
  	
    //
    // Host Controller must be Halt when Reset it
    //
    if (IsEhcHalted (HcDev)) {
      Status = ResetEhc (HcDev);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
      //
      // Set to zero by Host Controller when reset process completes
      //
      Status = WaitForEhcReset (HcDev, EHCI_GENERIC_TIMEOUT);
      if (EFI_ERROR (Status)) {
        Status = EFI_TIMEOUT;
        goto exit;
      }
    } else {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    //
    // only asynchronous interrupt transfers are always alive on the bus, need to cleanup
    //
    CleanUpAllAsyncRequestTransfer (HcDev);
    Status = ClearEhcAllStatus (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    //
    // Set appropriate 4G Segment Selector
    //
    Status = SetCtrlDataStructSeg (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    //
    // Init Perodic List Base Addr and Frame List
    //
    Status = SetFrameListBaseAddr (
               HcDev,
               (UINT32)GET_0B_TO_31B (HcDev->PeriodicFrameListBuffer)
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
    FrameEntryPtr = (FRAME_LIST_ENTRY *) HcDev->PeriodicFrameListBuffer;
    for (FrameIndex = 0; FrameIndex < HcDev->PeriodicFrameListLength; FrameIndex++) {
      FrameEntryPtr->LinkTerminate = TRUE;
      FrameEntryPtr++;
    }

    //
    // Start the Host Controller
    //
    if (IsEhcHalted (HcDev)) {
      Status = StartScheduleExecution (HcDev);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
    }

    //
    // Set all ports routing to EHC
    //
    Status = SetPortRoutingEhc (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
    break;

  case EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG:
  	
    Status = EFI_UNSUPPORTED;
    break;

  case EFI_USB_HC_RESET_HOST_WITH_DEBUG:
  	
    Status = EFI_UNSUPPORTED;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciGetState (
  IN  EFI_USB2_HC_PROTOCOL *This,
  OUT EFI_USB_HC_STATE     *State
  )
/*++

  Routine Description:

    Retrieves current state of the USB host controller.

  Arguments:

    This      A pointer to the EFI_USB2_HC_PROTOCOL instance.
    State     A pointer to the EFI_USB_HC_STATE data structure that
              indicates current state of the USB host controller.
              Type EFI_USB_HC_STATE is defined below.

    typedef enum {
      EfiUsbHcStateHalt,
      EfiUsbHcStateOperational,
      EfiUsbHcStateSuspend,
      EfiUsbHcStateMaximum
    } EFI_USB_HC_STATE;

  Returns:

    EFI_SUCCESS
            The state information of the host controller was returned in State.
    EFI_INVALID_PARAMETER
            State is NULL.
    EFI_DEVICE_ERROR
            An error was encountered while attempting to retrieve the
            host controller's current state.
--*/
{
  EFI_STATUS  Status;
  USB2_HC_DEV *HcDev;
  UINT32      UsbStatusAddr;
  UINT32      UsbStatusReg;

  if (State == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  UsbStatusAddr = USBSTS;
  HcDev         = USB2_HC_DEV_FROM_THIS (This);

  Status = ReadEhcOperationalReg (
             HcDev,
             UsbStatusAddr,
             &UsbStatusReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  if (UsbStatusReg & USBSTS_HCH) {
    *State = EfiUsbHcStateHalt;
  } else {
    *State = EfiUsbHcStateOperational;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciSetState (
  IN EFI_USB2_HC_PROTOCOL *This,
  IN EFI_USB_HC_STATE     State
  )
/*++

  Routine Description:

    Sets the USB host controller to a specific state.

  Arguments:

    This     - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    State    - Indicates the state of the host controller that will be set.

  Returns:

    EFI_SUCCESS
          The USB host controller was successfully placed in the state
          specified by State.
    EFI_INVALID_PARAMETER
          State is invalid.
    EFI_DEVICE_ERROR
          Failed to set the state specified by State due to device error.

--*/
{
  EFI_STATUS        Status;
  USB2_HC_DEV       *HcDev;
  UINT32            UsbCommandAddr;
  UINT32            UsbCommandReg;
  EFI_USB_HC_STATE  CurrentState;

  UsbCommandAddr  = USBCMD;
  HcDev           = USB2_HC_DEV_FROM_THIS (This);

  Status          = EhciGetState (This, &CurrentState);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  switch (State) {

  case EfiUsbHcStateHalt:

    if (EfiUsbHcStateHalt == CurrentState) {
      Status = EFI_SUCCESS;
      goto exit;
    } else if (EfiUsbHcStateOperational == CurrentState) {
      Status = ReadEhcOperationalReg (
                 HcDev,
                 UsbCommandAddr,
                 &UsbCommandReg
                 );
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }

      UsbCommandReg &= ~USBCMD_RS;
      Status = WriteEhcOperationalReg (
                 HcDev,
                 UsbCommandAddr,
                 UsbCommandReg
                 );
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
      //
      // Ensure the HC is in halt status after send the stop command
      //
      Status = WaitForEhcHalt (HcDev, EHCI_GENERIC_TIMEOUT);
      if (EFI_ERROR (Status)) {
        Status = EFI_TIMEOUT;
        goto exit;
      }
    }
    break;

  case EfiUsbHcStateOperational:
  	
    if (IsEhcSysError (HcDev)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
    if (EfiUsbHcStateOperational == CurrentState) {
      Status = EFI_SUCCESS;
      goto exit;
    } else if (EfiUsbHcStateHalt == CurrentState) {
      //
      // Set Host Controller Run
      //
      Status = ReadEhcOperationalReg (
                 HcDev,
                 UsbCommandAddr,
                 &UsbCommandReg
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      UsbCommandReg |= USBCMD_RS;
      Status = WriteEhcOperationalReg (
                 HcDev,
                 UsbCommandAddr,
                 UsbCommandReg
                 );
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
    }
    break;

  case EfiUsbHcStateSuspend:
  	
    Status = EFI_UNSUPPORTED;
    break;

  default:
  	
    Status = EFI_INVALID_PARAMETER;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciGetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL *This,
  IN  UINT8                PortNumber,
  OUT EFI_USB_PORT_STATUS  *PortStatus
  )
/*++

  Routine Description:

    Retrieves the current status of a USB root hub port.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL.
    PortNumber  - Specifies the root hub port from which the status
                  is to be retrieved.  This value is zero-based. For example,
                  if a root hub has two ports, then the first port is numbered 0,
                  and the second port is numbered 1.
    PortStatus  - A pointer to the current port status bits and
                  port status change bits.

  Returns:

    EFI_SUCCESS           The status of the USB root hub port specified
                          by PortNumber was returned in PortStatus.
    EFI_INVALID_PARAMETER PortNumber is invalid.
    EFI_DEVICE_ERROR      Can't read register

--*/
{
  EFI_STATUS  Status;
  USB2_HC_DEV *HcDev;
  UINT32      PortStatusControlAddr;
  UINT32      PortStatusControlReg;
  UINT8       MaxSpeed;
  UINT8       TotalPortNumber;
  UINT8       Is64BitCapable;

  if (PortStatus == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  EhciGetCapability (
    This,
    &MaxSpeed,
    &TotalPortNumber,
    &Is64BitCapable
    );

  if (PortNumber >= TotalPortNumber) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  HcDev                 = USB2_HC_DEV_FROM_THIS (This);
  PortStatusControlAddr = (UINT32) (PORTSC + (4 * PortNumber));

  //
  // Clear port status
  //
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  Status = ReadEhcOperationalReg (
             HcDev,
             PortStatusControlAddr,
             &PortStatusControlReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  //    Fill Port Status bits
  //

  //
  // Current Connect Status
  //
  if (PORTSC_CCS & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }
  //
  // Port Enabled/Disabled
  //
  if (PORTSC_PED & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }
  //
  // Port Suspend
  //
  if (PORTSC_SUSP & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }
  //
  // Over-current Active
  //
  if (PORTSC_OCA & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_OVERCURRENT;
  }
  //
  // Port Reset
  //
  if (PORTSC_PR & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_RESET;
  }
  //
  // Port Power
  //
  if (PORTSC_PP & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_POWER;
  }
  //
  // Port Owner
  //
  if (PORTSC_PO & PortStatusControlReg) {
    PortStatus->PortStatus |= USB_PORT_STAT_OWNER;
  }
  //
  // Identify device speed
  //
  if (PORTSC_LS_KSTATE & PortStatusControlReg) {
    //
    // Low Speed Device Attached
    //
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
  } else {
    //
    // Not Low Speed Device Attached
    //
    if ((PORTSC_CCS & PortStatusControlReg) && (PORTSC_CSC & PortStatusControlReg)) {
      HcDev->DeviceSpeed[PortNumber] = (UINT16) (IsHighSpeedDevice (This, PortNumber) ? USB_PORT_STAT_HIGH_SPEED : 0);
    }
    PortStatus->PortStatus = (UINT16) (PortStatus->PortStatus | HcDev->DeviceSpeed[PortNumber]);
  }
  //
  // Fill Port Status Change bits
  //
  //
  // Connect Status Change
  //
  if (PORTSC_CSC & PortStatusControlReg) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }
  //
  // Port Enabled/Disabled Change
  //
  if (PORTSC_PEDC & PortStatusControlReg) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }
  //
  // Port Over Current Change
  //
  if (PORTSC_OCC & PortStatusControlReg) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_OVERCURRENT;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciSetRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT8                    PortNumber,
  IN  EFI_USB_PORT_FEATURE     PortFeature
  )
/*++

  Routine Description:

    Sets a feature for the specified root hub port.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be set.
    PortFeature - Indicates the feature selector associated
                  with the feature set request.

  Returns:

    EFI_SUCCESS
        The feature specified by PortFeature was set for the
        USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER
        PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR
        Can't read register

--*/
{
  EFI_STATUS  Status;
  USB2_HC_DEV *HcDev;
  UINT32      PortStatusControlAddr;
  UINT32      PortStatusControlReg;
  UINT8       MaxSpeed;
  UINT8       TotalPortNumber;
  UINT8       Is64BitCapable;

  EhciGetCapability (
    This,
    &MaxSpeed,
    &TotalPortNumber,
    &Is64BitCapable
    );

  if (PortNumber >= TotalPortNumber) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  HcDev                 = USB2_HC_DEV_FROM_THIS (This);
  PortStatusControlAddr = (UINT32) (PORTSC + (4 * PortNumber));

  Status = ReadEhcOperationalReg (
             HcDev,
             PortStatusControlAddr,
             &PortStatusControlReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  switch (PortFeature) {

  case EfiUsbPortEnable:

    //
    // Sofeware can't set this bit, Port can only be enable by the Host Controller
    // as a part of the reset and enable
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_PED;
    break;

  case EfiUsbPortSuspend:

    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_SUSP;
    break;

  case EfiUsbPortReset:

    //
    // Make sure Host Controller not halt before reset it
    //
    if (IsEhcHalted (HcDev)) {
      Status = StartScheduleExecution (HcDev);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
      Status = WaitForEhcNotHalt (HcDev, EHCI_GENERIC_TIMEOUT);
      if (EFI_ERROR (Status)) {
        DEBUG ((gEHCDebugLevel, "EHCI: WaitForEhcNotHalt TimeOut\n"));
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
    }
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_PR;
    //
    // Set one to PortReset bit must also set zero to PortEnable bit
    //
    PortStatusControlReg &= ~PORTSC_PED;
    break;

  case EfiUsbPortPower:

    //
    // No support, no operation
    //
    goto exit;

  case EfiUsbPortOwner:

    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_PO;
    break;

  default:
  	
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  Status = WriteEhcOperationalReg (
             HcDev,
             PortStatusControlAddr,
             PortStatusControlReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciClearRootHubPortFeature (
  IN  EFI_USB2_HC_PROTOCOL     *This,
  IN  UINT8                    PortNumber,
  IN  EFI_USB_PORT_FEATURE     PortFeature
  )
/*++

  Routine Description:

    Clears a feature for the specified root hub port.

  Arguments:

    This        - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    PortNumber  - Specifies the root hub port whose feature
                  is requested to be cleared.
    PortFeature - Indicates the feature selector associated with the
                  feature clear request.

  Returns:

    EFI_SUCCESS
        The feature specified by PortFeature was cleared for the
        USB root hub port specified by PortNumber.
    EFI_INVALID_PARAMETER
        PortNumber is invalid or PortFeature is invalid.
    EFI_DEVICE_ERROR
        Can't read register

--*/
{
  EFI_STATUS  Status;
  USB2_HC_DEV *HcDev;
  UINT32      PortStatusControlAddr;
  UINT32      PortStatusControlReg;
  UINT8       MaxSpeed;
  UINT8       TotalPortNumber;
  UINT8       Is64BitCapable;

  EhciGetCapability (
    This,
    &MaxSpeed,
    &TotalPortNumber,
    &Is64BitCapable
    );

  if (PortNumber >= TotalPortNumber) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  HcDev                 = USB2_HC_DEV_FROM_THIS (This);
  PortStatusControlAddr = (UINT32) (PORTSC + (4 * PortNumber));

  Status = ReadEhcOperationalReg (
             HcDev,
             PortStatusControlAddr,
             &PortStatusControlReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  switch (PortFeature) {

  case EfiUsbPortEnable:

    //
    // Clear PORT_ENABLE feature means disable port.
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg &= ~PORTSC_PED;
    break;

  case EfiUsbPortSuspend:

    //
    // A write of zero to this bit is ignored by the host controller.
    // The host controller will unconditionally set this bit to a zero when:
    //   1. software sets the Forct Port Resume bit to a zero from a one.
    //   2. software sets the Port Reset bit to a one frome a zero.
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg &= ~PORTSC_FPR;
    break;

  case EfiUsbPortReset:

    //
    // Clear PORT_RESET means clear the reset signal.
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg &= ~PORTSC_PR;
    break;

  case EfiUsbPortPower:

    //
    // No support, no operation
    //
    goto exit;

  case EfiUsbPortOwner:

    //
    // Clear port owner means this port owned by EHC
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg &= ~PORTSC_PO;
    break;

  case EfiUsbPortConnectChange:

    //
    // Clear connect status change
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_CSC;
    break;

  case EfiUsbPortEnableChange:

    //
    // Clear enable status change
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_PEDC;
    break;

  case EfiUsbPortSuspendChange:

    //
    // No related bit, no operation
    //
    goto exit;

  case EfiUsbPortOverCurrentChange:

    //
    // Clear PortOverCurrent change
    //
    PortStatusControlReg &= 0xffffffd5;
    PortStatusControlReg |= PORTSC_OCC;
    break;

  case EfiUsbPortResetChange:

    //
    // No related bit, no operation
    //
    goto exit;

  default:
  	
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  Status = WriteEhcOperationalReg (
             HcDev,
             PortStatusControlAddr,
             PortStatusControlReg
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciControlTransfer (
  IN  EFI_USB2_HC_PROTOCOL                 *This,
  IN  UINT8                                DeviceAddress,
  IN  UINT8                                DeviceSpeed,
  IN  UINTN                                MaximumPacketLength,
  IN  EFI_USB_DEVICE_REQUEST               *Request,
  IN  EFI_USB_DATA_DIRECTION               TransferDirection,
  IN  OUT VOID                             *Data,
  IN  OUT UINTN                            *DataLength,
  IN  UINTN                                TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT UINT32                               *TransferResult
  )
/*++

  Routine Description:

    Submits control transfer to a target USB device.

  Arguments:

    This            - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress   - Represents the address of the target device on the USB,
                      which is assigned during USB enumeration.
    DeviceSpeed     - Indicates target device speed.
    MaximumPacketLength - Indicates the maximum packet size that the
                         default control transfer endpoint is capable of
                         sending or receiving.
    Request         - A pointer to the USB device request that will be sent
                      to the USB device.
    TransferDirection - Specifies the data direction for the transfer.
                        There are three values available, DataIn, DataOut
                        and NoData.
    Data            - A pointer to the buffer of data that will be transmitted
                      to USB device or received from USB device.
    DataLength      - Indicates the size, in bytes, of the data buffer
                      specified by Data.
    TimeOut         - Indicates the maximum time, in microseconds,
                      which the transfer is allowed to complete.
    Translator      - A pointr to the transaction translator data.
    TransferResult  - A pointer to the detailed result information generated
                      by this control transfer.

  Returns:

    EFI_SUCCESS
        The control transfer was completed successfully.
    EFI_OUT_OF_RESOURCES
        The control transfer could not be completed due to a lack of resources.
    EFI_INVALID_PARAMETER
        Some parameters are invalid.
    EFI_TIMEOUT
        The control transfer failed due to timeout.
    EFI_DEVICE_ERROR
        The control transfer failed due to host controller or device error.
        Caller should check TranferResult for detailed error information.

--*/
{
  EFI_STATUS      Status;
  USB2_HC_DEV     *HcDev;
  UINT8           PktId;
  EHCI_QH_ENTITY  *QhPtr;
  EHCI_QTD_ENTITY *ControlQtdsPtr;
  UINT8           *DataCursor;
  VOID            *DataMap;
  UINT8           *RequestCursor;
  VOID            *RequestMap;

  QhPtr           = NULL;
  ControlQtdsPtr  = NULL;
  DataCursor      = NULL;
  DataMap         = NULL;
  RequestCursor   = NULL;
  RequestMap      = NULL;
  HcDev           = USB2_HC_DEV_FROM_THIS (This);

  //
  // Parameters Checking
  //
  if (TransferDirection != EfiUsbDataIn &&
  	TransferDirection != EfiUsbDataOut &&
  	TransferDirection != EfiUsbNoData
  	) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EfiUsbNoData == TransferDirection) {
    if (NULL != Data || 0 != *DataLength) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  } else {
    if (NULL == Data || 0 == *DataLength) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  }

  if (Request == NULL || TransferResult == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_LOW == DeviceSpeed) {
    if (MaximumPacketLength != 8) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  } else if (MaximumPacketLength != 8 &&
           MaximumPacketLength != 16 &&
           MaximumPacketLength != 32 &&
           MaximumPacketLength != 64
          ) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  //
  // If errors exist that cause host controller halt,
  // then return EFI_DEVICE_ERROR.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    ClearEhcAllStatus (HcDev);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Map the Request for bus master access.
  // BusMasterRead means cpu write
  //
  Status = MapRequestBuffer (
             HcDev,
             Request,
             &RequestCursor,
             &RequestMap
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Map the source data buffer for bus master access.
  //
  Status = MapDataBuffer (
             HcDev,
             TransferDirection,
             Data,
             DataLength,
             &PktId,
             &DataCursor,
             &DataMap
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto unmap_request;
  }

  //
  // Create and init control Qh
  //
  Status = CreateControlQh (
             HcDev,
             DeviceAddress,
             DeviceSpeed,
             MaximumPacketLength,
             Translator,
             &QhPtr
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;
    goto unmap_data;
  }

  //
  // Create and init control Qtds
  //
  Status = CreateControlQtds (
             HcDev,
             PktId,
             RequestCursor,
             DataCursor,
             *DataLength,
             Translator,
             &ControlQtdsPtr
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;
    goto destory_qh;
  }

  //
  // Link Qtds to Qh
  //
  LinkQtdToQh (QhPtr, ControlQtdsPtr);

  ClearEhcAllStatus (HcDev);

  //
  // Link Qh and Qtds to Async Schedule List
  //
  Status = LinkQhToAsyncList (HcDev, QhPtr);
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto destory_qtds;
  }

  //
  // Poll Qh-Qtds execution and get result.
  // detail status is returned
  //
  Status = ExecuteTransfer (
             HcDev,
             TRUE,
             QhPtr,
             DataLength,
             0,
             TimeOut,
             TransferResult
             );
  if (EFI_ERROR (Status)) {
    goto destory_qtds;
  }

  //
  // If has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    *TransferResult |= EFI_USB_ERR_SYSTEM;
  }

  ClearEhcAllStatus (HcDev);

destory_qtds:
  UnlinkQhFromAsyncList (HcDev, QhPtr);
  DestoryQtds (HcDev, ControlQtdsPtr);
destory_qh:
  DestoryQh (HcDev, QhPtr);
unmap_data:
  HcDev->PciIo->Unmap (HcDev->PciIo, DataMap);
unmap_request:
  HcDev->PciIo->Unmap (HcDev->PciIo, RequestMap);
exit:
  HcDev->PciIo->Flush (HcDev->PciIo);
  return Status;
}

EFI_STATUS
EFIAPI
EhciBulkTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *TransferResult
  )
/*++

  Routine Description:

    Submits bulk transfer to a bulk endpoint of a USB device.

  Arguments:

    This              - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress     - Represents the address of the target device on the USB,
                        which is assigned during USB enumeration.
    EndPointAddress   - The combination of an endpoint number and an
                        endpoint direction of the target USB device.
                        Each endpoint address supports data transfer in
                        one direction except the control endpoint
                        (whose default endpoint address is 0).
                        It is the caller's responsibility to make sure that
                        the EndPointAddress represents a bulk endpoint.
    DeviceSpeed       - Indicates device speed. The supported values are EFI_USB_SPEED_FULL
                        and EFI_USB_SPEED_HIGH.
    MaximumPacketLength - Indicates the maximum packet size the target endpoint
                          is capable of sending or receiving.
    DataBuffersNumber - Number of data buffers prepared for the transfer.
    Data              - Array of pointers to the buffers of data that will be transmitted
                        to USB device or received from USB device.
    DataLength        - When input, indicates the size, in bytes, of the data buffer
                        specified by Data. When output, indicates the actually
                        transferred data size.
    DataToggle        - A pointer to the data toggle value. On input, it indicates
                        the initial data toggle value the bulk transfer should adopt;
                        on output, it is updated to indicate the data toggle value
                        of the subsequent bulk transfer.
    Translator        - A pointr to the transaction translator data.
    TimeOut           - Indicates the maximum time, in microseconds, which the
                        transfer is allowed to complete.
    TransferResult    - A pointer to the detailed result information of the
                        bulk transfer.

  Returns:

    EFI_SUCCESS
        The bulk transfer was completed successfully.
    EFI_OUT_OF_RESOURCES
        The bulk transfer could not be submitted due to lack of resource.
    EFI_INVALID_PARAMETER
        Some parameters are invalid.
    EFI_TIMEOUT
        The bulk transfer failed due to timeout.
    EFI_DEVICE_ERROR
        The bulk transfer failed due to host controller or device error.
        Caller should check TranferResult for detailed error information.

--*/
{
  EFI_STATUS              Status;
  USB2_HC_DEV             *HcDev;
  UINT8                   PktId;
  EHCI_QH_ENTITY          *QhPtr;
  EHCI_QTD_ENTITY         *BulkQtdsPtr;
  UINT8                   *DataCursor;
  VOID                    *DataMap;
  EFI_USB_DATA_DIRECTION  TransferDirection;

  QhPtr       = NULL;
  BulkQtdsPtr = NULL;
  DataCursor  = NULL;
  DataMap     = NULL;
  HcDev       = USB2_HC_DEV_FROM_THIS (This);

  //
  // Parameters Checking
  //
  if (NULL == DataLength ||
  	NULL == Data ||
  	NULL == Data[0] ||
  	NULL == TransferResult
  	) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (*DataLength == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (1 != *DataToggle && 0 != *DataToggle) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_LOW == DeviceSpeed) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_FULL == DeviceSpeed) {
    if (MaximumPacketLength > 64) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  }

  if (EFI_USB_SPEED_HIGH == DeviceSpeed) {
    if (MaximumPacketLength > 512) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    ClearEhcAllStatus (HcDev);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = ClearEhcAllStatus (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // construct QH and TD data structures,
  // and link them together
  //
  if (EndPointAddress & 0x80) {
    TransferDirection = EfiUsbDataIn;
  } else {
    TransferDirection = EfiUsbDataOut;
  }

  Status = MapDataBuffer (
             HcDev,
             TransferDirection,
             Data[0],
             DataLength,
             &PktId,
             &DataCursor,
             &DataMap
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Create and init Bulk Qh
  //
  Status = CreateBulkQh (
             HcDev,
             DeviceAddress,
             EndPointAddress,
             DeviceSpeed,
             *DataToggle,
             MaximumPacketLength,
             Translator,
             &QhPtr
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;
    goto unmap_data;
  }

  //
  // Create and init Bulk Qtds
  //
  Status = CreateBulkOrInterruptQtds (
             HcDev,
             PktId,
             DataCursor,
             *DataLength,
             Translator,
             &BulkQtdsPtr
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;
    goto destory_qh;
  }

  //
  // Link Qtds to Qh
  //
  LinkQtdToQh (QhPtr, BulkQtdsPtr);

  ClearEhcAllStatus (HcDev);

  //
  // Link Qh and qtds to Async Schedule List
  //
  Status = LinkQhToAsyncList (HcDev, QhPtr);
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto destory_qtds;
  }

  //
  // Poll QH-TDs execution and get result.
  // detail status is returned
  //
  Status = ExecuteTransfer (
             HcDev,
             FALSE,
             QhPtr,
             DataLength,
             DataToggle,
             TimeOut,
             TransferResult
             );
  if (EFI_ERROR (Status)) {
    goto destory_qtds;
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    *TransferResult |= EFI_USB_ERR_SYSTEM;
  }

  ClearEhcAllStatus (HcDev);

destory_qtds:
  UnlinkQhFromAsyncList (HcDev, QhPtr);
  DestoryQtds (HcDev, BulkQtdsPtr);
destory_qh:
  DestoryQh (HcDev, QhPtr);
unmap_data:
  HcDev->PciIo->Unmap (HcDev->PciIo, DataMap);
exit:
  HcDev->PciIo->Flush (HcDev->PciIo);
  return Status;
}

EFI_STATUS
EFIAPI
EhciAsyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  * This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    * Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  )
/*++

  Routine Description:

    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device.
    Translator parameter doesn't exist in UEFI2.0 spec, but it will be updated
    in the following specification version.

  Arguments:

    This            - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress   - Represents the address of the target device on the USB,
                      which is assigned during USB enumeration.
    EndPointAddress - The combination of an endpoint number and an endpoint
                      direction of the target USB device. Each endpoint address
                      supports data transfer in one direction except the
                      control endpoint (whose default endpoint address is 0).
                      It is the caller's responsibility to make sure that
                      the EndPointAddress represents an interrupt endpoint.
    DeviceSpeed     - Indicates device speed.
    MaximumPacketLength  - Indicates the maximum packet size the target endpoint
                           is capable of sending or receiving.
    IsNewTransfer   - If TRUE, an asynchronous interrupt pipe is built between
                      the host and the target interrupt endpoint.
                      If FALSE, the specified asynchronous interrupt pipe
                      is canceled.
    DataToggle      - A pointer to the data toggle value.  On input, it is valid
                      when IsNewTransfer is TRUE, and it indicates the initial
                      data toggle value the asynchronous interrupt transfer
                      should adopt.
                      On output, it is valid when IsNewTransfer is FALSE,
                      and it is updated to indicate the data toggle value of
                      the subsequent asynchronous interrupt transfer.
    PollingInterval - Indicates the interval, in milliseconds, that the
                      asynchronous interrupt transfer is polled.
                      This parameter is required when IsNewTransfer is TRUE.
    DataLength      - Indicates the length of data to be received at the
                      rate specified by PollingInterval from the target
                      asynchronous interrupt endpoint.  This parameter
                      is only required when IsNewTransfer is TRUE.
    Translator      - A pointr to the transaction translator data.
    CallBackFunction  - The Callback function.This function is called at the
                        rate specified by PollingInterval.This parameter is
                        only required when IsNewTransfer is TRUE.
    Context         - The context that is passed to the CallBackFunction.
                    - This is an optional parameter and may be NULL.

  Returns:

    EFI_SUCCESS
        The asynchronous interrupt transfer request has been successfully
        submitted or canceled.
    EFI_INVALID_PARAMETER
        Some parameters are invalid.
    EFI_OUT_OF_RESOURCES
        The request could not be completed due to a lack of resources.
    EFI_DEVICE_ERROR
        Can't read register

--*/
{
  EFI_STATUS          Status;
  USB2_HC_DEV         *HcDev;
  UINT8               PktId;
  EHCI_QH_ENTITY      *QhPtr;
  EHCI_QTD_ENTITY     *InterruptQtdsPtr;
  UINT8               *DataPtr;
  UINT8               *DataCursor;
  VOID                *DataMap;
  UINTN               MappedLength;
  EHCI_ASYNC_REQUEST  *AsyncRequestPtr;
  EFI_TPL             OldTpl;

  QhPtr             = NULL;
  InterruptQtdsPtr  = NULL;
  DataPtr           = NULL;
  DataCursor        = NULL;
  DataMap           = NULL;
  AsyncRequestPtr   = NULL;
  HcDev             = USB2_HC_DEV_FROM_THIS (This);

  //
  // Parameters Checking
  //
  if (!IsDataInTransfer (EndPointAddress)) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (IsNewTransfer) {
    if (0 == DataLength) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }

    if (*DataToggle != 1 && *DataToggle != 0) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }

    if (PollingInterval > 255 || PollingInterval < 1) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    ClearEhcAllStatus (HcDev);
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = ClearEhcAllStatus (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Delete Async interrupt transfer request
  //
  if (!IsNewTransfer) {

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

    Status = DeleteAsyncRequestTransfer (
               HcDev,
               DeviceAddress,
               EndPointAddress,
               DataToggle
               );

    gBS->RestoreTPL (OldTpl);

    goto exit;
  }

  Status = EhciAllocatePool (
  	         HcDev,
  	         (UINT8 **) &AsyncRequestPtr,
  	         sizeof (EHCI_ASYNC_REQUEST)
  	         );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  Status = EhciAllocatePool (HcDev, &DataPtr, DataLength);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto free_request;
  }

  MappedLength = DataLength;
  Status = MapDataBuffer (
             HcDev,
             EfiUsbDataIn,
             DataPtr,
             &MappedLength,
             &PktId,
             &DataCursor,
             &DataMap
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto free_data;
  }

  //
  // Create and init Interrupt Qh
  //
  Status = CreateInterruptQh (
             HcDev,
             DeviceAddress,
             EndPointAddress,
             DeviceSpeed,
             *DataToggle,
             MaximumPacketLength,
             PollingInterval,
             Translator,
             &QhPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto unmap_data;
  }

  //
  // Create and init Interrupt Qtds
  //
  Status = CreateBulkOrInterruptQtds (
             HcDev,
             PktId,
             DataCursor,
             MappedLength,
             Translator,
             &InterruptQtdsPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto destory_qh;
  }

  //
  // Link Qtds to Qh
  //
  LinkQtdToQh (QhPtr, InterruptQtdsPtr);

  //
  // Init AsyncRequest Entry
  //
  AsyncRequestPtr->Context      = Context;
  AsyncRequestPtr->CallBackFunc = CallBackFunction;
  AsyncRequestPtr->TransferType = ASYNC_INTERRUPT_TRANSFER;
  AsyncRequestPtr->QhPtr        = QhPtr;
  AsyncRequestPtr->Prev         = NULL;
  AsyncRequestPtr->Next         = NULL;

  if (NULL == HcDev->AsyncRequestList) {
    Status = StartPollingTimer (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      CleanUpAllAsyncRequestTransfer (HcDev);
      goto exit;
    }
  }

  //
  // Link Entry to AsyncRequest List
  //
  LinkToAsyncReqeust (HcDev, AsyncRequestPtr);

  ClearEhcAllStatus (HcDev);

  Status = DisablePeriodicSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForPeriodicScheduleDisable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
    goto exit;
  }

  //
  // Link Qh and Qtds to Periodic Schedule List
  //
  LinkQhToPeriodicList (HcDev, QhPtr);

  Status = EnablePeriodicSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForPeriodicScheduleEnable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
    goto exit;
  }

  if (IsEhcHalted (HcDev)) {
    Status = StartScheduleExecution (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
  }

  HcDev->PciIo->Flush (HcDev->PciIo);
  goto exit;

destory_qh:
  DestoryQh (HcDev, QhPtr);
free_data:
  EhciFreePool (HcDev, DataPtr, DataLength);
free_request:
  EhciFreePool (
  	HcDev,
  	(UINT8 *) AsyncRequestPtr,
  	sizeof (EHCI_ASYNC_REQUEST)
  	);
unmap_data:
  HcDev->PciIo->Unmap (HcDev->PciIo, DataMap);
exit:
  return Status;
}

EFI_STATUS
EFIAPI
EhciSyncInterruptTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  *This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  OUT VOID                              *Data,
  IN  OUT UINTN                             *DataLength,
  IN  OUT UINT8                             *DataToggle,
  IN  UINTN                                 TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  OUT UINT32                                *TransferResult
  )
/*++

  Routine Description:

    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device.
    Translator parameter doesn't exist in UEFI2.0 spec, but it will be updated
    in the following specification version.

  Arguments:

    This            - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress   - Represents the address of the target device on the USB,
                      which is assigned during USB enumeration.
    EndPointAddress - The combination of an endpoint number and an endpoint
                      direction of the target USB device. Each endpoint
                      address supports data transfer in one direction
                      except the control endpoint (whose default
                      endpoint address is 0). It is the caller's responsibility
                      to make sure that the EndPointAddress represents
                      an interrupt endpoint.
    DeviceSpeed     - Indicates device speed.
    MaximumPacketLength - Indicates the maximum packet size the target endpoint
                          is capable of sending or receiving.
    Data            - A pointer to the buffer of data that will be transmitted
                      to USB device or received from USB device.
    DataLength      - On input, the size, in bytes, of the data buffer specified
                      by Data. On output, the number of bytes transferred.
    DataToggle      - A pointer to the data toggle value. On input, it indicates
                      the initial data toggle value the synchronous interrupt
                      transfer should adopt;
                      on output, it is updated to indicate the data toggle value
                      of the subsequent synchronous interrupt transfer.
    TimeOut         - Indicates the maximum time, in microseconds, which the
                      transfer is allowed to complete.
    Translator      - A pointr to the transaction translator data.
    TransferResult  - A pointer to the detailed result information from
                      the synchronous interrupt transfer.

  Returns:

    EFI_SUCCESS
        The synchronous interrupt transfer was completed successfully.
    EFI_OUT_OF_RESOURCES
        The synchronous interrupt transfer could not be submitted due
        to lack of resource.
    EFI_INVALID_PARAMETER
        Some parameters are invalid.
    EFI_TIMEOUT
        The synchronous interrupt transfer failed due to timeout.
    EFI_DEVICE_ERROR
        The synchronous interrupt transfer failed due to host controller
        or device error. Caller should check TranferResult for detailed
        error information.

--*/
{
  EFI_STATUS      Status;
  USB2_HC_DEV     *HcDev;
  UINT8           PktId;
  EHCI_QH_ENTITY  *QhPtr;
  EHCI_QTD_ENTITY *InterruptQtdsPtr;
  UINT8           *DataCursor;
  VOID            *DataMap;

  QhPtr             = NULL;
  InterruptQtdsPtr  = NULL;
  DataCursor        = NULL;
  DataMap           = NULL;
  HcDev             = USB2_HC_DEV_FROM_THIS (This);

  //
  // Parameters Checking
  //
  if (DataLength == NULL ||
  	Data == NULL ||
  	TransferResult == NULL
  	) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (!IsDataInTransfer (EndPointAddress)) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (0 == *DataLength) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (*DataToggle != 1 && *DataToggle != 0) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_LOW == DeviceSpeed && 8 != MaximumPacketLength) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_FULL == DeviceSpeed && MaximumPacketLength > 64) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  if (EFI_USB_SPEED_HIGH == DeviceSpeed && MaximumPacketLength > 3072) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    ClearEhcAllStatus (HcDev);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = ClearEhcAllStatus (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = MapDataBuffer (
             HcDev,
             EfiUsbDataIn,
             Data,
             DataLength,
             &PktId,
             &DataCursor,
             &DataMap
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_DEVICE_ERROR;
    goto exit;
  }

  //
  // Create and init Interrupt Qh
  //
  Status = CreateInterruptQh (
             HcDev,
             DeviceAddress,
             EndPointAddress,
             DeviceSpeed,
             *DataToggle,
             MaximumPacketLength,
             0,
             Translator,
             &QhPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto unmap_data;
  }

  //
  // Create and init Interrupt Qtds
  //
  Status = CreateBulkOrInterruptQtds (
             HcDev,
             PktId,
             DataCursor,
             *DataLength,
             Translator,
             &InterruptQtdsPtr
             );
  if (EFI_ERROR (Status)) {
    *TransferResult = EFI_USB_ERR_SYSTEM;
    Status          = EFI_OUT_OF_RESOURCES;
    goto destory_qh;
  }

  //
  // Link Qtds to Qh
  //
  LinkQtdToQh (QhPtr, InterruptQtdsPtr);

  ClearEhcAllStatus (HcDev);

  Status = DisablePeriodicSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForPeriodicScheduleDisable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
    goto exit;
  }

  //
  // Link Qh and Qtds to Periodic Schedule List
  //
  LinkQhToPeriodicList (HcDev, QhPtr);

  Status = EnablePeriodicSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForPeriodicScheduleEnable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    Status = EFI_TIMEOUT;
    goto exit;
  }

  if (IsEhcHalted (HcDev)) {
    Status = StartScheduleExecution (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }
  }

  //
  // Poll QH-TDs execution and get result.
  // detail status is returned
  //
  Status = ExecuteTransfer (
             HcDev,
             FALSE,
             QhPtr,
             DataLength,
             DataToggle,
             TimeOut,
             TransferResult
             );
  if (EFI_ERROR (Status)) {
    goto destory_qtds;
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (IsEhcHalted (HcDev) || IsEhcSysError (HcDev)) {
    *TransferResult |= EFI_USB_ERR_SYSTEM;
  }

  ClearEhcAllStatus (HcDev);

destory_qtds:
  UnlinkQhFromPeriodicList (HcDev, QhPtr, 0);
  DestoryQtds (HcDev, InterruptQtdsPtr);
destory_qh:
  DestoryQh (HcDev, QhPtr);
unmap_data:
  HcDev->PciIo->Unmap (HcDev->PciIo, DataMap);
exit:
  HcDev->PciIo->Flush (HcDev->PciIo);
  return Status;
}

EFI_STATUS
EFIAPI
EhciIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                  *This,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  UINT8                                 DataBuffersNumber,
  IN  OUT VOID                              *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                                 DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR    *Translator,
  OUT UINT32                                *TransferResult
  )
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device.

  Arguments:

    This             - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress    - Represents the address of the target device on the USB,
                       which is assigned during USB enumeration.
    EndPointAddress  - End point address
    DeviceSpeed      - Indicates device speed.
    MaximumPacketLength    - Indicates the maximum packet size that the
                             default control transfer endpoint is capable of
                             sending or receiving.
    DataBuffersNumber - Number of data buffers prepared for the transfer.
    Data              - Array of pointers to the buffers of data that will be
                        transmitted to USB device or received from USB device.
    DataLength        - Indicates the size, in bytes, of the data buffer
                        specified by Data.
    Translator        - A pointr to the transaction translator data.
    TransferResult    - A pointer to the detailed result information generated
                        by this control transfer.

  Returns:

    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EhciAsyncIsochronousTransfer (
  IN  EFI_USB2_HC_PROTOCOL                *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN  OUT VOID                            *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  )
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device.

  Arguments:

    This                - A pointer to the EFI_USB2_HC_PROTOCOL instance.
    DeviceAddress       - Represents the address of the target device on the USB,
                          which is assigned during USB enumeration.
    EndPointAddress     - End point address
    DeviceSpeed         - Indicates device speed.
    MaximumPacketLength - Indicates the maximum packet size that the
                          default control transfer endpoint is capable of
                          sending or receiving.
    DataBuffersNumber   - Number of data buffers prepared for the transfer.
    Data                - Array of pointers to the buffers of data that will be transmitted
                          to USB device or received from USB device.
    Translator          - A pointr to the transaction translator data.
    IsochronousCallBack - When the transfer complete, the call back function will be called
    Context             - Pass to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}
