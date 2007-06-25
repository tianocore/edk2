/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Uhci.c

Abstract:


Revision History
--*/

#include "uhci.h"

//
// UHCI Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gUhciDriverBinding = {
  UHCIDriverBindingSupported,
  UHCIDriverBindingStart,
  UHCIDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
UHCIDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle. Any ControllerHandle
    that has UsbHcProtocol installed will be supported.

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS          OpenStatus;
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  USB_CLASSC          UsbClassCReg;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
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
    return EFI_UNSUPPORTED;
  }
  //
  // Test whether the controller belongs to UHCI type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL)         ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.PI != PCI_CLASSC_PI_UHCI)) {

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return EFI_UNSUPPORTED;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return EFI_SUCCESS;

}

EFI_STATUS
EFIAPI
UHCIDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

  Routine Description:
    Starting the Usb UHCI Driver

  Arguments:
    This                - Protocol instance pointer.
    Controller          - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
                          Error
    EFI_OUT_OF_RESOURCES

--*/
{
  EFI_STATUS              Status;
  UINTN                   FlBaseAddrReg;
  EFI_PCI_IO_PROTOCOL     *PciIo;
  USB_HC_DEV              *HcDev;
  UINT64                  Supports;

  HcDev = NULL;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Turn off USB emulation
  //
  TurnOffUSBEmulation (PciIo);

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
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_UNSUPPORTED;
  }

  //
  // allocate memory for UHC private data structure
  //
  HcDev = AllocateZeroPool (sizeof (USB_HC_DEV));
  if (HcDev == NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // init EFI_USB_HC_PROTOCOL protocol interface and install the protocol
  //
  HcDev->UsbHc.Reset                    = UHCIReset;
  HcDev->UsbHc.GetState                 = UHCIGetState;
  HcDev->UsbHc.SetState                 = UHCISetState;
  HcDev->UsbHc.ControlTransfer          = UHCIControlTransfer;
  HcDev->UsbHc.BulkTransfer             = UHCIBulkTransfer;
  HcDev->UsbHc.AsyncInterruptTransfer   = UHCIAsyncInterruptTransfer;
  HcDev->UsbHc.SyncInterruptTransfer    = UHCISyncInterruptTransfer;
  HcDev->UsbHc.IsochronousTransfer      = UHCIIsochronousTransfer;
  HcDev->UsbHc.AsyncIsochronousTransfer = UHCIAsyncIsochronousTransfer;
  HcDev->UsbHc.GetRootHubPortNumber     = UHCIGetRootHubPortNumber;
  HcDev->UsbHc.GetRootHubPortStatus     = UHCIGetRootHubPortStatus;
  HcDev->UsbHc.SetRootHubPortFeature    = UHCISetRootHubPortFeature;
  HcDev->UsbHc.ClearRootHubPortFeature  = UHCIClearRootHubPortFeature;

  HcDev->UsbHc.MajorRevision            = 0x1;
  HcDev->UsbHc.MinorRevision            = 0x1;

  //
  //
  // init EFI_USB2_HC_PROTOCOL protocol interface and install the protocol
  //
  HcDev->Usb2Hc.GetCapability            = UHCI2GetCapability;
  HcDev->Usb2Hc.Reset                    = UHCI2Reset;
  HcDev->Usb2Hc.GetState                 = UHCI2GetState;
  HcDev->Usb2Hc.SetState                 = UHCI2SetState;
  HcDev->Usb2Hc.ControlTransfer          = UHCI2ControlTransfer;
  HcDev->Usb2Hc.BulkTransfer             = UHCI2BulkTransfer;
  HcDev->Usb2Hc.AsyncInterruptTransfer   = UHCI2AsyncInterruptTransfer;
  HcDev->Usb2Hc.SyncInterruptTransfer    = UHCI2SyncInterruptTransfer;
  HcDev->Usb2Hc.IsochronousTransfer      = UHCI2IsochronousTransfer;
  HcDev->Usb2Hc.AsyncIsochronousTransfer = UHCI2AsyncIsochronousTransfer;
  HcDev->Usb2Hc.GetRootHubPortStatus     = UHCI2GetRootHubPortStatus;
  HcDev->Usb2Hc.SetRootHubPortFeature    = UHCI2SetRootHubPortFeature;
  HcDev->Usb2Hc.ClearRootHubPortFeature  = UHCI2ClearRootHubPortFeature;

  HcDev->Usb2Hc.MajorRevision            = 0x1;
  HcDev->Usb2Hc.MinorRevision            = 0x1;

  //
  //  Init UHCI private data structures
  //
  HcDev->Signature  = USB_HC_DEV_SIGNATURE;
  HcDev->PciIo      = PciIo;

  FlBaseAddrReg     = USBFLBASEADD;

  //
  // Allocate and Init Host Controller's Frame List Entry
  //
  Status = CreateFrameList (HcDev, (UINT32) FlBaseAddrReg);
  if (EFI_ERROR (Status)) {

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_OUT_OF_RESOURCES;
  }

  //
  //  Init interrupt list head in the HcDev structure.
  //
  InitializeListHead (&(HcDev->InterruptListHead));

  //
  //  Create timer for interrupt transfer result polling
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  MonitorInterruptTrans,
                  HcDev,
                  &HcDev->InterruptTransTimer
                  );
  if (EFI_ERROR (Status)) {

    FreeFrameListEntry (HcDev);

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_UNSUPPORTED;
  }

  //
  // Here set interrupt transfer polling timer in 50ms unit.
  //
  Status = gBS->SetTimer (
                  HcDev->InterruptTransTimer,
                  TimerPeriodic,
                  INTERRUPT_POLLING_TIME
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (HcDev->InterruptTransTimer);

    FreeFrameListEntry (HcDev);

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_UNSUPPORTED;
  }

  //
  // QH,TD structures must in common buffer that will be
  // accessed by both cpu and usb bus master at the same time.
  // so, there must has memory management for QH,TD structures.
  //
  Status = InitializeMemoryManagement (HcDev);
  if (EFI_ERROR (Status)) {

    gBS->CloseEvent (HcDev->InterruptTransTimer);

    FreeFrameListEntry (HcDev);

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return Status;
  }

  //
  // Install Host Controller Protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsbHcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HcDev->UsbHc
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (HcDev->InterruptTransTimer);
    FreeFrameListEntry (HcDev);
    DelMemoryManagement (HcDev);

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return Status;
  }

  //
  // Install USB2.0 Host Controller Protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HcDev->Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (HcDev->InterruptTransTimer);
    FreeFrameListEntry (HcDev);
    DelMemoryManagement (HcDev);

    if (HcDev != NULL) {
      gBS->FreePool (HcDev);
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return Status;
  }

  //
  // component name protocol.
  //

  HcDev->ControllerNameTable = NULL;
  AddUnicodeString (
    "eng",
    gUhciComponentName.SupportedLanguages,
    &HcDev->ControllerNameTable,
    (CHAR16 *) L"Usb Universal Host Controller"
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UnInstallUHCInterface (
  IN  EFI_HANDLE              Controller,
  IN  EFI_USB_HC_PROTOCOL     *This
  )
/*++
  Routine Description:
    UnInstall UHCInterface
  Arguments:
    Controller        - Controller handle
    This              - Protocol instance pointer.
  Returns:
    EFI_SUCCESS
    others
--*/
{
  USB_HC_DEV  *HcDev;
  EFI_STATUS  Status;
  UINT64      Supports;

  HcDev = USB_HC_DEV_FROM_THIS (This);

  gBS->UninstallProtocolInterface (
         Controller,
         &gEfiUsbHcProtocolGuid,
         &HcDev->UsbHc
         );

  gBS->UninstallProtocolInterface (
         Controller,
         &gEfiUsb2HcProtocolGuid,
         &HcDev->Usb2Hc
         );
  //
  // first stop USB Host Controller
  //
  This->SetState (This, EfiUsbHcStateHalt);

  //
  // Delete interrupt transfer polling timer
  //
  gBS->CloseEvent (HcDev->InterruptTransTimer);

  //
  // Delete all the asynchronous interrupt transfers in the interrupt list
  // and free associated memory
  //
  ReleaseInterruptList (HcDev, &(HcDev->InterruptListHead));

  //
  // free Frame List Entry.
  //
  FreeFrameListEntry (HcDev);

  //
  // Free common buffer allocated for QH,TD structures
  //
  DelMemoryManagement (HcDev);

  if (HcDev->ControllerNameTable) {
    FreeUnicodeStringTable (HcDev->ControllerNameTable);
  }
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

  gBS->FreePool (HcDev);

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UHCIDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
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
    others

--*/
{
  EFI_USB_HC_PROTOCOL *UsbHc;
  EFI_USB2_HC_PROTOCOL *Usb2Hc;
  EFI_STATUS          OpenStatus;

  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbHcProtocolGuid,
                      (VOID **)&UsbHc,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsb2HcProtocolGuid,
                      (VOID **) &Usb2Hc,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }
  //
  // free all the controller related memory and uninstall UHCI Protocol.
  //
  UnInstallUHCInterface (Controller, UsbHc);

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;

}


EFI_STATUS
EFIAPI
UHCIReset (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN UINT16                  Attributes
  )
/*++

  Routine Description:
    Provides software reset for the USB host controller.

  Arguments:

  This        A pointer to the EFI_USB_HC_PROTOCOL instance.

  Attributes  A bit mask of the reset operation to perform.
              See below for a list of the supported bit mask values.

  #define EFI_USB_HC_RESET_GLOBAL           0x0001
  #define EFI_USB_HC_RESET_HOST_CONTROLLER  0x0002

  EFI_USB_HC_RESET_GLOBAL
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host
        controller hardware and all the devices attached on the USB bus.
  EFI_USB_HC_RESET_HOST_CONTROLLER
        If this bit is set, the USB host controller hardware will be reset.
        No reset signal will be sent to the USB bus.

  Returns:
    EFI_SUCCESS
        The reset operation succeeded.
    EFI_INVALID_PARAMETER
        Attributes is not valid.
    EFI_DEVICE_ERROR
        An error was encountered while attempting to perform
        the reset operation.
--*/
{
  BOOLEAN     Match;
  USB_HC_DEV  *HcDev;
  UINT32      CommandRegAddr;
  UINT32      FlBaseAddrReg;
  UINT16      Command;
  EFI_STATUS  Status;

  Match           = FALSE;
  HcDev           = USB_HC_DEV_FROM_THIS (This);

  CommandRegAddr  = (UINT32) (USBCMD);
  FlBaseAddrReg   = (UINT32) (USBFLBASEADD);

  if ((Attributes & EFI_USB_HC_RESET_GLOBAL) != 0) {
    Match = TRUE;
    //
    // set the Global Reset bit in the command register
    //
    Status = ReadUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               &Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Command |= USBCMD_GRESET;
    Status = WriteUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Wait 50ms for root port to let reset complete
    // See UHCI spec page122 Reset signaling
    //
    gBS->Stall (ROOT_PORT_REST_TIME);

    //
    // Clear the Global Reset bit to zero.
    //
    Command &= ~USBCMD_GRESET;
    Status = WriteUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // UHCI spec page120 reset recovery time
    //
    gBS->Stall (PORT_RESET_RECOVERY_TIME);
  }

  if ((Attributes & EFI_USB_HC_RESET_HOST_CONTROLLER) != 0) {
    Match = TRUE;
    //
    // set Host Controller Reset bit to 1
    //
    Status = ReadUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               &Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Command |= USBCMD_HCRESET;
    Status = WriteUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // this bit will be reset by Host Controller when reset is completed.
    // wait 10ms to let reset complete
    //
    gBS->Stall (PORT_RESET_RECOVERY_TIME);
  }

  if (!Match) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete all old transactions on the USB bus
  //
  CleanUsbTransactions (HcDev);

  //
  // Initialize Universal Host Controller's Frame List Data Structure
  //
  InitFrameList (HcDev);

  //
  // Reset may cause Frame List Base Address Register reset to zero,
  // so set the original value back again.
  //
  SetFrameListBaseAddress (
    HcDev->PciIo,
    FlBaseAddrReg,
    (UINT32) ((UINTN) HcDev->FrameListEntry)
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCIGetState (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT EFI_USB_HC_STATE        *State
  )
/*++

  Routine Description:
    Retrieves current state of the USB host controller.

  Arguments:

    This      A pointer to the EFI_USB_HC_PROTOCOL instance.

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
  USB_HC_DEV  *HcDev;
  UINT32      CommandRegAddr;
  UINT32      StatusRegAddr;
  UINT16      UhcCommand;
  UINT16      UhcStatus;
  EFI_STATUS  Status;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HcDev           = USB_HC_DEV_FROM_THIS (This);

  CommandRegAddr  = (UINT32) (USBCMD);
  StatusRegAddr   = (UINT32) (USBSTS);

  Status = ReadUHCCommandReg (
             HcDev->PciIo,
             CommandRegAddr,
             &UhcCommand
             );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = ReadUHCCommandReg (
             HcDev->PciIo,
             StatusRegAddr,
             &UhcStatus
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (UhcCommand & USBCMD_EGSM) {
    *State = EfiUsbHcStateSuspend;
    return EFI_SUCCESS;
  }

  if ((UhcStatus & USBSTS_HCH) == 0) {
    *State = EfiUsbHcStateOperational;
  } else {
    *State = EfiUsbHcStateHalt;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UHCISetState (
  IN EFI_USB_HC_PROTOCOL     *This,
  IN EFI_USB_HC_STATE        State
  )
/*++

  Routine Description:
    Sets the USB host controller to a specific state.

  Arguments:

    This      A pointer to the EFI_USB_HC_PROTOCOL instance.

    State     Indicates the state of the host controller that will be set.

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
  USB_HC_DEV        *HcDev;
  UINT32            CommandRegAddr;
  UINT32            StatusRegAddr;
  UINT16            Command;
  EFI_USB_HC_STATE  CurrentState;
  EFI_STATUS        Status;

  HcDev           = USB_HC_DEV_FROM_THIS (This);

  CommandRegAddr  = (UINT32) (USBCMD);
  StatusRegAddr   = (UINT32) (USBSTS);

  Status          = UHCIGetState (This, &CurrentState);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  switch (State) {

  case EfiUsbHcStateHalt:
    if (CurrentState == EfiUsbHcStateHalt) {
      return EFI_SUCCESS;
    }

    Status = ReadUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               &Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Command &= ~USBCMD_RS;

    Status = WriteUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    StatusRegAddr = (UINT32) (USBSTS);
    //
    // ensure the HC is in halt status after send the stop command
    //
    if (WaitForUHCHalt (HcDev->PciIo, StatusRegAddr, STALL_1_SECOND) == EFI_TIMEOUT) {
      return EFI_DEVICE_ERROR;
    }
    break;

  case EfiUsbHcStateOperational:
    if (IsHostSysOrProcessErr (HcDev->PciIo, StatusRegAddr)) {
      return EFI_DEVICE_ERROR;
    }

    switch (CurrentState) {

    case EfiUsbHcStateOperational:
      return EFI_SUCCESS;

    case EfiUsbHcStateHalt:
      //
      // Set Run/Stop bit to 1.
      //
      Status = ReadUHCCommandReg (
                 HcDev->PciIo,
                 CommandRegAddr,
                 &Command
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      Command |= USBCMD_RS | USBCMD_MAXP;
      Status = WriteUHCCommandReg (
                 HcDev->PciIo,
                 CommandRegAddr,
                 Command
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      break;

    case EfiUsbHcStateSuspend:
      Status = ReadUHCCommandReg (
                 HcDev->PciIo,
                 CommandRegAddr,
                 &Command
                 );
      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }

      //
      // FGR(Force Global Resume) bit is 0
      //
      if ((Command | (~USBCMD_FGR)) != 0xFF) {
        //
        // Write FGR bit to 1
        //
        Command |= USBCMD_FGR;
        WriteUHCCommandReg (
          HcDev->PciIo,
          CommandRegAddr,
          Command
          );
      }

      //
      // wait 20ms to let resume complete
      // (20ms is specified by UHCI spec)
      //
      gBS->Stall (FORCE_GLOBAL_RESUME_TIME);

      //
      // Write FGR bit to 0 and EGSM(Enter Global Suspend Mode) bit to 0
      //
      Command &= ~USBCMD_FGR;
      Command &= ~USBCMD_EGSM;
      Command |= USBCMD_RS;
      WriteUHCCommandReg (
        HcDev->PciIo,
        CommandRegAddr,
        Command
        );
      break;

    default:
      break;
    }
    break;

  case EfiUsbHcStateSuspend:
    if (CurrentState == EfiUsbHcStateSuspend) {
      return EFI_SUCCESS;
    }

    Status = UHCISetState (This, EfiUsbHcStateHalt);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Set Enter Global Suspend Mode bit to 1.
    //
    Status = ReadUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               &Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Command |= USBCMD_EGSM;
    Status = WriteUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCIGetRootHubPortNumber (
  IN  EFI_USB_HC_PROTOCOL     *This,
  OUT UINT8                   *PortNumber
  )
/*++

  Routine Description:
    Retrieves the number of root hub ports.

  Arguments:

    This        A pointer to the EFI_USB_HC_PROTOCOL instance.

    PortNumber  A pointer to the number of the root hub ports.

  Returns:
    EFI_SUCCESS
          The port number was retrieved successfully.
    EFI_INVALID_PARAMETER
          PortNumber is NULL.
    EFI_DEVICE_ERROR
          An error was encountered while attempting to
          retrieve the port number.
--*/
{
  USB_HC_DEV  *HcDev;
  UINT32      PSAddr;
  UINT16      RHPortControl;
  UINT32      Index;
  EFI_STATUS  Status;

  HcDev = USB_HC_DEV_FROM_THIS (This);

  if (PortNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PortNumber = 0;

  for (Index = 0; Index < 2; Index++) {
    PSAddr = (UINT32) (USBPORTSC1 + Index * 2);
    Status = ReadRootPortReg (
               HcDev->PciIo,
               PSAddr,
               &RHPortControl
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Port Register content is valid
    //
    if (RHPortControl != 0xff) {
      (*PortNumber)++;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCIGetRootHubPortStatus (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  OUT EFI_USB_PORT_STATUS     *PortStatus
  )
/*++

  Routine Description:
    Retrieves the current status of a USB root hub port.

  Arguments:

    This        A pointer to the EFI_USB_HC_PROTOCOL.

    PortNumber  Specifies the root hub port from which the status
                is to be retrieved.  This value is zero-based. For example,
                if a root hub has two ports, then the first port is numbered 0,
                and the second port is numbered 1.

    PortStatus  A pointer to the current port status bits and
                port status change bits.

  Returns:
    EFI_SUCCESS
        The status of the USB root hub port specified by PortNumber
        was returned in PortStatus.
    EFI_INVALID_PARAMETER
        PortNumber is invalid.
    EFI_DEVICE_ERROR - Can't read register
--*/
{
  USB_HC_DEV  *HcDev;
  UINT32      PSAddr;
  UINT16      RHPortStatus;
  UINT8       TotalPortNumber;
  EFI_STATUS  Status;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UHCIGetRootHubPortNumber (This, &TotalPortNumber);
  if (PortNumber >= TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  HcDev   = USB_HC_DEV_FROM_THIS (This);
  PSAddr  = (UINT32) (USBPORTSC1 + PortNumber * 2);

  //
  // Clear port status
  //
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  Status = ReadRootPortReg (
             HcDev->PciIo,
             PSAddr,
             &RHPortStatus
             );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  //    Fill Port Status bits
  //

  //
  // Current Connect Status
  //
  if (RHPortStatus & USBPORTSC_CCS) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }
  //
  // Port Enabled/Disabled
  //
  if (RHPortStatus & USBPORTSC_PED) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }

  //
  // Port Suspend
  //
  if (RHPortStatus & USBPORTSC_SUSP) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  //
  // Port Reset
  //
  if (RHPortStatus & USBPORTSC_PR) {
    PortStatus->PortStatus |= USB_PORT_STAT_RESET;
  }

  //
  // Low Speed Device Attached
  //
  if (RHPortStatus & USBPORTSC_LSDA) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
  }
  //
  // CHC will always return one in this bit
  //
  PortStatus->PortStatus |= USB_PORT_STAT_OWNER;
  //
  //   Fill Port Status Change bits
  //

  //
  // Connect Status Change
  //
  if (RHPortStatus & USBPORTSC_CSC) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }

  //
  // Port Enabled/Disabled Change
  //
  if (RHPortStatus & USBPORTSC_PEDC) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCISetRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
/*++

  Routine Description:
    Sets a feature for the specified root hub port.

  Arguments:

    This        A pointer to the EFI_USB_HC_PROTOCOL.

    PortNumber  Specifies the root hub port whose feature
                is requested to be set.

    PortFeature Indicates the feature selector associated
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
  USB_HC_DEV  *HcDev;
  UINT32      PSAddr;
  UINT32      CommandRegAddr;
  //
  // root hub port status
  //
  UINT16      RHPortControl;
  UINT16      Command;
  UINT8       TotalPortNumber;
  EFI_STATUS  Status;

  UHCIGetRootHubPortNumber (This, &TotalPortNumber);
  if (PortNumber >= TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  HcDev           = USB_HC_DEV_FROM_THIS (This);

  PSAddr          = (UINT32) (USBPORTSC1 + PortNumber * 2);
  CommandRegAddr  = (UINT32) (USBCMD);

  Status = ReadRootPortReg (
            HcDev->PciIo,
            PSAddr,
            &RHPortControl
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  switch (PortFeature) {

  case EfiUsbPortSuspend:
    Status = ReadUHCCommandReg (
               HcDev->PciIo,
               CommandRegAddr,
               &Command
               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    if (!(Command & USBCMD_EGSM)) {
      //
      // if global suspend is not active, can set port suspend
      //
      RHPortControl &= 0xfff5;
      RHPortControl |= USBPORTSC_SUSP;
    }
    break;

  case EfiUsbPortReset:
    RHPortControl &= 0xfff5;
    //
    // Set the reset bit
    //
    RHPortControl |= USBPORTSC_PR;
    break;

  case EfiUsbPortPower:
    break;

  case EfiUsbPortEnable:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_PED;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  WriteRootPortReg (
    HcDev->PciIo,
    PSAddr,
    RHPortControl
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCIClearRootHubPortFeature (
  IN  EFI_USB_HC_PROTOCOL     *This,
  IN  UINT8                   PortNumber,
  IN  EFI_USB_PORT_FEATURE    PortFeature
  )
/*++

  Routine Description:
    Clears a feature for the specified root hub port.

  Arguments:

    This        A pointer to the EFI_USB_HC_PROTOCOL instance.

    PortNumber  Specifies the root hub port whose feature
                is requested to be cleared.

    PortFeature Indicates the feature selector associated with the
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
  USB_HC_DEV  *HcDev;
  UINT32      PSAddr;
  UINT16      RHPortControl;
  UINT8       TotalPortNumber;
  EFI_STATUS  Status;

  UHCIGetRootHubPortNumber (This, &TotalPortNumber);

  if (PortNumber >= TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  HcDev   = USB_HC_DEV_FROM_THIS (This);
  PSAddr  = (UINT32) (USBPORTSC1 + PortNumber * 2);

  Status = ReadRootPortReg (
             HcDev->PciIo,
             PSAddr,
             &RHPortControl
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  switch (PortFeature) {
  //
  // clear PORT_ENABLE feature means disable port.
  //
  case EfiUsbPortEnable:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_PED;
    break;

  //
  // clear PORT_SUSPEND feature means resume the port.
  // (cause a resume on the specified port if in suspend mode)
  //
  case EfiUsbPortSuspend:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_SUSP;
    break;

  //
  // no operation
  //
  case EfiUsbPortPower:
    break;

  //
  // clear PORT_RESET means clear the reset signal.
  //
  case EfiUsbPortReset:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_PR;
    break;

  //
  // clear connect status change
  //
  case EfiUsbPortConnectChange:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_CSC;
    break;

  //
  // clear enable/disable status change
  //
  case EfiUsbPortEnableChange:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_PEDC;
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortSuspendChange:
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortOverCurrentChange:
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortResetChange:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  WriteRootPortReg (
    HcDev->PciIo,
    PSAddr,
    RHPortControl
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCIControlTransfer (
  IN       EFI_USB_HC_PROTOCOL        *This,
  IN       UINT8                      DeviceAddress,
  IN       BOOLEAN                    IsSlowDevice,
  IN       UINT8                      MaximumPacketLength,
  IN       EFI_USB_DEVICE_REQUEST     *Request,
  IN       EFI_USB_DATA_DIRECTION     TransferDirection,
  IN OUT   VOID                       *Data, OPTIONAL
  IN OUT   UINTN                      *DataLength, OPTIONAL
  IN       UINTN                      TimeOut,
  OUT      UINT32                     *TransferResult
  )
/*++

  Routine Description:
    Submits control transfer to a target USB device.

  Arguments:

    This          A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.

    IsSlowDevice  Indicates whether the target device is slow device
                  or full-speed device.

    MaximumPacketLength Indicates the maximum packet size that the
                        default control transfer endpoint is capable of
                        sending or receiving.

    Request       A pointer to the USB device request that will be sent
                  to the USB device.

    TransferDirection Specifies the data direction for the transfer.
                      There are three values available, DataIn, DataOut
                      and NoData.

    Data          A pointer to the buffer of data that will be transmitted
                  to USB device or received from USB device.

    DataLength    Indicates the size, in bytes, of the data buffer
                  specified by Data.

    TimeOut       Indicates the maximum time, in microseconds,
                  which the transfer is allowed to complete.

    TransferResult  A pointer to the detailed result information generated
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
  USB_HC_DEV            *HcDev;
  UINT32                StatusReg;
  UINT32                FrameNumReg;
  UINT8                 PktID;
  QH_STRUCT             *PtrQH;
  TD_STRUCT             *PtrTD;
  TD_STRUCT             *PtrPreTD;
  TD_STRUCT             *PtrSetupTD;
  TD_STRUCT             *PtrStatusTD;
  EFI_STATUS            Status;
  UINTN                 Index;
  UINTN                 DataLen;
  UINT8                 *PtrDataSource;
  UINT8                 *Ptr;
  UINT8                 DataToggle;
  UINT16                LoadFrameListIndex;
  UINT8                 PktSize;

  UINT8                 *RequestMappedAddress;
  VOID                  *RequestMapping;
  UINTN                 RequestLen;

  EFI_PHYSICAL_ADDRESS  TempPtr;
  VOID                  *Mapping;

  TD_STRUCT             *PtrFirstDataTD;
  TD_STRUCT             *ptrLastDataTD;
  BOOLEAN               FirstTD;

  FirstTD               = FALSE;
  RequestMappedAddress  = NULL;
  RequestMapping        = NULL;
  Mapping               = NULL;
  PtrFirstDataTD        = NULL;
  ptrLastDataTD         = NULL;
  PktID                 = INPUT_PACKET_ID;
  Mapping               = NULL;
  HcDev                 = USB_HC_DEV_FROM_THIS (This);
  StatusReg             = (UINT32) (USBSTS);
  FrameNumReg           = (UINT32) (USBFRNUM);
  PtrPreTD              = NULL;
  PtrTD                 = NULL;

  //
  // Parameters Checking
  //
  if (Request == NULL || TransferResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if errors exist that cause host controller halt,
  // then return EFI_DEVICE_ERROR.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  //
  // low speed usb devices are limited to only an eight-byte
  // maximum data payload size
  //
  if (IsSlowDevice && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaximumPacketLength != 8  &&
      MaximumPacketLength != 16 &&
      MaximumPacketLength != 32 &&
      MaximumPacketLength != 64) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) && (DataLength == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (TransferDirection) {

  case EfiUsbDataIn:
    PktID         = INPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = *DataLength;

    //
    // map the source data buffer for bus master access.
    // BusMasterWrite means cpu read
    //
    Status = HcDev->PciIo->Map (
                             HcDev->PciIo,
                             EfiPciIoOperationBusMasterWrite,
                             PtrDataSource,
                             &DataLen,
                             &TempPtr,
                             &Mapping
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Ptr = (UINT8 *) ((UINTN) TempPtr);
    break;

  case EfiUsbDataOut:
    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = *DataLength;

    //
    // map the source data buffer for bus master access.
    // BusMasterRead means cpu write
    //
    Status = HcDev->PciIo->Map (
                             HcDev->PciIo,
                             EfiPciIoOperationBusMasterRead,
                             PtrDataSource,
                             &DataLen,
                             &TempPtr,
                             &Mapping
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Ptr = (UINT8 *) ((UINTN) TempPtr);
    break;

  //
  // no data stage
  //
  case EfiUsbNoData:
    if ((DataLength != NULL) && (*DataLength != 0)) {
      return EFI_INVALID_PARAMETER;
    }

    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = NULL;
    DataLen       = 0;
    Ptr           = NULL;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  Status = ClearStatusReg (HcDev->PciIo, StatusReg);
  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    return EFI_DEVICE_ERROR;
  }
  //
  // create QH structure and init
  //
  Status = CreateQH (HcDev, &PtrQH);
  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    return Status;
  }

  //
  // map the Request for bus master access.
  // BusMasterRead means cpu write
  //
  RequestLen = sizeof (EFI_USB_DEVICE_REQUEST);
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterRead,
                           (UINT8 *) Request,
                           &RequestLen,
                           &TempPtr,
                           &RequestMapping
                           );

  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
    return Status;
  }

  RequestMappedAddress = (UINT8 *) ((UINTN) TempPtr);

  //
  // generate Setup Stage TD
  //
  Status = GenSetupStageTD (
             HcDev,
             DeviceAddress,
             0,
             IsSlowDevice,
             (UINT8 *) RequestMappedAddress,
             sizeof (EFI_USB_DEVICE_REQUEST),
             &PtrSetupTD
             );

  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
    HcDev->PciIo->Unmap (HcDev->PciIo, RequestMapping);
    return Status;
  }

  //
  //  Data Stage of Control Transfer
  //
  DataToggle  = 1;
  FirstTD     = TRUE;
  while (DataLen > 0) {
    //
    // create TD structures and link together
    //

    //
    // PktSize is the data load size that each TD carries.
    //
    PktSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PktSize = MaximumPacketLength;
    }

    Status = GenDataTD (
               HcDev,
               DeviceAddress,
               0,
               Ptr,
               PktSize,
               PktID,
               DataToggle,
               IsSlowDevice,
               &PtrTD
               );

    if (EFI_ERROR (Status)) {
      //
      // free all resources occupied
      //
      HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
      HcDev->PciIo->Unmap (HcDev->PciIo, RequestMapping);
      DeleteQueuedTDs (HcDev, PtrSetupTD);
      DeleteQueuedTDs (HcDev, PtrFirstDataTD);
      return Status;
    }

    //
    // Link two TDs in vertical depth
    //
    if (FirstTD) {
      PtrFirstDataTD            = PtrTD;
      PtrFirstDataTD->ptrNextTD = NULL;
      FirstTD                   = FALSE;
    } else {
      LinkTDToTD (PtrPreTD, PtrTD);
    }

    PtrPreTD = PtrTD;

    DataToggle ^= 1;
    Ptr += PktSize;
    DataLen -= PktSize;
  }

  ptrLastDataTD = PtrTD;

  //
  // Status Stage of Control Transfer
  //
  if (PktID == OUTPUT_PACKET_ID) {
    PktID = INPUT_PACKET_ID;
  } else {
    PktID = OUTPUT_PACKET_ID;
  }

  //
  // create Status Stage TD structure
  //
  Status = CreateStatusTD (
             HcDev,
             DeviceAddress,
             0,
             PktID,
             IsSlowDevice,
             &PtrStatusTD
             );

  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
    HcDev->PciIo->Unmap (HcDev->PciIo, RequestMapping);
    DeleteQueuedTDs (HcDev, PtrSetupTD);
    DeleteQueuedTDs (HcDev, PtrFirstDataTD);
    return Status;
  }

  if (IsSlowDevice) {
    //
    // link setup TD structures to QH structure
    //
    LinkTDToQH (PtrQH, PtrSetupTD);

    LoadFrameListIndex = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);

    //
    // link QH-TDs to total 100 frame list entry to speed up the execution.
    //
    for (Index = 0; Index < 100; Index++) {
      LinkQHToFrameList (
        HcDev->FrameListEntry,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        PtrQH
        );
    }
    //
    // Poll QH-TDs execution and get result.
    // detail status is returned
    //
    Status = ExecuteControlTransfer (
               HcDev,
               PtrSetupTD,
               LoadFrameListIndex,
               DataLength,
               TimeOut,
               TransferResult
               );
    //
    // Remove Control Transfer QH-TDs structure from the frame list
    // and update the pointers in the Frame List
    // and other pointers in other related QH structures.
    //
    for (Index = 0; Index < 100; Index++) {
      DelLinkSingleQH (
        HcDev,
        PtrQH,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        FALSE,
        FALSE
        );
    }
    //
    // delete setup stage TD; the QH is reserved for the next stages.
    //
    DeleteQueuedTDs (HcDev, PtrSetupTD);

    //
    // if setup stage error, return error
    //
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // some control transfers do not have Data Stage
    //
    if (PtrFirstDataTD != NULL) {

      LinkTDToQH (PtrQH, PtrFirstDataTD);
      LoadFrameListIndex = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);

      for (Index = 0; Index < 500; Index++) {
        LinkQHToFrameList (
          HcDev->FrameListEntry,
          (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
          PtrQH
          );
      }

      Status = ExecuteControlTransfer (
                 HcDev,
                 PtrFirstDataTD,
                 LoadFrameListIndex,
                 DataLength,
                 TimeOut,
                 TransferResult
                 );

      for (Index = 0; Index < 500; Index++) {
        DelLinkSingleQH (
          HcDev,
          PtrQH,
          (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
          FALSE,
          FALSE
          );
      }
      //
      // delete data stage TD; the QH is reserved for the next stage.
      //
      DeleteQueuedTDs (HcDev, PtrFirstDataTD);
    }
    //
    // if data stage error, goto done and return error
    //
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    LinkTDToQH (PtrQH, PtrStatusTD);
    //
    // get the frame list index that the QH-TDs will be linked to.
    //
    LoadFrameListIndex = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);

    for (Index = 0; Index < 100; Index++) {
      //
      // put the QH-TDs directly or indirectly into the proper place
      // in the Frame List
      //
      LinkQHToFrameList (
        HcDev->FrameListEntry,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        PtrQH
        );
    }
    //
    // Poll QH-TDs execution and get result.
    // detail status is returned
    //
    Status = ExecuteControlTransfer (
               HcDev,
               PtrStatusTD,
               LoadFrameListIndex,
               DataLength,
               TimeOut,
               TransferResult
               );

    //
    // Delete Control Transfer QH-TDs structure
    // and update the pointers in the Frame List
    // and other pointers in other related QH structures.
    //
    // TRUE means must search other framelistindex
    //
    for (Index = 0; Index < 100; Index++) {
      DelLinkSingleQH (
        HcDev,
        PtrQH,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        FALSE,
        FALSE
        );
    }

    DeleteQueuedTDs (HcDev, PtrStatusTD);

  } else {
    //
    // link setup stage TD with data stage TD
    //
    PtrPreTD = PtrSetupTD;
    if (PtrFirstDataTD != NULL) {
      LinkTDToTD (PtrSetupTD, PtrFirstDataTD);
      PtrPreTD = ptrLastDataTD;
    }
    //
    // link status TD with previous TD
    //
    LinkTDToTD (PtrPreTD, PtrStatusTD);

    //
    // link QH with TD
    //
    LinkTDToQH (PtrQH, PtrSetupTD);

    LoadFrameListIndex = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);
    for (Index = 0; Index < 500; Index++) {
      //
      // put the QH-TDs directly or indirectly into the proper place
      // in the Frame List
      //
      LinkQHToFrameList (
        HcDev->FrameListEntry,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        PtrQH
        );
    }
    //
    // Poll QH-TDs execution and get result.
    // detail status is returned
    //
    Status = ExecuteControlTransfer (
               HcDev,
               PtrSetupTD,
               LoadFrameListIndex,
               DataLength,
               TimeOut,
               TransferResult
               );
    //
    // Remove Control Transfer QH-TDs structure from the frame list
    // and update the pointers in the Frame List
    // and other pointers in other related QH structures.
    //
    for (Index = 0; Index < 500; Index++) {
      DelLinkSingleQH (
        HcDev,
        PtrQH,
        (UINT16) ((LoadFrameListIndex + Index) & 0x3FF),
        FALSE,
        FALSE
        );
    }

    DeleteQueuedTDs (HcDev, PtrSetupTD);
  }

Done:

  UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));

  if (Mapping != NULL) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
  }

  if (RequestMapping != NULL) {
    HcDev->PciIo->Unmap (HcDev->PciIo, RequestMapping);
  }
  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult |= EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (HcDev->PciIo, StatusReg);
  HcDev->PciIo->Flush (HcDev->PciIo);
  return Status;
}

EFI_STATUS
EFIAPI
UHCIBulkTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  )
/*++

  Routine Description:
    Submits bulk transfer to a bulk endpoint of a USB device.

  Arguments:

    This          A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.
    EndPointAddress   The combination of an endpoint number and an
                      endpoint direction of the target USB device.
                      Each endpoint address supports data transfer in
                      one direction except the control endpoint
                      (whose default endpoint address is 0).
                      It is the caller's responsibility to make sure that
                      the EndPointAddress represents a bulk endpoint.

    MaximumPacketLength Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    Data          A pointer to the buffer of data that will be transmitted
                  to USB device or received from USB device.
    DataLength    When input, indicates the size, in bytes, of the data buffer
                  specified by Data. When output, indicates the actually
                  transferred data size.

    DataToggle    A pointer to the data toggle value. On input, it indicates
                  the initial data toggle value the bulk transfer should adopt;
                  on output, it is updated to indicate the data toggle value
                  of the subsequent bulk transfer.

    TimeOut       Indicates the maximum time, in microseconds, which the
                  transfer is allowed to complete.

    TransferResult  A pointer to the detailed result information of the
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
  USB_HC_DEV              *HcDev;
  UINT32                  StatusReg;
  UINT32                  FrameNumReg;
  UINTN                   DataLen;
  QH_STRUCT               *PtrQH;
  TD_STRUCT               *PtrFirstTD;
  TD_STRUCT               *PtrTD;
  TD_STRUCT               *PtrPreTD;
  UINT16                  LoadFrameListIndex;
  UINT16                  SavedFrameListIndex;
  UINT8                   PktID;
  UINT8                   *PtrDataSource;
  UINT8                   *Ptr;
  BOOLEAN                 IsFirstTD;
  EFI_STATUS              Status;
  UINT32                  Index;
  UINT8                   PktSize;

  EFI_USB_DATA_DIRECTION  TransferDirection;
  //
  //  Used to calculate how many entries are linked to the
  //  specified bulk transfer QH-TDs
  //
  UINT32                  LinkTimes;

  BOOLEAN                 ShortPacketEnable;
  EFI_PHYSICAL_ADDRESS    TempPtr;
  VOID                    *Mapping;

  HcDev             = USB_HC_DEV_FROM_THIS (This);
  StatusReg         = (UINT32) (USBSTS);
  FrameNumReg       = (UINT32) (USBFRNUM);
  PktID             = INPUT_PACKET_ID;
  PtrTD             = NULL;
  PtrFirstTD        = NULL;
  PtrPreTD          = NULL;
  LinkTimes         = 1;
  DataLen           = 0;
  Ptr               = NULL;
  ShortPacketEnable = FALSE;
  Mapping           = NULL;

  //
  // Parameters Checking
  //

  if ((DataLength == NULL) ||
      (Data == NULL)       ||
      (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  if (*DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaximumPacketLength != 8  &&
      MaximumPacketLength != 16 &&
      MaximumPacketLength != 32 &&
      MaximumPacketLength != 64) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Enable the maximum packet size (64bytes)
  // that can be used for full speed bandwidth reclamation
  // at the end of a frame.
  //
  EnableMaxPacketSize (HcDev);

  Status = ClearStatusReg (HcDev->PciIo, StatusReg);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
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

  switch (TransferDirection) {

  case EfiUsbDataIn:
    ShortPacketEnable = TRUE;
    PktID             = INPUT_PACKET_ID;
    PtrDataSource     = Data;
    DataLen           = *DataLength;

    //
    // BusMasterWrite means cpu read
    //
    Status = HcDev->PciIo->Map (
                             HcDev->PciIo,
                             EfiPciIoOperationBusMasterWrite,
                             PtrDataSource,
                             &DataLen,
                             &TempPtr,
                             &Mapping
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Ptr = (UINT8 *) ((UINTN) TempPtr);
    break;

  case EfiUsbDataOut:
    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = *DataLength;

    //
    // BusMasterRead means cpu write
    //
    Status = HcDev->PciIo->Map (
                             HcDev->PciIo,
                             EfiPciIoOperationBusMasterRead,
                             PtrDataSource,
                             &DataLen,
                             &TempPtr,
                             &Mapping
                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Ptr = (UINT8 *) ((UINTN) TempPtr);
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  //
  //  create QH structure and init
  //
  Status = CreateQH (HcDev, &PtrQH);
  if (EFI_ERROR (Status)) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
    return Status;
  }

  //
  // i is used to calculate the total number of TDs.
  //
  Index     = 0;

  IsFirstTD = TRUE;
  while (DataLen > 0) {

    //
    // create TD structures and link together
    //

    PktSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PktSize = MaximumPacketLength;
    }

    Status = GenDataTD (
               HcDev,
               DeviceAddress,
               EndPointAddress,
               Ptr,
               PktSize,
               PktID,
               *DataToggle,
               FALSE,
               &PtrTD
               );

    if (EFI_ERROR (Status)) {
      HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
      DeleteQueuedTDs (HcDev, PtrFirstTD);
      return Status;
    }

    //
    // Enable short packet detection.
    // (default action is disabling short packet detection)
    //
    if (ShortPacketEnable) {
      EnableorDisableTDShortPacket (PtrTD, TRUE);
    }

    if (IsFirstTD) {
      PtrFirstTD            = PtrTD;
      PtrFirstTD->ptrNextTD = NULL;
      IsFirstTD             = FALSE;
    } else {
      //
      // Link two TDs in vertical depth
      //
      LinkTDToTD (PtrPreTD, PtrTD);
    }

    Index++;

    PtrPreTD = PtrTD;

    *DataToggle ^= 1;
    Ptr += PktSize;
    DataLen -= PktSize;
  }

  //
  // link TD structures to QH structure
  //
  LinkTDToQH (PtrQH, PtrFirstTD);

  //
  // calculate how many entries are linked to the specified bulk transfer QH-TDs
  // the below values are referred to the USB spec revision1.1.
  //
  switch (MaximumPacketLength) {
  case 8:
    LinkTimes = Index / 71 + 1;
    break;

  case 16:
    LinkTimes = Index / 51 + 1;
    break;

  case 32:
    LinkTimes = Index / 33 + 1;
    break;

  case 64:
    LinkTimes = Index / 19 + 1;
    break;
  }

  LinkTimes += 500;

  //
  // put QH-TDs into  Frame list
  //
  LoadFrameListIndex  = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);
  SavedFrameListIndex = LoadFrameListIndex;

  for (Index = 0; Index <= LinkTimes; Index++) {

    //
    // put the QH-TD directly or indirectly into the proper place
    // in the Frame List
    //
    LinkQHToFrameList (HcDev->FrameListEntry, LoadFrameListIndex, PtrQH);

    LoadFrameListIndex += 1;
    LoadFrameListIndex &= 0x3FF;
  }

  LoadFrameListIndex = SavedFrameListIndex;

  //
  // Execute QH-TD and get result
  //
  //
  // detail status is put into the Result field in the pIRP
  // the Data Toggle value is also re-updated to the value
  // of the last successful TD
  //
  Status = ExecBulkorSyncInterruptTransfer (
             HcDev,
             PtrFirstTD,
             LoadFrameListIndex,
             DataLength,
             DataToggle,
             TimeOut,
             TransferResult
             );

  //
  // Delete Bulk transfer QH-TD structure
  // and maitain the pointers in the Frame List
  // and other pointers in related QH structure
  //
  // TRUE means must search other framelistindex
  //
  for (Index = 0; Index <= LinkTimes; Index++) {
    DelLinkSingleQH (
      HcDev,
      PtrQH,
      LoadFrameListIndex,
      FALSE,
      FALSE
      );
    LoadFrameListIndex += 1;
    LoadFrameListIndex &= 0x3FF;
  }

  UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));

  DeleteQueuedTDs (HcDev, PtrFirstTD);

  if (Mapping != NULL) {
    HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult |= EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (HcDev->PciIo, StatusReg);

  HcDev->PciIo->Flush (HcDev->PciIo);

  return Status;
}

EFI_STATUS
EFIAPI
UHCIAsyncInterruptTransfer (
  IN     EFI_USB_HC_PROTOCOL                * This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     BOOLEAN                            IsSlowDevice,
  IN     UINT8                              MaximumPacketLength,
  IN     BOOLEAN                            IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              PollingInterval, OPTIONAL
  IN     UINTN                              DataLength, OPTIONAL
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction, OPTIONAL
  IN     VOID                               *Context OPTIONAL
  )
/*++

  Routine Description:
    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device.

  Arguments:

    This            A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress   Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.

    EndPointAddress The combination of an endpoint number and an endpoint
                    direction of the target USB device. Each endpoint address
                    supports data transfer in one direction except the
                    control endpoint (whose default endpoint address is 0).
                    It is the caller's responsibility to make sure that
                    the EndPointAddress represents an interrupt endpoint.

    IsSlowDevice    Indicates whether the target device is slow device
                    or full-speed device.

    MaximumPacketLength  Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    IsNewTransfer   If TRUE, an asynchronous interrupt pipe is built between
                    the host and the target interrupt endpoint.
                    If FALSE, the specified asynchronous interrupt pipe
                    is canceled.

    DataToggle      A pointer to the data toggle value.  On input, it is valid
                    when IsNewTransfer is TRUE, and it indicates the initial
                    data toggle value the asynchronous interrupt transfer
                    should adopt.
                    On output, it is valid when IsNewTransfer is FALSE,
                    and it is updated to indicate the data toggle value of
                    the subsequent asynchronous interrupt transfer.

    PollingInterval Indicates the interval, in milliseconds, that the
                    asynchronous interrupt transfer is polled.
                    This parameter is required when IsNewTransfer is TRUE.

    DataLength      Indicates the length of data to be received at the
                    rate specified by PollingInterval from the target
                    asynchronous interrupt endpoint.  This parameter
                    is only required when IsNewTransfer is TRUE.

    CallBackFunction  The Callback function.This function is called at the
                      rate specified by PollingInterval.This parameter is
                      only required when IsNewTransfer is TRUE.

    Context         The context that is passed to the CallBackFunction.
                    This is an optional parameter and may be NULL.

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
  USB_HC_DEV            *HcDev;
  UINT32                StatusReg;
  UINT32                FrameNumReg;
  UINTN                 DataLen;
  QH_STRUCT             *ptrFirstQH;
  QH_STRUCT             *PtrQH;
  QH_STRUCT             *ptrPreQH;
  TD_STRUCT             *PtrFirstTD;
  TD_STRUCT             *PtrTD;
  TD_STRUCT             *PtrPreTD;
  UINT16                LoadFrameListIndex;
  UINT16                Index;
  UINT8                 PktID;
  UINT8                 *Ptr;
  UINT8                 *MappedPtr;
  BOOLEAN               IsFirstTD;
  BOOLEAN               IsFirstQH;
  EFI_STATUS            Status;
  BOOLEAN               ShortPacketEnable;
  UINT8                 CurrentDataToggle;
  EFI_PHYSICAL_ADDRESS  TempPtr;
  VOID                  *Mapping;
  UINT8                 PktSize;
  QH_STRUCT             *TempQH;
  EFI_TPL               OldTpl;

  HcDev             = USB_HC_DEV_FROM_THIS (This);
  StatusReg         = (UINT32) (USBSTS);
  FrameNumReg       = (UINT32) (USBFRNUM);
  Mapping           = NULL;
  ShortPacketEnable = FALSE;

  PktID             = INPUT_PACKET_ID;
  PtrTD             = NULL;
  PtrFirstTD        = NULL;
  PtrPreTD          = NULL;
  Ptr               = NULL;
  PtrQH             = NULL;
  ptrPreQH          = NULL;
  ptrFirstQH        = NULL;

  if ((EndPointAddress & 0x80) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // delete Async interrupt transfer request
  //
  if (!IsNewTransfer) {

    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

    Status = DeleteAsyncINTQHTDs (
               HcDev,
               DeviceAddress,
               EndPointAddress,
               DataToggle
               );

    gBS->RestoreTPL (OldTpl);

    return Status;
  }
  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (HcDev->PciIo, StatusReg);

  //
  // submit Async interrupt transfer request
  //
  if (PollingInterval < 1 || PollingInterval > 255) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  ShortPacketEnable = TRUE;
  PktID             = INPUT_PACKET_ID;
  DataLen           = DataLength;
  Ptr               = AllocatePool (DataLen);
  if (Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // BusMasterWrite means cpu read
  //
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterWrite,
                           Ptr,
                           &DataLen,
                           &TempPtr,
                           &Mapping
                           );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (Ptr);
    return Status;
  }

  MappedPtr         = (UINT8 *) ((UINTN) TempPtr);

  CurrentDataToggle = *DataToggle;

  IsFirstTD         = TRUE;

  while (DataLen > 0) {
    //
    // create TD structures and link together
    //

    PktSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PktSize = MaximumPacketLength;
    }

    Status = GenDataTD (
               HcDev,
               DeviceAddress,
               EndPointAddress,
               MappedPtr,
               PktSize,
               PktID,
               CurrentDataToggle,
               IsSlowDevice,
               &PtrTD
               );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (Ptr);
      HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      DeleteQueuedTDs (HcDev, PtrFirstTD);
      return Status;
    }
    //
    // Enable short packet detection.
    //
    if (ShortPacketEnable) {
      EnableorDisableTDShortPacket (PtrTD, TRUE);
    }

    if (IsFirstTD) {
      PtrFirstTD            = PtrTD;
      PtrFirstTD->ptrNextTD = NULL;
      IsFirstTD             = FALSE;
    } else {
      //
      // Link two TDs in vertical depth
      //
      LinkTDToTD (PtrPreTD, PtrTD);
    }

    PtrPreTD = PtrTD;

    CurrentDataToggle ^= 1;
    MappedPtr += PktSize;
    DataLen -= PktSize;
  }

  //
  // roll one value back
  //
  CurrentDataToggle ^= 1;

  //
  // create a list of QH structures and init,
  // link TDs to all the QHs, and link all the QHs together using internal
  // defined pointer of the QH_STRUCT.
  //
  IsFirstQH = TRUE;
  ptrPreQH  = NULL;
  for (Index = 0; Index < 1024;) {

    Status = CreateQH (HcDev, &PtrQH);
    if (EFI_ERROR (Status)) {
      gBS->FreePool (Ptr);
      HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      DeleteQueuedTDs (HcDev, PtrFirstTD);
      PtrQH = ptrFirstQH;
      while (PtrQH) {
        TempQH  = PtrQH;
        PtrQH   = TempQH->ptrNextIntQH;
        UhciFreePool (HcDev, (UINT8 *) TempQH, sizeof (QH_STRUCT));
      }

      return Status;
    }

    //
    // link TD structures to QH structure
    //
    LinkTDToQH (PtrQH, PtrFirstTD);

    if (IsFirstQH) {
      ptrFirstQH                = PtrQH;
      ptrFirstQH->ptrNextIntQH  = NULL;
      IsFirstQH                 = FALSE;
    } else {
      //
      // link neighbor QH structures together
      //
      ptrPreQH->ptrNextIntQH = PtrQH;
    }

    ptrPreQH  = PtrQH;

    Index     = (UINT16) (PollingInterval + Index);
  }
  //
  // last QH in QH list should set its next QH pointer to NULL.
  //
  PtrQH->ptrNextIntQH = NULL;

  //
  // Save QH-TD structures in Interrupt transfer list,
  // for monitor interrupt transfer execution routine use.
  //
  InsertQHTDToINTList (
    HcDev,
    ptrFirstQH,
    PtrFirstTD,
    DeviceAddress,
    EndPointAddress,
    CurrentDataToggle,
    DataLength,
    PollingInterval,
    Mapping,
    Ptr,
    CallBackFunction,
    Context
    );

  //
  // put QHs-TDs into  Frame list
  //
  LoadFrameListIndex  = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);

  PtrQH               = ptrFirstQH;

  for (Index = LoadFrameListIndex; Index < (1024 + LoadFrameListIndex);) {

    //
    // put the QH-TD directly or indirectly into the proper place
    // in the Frame List
    //
    LinkQHToFrameList (HcDev->FrameListEntry, (UINT16) (Index & 0x3FF), PtrQH);

    Index = (UINT16) (PollingInterval + Index);

    PtrQH = PtrQH->ptrNextIntQH;
  }

  HcDev->PciIo->Flush (HcDev->PciIo);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UHCISyncInterruptTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       BOOLEAN                 IsSlowDevice,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN OUT   UINTN                   *DataLength,
  IN OUT   UINT8                   *DataToggle,
  IN       UINTN                   TimeOut,
  OUT      UINT32                  *TransferResult
  )
/*++

  Routine Description:
    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device.

  Arguments:

    This            A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress   Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.

    EndPointAddress   The combination of an endpoint number and an endpoint
                      direction of the target USB device. Each endpoint
                      address supports data transfer in one direction
                      except the control endpoint (whose default
                      endpoint address is 0). It is the caller's responsibility
                      to make sure that the EndPointAddress represents
                      an interrupt endpoint.

    IsSlowDevice    Indicates whether the target device is slow device
                    or full-speed device.

    MaximumPacketLength Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    Data            A pointer to the buffer of data that will be transmitted
                    to USB device or received from USB device.

    DataLength      On input, the size, in bytes, of the data buffer specified
                    by Data. On output, the number of bytes transferred.

    DataToggle      A pointer to the data toggle value. On input, it indicates
                    the initial data toggle value the synchronous interrupt
                    transfer should adopt;
                    on output, it is updated to indicate the data toggle value
                    of the subsequent synchronous interrupt transfer.

    TimeOut         Indicates the maximum time, in microseconds, which the
                    transfer is allowed to complete.

    TransferResult  A pointer to the detailed result information from
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
  USB_HC_DEV            *HcDev;
  UINT32                StatusReg;
  UINT32                FrameNumReg;
  UINTN                 DataLen;
  QH_STRUCT             *PtrQH;
  TD_STRUCT             *PtrFirstTD;
  TD_STRUCT             *PtrTD;
  TD_STRUCT             *PtrPreTD;
  UINT16                LoadFrameListIndex;
  UINT16                SavedFrameListIndex;
  UINT32                Index;
  UINT32                LinkTimes;
  UINT8                 PktID;
  UINT8                 *PtrDataSource;
  UINT8                 *Ptr;
  BOOLEAN               IsFirstTD;
  EFI_STATUS            Status;
  BOOLEAN               ShortPacketEnable;
  EFI_PHYSICAL_ADDRESS  TempPtr;
  VOID                  *Mapping;
  UINT8                 PktSize;

  HcDev             = USB_HC_DEV_FROM_THIS (This);
  StatusReg         = (UINT32) (USBSTS);
  FrameNumReg       = (UINT32) (USBFRNUM);
  ShortPacketEnable = FALSE;
  Mapping           = NULL;
  PktID             = INPUT_PACKET_ID;
  PtrTD             = NULL;
  PtrFirstTD        = NULL;
  PtrPreTD          = NULL;
  DataLen           = 0;
  Ptr               = NULL;
  Index             = 0;
  LinkTimes         = 0;

  //
  // Parameters Checking
  //

  if ((DataLength == NULL) ||
      (Data == NULL)       ||
      (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  if ((EndPointAddress & 0x80) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (*DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaximumPacketLength > 64) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsSlowDevice && (MaximumPacketLength > 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if (TransferResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ClearStatusReg (HcDev->PciIo, StatusReg);

  //
  // submit Sync interrupt transfer request
  //
  ShortPacketEnable = TRUE;
  PktID             = INPUT_PACKET_ID;
  DataLen           = *DataLength;
  PtrDataSource     = Data;

  //
  // create QH structure and init
  //
  Status = CreateQH (HcDev, &PtrQH);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // BusMasterWrite means cpu read
  //
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterWrite,
                           PtrDataSource,
                           &DataLen,
                           &TempPtr,
                           &Mapping
                           );
  if (EFI_ERROR (Status)) {
    UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
    return Status;
  }

  Ptr       = (UINT8 *) ((UINTN) TempPtr);

  IsFirstTD = TRUE;
  while (DataLen > 0) {
    //
    // create TD structures and link together
    //
    PktSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PktSize = MaximumPacketLength;
    }

    Status = GenDataTD (
               HcDev,
               DeviceAddress,
               EndPointAddress,
               Ptr,
               PktSize,
               PktID,
               *DataToggle,
               IsSlowDevice,
               &PtrTD
               );
    if (EFI_ERROR (Status)) {
      UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));
      HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);
      DeleteQueuedTDs (HcDev, PtrFirstTD);
      return Status;
    }
    //
    // Enable short packet detection.
    //
    if (ShortPacketEnable) {
      EnableorDisableTDShortPacket (PtrTD, TRUE);
    }

    if (IsFirstTD) {
      PtrFirstTD            = PtrTD;
      PtrFirstTD->ptrNextTD = NULL;
      IsFirstTD             = FALSE;
    } else {
      //
      // Link two TDs in vertical depth
      //
      LinkTDToTD (PtrPreTD, PtrTD);
    }

    Index++;

    PtrPreTD = PtrTD;

    *DataToggle ^= 1;
    Ptr += PktSize;
    DataLen -= PktSize;
  }

  //
  // link TD structures to QH structure
  //
  LinkTDToQH (PtrQH, PtrFirstTD);

  switch (MaximumPacketLength) {
  case 8:
    LinkTimes = Index / 71 + 1;
    break;

  case 16:
    LinkTimes = Index / 51 + 1;
    break;

  case 32:
    LinkTimes = Index / 33 + 1;
    break;

  case 64:
    LinkTimes = Index / 19 + 1;
    break;
  }

  LinkTimes += 100;

  LoadFrameListIndex  = (UINT16) ((GetCurrentFrameNumber (HcDev->PciIo, FrameNumReg)) & 0x3FF);
  SavedFrameListIndex = LoadFrameListIndex;

  for (Index = 0; Index < LinkTimes; Index++) {

    //
    // put the QH-TD directly or indirectly into the proper place
    // in the Frame List
    //
    LinkQHToFrameList (HcDev->FrameListEntry, LoadFrameListIndex, PtrQH);

    LoadFrameListIndex += 1;
    LoadFrameListIndex &= 0x3FF;
  }

  LoadFrameListIndex = SavedFrameListIndex;
  //
  // detail status is put into the Result field in the pIRP
  // the Data Toggle value is also re-updated to the value
  // of the last successful TD
  //
  Status = ExecBulkorSyncInterruptTransfer (
             HcDev,
             PtrFirstTD,
             LoadFrameListIndex,
             DataLength,
             DataToggle,
             TimeOut,
             TransferResult
             );
  //
  // Delete Sync Interrupt transfer QH-TD structure
  // and maintain the pointers in the Frame List
  // and other pointers in related QH structure
  //
  // TRUE means must search other framelistindex
  //
  for (Index = 0; Index <= LinkTimes; Index++) {
    DelLinkSingleQH (
      HcDev,
      PtrQH,
      LoadFrameListIndex,
      FALSE,
      FALSE
      );
    LoadFrameListIndex += 1;
    LoadFrameListIndex &= 0x3FF;
  }

  UhciFreePool (HcDev, (UINT8 *) PtrQH, sizeof (QH_STRUCT));

  DeleteQueuedTDs (HcDev, PtrFirstTD);

  HcDev->PciIo->Unmap (HcDev->PciIo, Mapping);

  //
  // if has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (HcDev->PciIo, StatusReg)) {

    ClearStatusReg (HcDev->PciIo, StatusReg);
    *TransferResult |= EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (HcDev->PciIo, StatusReg);

  HcDev->PciIo->Flush (HcDev->PciIo);

  return Status;
}

EFI_STATUS
EFIAPI
UHCIIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL     *This,
  IN       UINT8                   DeviceAddress,
  IN       UINT8                   EndPointAddress,
  IN       UINT8                   MaximumPacketLength,
  IN OUT   VOID                    *Data,
  IN       UINTN                   DataLength,
  OUT      UINT32                  *TransferResult
  )
/*++

  Routine Description:
    Submits isochronous transfer to a target USB device.

  Arguments:

    This                - A pointer to the EFI_USB_HC_PROTOCOL instance.
    DeviceAddress       - Represents the address of the target device on the USB,
                           which is assigned during USB enumeration.
    EndPointAddress     - End point address
    MaximumPacketLength - Indicates the maximum packet size that the
                           default control transfer endpoint is capable of
                           sending or receiving.
    Data                - A pointer to the buffer of data that will be transmitted
                           to USB device or received from USB device.
    DataLength          - Indicates the size, in bytes, of the data buffer
                           specified by Data.
    TransferResult      - A pointer to the detailed result information generated
                           by this control transfer.
  Returns:
    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
UHCIAsyncIsochronousTransfer (
  IN       EFI_USB_HC_PROTOCOL                 * This,
  IN       UINT8                               DeviceAddress,
  IN       UINT8                               EndPointAddress,
  IN       UINT8                               MaximumPacketLength,
  IN OUT   VOID                                *Data,
  IN       UINTN                               DataLength,
  IN       EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN       VOID                                *Context OPTIONAL
  )
/*++

  Routine Description:
    Submits Async isochronous transfer to a target USB device.

  Arguments:

    This                - A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress       - Represents the address of the target device on the USB,
                           which is assigned during USB enumeration.

    EndPointAddress     - End point address

    MaximumPacketLength - Indicates the maximum packet size that the
                           default control transfer endpoint is capable of
                           sending or receiving.

    Data                - A pointer to the buffer of data that will be transmitted
                           to USB device or received from USB device.

    IsochronousCallBack - When the transfer complete, the call back function will be called

    Context             - Pass to the call back function as parameter

  Returns:
    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}

//
// UEFI 2.0 Protocol
//
EFI_STATUS
EFIAPI
UHCI2GetCapability(
  IN  EFI_USB2_HC_PROTOCOL  * This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
/*++

  Routine Description:
    Retrieves capabilities of USB host controller according to UEFI 2.0 spec.

  Arguments:
    This                      - A pointer to the EFI_USB2_HC_PROTOCOL instance.

    MaxSpeed             - A pointer to the max speed USB host controller supports.

    PortNumber           - A pointer to the number of root hub ports.

    Is64BitCapable      - A pointer to an integer to show whether USB host controller
                                  supports 64-bit memory addressing.
  Returns:
    EFI_SUCCESS
        The host controller capabilities were retrieved successfully.
    EFI_INVALID_PARAMETER
        MaxSpeed or PortNumber or Is64BitCapable is NULL.
    EFI_DEVICE_ERROR
	An error was encountered while attempting to retrieve the capabilities.

--*/
{
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  if ((NULL == MaxSpeed)
  	||(NULL == PortNumber)
  	|| (NULL == Is64BitCapable))
  {
    return EFI_INVALID_PARAMETER;
  }

  *MaxSpeed = EFI_USB_SPEED_FULL;
  *Is64BitCapable = (UINT8)FALSE;
  return  UHCIGetRootHubPortNumber(&HcDev->UsbHc, PortNumber);
}

EFI_STATUS
EFIAPI
UHCI2Reset (
  IN EFI_USB2_HC_PROTOCOL   * This,
  IN UINT16                 Attributes
  )
/*++

  Routine Description:
    Provides software reset for the USB host controller according to UEFI 2.0 spec.

  Arguments:
    This           - A pointer to the EFI_USB2_HC_PROTOCOL instance.

    Attributes   - A bit mask of the reset operation to perform.
                       See below for a list of the supported bit mask values.

  #define EFI_USB_HC_RESET_GLOBAL                      0x0001
  #define EFI_USB_HC_RESET_HOST_CONTROLLER             0x0002
  #define EFI_USB_HC_RESET_GLOBAL _WITH_DEBUG          0x0004
  #define EFI_USB_HC_RESET_HOST_WITH_DEBUG             0x0008

  EFI_USB_HC_RESET_GLOBAL
        If this bit is set, a global reset signal will be sent to the USB bus.
        This resets all of the USB bus logic, including the USB host
        controller hardware and all the devices attached on the USB bus.
  EFI_USB_HC_RESET_HOST_CONTROLLER
        If this bit is set, the USB host controller hardware will be reset.
        No reset signal will be sent to the USB bus.

  Returns:
    EFI_SUCCESS
        The reset operation succeeded.
    EFI_INVALID_PARAMETER
        Attributes is not valid.
    EFI_UNSUPPORTED
    	 The type of reset specified by Attributes is not currently supported by the host controller hardware.
    EFI_ACCESS_DENIED
    	 Reset operation is rejected due to the debug port being configured and active.
    EFI_DEVICE_ERROR
        An error was encountered while attempting to perform
        the reset operation.
--*/
{
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  if (Attributes==EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG || Attributes==EFI_USB_HC_RESET_HOST_WITH_DEBUG)
  	return EFI_UNSUPPORTED;

  return UHCIReset(
  	&HcDev->UsbHc,
  	Attributes
  	);
}

EFI_STATUS
EFIAPI
UHCI2GetState (
  IN  EFI_USB2_HC_PROTOCOL   * This,
  OUT EFI_USB_HC_STATE       * State
  )
/*++

  Routine Description:
    Retrieves current state of the USB host controller according to UEFI 2.0 spec.

  Arguments:

    This     - A pointer to the EFI_USB_HC_PROTOCOL instance.

    State    - A pointer to the EFI_USB_HC_STATE data structure that
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
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  return UHCIGetState(
  	&HcDev->UsbHc,
  	State
  	);
}

EFI_STATUS
EFIAPI
UHCI2SetState (
  IN EFI_USB2_HC_PROTOCOL    * This,
  IN EFI_USB_HC_STATE        State
  )
/*++

  Routine Description:
    Sets the USB host controller to a specific state according to UEFI 2.0 spec.

  Arguments:

    This     - A pointer to the EFI_USB_HC_PROTOCOL instance.

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
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  return UHCISetState(
  	&HcDev->UsbHc,
  	State
  	);
}

EFI_STATUS
EFIAPI
UHCI2ControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL                           * This,
  IN     UINT8                                          DeviceAddress,
  IN     UINT8                                          DeviceSpeed,
  IN     UINTN                                          MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST                         * Request,
  IN     EFI_USB_DATA_DIRECTION                         TransferDirection,
  IN OUT VOID                                           *Data,
  IN OUT UINTN                                          *DataLength,
  IN     UINTN                                          TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR             *Translator,
  OUT    UINT32                                         *TransferResult
  )
/*++

  Routine Description:
    Submits control transfer to a target USB device accroding to UEFI 2.0 spec..

  Arguments:

    This         - A pointer to the EFI_USB_HC_PROTOCOL instance.

    DeviceAddress -Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.

    DeviceSpeed  - Indicates transfer speed of device.

    MaximumPacketLength - Indicates the maximum packet size that the
                        default control transfer endpoint is capable of
                        sending or receiving.

    Request      - A pointer to the USB device request that will be sent
                  to the USB device.

    TransferDirection - Specifies the data direction for the transfer.
                      There are three values available, DataIn, DataOut
                      and NoData.

    Data          -A pointer to the buffer of data that will be transmitted
                  to USB device or received from USB device.

    DataLength    - Indicates the size, in bytes, of the data buffer
                  specified by Data.

    TimeOut       - Indicates the maximum time, in microseconds,
                  which the transfer is allowed to complete.

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
  USB_HC_DEV *HcDev;
  BOOLEAN IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  return UHCIControlTransfer(
  	&HcDev->UsbHc,
  	DeviceAddress,
  	IsSlowDevice,
  	(UINT8) MaximumPacketLength,
  	Request,
  	TransferDirection,
  	Data,
  	DataLength,
  	TimeOut,
  	TransferResult
  	);
}

EFI_STATUS
EFIAPI
UHCI2BulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL                            * This,
  IN     UINT8                                           DeviceAddress,
  IN     UINT8                                           EndPointAddress,
  IN     UINT8                                           DeviceSpeed,
  IN     UINTN                                           MaximumPacketLength,
  IN     UINT8                                           DataBuffersNumber,
  IN OUT VOID                                            *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                                           *DataLength,
  IN OUT UINT8                                           *DataToggle,
  IN     UINTN                                           TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR              *Translator,
  OUT    UINT32                                          *TransferResult
  )
/*++

  Routine Description:
    Submits bulk transfer to a bulk endpoint of a USB device according to UEFI 2.0 spec.

  Arguments:

    This          A pointer to the EFI_USB2_HC_PROTOCOL instance.

    DeviceAddress Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.

    EndPointAddress   The combination of an endpoint number and an
                      endpoint direction of the target USB device.
                      Each endpoint address supports data transfer in
                      one direction except the control endpoint
                      (whose default endpoint address is 0).
                      It is the caller's responsibility to make sure that
                      the EndPointAddress represents a bulk endpoint.

    DeviceSpeed  Indicates device speed. The supported values are EFI_USB_SPEED_FULL
                          and EFI_USB_SPEED_HIGH.

    MaximumPacketLength Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    DataBuffersNumber  Number of data buffers prepared for the transfer.

    Data          Array of pointers to the buffers of data that will be transmitted
                  to USB device or received from USB device.

    DataLength    When input, indicates the size, in bytes, of the data buffer
                  specified by Data. When output, indicates the actually
                  transferred data size.

    DataToggle    A pointer to the data toggle value. On input, it indicates
                  the initial data toggle value the bulk transfer should adopt;
                  on output, it is updated to indicate the data toggle value
                  of the subsequent bulk transfer.

    Translator  A pointr to the transaction translator data.

    TimeOut       Indicates the maximum time, in microseconds, which the
                  transfer is allowed to complete.

    TransferResult  A pointer to the detailed result information of the
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
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  if( Data == NULL || DeviceSpeed==EFI_USB_SPEED_LOW)
  	return EFI_INVALID_PARAMETER;
  /* For full-speed bulk transfers only the data pointed by Data[0] shall be used */

  return UHCIBulkTransfer (
  	&HcDev->UsbHc,
  	DeviceAddress,
  	EndPointAddress,
  	(UINT8) MaximumPacketLength,
  	*Data,
  	DataLength,
  	DataToggle,
  	TimeOut,
  	TransferResult
  	);
}

EFI_STATUS
EFIAPI
UHCI2AsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                        * This,
  IN     UINT8                                       DeviceAddress,
  IN     UINT8                                       EndPointAddress,
  IN     UINT8                                       DeviceSpeed,
  IN     UINTN                                       MaximumPacketLength,
  IN     BOOLEAN                                     IsNewTransfer,
  IN OUT UINT8                                       *DataToggle,
  IN     UINTN                                       PollingInterval,
  IN     UINTN                                       DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR          *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK             CallBackFunction,
  IN     VOID                                        *Context
  )
/*++

  Routine Description:
    Submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device according to UEFI 2.0 spec.

  Arguments:

    This            A pointer to the EFI_USB2_HC_PROTOCOL instance.

    DeviceAddress   Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.

    EndPointAddress The combination of an endpoint number and an endpoint
                    direction of the target USB device. Each endpoint address
                    supports data transfer in one direction except the
                    control endpoint (whose default endpoint address is 0).
                    It is the caller's responsibility to make sure that
                    the EndPointAddress represents an interrupt endpoint.

    DeviceSpeed     Indicates device speed.

    MaximumPacketLength  Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    IsNewTransfer   If TRUE, an asynchronous interrupt pipe is built between
                    the host and the target interrupt endpoint.
                    If FALSE, the specified asynchronous interrupt pipe
                    is canceled.

    DataToggle      A pointer to the data toggle value.  On input, it is valid
                    when IsNewTransfer is TRUE, and it indicates the initial
                    data toggle value the asynchronous interrupt transfer
                    should adopt.
                    On output, it is valid when IsNewTransfer is FALSE,
                    and it is updated to indicate the data toggle value of
                    the subsequent asynchronous interrupt transfer.

    PollingInterval Indicates the interval, in milliseconds, that the
                    asynchronous interrupt transfer is polled.
                    This parameter is required when IsNewTransfer is TRUE.

    DataLength      Indicates the length of data to be received at the
                    rate specified by PollingInterval from the target
                    asynchronous interrupt endpoint.  This parameter
                    is only required when IsNewTransfer is TRUE.

    Translator  A pointr to the transaction translator data.

    CallBackFunction  The Callback function.This function is called at the
                      rate specified by PollingInterval.This parameter is
                      only required when IsNewTransfer is TRUE.

    Context         The context that is passed to the CallBackFunction.
                    This is an optional parameter and may be NULL.

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
  USB_HC_DEV *HcDev;
  BOOLEAN IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  return UHCIAsyncInterruptTransfer(
  	&HcDev->UsbHc,
  	DeviceAddress,
  	EndPointAddress,
  	IsSlowDevice,
  	(UINT8) MaximumPacketLength,
  	IsNewTransfer,
  	DataToggle,
  	PollingInterval,
  	DataLength,
  	CallBackFunction,
  	Context
  	);
}

EFI_STATUS
EFIAPI
UHCI2SyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                      * This,
  IN     UINT8                                     DeviceAddress,
  IN     UINT8                                     EndPointAddress,
  IN     UINT8                                     DeviceSpeed,
  IN     UINTN                                     MaximumPacketLength,
  IN OUT VOID                                      *Data,
  IN OUT UINTN                                     *DataLength,
  IN OUT UINT8                                     *DataToggle,
  IN     UINTN                                     TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR        *Translator,
  OUT    UINT32                                    *TransferResult
  )
/*++

  Routine Description:
    Submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device according to UEFI 2.0 spec.

  Arguments:

    This            A pointer to the EFI_USB2_HC_PROTOCOL instance.

    DeviceAddress   Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.

    EndPointAddress   The combination of an endpoint number and an endpoint
                      direction of the target USB device. Each endpoint
                      address supports data transfer in one direction
                      except the control endpoint (whose default
                      endpoint address is 0). It is the caller's responsibility
                      to make sure that the EndPointAddress represents
                      an interrupt endpoint.

    DeviceSpeed  Indicates device speed.

    MaximumPacketLength Indicates the maximum packet size the target endpoint
                        is capable of sending or receiving.

    Data            A pointer to the buffer of data that will be transmitted
                    to USB device or received from USB device.

    DataLength      On input, the size, in bytes, of the data buffer specified
                    by Data. On output, the number of bytes transferred.

    DataToggle      A pointer to the data toggle value. On input, it indicates
                    the initial data toggle value the synchronous interrupt
                    transfer should adopt;
                    on output, it is updated to indicate the data toggle value
                    of the subsequent synchronous interrupt transfer.

    TimeOut         Indicates the maximum time, in microseconds, which the
                    transfer is allowed to complete.
    Translator  A pointr to the transaction translator data.
    TransferResult  A pointer to the detailed result information from
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
  USB_HC_DEV *HcDev;
  BOOLEAN IsSlowDevice;

  if(DeviceSpeed==EFI_USB_SPEED_HIGH)
  	return EFI_INVALID_PARAMETER;

  IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);
  HcDev = USB2_HC_DEV_FROM_THIS (This);

  return UHCISyncInterruptTransfer(
  	&HcDev->UsbHc,
  	DeviceAddress,
  	EndPointAddress,
  	IsSlowDevice,
  	(UINT8) MaximumPacketLength,
  	Data,
  	DataLength,
  	DataToggle,
  	TimeOut,
  	TransferResult
  	);
}

EFI_STATUS
EFIAPI
UHCI2IsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                       * This,
  IN     UINT8                                      DeviceAddress,
  IN     UINT8                                      EndPointAddress,
  IN     UINT8                                      DeviceSpeed,
  IN     UINTN                                      MaximumPacketLength,
  IN     UINT8                                      DataBuffersNumber,
  IN OUT VOID                                       *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                                      DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR         *Translator,
  OUT    UINT32                                     *TransferResult
  )
/*++

  Routine Description:

    Submits isochronous transfer to a target USB device according to UEFI 2.0 spec.

  Arguments:

    This             A pointer to the EFI_USB2_HC_PROTOCOL instance.

    DeviceAddress    Represents the address of the target device on the USB,
                           which is assigned during USB enumeration.

    EndPointAddress  End point address

    DeviceSpeed      Indicates device speed.

    MaximumPacketLength    Indicates the maximum packet size that the
                           default control transfer endpoint is capable of
                           sending or receiving.

    DataBuffersNumber Number of data buffers prepared for the transfer.

    Data              Array of pointers to the buffers of data that will be
                      transmitted to USB device or received from USB device.

    DataLength        Indicates the size, in bytes, of the data buffer
                      specified by Data.

    Translator        A pointr to the transaction translator data.

    TransferResult    A pointer to the detailed result information generated
                      by this control transfer.
  Returns:

    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
UHCI2AsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                         * This,
  IN     UINT8                                        DeviceAddress,
  IN     UINT8                                        EndPointAddress,
  IN     UINT8                                        DeviceSpeed,
  IN     UINTN                                        MaximumPacketLength,
  IN     UINT8                                        DataBuffersNumber,
  IN OUT VOID                                         *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                                        DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR           *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK              IsochronousCallBack,
  IN     VOID                                         *Context
  )
/*++

  Routine Description:

    Submits Async isochronous transfer to a target USB device according to UEFI 2.0 spec.

  Arguments:

    This                A pointer to the EFI_USB2_HC_PROTOCOL instance.

    DeviceAddress       Represents the address of the target device on the USB,
                           which is assigned during USB enumeration.

    EndPointAddress     End point address

    DeviceSpeed         Indicates device speed.

    MaximumPacketLength Indicates the maximum packet size that the
                        default control transfer endpoint is capable of
                        sending or receiving.

    DataBuffersNumber   Number of data buffers prepared for the transfer.

    Data                Array of pointers to the buffers of data that will be transmitted
                        to USB device or received from USB device.

    Translator          A pointr to the transaction translator data.

    IsochronousCallBack When the transfer complete, the call back function will be called

    Context             Pass to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
UHCI2GetRootHubPortStatus (
  IN  EFI_USB2_HC_PROTOCOL   * This,
  IN  UINT8                  PortNumber,
  OUT EFI_USB_PORT_STATUS    * PortStatus
  )
/*++

  Routine Description:
    Retrieves the current status of a USB root hub port according to UEFI 2.0 spec.

  Arguments:

    This        A pointer to the EFI_USB2_HC_PROTOCOL.

    PortNumber  Specifies the root hub port from which the status
                is to be retrieved.  This value is zero-based. For example,
                if a root hub has two ports, then the first port is numbered 0,
                and the second port is numbered 1.

    PortStatus  A pointer to the current port status bits and
                port status change bits.

  Returns:
    EFI_SUCCESS
        The status of the USB root hub port specified by PortNumber
        was returned in PortStatus.
    EFI_INVALID_PARAMETER
        PortNumber is invalid.
    EFI_DEVICE_ERROR - Can't read register
--*/
{
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);

  return UHCIGetRootHubPortStatus(
  	&HcDev->UsbHc,
  	PortNumber,
  	PortStatus
  	);
}

EFI_STATUS
EFIAPI
UHCI2SetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL     * This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
/*++

  Routine Description:
    Sets a feature for the specified root hub port according to UEFI 2.0 spec.

  Arguments:

    This        A pointer to the EFI_USB2_HC_PROTOCOL.

    PortNumber  Specifies the root hub port whose feature
                is requested to be set.

    PortFeature Indicates the feature selector associated
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
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  return UHCISetRootHubPortFeature(
  	&HcDev->UsbHc,
  	PortNumber,
  	PortFeature
  	);
}

EFI_STATUS
EFIAPI
UHCI2ClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    * This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
/*++

  Routine Description:
    Clears a feature for the specified root hub port according to Uefi 2.0 spec.

  Arguments:

    This        A pointer to the EFI_USB2_HC_PROTOCOL instance.

    PortNumber  Specifies the root hub port whose feature
                is requested to be cleared.

    PortFeature Indicates the feature selector associated with the
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
  USB_HC_DEV *HcDev;

  HcDev = USB2_HC_DEV_FROM_THIS (This);
  return UHCIClearRootHubPortFeature(
  	&HcDev->UsbHc,
  	PortNumber,
  	PortFeature
  	);
}

VOID
EFIAPI
MonitorInterruptTrans (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++
  Routine Description:
    Interrupt transfer periodic check handler
  Arguments:
    Event  - Interrupt event
    Contex - Pointer to USB_HC_DEV
  Returns:
    None
--*/
{

  USB_HC_DEV      *HcDev;
  INTERRUPT_LIST  *PtrList;
  LIST_ENTRY      *Link;
  UINT32          Result;
  VOID            *DataBuffer;
  UINTN           DataLen;
  UINTN           ActualLen;
  UINTN           ErrTDPos;
  LIST_ENTRY      *NextLink;

  HcDev       = (USB_HC_DEV *) Context;

  //
  // interrupt transfer list is empty, means that no interrupt transfer
  // is submitted by far.
  //
  if (IsListEmpty (&(HcDev->InterruptListHead))) {
    return ;
  }

  NextLink = HcDev->InterruptListHead.ForwardLink;
  do {

    Link      = NextLink;
    NextLink  = Link->ForwardLink;

    PtrList   = INTERRUPT_LIST_FROM_LINK (Link);

    //
    // get TD execution results.
    // ErrTDPos is zero-based value indicating the first error TD's position
    // in the TDs' list.
    // This value is only valid when Result not equal NOERROR.
    //
    ExecuteAsyncINTTDs (
      HcDev,
      PtrList,
      &Result,
      &ErrTDPos,
      &ActualLen
      );

    //
    // interrupt transfer has not been executed yet.
    //
    if (((Result & EFI_USB_ERR_NAK) == EFI_USB_ERR_NAK) ||
        ((Result & EFI_USB_ERR_NOTEXECUTE) == EFI_USB_ERR_NOTEXECUTE)) {
      continue;
    }
    //
    // get actual data length transferred data and its data length.
    //
    DataLen     = ActualLen;
    DataBuffer  = AllocatePool (DataLen);
    if (DataBuffer == NULL) {
      return ;
    }

    CopyMem (
      DataBuffer,
      PtrList->PtrFirstTD->pTDBuffer,
      DataLen
      );

    //
    // only if interrupt endpoint responds
    // and the interrupt transfer stops because of completion
    // or error, then we will call callback function.
    //
    if (Result == EFI_USB_NOERROR) {
      //
      // add for real platform debug
      //
      if (PtrList->InterruptCallBack != NULL) {
        (PtrList->InterruptCallBack) (
                    DataBuffer,
                    DataLen,
                    PtrList->InterruptContext,
                    Result
                    );
      }

      if (DataBuffer) {
        gBS->FreePool (DataBuffer);
      }

      //
      // update should done after data buffer got.
      //
      UpdateAsyncINTQHTDs (PtrList, Result, (UINT32) ErrTDPos);

    } else {

      DEBUG ((EFI_D_ERROR, "interrupt transfer error code is %x\n", Result));

      if (DataBuffer) {
        gBS->FreePool (DataBuffer);
      }
      //
      // leave error recovery to its related device driver.
      // A common case of the error recovery is to re-submit the interrupt
      // transfer.
      // When an interrupt transfer is re-submitted, its position in the linked
      // list is changed. It is inserted to the head of the linked list, while
      // this function scans the whole list from head to tail. Thus, the
      // re-submitted interrupt transfer's callback function will not be called
      // again in this round.
      //
      if (PtrList->InterruptCallBack != NULL) {
        (PtrList->InterruptCallBack) (
                    NULL,
                    0,
                    PtrList->InterruptContext,
                    Result
                    );
      }
    }
  } while (NextLink != &(HcDev->InterruptListHead));

}
