/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbBus.c

  Abstract:

    USB Bus Driver

  Revision History

--*/

#include "usbbus.h"


GLOBAL_REMOVE_IF_UNREFERENCED    UINTN    gUSBDebugLevel  = EFI_D_INFO;
GLOBAL_REMOVE_IF_UNREFERENCED    UINTN    gUSBErrorLevel  = EFI_D_ERROR;

//
// The UsbBusProtocol is just used to locate USB_BUS_CONTROLLER
// structure in the UsbBusDriverControllerDriverStop(). Then we can
// Close all opened protocols and release this structure.
//
STATIC EFI_GUID             mUsbBusProtocolGuid = EFI_USB_BUS_PROTOCOL_GUID;

EFI_DRIVER_BINDING_PROTOCOL gUsbBusDriverBinding = {
  UsbBusControllerDriverSupported,
  UsbBusControllerDriverStart,
  UsbBusControllerDriverStop,
  0xa,
  NULL,
  NULL
};

//
// Internal use only
//
STATIC
EFI_STATUS
ReportUsbStatusCode (
  IN USB_BUS_CONTROLLER_DEVICE     *UsbBusController,
  IN EFI_STATUS_CODE_TYPE          Type,
  IN EFI_STATUS_CODE_VALUE         Code
  );

STATIC
USB_IO_CONTROLLER_DEVICE    *
CreateUsbIoControllerDevice (
  VOID
  );

STATIC
EFI_STATUS
InitUsbIoController (
  IN USB_IO_CONTROLLER_DEVICE     *UsbIoController
  );

//
// USB Device Configuration / Deconfiguration
//
STATIC
EFI_STATUS
UsbDeviceConfiguration (
  IN USB_IO_CONTROLLER_DEVICE     *ParentHubController,
  IN EFI_HANDLE                   HostController,
  IN UINT8                        ParentPort,
  IN USB_IO_DEVICE                *UsbIoDevice
  );

//
// Usb Bus enumeration function
//
STATIC
VOID
RootHubEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

STATIC
VOID
HubEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

STATIC
EFI_STATUS
UsbSetTransactionTranslator (
  IN USB_IO_CONTROLLER_DEVICE     *ParentHubController,
  IN UINT8                        ParentPort,
  IN OUT USB_IO_DEVICE            *Device
  );

STATIC
EFI_STATUS
UsbUnsetTransactionTranslator (
  USB_IO_DEVICE *Device
  );

STATIC
EFI_STATUS
IdentifyDeviceSpeed (
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  USB_IO_DEVICE             *NewDevice,
  UINT8                     Index
  );

STATIC
EFI_STATUS
ReleasePortToCHC (
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  UINT8                     PortNum
  );

STATIC
EFI_STATUS
ResetRootPort (
  IN USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN UINT8                     PortNum,
  IN UINT8                     RetryTimes
  );

STATIC
EFI_STATUS
ResetHubPort (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN UINT8                       PortIndex
  );

STATIC
EFI_STATUS
ParentPortReset (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN BOOLEAN                     ReConfigure,
  IN UINT8                       RetryTimes
  );

//
// Following are address allocate and free functions
//
STATIC
UINT8
UsbAllocateAddress (
  IN UINT8    *AddressPool
  )
/*++

  Routine Description:
    Allocate address for usb device

  Arguments:
   AddressPool - Pool of usb device address

  Returns:
   Usb device address

--*/
{
  UINT8 ByteIndex;
  UINT8 BitIndex;

  for (ByteIndex = 0; ByteIndex < 16; ByteIndex++) {
    for (BitIndex = 0; BitIndex < 8; BitIndex++) {
      if ((AddressPool[ByteIndex] & (1 << BitIndex)) == 0) {
        //
        // Found one, covert to address, and mark it use
        //
        AddressPool[ByteIndex] = (UINT8) (AddressPool[ByteIndex] | (1 << BitIndex));
        return (UINT8) (ByteIndex * 8 + BitIndex);
      }
    }
  }

  return 0;

}

STATIC
VOID
UsbFreeAddress (
  IN UINT8     DevAddress,
  IN UINT8     *AddressPool
  )
/*++

  Routine Description:
    Free address for usb device

  Arguments:
   DevAddress  - Usb device address
   AddressPool - Pool of usb device address

  Returns:
   VOID

--*/
{
  UINT8 WhichByte;
  UINT8 WhichBit;
  //
  // Locate the position
  //
  WhichByte = (UINT8) (DevAddress / 8);
  WhichBit  = (UINT8) (DevAddress & 0x7);

  AddressPool[WhichByte] = (UINT8) (AddressPool[WhichByte] & (~(1 << WhichBit)));
}

EFI_STATUS
EFIAPI
UsbBusControllerDriverSupported (
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
    RemainingDevicePath - Device Path Protocol instance pointer

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *ParentDevicePath;
  EFI_USB2_HC_PROTOCOL       *Usb2Hc;
  EFI_USB_HC_PROTOCOL        *UsbHc;
  EFI_DEV_PATH_PTR           Node;

  //
  // Check Device Path
  //
  if (RemainingDevicePath != NULL) {
    Node.DevPath = RemainingDevicePath;
    if (Node.DevPath->Type != MESSAGING_DEVICE_PATH ||
        Node.DevPath->SubType != MSG_USB_DP         ||
        DevicePathNodeLength(Node.DevPath) != sizeof(USB_DEVICE_PATH)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Check whether USB Host Controller Protocol is already
  // installed on this handle. If it is installed, we can start
  // USB Bus Driver now.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUsbHcProtocolGuid,
                    (VOID **) &UsbHc,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
    if (Status == EFI_ALREADY_STARTED) {
      return EFI_SUCCESS;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    gBS->CloseProtocol (
      Controller,
      &gEfiUsbHcProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );
    return EFI_SUCCESS;
  }

  gBS->CloseProtocol (
    Controller,
    &gEfiUsb2HcProtocolGuid,
    This->DriverBindingHandle,
    Controller
    );

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
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
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
    EFI_OUT_OF_RESOURCES- Can't allocate memory resources

--*/
{
  EFI_STATUS                Status;
  EFI_STATUS                OpenStatus;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  USB_IO_DEVICE             *RootHub;
  USB_IO_CONTROLLER_DEVICE  *RootHubController;
  UINT8                     MaxSpeed;
  UINT8                     PortNumber;
  UINT8                     Is64BitCapable;

  //
  // Allocate USB_BUS_CONTROLLER_DEVICE structure
  //
  UsbBusDev = NULL;
  UsbBusDev = AllocateZeroPool (sizeof (USB_BUS_CONTROLLER_DEVICE));
  if (UsbBusDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UsbBusDev->Signature      = USB_BUS_DEVICE_SIGNATURE;
  UsbBusDev->AddressPool[0] = 1;

  //
  // Get the Device Path Protocol on Controller's handle
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &UsbBusDev->DevicePath,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

  if (EFI_ERROR (OpenStatus)) {
    gBS->FreePool (UsbBusDev);
    return OpenStatus;
  }
  //
  // Locate the Host Controller Interface
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsb2HcProtocolGuid,
                      (VOID **) &(UsbBusDev->Usb2HCInterface),
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
  if (EFI_ERROR (OpenStatus)) {

    UsbBusDev->Hc2ProtocolSupported = FALSE;
    OpenStatus = gBS->OpenProtocol (
                        Controller,
                        &gEfiUsbHcProtocolGuid,
                        (VOID **) &(UsbBusDev->UsbHCInterface),
                        This->DriverBindingHandle,
                        Controller,
                        EFI_OPEN_PROTOCOL_BY_DRIVER
                        );
    if (EFI_ERROR (OpenStatus)) {
      //
      // Report Status Code here since we will reset the host controller
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        EFI_IO_BUS_USB | EFI_IOB_EC_CONTROLLER_ERROR,
        UsbBusDev->DevicePath
        );

      gBS->CloseProtocol (
             Controller,
             &gEfiDevicePathProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      gBS->FreePool (UsbBusDev);
      return OpenStatus;
    }

    DEBUG ((gUSBDebugLevel, "UsbHcProtocol Opened.\n"));
  } else {
    DEBUG ((gUSBDebugLevel, "Usb2HcProtocol Opened.\n"));
    UsbBusDev->Hc2ProtocolSupported = TRUE;
  }

  //
  // Attach EFI_USB_BUS_PROTOCOL to controller handle,
  // for locate UsbBusDev later
  //
  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &mUsbBusProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &UsbBusDev->BusIdentify
                  );

  if (EFI_ERROR (Status)) {

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (UsbBusDev->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsb2HcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    gBS->FreePool (UsbBusDev);
    return Status;
  }
  //
  // Add root hub to the tree
  //
  RootHub = NULL;
  RootHub = AllocateZeroPool (sizeof (USB_IO_DEVICE));
  if (RootHub == NULL) {
    gBS->UninstallProtocolInterface (
           Controller,
           &mUsbBusProtocolGuid,
           &UsbBusDev->BusIdentify
           );
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (UsbBusDev->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsb2HcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    gBS->FreePool (UsbBusDev);
    return EFI_OUT_OF_RESOURCES;
  }

  RootHub->BusController  = UsbBusDev;
  RootHub->DeviceAddress  = UsbAllocateAddress (UsbBusDev->AddressPool);

  UsbBusDev->Root         = RootHub;

  //
  // Allocate Root Hub Controller
  //
  RootHubController = CreateUsbIoControllerDevice ();
  if (RootHubController == NULL) {
    gBS->UninstallProtocolInterface (
           Controller,
           &mUsbBusProtocolGuid,
           &UsbBusDev->BusIdentify
           );
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (UsbBusDev->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsb2HcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }
    gBS->FreePool (UsbBusDev);
    gBS->FreePool (RootHub);
    return EFI_OUT_OF_RESOURCES;
  }

  UsbVirtualHcGetCapability (
    UsbBusDev,
    &MaxSpeed,
    &PortNumber,
    &Is64BitCapable
    );
  RootHubController->DownstreamPorts  = PortNumber;
  RootHubController->UsbDevice        = RootHub;
  RootHubController->IsUsbHub         = TRUE;
  RootHubController->DevicePath       = UsbBusDev->DevicePath;
  RootHubController->HostController   = Controller;

  RootHub->NumOfControllers           = 1;
  RootHub->UsbController[0]           = RootHubController;
  RootHub->DeviceSpeed                = MaxSpeed;

  //
  // Report Status Code here since we will reset the host controller
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_USB | EFI_IOB_PC_RESET,
    UsbBusDev->DevicePath
    );

  //
  // Reset USB Host Controller
  //
  UsbVirtualHcReset (
    UsbBusDev,
    EFI_USB_HC_RESET_GLOBAL
    );

  //
  // Report Status Code while we are going to bring up the Host Controller
  // and start bus enumeration
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_USB | EFI_IOB_PC_ENABLE,
    UsbBusDev->DevicePath
    );

  //
  // Start USB Host Controller
  //
  UsbVirtualHcSetState (
    UsbBusDev,
    EfiUsbHcStateOperational
    );

  //
  // Create a timer to query root ports periodically
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RootHubEnumeration,
                  RootHubController,
                  &RootHubController->HubNotify
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           Controller,
           &mUsbBusProtocolGuid,
           &UsbBusDev->BusIdentify
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (UsbBusDev->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsb2HcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    gBS->FreePool (RootHubController);
    gBS->FreePool (RootHub);
    gBS->FreePool (UsbBusDev);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Before depending on the timer to check root ports periodically,
  // here we should check them immediately for the first time, or
  // there will be an interval between bus start and devices start.
  //
  gBS->SignalEvent (RootHubController->HubNotify);

  Status = gBS->SetTimer (
                  RootHubController->HubNotify,
                  TimerPeriodic,
                  BUSPOLLING_PERIOD
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           Controller,
           &mUsbBusProtocolGuid,
           &UsbBusDev->BusIdentify
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (UsbBusDev->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsb2HcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    } else {
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    gBS->CloseEvent (RootHubController->HubNotify);
    gBS->FreePool (RootHubController);
    gBS->FreePool (RootHub);
    gBS->FreePool (UsbBusDev);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

//
// Stop the bus controller
//
EFI_STATUS
EFIAPI
UsbBusControllerDriverStop (
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
    EFI_DEVICE_ERROR
    others

--*/
{
  EFI_STATUS                Status;
  USB_IO_DEVICE             *Root;
  USB_IO_CONTROLLER_DEVICE  *RootHubController;
  USB_BUS_CONTROLLER_DEVICE *UsbBusController;
  EFI_USB_BUS_PROTOCOL      *UsbIdentifier;
  UINT8                     Index2;
  USB_IO_CONTROLLER_DEVICE  *UsbController;
  USB_IO_DEVICE             *UsbIoDevice;
  USB_IO_CONTROLLER_DEVICE  *HubController;
  UINTN                     Index;
  EFI_USB_IO_PROTOCOL       *UsbIo;

  if (NumberOfChildren > 0) {

    for (Index = 0; Index < NumberOfChildren; Index++) {
      Status = gBS->OpenProtocol (
                      ChildHandleBuffer[Index],
                      &gEfiUsbIoProtocolGuid,
                      (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        //
        // We are here since the handle passed in does not support
        // UsbIo protocol. There are several reasons that will cause
        // this.
        // For combo device such as keyboard, it may have 2 devices
        // in one, namely, keyboard and mouse. If we deconfigure one
        // of them, the other will be freed at the same time. This will
        // cause the status error. But this is the correct behavior.
        // For hub device, if we deconfigure hub first, the other chile
        // device will be disconnected also, this will also provide us
        // a status error. Now we will only report EFI_SUCCESS since Uhc
        // driver will be disconnected at the second time.(pls see
        // CoreDisconnectController for details)
        //
        continue;
      }

      UsbController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (UsbIo);
      UsbIoDevice   = UsbController->UsbDevice;
      HubController = UsbController->Parent;
      UsbDeviceDeConfiguration (UsbIoDevice);
      for (Index2 = 0; Index2 < HubController->DownstreamPorts; Index2++) {
        if (HubController->Children[Index2] == UsbIoDevice) {
          HubController->Children[Index2] = NULL;
        }
      }
    }

    return EFI_SUCCESS;
  }
  //
  // Get the USB_BUS_CONTROLLER_DEVICE
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &mUsbBusProtocolGuid,
                  (VOID **) &UsbIdentifier,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  UsbBusController = USB_BUS_CONTROLLER_DEVICE_FROM_THIS (UsbIdentifier);

  //
  // Stop USB Host Controller
  //

  //
  // Report Status Code here since we will reset the host controller
  //
  ReportUsbStatusCode (
    UsbBusController,
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_USB | EFI_IOB_PC_RESET
    );

  UsbVirtualHcSetState (
    UsbBusController,
    EfiUsbHcStateHalt
    );

  //
  // Deconfiguration all its devices
  //
  Root              = UsbBusController->Root;
  RootHubController = Root->UsbController[0];

  gBS->CloseEvent (RootHubController->HubNotify);

  for (Index2 = 0; Index2 < RootHubController->DownstreamPorts; Index2++) {
    if (RootHubController->Children[Index2]) {
      UsbDeviceDeConfiguration (RootHubController->Children[Index2]);
      RootHubController->Children[Index2] = NULL;
    }
  }

  gBS->FreePool (RootHubController);
  gBS->FreePool (Root);

  //
  // Uninstall USB Bus Protocol
  //
  gBS->UninstallProtocolInterface (
        Controller,
        &mUsbBusProtocolGuid,
        &UsbBusController->BusIdentify
        );

  //
  // Close USB_HC_PROTOCOL & DEVICE_PATH_PROTOCOL
  // Opened by this Controller
  //
  if (UsbBusController->Hc2ProtocolSupported) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsb2HcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  } else {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbHcProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  gBS->FreePool (UsbBusController);

  return EFI_SUCCESS;
}
//
// USB Device Configuration
//
STATIC
EFI_STATUS
UsbDeviceConfiguration (
  IN USB_IO_CONTROLLER_DEVICE     *ParentHubController,
  IN EFI_HANDLE                   HostController,
  IN UINT8                        ParentPort,
  IN USB_IO_DEVICE                *UsbIoDevice
  )
/*++

  Routine Description:
    Configurate a new device attached to the usb bus

  Arguments:
    ParentHubController   -   Parent Hub which this device is connected.
    HostController        -   Host Controller handle
    ParentPort            -   Parent Hub port which this device is connected.
    UsbIoDevice           -   The device to be configured.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_OUT_OF_RESOURCES

--*/
{
  UINT8                     DevAddress;
  UINT8                     Index;
  EFI_STATUS                Result;
  UINT32                    Status;
  CHAR16                    *StrManufacturer;
  CHAR16                    *StrProduct;
  CHAR16                    *StrSerialNumber;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT8                     NumOfInterface;
  USB_IO_CONTROLLER_DEVICE  *FirstController;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;

  UsbBusDev = UsbIoDevice->BusController;

  UsbSetTransactionTranslator (
    ParentHubController,
    ParentPort,
    UsbIoDevice
    );

  //
  // Since a USB device must have at least on interface,
  // so create this instance first
  //
  FirstController                   = CreateUsbIoControllerDevice ();
  FirstController->UsbDevice        = UsbIoDevice;
  UsbIoDevice->UsbController[0]     = FirstController;
  FirstController->InterfaceNumber  = 0;
  FirstController->ParentPort       = ParentPort;
  FirstController->Parent           = ParentHubController;
  FirstController->HostController   = HostController;

  InitializeUsbIoInstance (FirstController);

  DEBUG ((gUSBDebugLevel, "Configuration Usb Device at 0x%x...\n", ParentPort));

  //
  // Ensure we used the correctly USB I/O instance
  //
  UsbIo = &FirstController->UsbIo;

  if (UsbIoDevice->DeviceSpeed != EFI_USB_SPEED_HIGH) {
    ParentPortReset (FirstController, FALSE, 0);
  }

  //
  // First retrieve the 1st 8 bytes of
  // in order to get the MaxPacketSize for Endpoint 0
  //
  for (Index = 0; Index < 3; Index++) {

    UsbIoDevice->DeviceDescriptor.MaxPacketSize0 = 8;

    gBS->Stall (100 * 1000);

    Result = UsbGetDescriptor (
              UsbIo,
              (USB_DT_DEVICE << 8),
              0,
              8,
              &UsbIoDevice->DeviceDescriptor,
              &Status
              );
    if (!EFI_ERROR (Result)) {
      DEBUG (
        (gUSBDebugLevel,
        "Get Device Descriptor Success, MaxPacketSize0 = 0x%x\n",
        UsbIoDevice->DeviceDescriptor.MaxPacketSize0)
        );
      break;
    }

  }

  if (Index == 3) {
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_READ_ERROR
      );
    DEBUG ((gUSBErrorLevel, "Get Device Descriptor Fail when configing\n"));
    gBS->FreePool (FirstController);
    return EFI_DEVICE_ERROR;
  }

  DevAddress = UsbAllocateAddress (UsbIoDevice->BusController->AddressPool);
  if (DevAddress == 0) {
    DEBUG ((gUSBErrorLevel, "Cannot allocate address\n"));
    gBS->FreePool (FirstController);
    return EFI_OUT_OF_RESOURCES;
  }

  Result = UsbSetDeviceAddress (UsbIo, DevAddress, &Status);

  if (EFI_ERROR (Result)) {
    DEBUG ((gUSBErrorLevel, "Set address error\n"));
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_WRITE_ERROR
      );

    UsbFreeAddress (
      DevAddress,
      UsbIoDevice->BusController->AddressPool
      );

    gBS->FreePool (FirstController);
    return EFI_DEVICE_ERROR;
  }

  UsbIoDevice->DeviceAddress = DevAddress;

  //
  // SetAddress Complete Time by Spec, Max 50ms
  //
  gBS->Stall (10 * 1000);

  //
  // Get the whole device descriptor
  //
  Result = UsbGetDescriptor (
            UsbIo,
            (USB_DT_DEVICE << 8),
            0,
            sizeof (EFI_USB_DEVICE_DESCRIPTOR),
            &UsbIoDevice->DeviceDescriptor,
            &Status
            );

  if (EFI_ERROR (Result)) {
    DEBUG ((gUSBErrorLevel, "Get whole Device Descriptor error\n"));
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_READ_ERROR
      );
    UsbFreeAddress (
      DevAddress,
      UsbIoDevice->BusController->AddressPool
      );

    gBS->FreePool (FirstController);
    return EFI_DEVICE_ERROR;
  }
  //
  // Get & parse all configurations for this device, including
  // all configuration descriptors, all interface descriptors, all
  // endpoint descriptors
  //
  Result = UsbGetAllConfigurations (UsbIoDevice);

  if (EFI_ERROR (Result)) {
    DEBUG ((gUSBErrorLevel, "Failed to get device configuration\n"));
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_READ_ERROR
      );
    UsbFreeAddress (
      DevAddress,
      UsbIoDevice->BusController->AddressPool
      );

    gBS->FreePool (FirstController);
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the 1st configuration value
  //
  Result = UsbSetDefaultConfiguration (UsbIoDevice);
  if (EFI_ERROR (Result)) {
    DEBUG ((gUSBErrorLevel, "Failed to set device configuration\n"));
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_WRITE_ERROR
      );
    UsbFreeAddress (
      DevAddress,
      UsbIoDevice->BusController->AddressPool
      );

    gBS->FreePool (FirstController);
    return EFI_DEVICE_ERROR;
  }

  UsbIoDevice->IsConfigured = TRUE;

  //
  // Get all string table if applicable
  //
  Result = UsbGetStringtable (UsbIoDevice);
  if (EFI_ERROR (Result)) {
    DEBUG ((gUSBDebugLevel, "Device doesn't support string table\n"));
  } else {

    StrManufacturer = NULL;
    UsbIo->UsbGetStringDescriptor (
            UsbIo,
            UsbIoDevice->LangID[0],
            (UsbIoDevice->DeviceDescriptor).StrManufacturer,
            &StrManufacturer
            );

    StrProduct = NULL;
    UsbIo->UsbGetStringDescriptor (
            UsbIo,
            UsbIoDevice->LangID[0],
            (UsbIoDevice->DeviceDescriptor).StrProduct,
            &StrProduct
            );

    StrSerialNumber = NULL;
    UsbIo->UsbGetStringDescriptor (
            UsbIo,
            UsbIoDevice->LangID[0],
            (UsbIoDevice->DeviceDescriptor).StrSerialNumber,
            &StrSerialNumber
            );

    if (StrManufacturer) {
      gBS->FreePool (StrManufacturer);
    }

    if (StrProduct) {
      gBS->FreePool (StrProduct);
    }

    if (StrSerialNumber) {
      gBS->FreePool (StrSerialNumber);
    }
  }
  //
  // Create USB_IO_CONTROLLER_DEVICE for
  // each detected interface
  //
  FirstController->CurrentConfigValue =
  UsbIoDevice->ActiveConfig->CongfigDescriptor.ConfigurationValue;

  NumOfInterface                      =
  UsbIoDevice->ActiveConfig->CongfigDescriptor.NumInterfaces;
  UsbIoDevice->NumOfControllers       = NumOfInterface;

  Result = InitUsbIoController (FirstController);
  if (EFI_ERROR (Result)) {
    ReportUsbStatusCode (
      UsbBusDev,
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_USB | EFI_IOB_EC_INTERFACE_ERROR
      );
    gBS->FreePool (FirstController);
    UsbIoDevice->UsbController[0] = NULL;
    return EFI_DEVICE_ERROR;
  }

  for (Index = 1; Index < NumOfInterface; Index++) {
    UsbIoController                     = CreateUsbIoControllerDevice ();
    UsbIoController->UsbDevice          = UsbIoDevice;
    UsbIoController->CurrentConfigValue =
    UsbIoDevice->ActiveConfig->CongfigDescriptor.ConfigurationValue;
    UsbIoController->InterfaceNumber    = Index;
    UsbIoDevice->UsbController[Index]   = UsbIoController;
    UsbIoController->ParentPort         = ParentPort;
    UsbIoController->Parent             = ParentHubController;
    UsbIoController->HostController     = HostController;

    //
    // First copy the USB_IO Protocol instance
    //
    CopyMem (
      &UsbIoController->UsbIo,
      UsbIo,
      sizeof (EFI_USB_IO_PROTOCOL)
      );

    Result = InitUsbIoController (UsbIoController);
    if (EFI_ERROR (Result)) {
      ReportUsbStatusCode (
        UsbBusDev,
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        EFI_IO_BUS_USB | EFI_IOB_EC_INTERFACE_ERROR
        );
      gBS->FreePool (UsbIoController);
      UsbIoDevice->UsbController[Index] = NULL;
    }
  }

  return EFI_SUCCESS;
}
//
// USB Device DeConfiguration
//
EFI_STATUS
UsbDeviceDeConfiguration (
  IN USB_IO_DEVICE     *UsbIoDevice
  )
/*++

  Routine Description:
    Remove Device, Device Handles, Uninstall Protocols.

  Arguments:
    UsbIoDevice     -   The device to be deconfigured.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbController;
  UINT8                     index;
  USB_IO_DEVICE             *ChildDevice;
  UINT8                     Index;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_STATUS                Status;

  //
  // Double check UsbIoDevice exists
  //
  if (UsbIoDevice == NULL) {
    return EFI_SUCCESS;
  }

  UsbUnsetTransactionTranslator (UsbIoDevice);

  for (index = 0; index < UsbIoDevice->NumOfControllers; index++) {
    //
    // Check if it is a hub, if so, de configuration all its
    // downstream ports
    //
    UsbController = UsbIoDevice->UsbController[index];

    //
    // Check the controller pointer
    //
    if (UsbController == NULL) {
      continue;
    }

    if (UsbController->IsUsbHub) {

      DEBUG ((gUSBDebugLevel, "Hub Deconfig, First Deconfig its downstream ports\n"));

      //
      // First Remove interrupt transfer request for the status
      // change port
      //
      UsbIo = &UsbController->UsbIo;
      UsbIo->UsbAsyncInterruptTransfer (
              UsbIo,
              UsbController->HubEndpointAddress,
              FALSE,
              0,
              0,
              NULL,
              NULL
              );

      if (NULL != UsbController->HubNotify) {
        gBS->CloseEvent (UsbController->HubNotify);
      }

      for (Index = 0; Index < UsbController->DownstreamPorts; Index++) {
        if (UsbController->Children[Index]) {
          ChildDevice = UsbController->Children[Index];
          UsbDeviceDeConfiguration (ChildDevice);
          UsbController->Children[Index] = NULL;
        }
      }
    }
    //
    // If the controller is managed by a device driver, we need to
    // disconnect them
    //
    if (UsbController->IsManagedByDriver) {
      gBS->DisconnectController (
            UsbController->Handle,
            NULL,
            NULL
            );
    }

    //
    // remove child handle reference to the USB_HC_PROTOCOL
    //
    if (UsbIoDevice->BusController->Hc2ProtocolSupported) {
      gBS->CloseProtocol (
            UsbController->HostController,
            &gEfiUsb2HcProtocolGuid,
            gUsbBusDriverBinding.DriverBindingHandle,
            UsbController->Handle
            );
    } else {
      gBS->CloseProtocol (
            UsbController->HostController,
            &gEfiUsbHcProtocolGuid,
            gUsbBusDriverBinding.DriverBindingHandle,
            UsbController->Handle
            );
    }
    //
    // Uninstall EFI_USB_IO_PROTOCOL & DEVICE_PATH_PROTOCOL
    // installed on this handle
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    UsbController->Handle,
                    &gEfiDevicePathProtocolGuid,
                    UsbController->DevicePath,
                    &gEfiUsbIoProtocolGuid,
                    &UsbController->UsbIo,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (UsbController->DevicePath != NULL) {
      gBS->FreePool (UsbController->DevicePath);
    }

    gBS->FreePool (UsbController);
    UsbIoDevice->UsbController[index] = NULL;
  }
  //
  // Free address for later use
  //
  UsbFreeAddress (
    UsbIoDevice->DeviceAddress,
    UsbIoDevice->BusController->AddressPool
    );

  //
  // Free all resouces allocated for all its configurations
  //
  UsbDestroyAllConfiguration (UsbIoDevice);

  if (UsbIoDevice) {
    gBS->FreePool (UsbIoDevice);
    UsbIoDevice = NULL;
  }

  return EFI_SUCCESS;
}
//
// After interrupt complete, this function will be called,
// This function need to be well-defined later
//
STATIC
EFI_STATUS
EFIAPI
OnHubInterruptComplete (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  )
/*++

  Routine Description:
    Whenever hub interrupt occurs, this routine will be called to check
    which event happens.

  Arguments:
    Data          -   Hub interrupt transfer data.
    DataLength    -   The length of the Data.
    Context       -   Hub Controller Device.
    Result        -   Hub interrupt transfer status.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_CONTROLLER_DEVICE  *HubController;
  UINT8                     Index;
  UINT8                     *ptr;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT32                    UsbResult;
  BOOLEAN                   Disconnected;
  EFI_STATUS                Status;

  HubController = (USB_IO_CONTROLLER_DEVICE *) Context;
  UsbIo         = &HubController->UsbIo;

  //
  // If something error in this interrupt transfer,
  //
  if (Result != EFI_USB_NOERROR) {
    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      UsbClearEndpointHalt (
        UsbIo,
        HubController->HubEndpointAddress,
        &UsbResult
        );
    }

    //
    // Delete & Submit this interrupt again
    //
    UsbIo->UsbAsyncInterruptTransfer (
            UsbIo,
            HubController->HubEndpointAddress,
            FALSE,
            0,
            0,
            NULL,
            NULL
            );

    //
    // try to detect if the hub itself was disconnected or not
    //
    Status = IsDeviceDisconnected (
              HubController,
              &Disconnected
              );

    if (!EFI_ERROR (Status) && Disconnected == TRUE) {
      DEBUG ((gUSBErrorLevel, "Hub is disconnected\n"));
      return EFI_DEVICE_ERROR;
    }
    //
    // Hub ports < 7
    //
    UsbIo->UsbAsyncInterruptTransfer (
            UsbIo,
            HubController->HubEndpointAddress,
            TRUE,
            100,
            1,
            OnHubInterruptComplete,
            HubController
            );

    return EFI_DEVICE_ERROR;
  }

  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Scan which port has status change
  // Bit 0 stands for hub itself, other bit stands for
  // the corresponding port
  //
  for (Index = 0; Index < DataLength * 8; Index++) {
    ptr = (UINT8 *) Data + Index / 8;
    if ((*ptr) & (1 << (Index & 0x7))) {
      HubController->StatusChangePort = Index;
      break;
    }
  }
  //
  // Signal hub notify event
  //
  gBS->SignalEvent (HubController->HubNotify);

  return EFI_SUCCESS;
}
//
// USB Root Hub Enumerator
//
STATIC
VOID
EFIAPI
RootHubEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++

  Routine Description:

    This is USB RootHub enumerator

  Arguments:

    Event   -   Indicating which event is signaled
    Context -  actually it is a USB_IO_DEVICE

  Returns:

    VOID

--*/
{
  USB_IO_CONTROLLER_DEVICE  *HubController;
  EFI_USB_PORT_STATUS       HubPortStatus;
  EFI_STATUS                Status;
  UINT8                     Index;
  USB_IO_DEVICE             *UsbIoDev;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  EFI_HANDLE                HostController;
  USB_IO_DEVICE             *OldUsbIoDevice;
  USB_IO_DEVICE             *NewDevice;
  USB_IO_CONTROLLER_DEVICE  *NewController;
  UINT8                     Index2;
  EFI_USB_IO_PROTOCOL       *UsbIo;

  HubController   = (USB_IO_CONTROLLER_DEVICE *) Context;
  HostController  = HubController->HostController;
  UsbBusDev       = HubController->UsbDevice->BusController;

  //
  // Root hub has the address 1
  //
  UsbIoDev = HubController->UsbDevice;

  for (Index = 0; Index < HubController->DownstreamPorts; Index++) {

    UsbVirtualHcGetRootHubPortStatus (
      UsbBusDev,
      Index,
      (EFI_USB_PORT_STATUS *) &HubPortStatus
      );

    if (!IsPortConnectChange (HubPortStatus.PortChangeStatus)) {
      continue;
    }
    //
    // Clear root hub status change status
    //
    UsbVirtualHcClearRootHubPortFeature (
      UsbBusDev,
      Index,
      EfiUsbPortConnectChange
      );

    gBS->Stall (100 * 1000);

    UsbVirtualHcGetRootHubPortStatus (
      UsbBusDev,
      Index,
      (EFI_USB_PORT_STATUS *) &HubPortStatus
      );

    if (IsPortConnect (HubPortStatus.PortStatus)) {

      //
      // There is something connected to this port
      //
      DEBUG ((gUSBDebugLevel, "Something connected to Root Hub at Port0x%x\n", Index));

      ReportUsbStatusCode (
        UsbBusDev,
        EFI_PROGRESS_CODE,
        EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG
        );
      //
      // if there is something physically detached, but still logically
      // attached...
      //
      OldUsbIoDevice = HubController->Children[Index];

      if (NULL != OldUsbIoDevice) {
        UsbDeviceDeConfiguration (OldUsbIoDevice);
        HubController->Children[Index] = NULL;
      }

      NewDevice = AllocateZeroPool (sizeof (USB_IO_DEVICE));
      if (NewDevice == NULL) {
        return ;
      }
      //
      // Initialize some fields by copying data from
      // its parents
      //
      NewDevice->DeviceDescriptor.MaxPacketSize0  = 8;
      NewDevice->BusController                    = UsbIoDev->BusController;

      //
      // Process of identify device speed
      //
      Status = IdentifyDeviceSpeed (
                 UsbBusDev,
                 NewDevice,
                 Index
                 );
      if (EFI_ERROR (Status)) {
        gBS->FreePool (NewDevice);
        continue;
      }

      //
      // Configure that device
      //
      Status = UsbDeviceConfiguration (
                HubController,
                HostController,
                Index,
                NewDevice
                );
      if (EFI_ERROR (Status)) {
        gBS->FreePool (NewDevice);
        return ;
      }
      //
      // Add this device to the usb bus tree
      //
      HubController->Children[Index] = NewDevice;

      for (Index2 = 0; Index2 < NewDevice->NumOfControllers; Index2++) {
        //
        // If this device is hub, add to the hub index
        //
        NewController = NewDevice->UsbController[Index2];

        Status = gBS->ConnectController (
                        NewController->Handle,
                        NULL,
                        NULL,
                        TRUE
                        );
        //
        // If connect success, we need to disconnect when
        // stop the controller, otherwise we need not call
        // gBS->DisconnectController ()
        // This is used by those usb devices we don't plan
        // to support. We can allocate
        // controller handles for them, but we don't have
        // device drivers to manage them.
        //
        NewController->IsManagedByDriver = (BOOLEAN) (!EFI_ERROR (Status));

        if (IsHub (NewController)) {

          NewController->IsUsbHub = TRUE;

          //
          // Configure Hub Controller
          //
          Status = DoHubConfig (NewController);
          if (EFI_ERROR (Status)) {
            continue;
          }
          //
          // Create an event to do hub enumeration
          //
          gBS->CreateEvent (
                EVT_NOTIFY_SIGNAL,
                TPL_CALLBACK,
                HubEnumeration,
                NewController,
                &NewController->HubNotify
                );

          //
          // Add request to do query hub status
          // change endpoint
          // Hub ports < 7
          //
          UsbIo = &NewController->UsbIo;
          UsbIo->UsbAsyncInterruptTransfer (
                  UsbIo,
                  NewController->HubEndpointAddress,
                  TRUE,
                  100,
                  1,
                  OnHubInterruptComplete,
                  NewController
                  );

        }
      }
    } else {
      //
      // Something disconnected from USB root hub
      //
      DEBUG ((gUSBDebugLevel, "Something disconnected from Root Hub at Port0x%x\n", Index));

      OldUsbIoDevice = HubController->Children[Index];

      UsbDeviceDeConfiguration (OldUsbIoDevice);

      HubController->Children[Index] = NULL;

      UsbVirtualHcClearRootHubPortFeature (
        UsbBusDev,
        Index,
        EfiUsbPortEnableChange
        );
    }
  }

  return ;
}
//
// USB Root Hub Enumerator
//
STATIC
VOID
EFIAPI
HubEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++

  Routine Description:

    This is Usb Hub enumerator

  Arguments:

    Event    -   Indicating which event is signaled
    Context  -  actually it is a USB_IO_DEVICE

  Returns:

    VOID

--*/
{
  USB_IO_CONTROLLER_DEVICE  *HubController;
  EFI_USB_PORT_STATUS       HubPortStatus;
  EFI_STATUS                Status;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  EFI_HANDLE                HostController;
  USB_IO_DEVICE             *OldUsbIoDevice;
  USB_IO_DEVICE             *NewDevice;
  USB_IO_CONTROLLER_DEVICE  *NewController;
  UINT8                     Index2;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT8                     StatusChangePort;
  UINT8                     Number;

  HubController   = (USB_IO_CONTROLLER_DEVICE *) Context;
  HostController  = HubController->HostController;
  UsbBusDev       = HubController->UsbDevice->BusController;

  //
  // Event from Hub, Get the hub controller handle
  //
  //
  // Get the status change endpoint
  //
  StatusChangePort = HubController->StatusChangePort;

  //
  // Clear HubController Status Change Bit
  //
  HubController->StatusChangePort = 0;

  if (StatusChangePort == 0) {
    //
    // Hub changes, we don't handle here
    //
    return ;
  }
  //
  // Check which event took place at that port
  //
  UsbIo = &HubController->UsbIo;
  Status = HubGetPortStatus (
            UsbIo,
            StatusChangePort,
            (UINT32 *) &HubPortStatus
            );

  if (EFI_ERROR (Status)) {
    return ;
  }
  //
  // Clear some change status
  //
  if (HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_ENABLE) {
    //
    // Clear Hub port enable change
    //
    DEBUG ((gUSBDebugLevel, "Port Enable Change\n"));
    HubClearPortFeature (
      UsbIo,
      StatusChangePort,
      EfiUsbPortEnableChange
      );

    HubGetPortStatus (
      UsbIo,
      StatusChangePort,
      (UINT32 *) &HubPortStatus
      );
  }

  if (HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_RESET) {
    //
    // Clear Hub reset change
    //
    DEBUG ((gUSBDebugLevel, "Port Reset Change\n"));
    HubClearPortFeature (
      UsbIo,
      StatusChangePort,
      EfiUsbPortResetChange
      );

    HubGetPortStatus (
      UsbIo,
      StatusChangePort,
      (UINT32 *) &HubPortStatus
      );
  }

  if (HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_OVERCURRENT) {
    //
    // Clear Hub overcurrent change
    //
    DEBUG ((gUSBDebugLevel, "Port Overcurrent Change\n"));
    HubClearPortFeature (
      UsbIo,
      StatusChangePort,
      EfiUsbPortOverCurrentChange
      );

    HubGetPortStatus (
      UsbIo,
      StatusChangePort,
      (UINT32 *) &HubPortStatus
      );
  }

  if (IsPortConnectChange (HubPortStatus.PortChangeStatus)) {
    //
    // First clear port connection change
    //
    DEBUG ((gUSBDebugLevel, "Port Connection Change\n"));
    HubClearPortFeature (
      UsbIo,
      StatusChangePort,
      EfiUsbPortConnectChange
      );

    HubGetPortStatus (
      UsbIo,
      StatusChangePort,
      (UINT32 *) &HubPortStatus
      );

    if (IsPortConnect (HubPortStatus.PortStatus)) {

      DEBUG ((gUSBDebugLevel, "New Device Connect on Hub port \n"));

      ReportUsbStatusCode (
        UsbBusDev,
        EFI_PROGRESS_CODE,
        EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG
        );

      //
      // if there is something physically detached, but still logically
      // attached...
      //
      OldUsbIoDevice = HubController->Children[StatusChangePort - 1];

      if (NULL != OldUsbIoDevice) {
        UsbDeviceDeConfiguration (OldUsbIoDevice);
        HubController->Children[StatusChangePort - 1] = NULL;
      }

      NewDevice = AllocateZeroPool (sizeof (USB_IO_DEVICE));
      if (NewDevice == NULL) {
        return ;
      }
      //
      // Initialize some fields
      //
      NewDevice->DeviceDescriptor.MaxPacketSize0  = 8;
      NewDevice->BusController                    = HubController->UsbDevice->BusController;

      //
      // There is something connected to this port,
      // reset that port
      //
      // Disable the enable bit in port status
      //
      HubClearPortFeature (
        UsbIo,
        StatusChangePort,
        EfiUsbPortEnable
        );

      gBS->Stall (50 * 1000);

      //
      // Wait for bit change
      //
      Number = 10;
      do {
        HubGetPortStatus (
          UsbIo,
          StatusChangePort,
          (UINT32 *) &HubPortStatus
          );
        gBS->Stall (10 * 1000);
        Number -= 1;
      } while ((HubPortStatus.PortStatus & USB_PORT_STAT_ENABLE) == 1 && Number > 0);

      if (Number == 0) {
        //
        // Cannot disable port, return error
        //
        DEBUG ((gUSBErrorLevel, "Disable Port Failed\n"));
        gBS->FreePool (NewDevice);
        return ;
      }

      HubSetPortFeature (
        UsbIo,
        StatusChangePort,
        EfiUsbPortReset
        );

      gBS->Stall (50 * 1000);

      //
      // Wait for port reset complete
      //
      Number = 10;
      do {
        HubGetPortStatus (
          UsbIo,
          StatusChangePort,
          (UINT32 *) &HubPortStatus
          );
        gBS->Stall (10 * 1000);
        Number -= 1;
      } while ((HubPortStatus.PortStatus & USB_PORT_STAT_RESET) == 1 && Number > 0);

      if (Number == 0) {
        //
        // Cannot reset port, return error
        //
        DEBUG ((gUSBErrorLevel, "Reset Port Failed\n"));
        gBS->FreePool (NewDevice);
        return ;
      }
      //
      // Check high speed or full speed device
      //
      if (HubPortStatus.PortStatus & USB_PORT_STAT_LOW_SPEED) {
        DEBUG ((gUSBDebugLevel, "Low Speed Device Attached to Hub\n"));
        NewDevice->DeviceSpeed = EFI_USB_SPEED_LOW;
      } else if (HubPortStatus.PortStatus & USB_PORT_STAT_HIGH_SPEED) {
        DEBUG ((gUSBDebugLevel, "High Speed Device Attached to Hub\n"));
        NewDevice->DeviceSpeed = EFI_USB_SPEED_HIGH;
      } else {
        DEBUG ((gUSBDebugLevel, "Full Speed Device Attached to Hub\n"));
        NewDevice->DeviceSpeed = EFI_USB_SPEED_FULL;
      }
      //
      // Configure that device
      //
      Status = UsbDeviceConfiguration (
                HubController,
                HostController,
                (UINT8) (StatusChangePort - 1),
                NewDevice
                );

      if (EFI_ERROR (Status)) {
        gBS->FreePool (NewDevice);
        return ;
      }
      //
      // Add this device to the usb bus tree
      // StatusChangePort is begin from 1,
      //
      HubController->Children[StatusChangePort - 1] = NewDevice;

      for (Index2 = 0; Index2 < NewDevice->NumOfControllers; Index2++) {
        //
        // If this device is hub, add to the hub index
        //
        NewController = NewDevice->UsbController[Index2];

        //
        // Connect the controller to the driver image
        //
        Status = gBS->ConnectController (
                        NewController->Handle,
                        NULL,
                        NULL,
                        TRUE
                        );
        //
        // If connect success, we need to disconnect when
        // stop the controller, otherwise we need not call
        // gBS->DisconnectController ()
        // This is used by those usb devices we don't plan
        // to support. We can allocate
        // controller handles for them, but we don't have
        // device drivers to manage them.
        //
        NewController->IsManagedByDriver = (BOOLEAN) (!EFI_ERROR (Status));

        //
        // If this device is hub, add to the hub index
        //
        if (IsHub (NewController)) {

          NewController->IsUsbHub = TRUE;

          //
          // Configure Hub
          //
          Status = DoHubConfig (NewController);

          if (EFI_ERROR (Status)) {
            continue;
          }
          //
          // Create an event to do hub enumeration
          //
          gBS->CreateEvent (
                EVT_NOTIFY_SIGNAL,
                TPL_CALLBACK,
                HubEnumeration,
                NewController,
                &NewController->HubNotify
                );

          //
          // Add request to do query hub status
          // change endpoint
          //
          UsbIo = &NewController->UsbIo;
          UsbIo->UsbAsyncInterruptTransfer (
                  UsbIo,
                  NewController->HubEndpointAddress,  // Hub endpoint address
                  TRUE,
                  100,
                  1,                                  // Hub ports < 7
                  OnHubInterruptComplete,
                  NewController
                  );
        }
      }
    } else {
      //
      // Something disconnected from USB hub
      //
      DEBUG ((gUSBDebugLevel, "Something Device Detached on Hub port\n"));

      OldUsbIoDevice = HubController->Children[StatusChangePort - 1];

      UsbDeviceDeConfiguration (OldUsbIoDevice);

      HubController->Children[StatusChangePort - 1] = NULL;

    }

    return ;
  }

  return ;
}

STATIC
USB_IO_CONTROLLER_DEVICE *
CreateUsbIoControllerDevice (
  VOID
  )
/*++

  Routine Description:
    Allocate a structure for USB_IO_CONTROLLER_DEVICE

  Arguments:
    N/A

  Returns:
    A pointer to a USB_IO_CONTROLLER_DEVICE structure,
    Or NULL.

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbIoControllerDev;

  //
  // Allocate USB_IO_CONTROLLER_DEVICE structure
  //
  UsbIoControllerDev  = NULL;
  UsbIoControllerDev  = AllocateZeroPool (sizeof (USB_IO_CONTROLLER_DEVICE));

  if (UsbIoControllerDev == NULL) {
    return NULL;
  }

  UsbIoControllerDev->Signature = USB_IO_CONTROLLER_SIGNATURE;

  return UsbIoControllerDev;
}

STATIC
EFI_STATUS
InitUsbIoController (
  IN USB_IO_CONTROLLER_DEVICE     *UsbIoController
  )
/*++

  Routine Description:
    Init and install EFI_USB_IO_PROTOCOL onto that controller.

  Arguments:
    UsbIoController   -   The Controller to be operated.

  Returns:
    EFI_SUCCESS
    Others

--*/
{
  USB_DEVICE_PATH           UsbNode;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_USB_HC_PROTOCOL       *UsbHcProtocol;
  EFI_USB2_HC_PROTOCOL      *Usb2HcProtocol;

  //
  // Build the child device path for each new USB_IO device
  //
  ZeroMem (&UsbNode, sizeof (UsbNode));
  UsbNode.Header.Type     = MESSAGING_DEVICE_PATH;
  UsbNode.Header.SubType  = MSG_USB_DP;
  SetDevicePathNodeLength (&UsbNode.Header, sizeof (UsbNode));
  UsbNode.InterfaceNumber     = UsbIoController->InterfaceNumber;
  UsbNode.ParentPortNumber    = UsbIoController->ParentPort;
  ParentDevicePath            = UsbIoController->Parent->DevicePath;

  UsbIoController->DevicePath =
  AppendDevicePathNode (ParentDevicePath, &UsbNode.Header);
  if (UsbIoController->DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &UsbIoController->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIoController->DevicePath,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIoController->UsbIo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (UsbIoController->UsbDevice->BusController->Hc2ProtocolSupported) {
    Status = gBS->OpenProtocol (
                    UsbIoController->HostController,
                    &gEfiUsb2HcProtocolGuid,
                    (VOID **)&Usb2HcProtocol,
                    gUsbBusDriverBinding.DriverBindingHandle,
                    UsbIoController->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  } else {
    Status = gBS->OpenProtocol (
                    UsbIoController->HostController,
                    &gEfiUsbHcProtocolGuid,
                    (VOID **)&UsbHcProtocol,
                    gUsbBusDriverBinding.DriverBindingHandle,
                    UsbIoController->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
  }

  return Status;
}

STATIC
EFI_STATUS
ParentPortReset (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN BOOLEAN                     ReConfigure,
  IN UINT8                       RetryTimes
  )
/*++

  Routine Description:
    Reset parent hub port to which this device is connected.

  Arguments:
    UsbIoController   - Indicating the Usb Controller Device.
    ReConfigure       - Do we need to reconfigure it.
    RetryTimes        - Retry Times when failed

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_DEVICE             *ParentIoDev;
  USB_IO_DEVICE             *UsbIoDev;
  USB_IO_CONTROLLER_DEVICE  *ParentController;
  UINT8                     HubPort;
  UINT32                    Status;
  EFI_STATUS                Result;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT8                     Address;

  ParentController  = UsbIoController->Parent;
  ParentIoDev       = ParentController->UsbDevice;
  UsbIoDev          = UsbIoController->UsbDevice;
  HubPort           = UsbIoController->ParentPort;

  gBS->Stall (100 * 1000);

  if (ParentIoDev->DeviceAddress == 1) {
    DEBUG ((gUSBDebugLevel, "Reset from Root Hub 0x%x\n", HubPort));
    ResetRootPort (ParentIoDev->BusController, HubPort, RetryTimes);
  } else {
    DEBUG ((gUSBDebugLevel, "Reset from Hub, Addr 0x%x\n", ParentIoDev->DeviceAddress));
    ResetHubPort (ParentController, (UINT8) (HubPort + 1));
  }
  //
  // If we only need port reset, just return
  //
  if (!ReConfigure) {
    return EFI_SUCCESS;
  }
  //
  // Re-config that USB device
  //
  UsbIo = &UsbIoController->UsbIo;

  //
  // Assign a unique address to this device
  //
  Address                 = UsbIoDev->DeviceAddress;
  UsbIoDev->DeviceAddress = 0;

  Result                  = UsbSetDeviceAddress (UsbIo, Address, &Status);
  UsbIoDev->DeviceAddress = Address;

  if (EFI_ERROR (Result)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set the device to the default configuration
  //
  Result = UsbSetDefaultConfiguration (UsbIoDev);
  if (EFI_ERROR (Result)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbPortReset (
  IN EFI_USB_IO_PROTOCOL      *This
  )
/*++

  Routine Description:
    Resets and reconfigures the USB controller.  This function will
    work for all USB devices except USB Hub Controllers.

  Arguments:
    This          -   Indicates the calling context.

  Returns:
    EFI_SUCCESS
    EFI_INVALID_PARAMETER
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_CONTROLLER_DEVICE  *UsbIoController;

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);

  if (IsHub (UsbIoController)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Since at this time, this device has already been configured,
  // it needs to be re-configured.
  //
  return ParentPortReset (UsbIoController, TRUE, 0);
}

STATIC
EFI_STATUS
ResetRootPort (
  IN USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN UINT8                     PortNum,
  IN UINT8                     RetryTimes
  )
/*++

  Routine Description:
    Reset Root Hub port.

  Arguments:
    UsbBusDev       - Bus controller of the device.
    PortNum         - The given port to be reset.
    RetryTimes      - RetryTimes when failed

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  EFI_STATUS          Status;
  EFI_USB_PORT_STATUS PortStatus;

  //
  // reset root port
  //
  Status = UsbVirtualHcSetRootHubPortFeature (
            UsbBusDev,
            PortNum,
            EfiUsbPortReset
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  gBS->Stall (50 * 1000);

  //
  // clear reset root port
  //
  Status = UsbVirtualHcClearRootHubPortFeature (
            UsbBusDev,
            PortNum,
            EfiUsbPortReset
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  gBS->Stall (1000);

  Status = UsbVirtualHcClearRootHubPortFeature (
            UsbBusDev,
            PortNum,
            EfiUsbPortConnectChange
            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  UsbVirtualHcGetRootHubPortStatus (
    UsbBusDev,
    PortNum,
    &PortStatus
    );
  if (PortStatus.PortStatus & USB_PORT_STAT_OWNER) {
    //
    // Set port enable
    //
    Status = UsbVirtualHcSetRootHubPortFeature (
              UsbBusDev,
              PortNum,
              EfiUsbPortEnable
              );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Status = UsbVirtualHcClearRootHubPortFeature (
              UsbBusDev,
              PortNum,
              EfiUsbPortEnableChange
              );
  }

  gBS->Stall ((1 + RetryTimes) * 50 * 1000);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ResetHubPort (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN UINT8                       PortIndex
  )
/*++

  Routine Description:
    Reset Hub port.

  Arguments:
    UsbIoController  -   The USB_IO_CONTROLLER_DEVICE instance.
    PortIndex        -   The given port to be reset.

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  EFI_USB_IO_PROTOCOL *UsbIo;
  EFI_USB_PORT_STATUS HubPortStatus;
  UINT8               Number;

  ASSERT (UsbIoController->IsUsbHub == TRUE);

  UsbIo = &UsbIoController->UsbIo;

  HubSetPortFeature (
    UsbIo,
    PortIndex,
    EfiUsbPortReset
    );

  gBS->Stall (10 * 1000);

  //
  // Wait for port reset complete
  //
  Number = 10;
  do {
    HubGetPortStatus (
      UsbIo,
      PortIndex,
      (UINT32 *) &HubPortStatus
      );
    gBS->Stall (10 * 100);
    Number -= 1;
  } while ((HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_RESET) == 0 && Number > 0);

  if (Number == 0) {
    //
    // Cannot reset port, return error
    //
    return EFI_DEVICE_ERROR;
  }

  gBS->Stall (1000);

  HubGetPortStatus (
    UsbIo,
    PortIndex,
    (UINT32 *) &HubPortStatus
    );
  //
  // reset port will cause some bits change, clear them
  //
  if (HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_ENABLE) {
    DEBUG ((gUSBDebugLevel, "Port Enable Change\n"));
    HubClearPortFeature (
      UsbIo,
      PortIndex,
      EfiUsbPortEnableChange
      );
  }

  if (HubPortStatus.PortChangeStatus & USB_PORT_STAT_C_RESET) {
    DEBUG ((gUSBDebugLevel, "Port Reset Change\n"));
    HubClearPortFeature (
      UsbIo,
      PortIndex,
      EfiUsbPortResetChange
      );
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ReportUsbStatusCode (
  IN USB_BUS_CONTROLLER_DEVICE     *UsbBusController,
  IN EFI_STATUS_CODE_TYPE          Type,
  IN EFI_STATUS_CODE_VALUE         Code
  )
/*++

Routine Description:

  report a error Status code of USB bus driver controller

 Arguments:
   UsbBusController - USB_BUS_CONTROLLER_DEVICE
   Type             - EFI_STATUS_CODE_TYPE
   Code             - EFI_STATUS_CODE_VALUE
 Returns:

  None

--*/
{
  return REPORT_STATUS_CODE_WITH_DEVICE_PATH (
          Type,
          Code,
          UsbBusController->DevicePath
          );
}

EFI_STATUS
IsDeviceDisconnected (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN OUT BOOLEAN                 *Disconnected
  )
/*++

  Routine Description:
    Reset if the device is disconencted or not

  Arguments:
    UsbIoController   -   Indicating the Usb Controller Device.
    Disconnected      -   Indicate whether the device is disconencted or not

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  USB_IO_DEVICE             *ParentIoDev;
  USB_IO_CONTROLLER_DEVICE  *ParentController;
  UINT8                     HubPort;
  EFI_STATUS                Status;
  EFI_USB_PORT_STATUS       PortStatus;

  ParentController  = UsbIoController->Parent;
  ParentIoDev       = ParentController->UsbDevice;
  HubPort           = UsbIoController->ParentPort;

  if (ParentIoDev->DeviceAddress == 1) {
    //
    // Connected to the root hub
    //
    UsbVirtualHcGetRootHubPortStatus (
      ParentIoDev->BusController,
      HubPort,
      &PortStatus
      );

  } else {
    Status = HubGetPortStatus (
              &ParentController->UsbIo,
              (UINT8) (HubPort + 1),
              (UINT32 *) &PortStatus
              );

    if (EFI_ERROR (Status)) {
      return IsDeviceDisconnected (ParentController, Disconnected);
    }
  }

  *Disconnected = FALSE;

  if (!IsPortConnect (PortStatus.PortStatus)) {
    *Disconnected = TRUE;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UsbSetTransactionTranslator (
  IN USB_IO_CONTROLLER_DEVICE     *ParentHubController,
  IN UINT8                        ParentPort,
  IN OUT USB_IO_DEVICE            *Device
  )
/*++

  Routine Description:

    Set Transaction Translator parameter

  Arguments:

    ParentHubController  - Controller structure of the parent Hub device
    ParentPort           - Number of parent port
    Device               - Structure of the device

  Returns:

    EFI_SUCCESS            Success
    EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  USB_IO_CONTROLLER_DEVICE  *AncestorHubController;

  AncestorHubController = ParentHubController;
  Device->Translator    = NULL;

  if (EFI_USB_SPEED_HIGH == Device->DeviceSpeed) {
    return EFI_SUCCESS;
  }

  do {
    if (EFI_USB_SPEED_HIGH == AncestorHubController->UsbDevice->DeviceSpeed) {
      break;
    }

    if (NULL == AncestorHubController->Parent) {
      return EFI_SUCCESS;
    }

    AncestorHubController = AncestorHubController->Parent;
  } while (1);

  Device->Translator = AllocatePool (sizeof (EFI_USB2_HC_TRANSACTION_TRANSLATOR));
  if (NULL == Device->Translator) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Translator->TranslatorHubAddress  = AncestorHubController->UsbDevice->DeviceAddress;
  Device->Translator->TranslatorPortNumber  = ParentPort;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UsbUnsetTransactionTranslator (
  USB_IO_DEVICE *Device
  )
/*++

  Routine Description:

    Unset Transaction Translator parameter

  Arguments:

    Device - Structure of the device

  Returns:

    EFI_SUCCESS    Success

--*/
{
  if (Device->Translator) {
    gBS->FreePool (Device->Translator);
    Device->Translator = NULL;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
IdentifyDeviceSpeed (
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  USB_IO_DEVICE             *NewDevice,
  UINT8                     Index
  )
/*++

  Routine Description:

    Identify speed of USB device

  Arguments:

    UsbBusDev  - UsbBus controller structure of the device
    NewDevice  - Devcie controller structure
    Index      - Number of the port

  Returns:

    EFI_SUCCESS        Success
    EFI_NOT_FOUND      Device release to CHC or can't be found

--*/
{
  EFI_STATUS             Status;
  EFI_USB_PORT_STATUS    HubPortStatus;

  UsbVirtualHcGetRootHubPortStatus (
    UsbBusDev,
    Index,
    (EFI_USB_PORT_STATUS *) &HubPortStatus
    );

  //
  // Check device device
  //
  if (!(HubPortStatus.PortStatus & USB_PORT_STAT_OWNER)) {
    //
    // EHC Port Owner
    //
    if (HubPortStatus.PortStatus & USB_PORT_STAT_HIGH_SPEED) {
      DEBUG ((gUSBDebugLevel, "High Speed Device attached to EHC\n"));
      NewDevice->DeviceSpeed = EFI_USB_SPEED_HIGH;
    } else {
      Status = ReleasePortToCHC (UsbBusDev, Index);
      if (EFI_ERROR (Status)) {
        DEBUG ((gUSBErrorLevel, "Fail to release port to CHC\n"));
      } else {
        DEBUG ((gUSBDebugLevel, "Success to release port to CHC\n"));
      }
      return EFI_DEVICE_ERROR;
    }
  } else {
    //
    // CHC Port Owner
    //
    if (HubPortStatus.PortStatus & USB_PORT_STAT_LOW_SPEED) {
      DEBUG ((gUSBDebugLevel, "Low Speed Device attached to CHC\n"));
      NewDevice->DeviceSpeed = EFI_USB_SPEED_LOW;
    } else {
      DEBUG ((gUSBDebugLevel, "FULL Speed Device attached to CHC\n"));
      NewDevice->DeviceSpeed = EFI_USB_SPEED_FULL;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ReleasePortToCHC (
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  UINT8                     PortNum
  )
/*++

  Routine Description:

    Set bit to release the port owner to CHC

  Arguments:

    UsbBusDev  - UsbBus controller structure of the device
    PortNum    - Number of the port

  Returns:

    EFI_SUCCESS        Success
    EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS  Status;

  Status = UsbVirtualHcSetRootHubPortFeature (
            UsbBusDev,
            PortNum,
            EfiUsbPortOwner
            );

  gBS->Stall (100 * 1000);

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcGetCapability (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  OUT UINT8                     *MaxSpeed,
  OUT UINT8                     *PortNumber,
  OUT UINT8                     *Is64BitCapable
  )
/*++

  Routine Description:

    Virtual interface to Retrieves the capablility of root hub ports
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev       - A pointer to bus controller of the device.
    MaxSpeed        - A pointer to the number of the host controller.
    PortNumber      - A pointer to the number of the root hub ports.
    Is64BitCapable  - A pointer to the flag for whether controller supports
                      64-bit memory addressing.

  Returns:

    EFI_SUCCESS
          The host controller capability were retrieved successfully.
    EFI_INVALID_PARAMETER
          MaxSpeed or PortNumber or Is64BitCapable is NULL.
    EFI_DEVICE_ERROR
          An error was encountered while attempting to retrieve the capabilities.

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->GetCapability (
                                          UsbBusDev->Usb2HCInterface,
                                          MaxSpeed,
                                          PortNumber,
                                          Is64BitCapable
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->GetRootHubPortNumber (
                                          UsbBusDev->UsbHCInterface,
                                          PortNumber
                                          );
    *MaxSpeed       = EFI_USB_SPEED_FULL;
    *Is64BitCapable = (UINT8) FALSE;
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcReset (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN UINT16                     Attributes
  )
/*++

  Routine Description:

    Virtual interface to provides software reset for the USB host controller
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev   - A pointer to bus controller of the device.
    Attributes  - A bit mask of the reset operation to perform.
                See below for a list of the supported bit mask values.

  #define EFI_USB_HC_RESET_GLOBAL  0x0001               // Hc2 and Hc
  #define EFI_USB_HC_RESET_HOST_CONTROLLER  0x0002      // Hc2 and Hc
  #define EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG  0x0004    // Hc2
  #define EFI_USB_HC_RESET_HOST_WITH_DEBUG  0x0008      // Hc2

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
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->Reset (
                                          UsbBusDev->Usb2HCInterface,
                                          EFI_USB_HC_RESET_GLOBAL
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->Reset (
                                          UsbBusDev->UsbHCInterface,
                                          EFI_USB_HC_RESET_GLOBAL
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcGetState (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  OUT EFI_USB_HC_STATE          *State
  )
/*++

  Routine Description:

    Virtual interface to retrieves current state of the USB host controller
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev - A pointer to bus controller of the device.
    State     - A pointer to the EFI_USB_HC_STATE data structure that
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

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->GetState (
                                          UsbBusDev->Usb2HCInterface,
                                          State
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->GetState (
                                          UsbBusDev->UsbHCInterface,
                                          State
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcSetState (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN EFI_USB_HC_STATE           State
  )
/*++

  Routine Description:

    Virtual interface to sets the USB host controller to a specific state
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev   - A pointer to bus controller of the device.
    State       - Indicates the state of the host controller that will be set.

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
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->SetState (
                                          UsbBusDev->Usb2HCInterface,
                                          State
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->SetState (
                                          UsbBusDev->UsbHCInterface,
                                          State
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcGetRootHubPortStatus (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  OUT EFI_USB_PORT_STATUS       *PortStatus
  )
/*++

  Routine Description:

    Virtual interface to retrieves the current status of a USB root hub port
    both for Hc2 and Hc protocol.

  Arguments:

    UsbBusDev   - A pointer to bus controller of the device.
    PortNumber  - Specifies the root hub port from which the status
                is to be retrieved.  This value is zero-based. For example,
                if a root hub has two ports, then the first port is numbered 0,
                and the second port is numbered 1.
    PortStatus  - A pointer to the current port status bits and
                port status change bits.

  Returns:

    EFI_SUCCESS  The status of the USB root hub port specified by PortNumber
                 was returned in PortStatus.
    EFI_INVALID_PARAMETER PortNumber is invalid.
    EFI_DEVICE_ERROR      Can't read register

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->GetRootHubPortStatus (
                                          UsbBusDev->Usb2HCInterface,
                                          PortNumber,
                                          PortStatus
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->GetRootHubPortStatus (
                                          UsbBusDev->UsbHCInterface,
                                          PortNumber,
                                          PortStatus
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcSetRootHubPortFeature (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  IN  EFI_USB_PORT_FEATURE      PortFeature
  )
/*++

  Routine Description:
    Virual interface to sets a feature for the specified root hub port
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev   - A pointer to bus controller of the device.
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

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->SetRootHubPortFeature (
                                          UsbBusDev->Usb2HCInterface,
                                          PortNumber,
                                          PortFeature
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->SetRootHubPortFeature (
                                          UsbBusDev->UsbHCInterface,
                                          PortNumber,
                                          PortFeature
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcClearRootHubPortFeature (
  IN  USB_BUS_CONTROLLER_DEVICE *UsbBusDev,
  IN  UINT8                     PortNumber,
  IN  EFI_USB_PORT_FEATURE      PortFeature
  )
/*++

  Routine Description:

    Virtual interface to clears a feature for the specified root hub port
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev   - A pointer to bus controller of the device.
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

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->ClearRootHubPortFeature (
                                          UsbBusDev->Usb2HCInterface,
                                          PortNumber,
                                          PortFeature
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->ClearRootHubPortFeature (
                                          UsbBusDev->UsbHCInterface,
                                          PortNumber,
                                          PortFeature
                                          );
  }

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcControlTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE            *UsbBusDev,
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

    Virtual interface to submits control transfer to a target USB device
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev     - A pointer to bus controller of the device.
    DeviceAddress - Represents the address of the target device on the USB,
                  which is assigned during USB enumeration.
    DeviceSpeed   - Indicates target device speed.
    MaximumPacketLength - Indicates the maximum packet size that the
                        default control transfer endpoint is capable of
                        sending or receiving.
    Request       - A pointer to the USB device request that will be sent
                  to the USB device.
    TransferDirection - Specifies the data direction for the transfer.
                      There are three values available, DataIn, DataOut
                      and NoData.
    Data          - A pointer to the buffer of data that will be transmitted
                  to USB device or received from USB device.
    DataLength    - Indicates the size, in bytes, of the data buffer
                  specified by Data.
    TimeOut       - Indicates the maximum time, in microseconds,
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
  EFI_STATUS  Status;
  BOOLEAN     IsSlowDevice;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->ControlTransfer (
                                          UsbBusDev->Usb2HCInterface,
                                          DeviceAddress,
                                          DeviceSpeed,
                                          MaximumPacketLength,
                                          Request,
                                          TransferDirection,
                                          Data,
                                          DataLength,
                                          TimeOut,
                                          Translator,
                                          TransferResult
                                          );
  } else {
    IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);
    Status = UsbBusDev->UsbHCInterface->ControlTransfer (
                                          UsbBusDev->UsbHCInterface,
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

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcBulkTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE           *UsbBusDev,
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

    Virtual interface to submits bulk transfer to a bulk endpoint of a USB device
    both for Hc2 and Hc protocol.

  Arguments:

    UsbBusDev         - A pointer to bus controller of the device.
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
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->BulkTransfer (
                                          UsbBusDev->Usb2HCInterface,
                                          DeviceAddress,
                                          EndPointAddress,
                                          DeviceSpeed,
                                          MaximumPacketLength,
                                          DataBuffersNumber,
                                          Data,
                                          DataLength,
                                          DataToggle,
                                          TimeOut,
                                          Translator,
                                          TransferResult
                                          );
  } else {
    Status = UsbBusDev->UsbHCInterface->BulkTransfer (
                                          UsbBusDev->UsbHCInterface,
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

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcAsyncInterruptTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             * UsbBusDev,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN  BOOLEAN                               IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN  UINTN                                 PollingInterval,
  IN  UINTN                                 DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR * Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK       CallBackFunction,
  IN  VOID                                  *Context OPTIONAL
  )
/*++

  Routine Description:

    Virtual interface to submits an asynchronous interrupt transfer to an
    interrupt endpoint of a USB device for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev       - A pointer to bus controller of the device.
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
  EFI_STATUS  Status;
  BOOLEAN     IsSlowDevice;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->AsyncInterruptTransfer (
                                          UsbBusDev->Usb2HCInterface,
                                          DeviceAddress,
                                          EndPointAddress,
                                          DeviceSpeed,
                                          MaximumPacketLength,
                                          IsNewTransfer,
                                          DataToggle,
                                          PollingInterval,
                                          DataLength,
                                          Translator,
                                          CallBackFunction,
                                          Context
                                          );
  } else {
    IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);
    Status = UsbBusDev->UsbHCInterface->AsyncInterruptTransfer (
                                          UsbBusDev->UsbHCInterface,
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

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcSyncInterruptTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             *UsbBusDev,
  IN  UINT8                                 DeviceAddress,
  IN  UINT8                                 EndPointAddress,
  IN  UINT8                                 DeviceSpeed,
  IN  UINTN                                 MaximumPacketLength,
  IN OUT VOID                               *Data,
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN  UINTN                                 TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT UINT32                                *TransferResult
  )
/*++

  Routine Description:

    Vitual interface to submits synchronous interrupt transfer to an interrupt endpoint
    of a USB device for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev       - A pointer to bus controller of the device.
    DeviceAddress   - Represents the address of the target device on the USB,
                    which is assigned during USB enumeration.
    EndPointAddress   - The combination of an endpoint number and an endpoint
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
  EFI_STATUS  Status;
  BOOLEAN     IsSlowDevice;

  Status = EFI_SUCCESS;

  if (UsbBusDev->Hc2ProtocolSupported) {
    Status = UsbBusDev->Usb2HCInterface->SyncInterruptTransfer (
                                          UsbBusDev->Usb2HCInterface,
                                          DeviceAddress,
                                          EndPointAddress,
                                          DeviceSpeed,
                                          MaximumPacketLength,
                                          Data,
                                          DataLength,
                                          DataToggle,
                                          TimeOut,
                                          Translator,
                                          TransferResult
                                          );
  } else {
    IsSlowDevice = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);
    Status = UsbBusDev->UsbHCInterface->SyncInterruptTransfer (
                                          UsbBusDev->UsbHCInterface,
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

  return Status;
}

EFI_STATUS
EFIAPI
UsbVirtualHcIsochronousTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE             *UsbBusDev,
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

    Virtual interface to submits isochronous transfer to a target USB device
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev        - A pointer to bus controller of the device.
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
UsbVirtualHcAsyncIsochronousTransfer (
  IN  USB_BUS_CONTROLLER_DEVICE           *UsbBusDev,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaximumPacketLength,
  IN  UINT8                               DataBuffersNumber,
  IN OUT VOID                             *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN  VOID                                *Context
  )
/*++

  Routine Description:

    Vitual interface to submits Async isochronous transfer to a target USB device
    for both Hc2 and Hc protocol.

  Arguments:

    UsbBusDev           - A pointer to bus controller of the device.
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
    DataLength          - Indicates the size, in bytes, of the data buffer
                        specified by Data.
    Translator          - A pointr to the transaction translator data.
    IsochronousCallBack - When the transfer complete, the call back function will be called
    Context             - Pass to the call back function as parameter

  Returns:

    EFI_UNSUPPORTED

--*/
{
  return EFI_UNSUPPORTED;
}
