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

//#ifdef EFI_DEBUG
UINTN                       gUSBDebugLevel  = EFI_D_ERROR;
UINTN                       gUSBErrorLevel  = EFI_D_ERROR;
//#endif
//
// The UsbBusProtocol is just used to locate USB_BUS_CONTROLLER
// structure in the UsbBusDriverControllerDriverStop(). Then we can
// Close all opened protocols and release this structure.
//
STATIC EFI_GUID             mUsbBusProtocolGuid = EFI_USB_BUS_PROTOCOL_GUID;



//
// EFI_DRIVER_BINDING_PROTOCOL Protocol Interface
//
EFI_STATUS
EFIAPI
UsbBusControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UsbBusControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  );

EFI_DRIVER_BINDING_PROTOCOL gUsbBusDriverBinding = {
  UsbBusControllerDriverSupported,
  UsbBusControllerDriverStart,
  UsbBusControllerDriverStop,
  0x10,
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

//
// Supported function
//
VOID
InitializeUsbIoInstance (
  IN USB_IO_CONTROLLER_DEVICE     *UsbIoController
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
EFIAPI
UsbEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

EFI_STATUS
ResetRootPort (
  IN EFI_USB_HC_PROTOCOL     *UsbHCInterface,
  IN UINT8                   PortNum,
  IN UINT8                   RetryTimes
  );

EFI_STATUS
ResetHubPort (
  IN USB_IO_CONTROLLER_DEVICE    *UsbIoController,
  IN UINT8                       PortIndex
  );

EFI_STATUS
ClearRootPortConnectionChangeStatus (
  IN UINT8                   PortNum,
  IN EFI_USB_HC_PROTOCOL     *UsbHCInterface
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
{
  UINT8 ByteIndex;
  UINT8 BitIndex;

  for (ByteIndex = 0; ByteIndex < 16; ByteIndex++) {
    for (BitIndex = 0; BitIndex < 8; BitIndex++) {
      if ((AddressPool[ByteIndex] & (1 << BitIndex)) == 0) {
        //
        // Found one, covert to address, and mark it use
        //
        AddressPool[ByteIndex] |= (1 << BitIndex);
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
{
  UINT8 WhichByte;
  UINT8 WhichBit;
  //
  // Locate the position
  //
  WhichByte = (UINT8) (DevAddress / 8);
  WhichBit  = (UINT8) (DevAddress & 0x7);

  AddressPool[WhichByte] &= (~(1 << WhichBit));
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
    Controller         - Handle of device to test
    RemainingDevicePath - Not used

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_UNSUPPORTED     - This driver does not support this device.

--*/
{
  EFI_STATUS  OpenStatus;

  //
  // Check whether USB Host Controller Protocol is already
  // installed on this handle. If it is installed, we can start
  // USB Bus Driver now.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbHcProtocolGuid,
                      NULL,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                      );

  if (EFI_ERROR (OpenStatus) && (OpenStatus != EFI_ALREADY_STARTED)) {
    return EFI_UNSUPPORTED;
  }

  return OpenStatus;
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
    EFI_UNSUPPORTED     - This driver does not support this device.
    EFI_DEVICE_ERROR    - This driver cannot be started due to device
                          Error
    EFI_OUT_OF_RESOURCES- Can't allocate memory resources
    EFI_ALREADY_STARTED - This driver has been started

--*/
{
  EFI_STATUS                Status;
  EFI_STATUS                OpenStatus;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  USB_IO_DEVICE             *RootHub;
  USB_IO_CONTROLLER_DEVICE  *RootHubController;
  EFI_USB_HC_PROTOCOL       *UsbHCInterface;

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
    return EFI_UNSUPPORTED;
  }
  //
  // Locate the Host Controller Interface
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbHcProtocolGuid,
                      (VOID **) &UsbHCInterface,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

  if (EFI_ERROR (OpenStatus) && (OpenStatus != EFI_ALREADY_STARTED)) {
    
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
    return EFI_UNSUPPORTED;
  }

  if (OpenStatus == EFI_ALREADY_STARTED) {
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    gBS->FreePool (UsbBusDev);
    return EFI_ALREADY_STARTED;
  }

  UsbBusDev->UsbHCInterface = UsbHCInterface;

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
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
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
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
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
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    gBS->FreePool (UsbBusDev);
    gBS->FreePool (RootHub);
    return EFI_OUT_OF_RESOURCES;
  }

  UsbHCInterface->GetRootHubPortNumber (
                    UsbHCInterface,
                    &RootHubController->DownstreamPorts
                    );
  RootHubController->UsbDevice      = RootHub;
  RootHubController->IsUsbHub       = TRUE;
  RootHubController->DevicePath     = UsbBusDev->DevicePath;
  RootHubController->HostController = Controller;

  RootHub->NumOfControllers         = 1;
  RootHub->UsbController[0]         = RootHubController;

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
  UsbHCInterface->Reset (
                    UsbHCInterface,
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
  UsbHCInterface->SetState (
                    UsbHCInterface,
                    EfiUsbHcStateOperational
                    );

  //
  // Create a timer to query root ports periodically
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  UsbEnumeration,
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

    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->FreePool (RootHubController);
    gBS->FreePool (RootHub);
    gBS->FreePool (UsbBusDev);
    return EFI_UNSUPPORTED;
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

    gBS->CloseProtocol (
           Controller,
           &gEfiUsbHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseEvent (RootHubController->HubNotify);
    gBS->FreePool (RootHubController);
    gBS->FreePool (RootHub);
    gBS->FreePool (UsbBusDev);
    return EFI_UNSUPPORTED;
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
  EFI_USB_HC_PROTOCOL       *UsbHCInterface;
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
  UsbHCInterface = UsbBusController->UsbHCInterface;

  //
  // Report Status Code here since we will reset the host controller
  //
  ReportUsbStatusCode (
    UsbBusController,
    EFI_PROGRESS_CODE,
    EFI_IO_BUS_USB | EFI_IOB_PC_RESET
    );

  UsbHCInterface->SetState (
                    UsbHCInterface,
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
  gBS->CloseProtocol (
        Controller,
        &gEfiUsbHcProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

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

  //
  // First retrieve the 1st 8 bytes of
  // in order to get the MaxPacketSize for Endpoint 0
  //
  for (Index = 0; Index < 3; Index++) {

    UsbIoDevice->DeviceDescriptor.MaxPacketSize0 = 8;

    ParentPortReset (FirstController, FALSE, Index);

    Result = UsbGetDescriptor (
              UsbIo,
              (USB_DT_DEVICE << 8),
              0,
              8,
              &UsbIoDevice->DeviceDescriptor,
              &Status
              );
    if (!EFI_ERROR (Result)) {
      DEBUG ((gUSBDebugLevel,
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

  DEBUG ((gUSBDebugLevel, "Enter Usb Device Deconfiguration\n"));

  //
  // Double check UsbIoDevice exists
  //
  if (UsbIoDevice == NULL) {
    return EFI_SUCCESS;
  }

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
    gBS->CloseProtocol (
          UsbController->HostController,
          &gEfiUsbHcProtocolGuid,
          gUsbBusDriverBinding.DriverBindingHandle,
          UsbController->Handle
          );

    //
    // Uninstall EFI_USB_IO_PROTOCOL & DEVICE_PATH_PROTOCOL
    // installed on this handle
    //
    gBS->UninstallMultipleProtocolInterfaces (
          UsbController->Handle,
          &gEfiDevicePathProtocolGuid,
          UsbController->DevicePath,
          &gEfiUsbIoProtocolGuid,
          &UsbController->UsbIo,
          NULL
          );

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
UsbEnumeration (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++

  Routine Description:
    This is USB enumerator

  Arguments:
    Event   -   Indicating which event is signaled
    Context -  actually it is a USB_IO_DEVICE

  Returns:
    EFI_SUCCESS
    Others

--*/
{
  USB_IO_CONTROLLER_DEVICE  *HubController;
  EFI_USB_PORT_STATUS       HubPortStatus;
  EFI_STATUS                Status;
  UINT8                     Index;
  EFI_USB_HC_PROTOCOL       *UsbHCInterface;
  USB_IO_DEVICE             *UsbIoDev;
  USB_BUS_CONTROLLER_DEVICE *UsbBusDev;
  EFI_HANDLE                HostController;
  USB_IO_DEVICE             *OldUsbIoDevice;
  USB_IO_DEVICE             *NewDevice;
  USB_IO_CONTROLLER_DEVICE  *NewController;
  UINT8                     Index2;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT8                     StatusChangePort;

  HubController   = (USB_IO_CONTROLLER_DEVICE *) Context;
  HostController  = HubController->HostController;
  UsbBusDev       = HubController->UsbDevice->BusController;

  if (HubController->UsbDevice->DeviceAddress == 1) {
    //
    // Root hub has the address 1
    //
    UsbIoDev        = HubController->UsbDevice;
    UsbHCInterface  = UsbIoDev->BusController->UsbHCInterface;

    for (Index = 0; Index < HubController->DownstreamPorts; Index++) {
      UsbHCInterface->GetRootHubPortStatus (
                        UsbHCInterface,
                        Index,
                        (EFI_USB_PORT_STATUS *) &HubPortStatus
                        );

      if (!IsPortConnectChange (HubPortStatus.PortChangeStatus)) {
        continue;
      }
      //
      // Clear root hub status change status
      //
      ClearRootPortConnectionChangeStatus (
        Index,
        UsbHCInterface
        );

      gBS->Stall (100 * 1000);

      UsbHCInterface->GetRootHubPortStatus (
                        UsbHCInterface,
                        Index,
                        (EFI_USB_PORT_STATUS *) &HubPortStatus
                        );

      if (IsPortConnect (HubPortStatus.PortStatus)) {
        
        //
        // There is something connected to this port
        //
        DEBUG ((gUSBDebugLevel, "Something attached from Root Hub in 0x%x\n", Index));

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
        NewDevice->IsSlowDevice = IsPortLowSpeedDeviceAttached (HubPortStatus.PortStatus);

        DEBUG ((gUSBDebugLevel, "DeviceSpeed 0x%x\n", NewDevice->IsSlowDevice));

        NewDevice->BusController = UsbIoDev->BusController;

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
                  EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  UsbEnumeration,
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
        DEBUG ((gUSBDebugLevel, "Something deteached from Root Hub\n"));

        OldUsbIoDevice = HubController->Children[Index];

        UsbDeviceDeConfiguration (OldUsbIoDevice);

        HubController->Children[Index] = NULL;

        UsbHCInterface->ClearRootHubPortFeature (
                          UsbHCInterface,
                          Index,
                          EfiUsbPortEnableChange
                          );

        UsbHCInterface->GetRootHubPortStatus (
                          UsbHCInterface,
                          Index,
                          (EFI_USB_PORT_STATUS *) &HubPortStatus
                          );

      }
    }

    return ;
  } else {
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

        ResetHubPort (HubController, StatusChangePort);

        HubGetPortStatus (
          UsbIo,
          StatusChangePort,
          (UINT32 *) &HubPortStatus
          );

        //
        // Initialize some fields
        //
        NewDevice->IsSlowDevice   = IsPortLowSpeedDeviceAttached (HubPortStatus.PortStatus);

        NewDevice->BusController  = HubController->UsbDevice->BusController;

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
                  EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  UsbEnumeration,
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
}
//
// Clear port connection change status over a given root hub port
//
EFI_STATUS
ClearRootPortConnectionChangeStatus (
  UINT8                   PortNum,
  EFI_USB_HC_PROTOCOL     *UsbHCInterface
  )
/*++

  Routine Description:
    Clear port connection change status over a given root hub port

  Arguments:
    PortNum         -   The given port.
    UsbHCInterface  -   The EFI_USB_HC_PROTOCOL instance.

  Returns:
     EFI_SUCCESS

--*/
{
  EFI_STATUS  Status;
  Status = UsbHCInterface->ClearRootHubPortFeature (
                            UsbHCInterface,
                            PortNum,
                            EfiUsbPortConnectChange
                            );
  return Status;
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

  Status = gBS->OpenProtocol (
                  UsbIoController->HostController,
                  &gEfiUsbHcProtocolGuid,
                  (VOID **) &UsbHcProtocol,
                  gUsbBusDriverBinding.DriverBindingHandle,
                  UsbIoController->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

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
    Reconfigure       - Do we need to reconfigure it.
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
    ResetRootPort (ParentIoDev->BusController->UsbHCInterface, HubPort, RetryTimes);
  } else {
    DEBUG ((gUSBDebugLevel, "Reset from Hub, Addr 0x%x\n", ParentIoDev->DeviceAddress));
    ResetHubPort (ParentController, HubPort + 1);
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
  EFI_STATUS                Status;

  UsbIoController = USB_IO_CONTROLLER_DEVICE_FROM_USB_IO_THIS (This);

  //
  // Since at this time, this device has already been configured,
  // it needs to be re-configured.
  //
  Status = ParentPortReset (UsbIoController, TRUE, 0);

  return Status;
}

EFI_STATUS
ResetRootPort (
  IN EFI_USB_HC_PROTOCOL     *UsbHCInterface,
  IN UINT8                   PortNum,
  IN UINT8                   RetryTimes
  )
/*++

  Routine Description:
    Reset Root Hub port.

  Arguments:
    UsbHCInterface  -   The EFI_USB_HC_PROTOCOL instance.
    PortNum         -   The given port to be reset.
    RetryTimes      -   RetryTimes when failed
  Returns:
    N/A

--*/
{
  EFI_STATUS  Status;

  //
  // reset root port
  //
  Status = UsbHCInterface->SetRootHubPortFeature (
                            UsbHCInterface,
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
  Status = UsbHCInterface->ClearRootHubPortFeature (
                            UsbHCInterface,
                            PortNum,
                            EfiUsbPortReset
                            );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  gBS->Stall (1000);

  Status = ClearRootPortConnectionChangeStatus (PortNum, UsbHCInterface);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Set port enable
  //
  Status = UsbHCInterface->SetRootHubPortFeature (
                            UsbHCInterface,
                            PortNum,
                            EfiUsbPortEnable
                            );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = UsbHCInterface->ClearRootHubPortFeature (
                            UsbHCInterface,
                            PortNum,
                            EfiUsbPortEnableChange
                            );
  gBS->Stall ((1 + RetryTimes) * 50 * 1000);

  return EFI_SUCCESS;
}


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
  USB_IO_DEVICE             *UsbIoDev;
  USB_IO_CONTROLLER_DEVICE  *ParentController;
  UINT8                     HubPort;
  EFI_STATUS                Status;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_USB_PORT_STATUS       PortStatus;
  EFI_USB_HC_PROTOCOL       *UsbHCInterface;

  ParentController  = UsbIoController->Parent;
  ParentIoDev       = ParentController->UsbDevice;
  UsbIoDev          = UsbIoController->UsbDevice;
  HubPort           = UsbIoController->ParentPort;

  if (ParentIoDev->DeviceAddress == 1) {
    //
    // Connected to the root hub
    //
    UsbHCInterface = ParentIoDev->BusController->UsbHCInterface;
    Status = UsbHCInterface->GetRootHubPortStatus (
                              UsbHCInterface,
                              HubPort,
                              &PortStatus
                              );

  } else {
    UsbIo = &UsbIoController->UsbIo;
    Status = HubGetPortStatus (
              &ParentController->UsbIo,
              HubPort + 1,
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
