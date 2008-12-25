/** @file
   Driver Binding functions for PCI bus module.
   
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "PciBus.h"

//
// PCI Bus Driver Global Variables
//

EFI_DRIVER_BINDING_PROTOCOL                   gPciBusDriverBinding = {
  PciBusDriverBindingSupported,
  PciBusDriverBindingStart,
  PciBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL  *gEfiIncompatiblePciDeviceSupport = NULL;
EFI_HANDLE                                    gPciHostBrigeHandles[PCI_MAX_HOST_BRIDGE_NUM];
UINTN                                         gPciHostBridgeNumber;
BOOLEAN                                       gFullEnumeration;
UINT64                                        gAllOne   = 0xFFFFFFFFFFFFFFFFULL;
UINT64                                        gAllZero  = 0;

EFI_PCI_PLATFORM_PROTOCOL                     *gPciPlatformProtocol;

//
// PCI Bus Driver Support Functions
//
/**
  Initialize the global variables
  publish the driver binding protocol

  @param[in] ImageHandle,
  @param[in] *SystemTable

  @retval status of installing driver binding component name protocol.

**/
EFI_STATUS
EFIAPI
PciBusEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  InitializePciDevicePool ();

  gFullEnumeration      = TRUE;

  gPciHostBridgeNumber  = 0;
  
  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gPciBusDriverBinding,
             ImageHandle,
             &gPciBusComponentName,
             &gPciBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  InstallHotPlugRequestProtocol (&Status);
  
  return Status;
}

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a gEfiPciRootBridgeIoProtocolGuid protocol can be supported.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_DEV_PATH_PTR                Node;

  if (RemainingDevicePath != NULL) {
    Node.DevPath = RemainingDevicePath;
    if (Node.DevPath->Type != HARDWARE_DEVICE_PATH ||
        Node.DevPath->SubType != HW_PCI_DP         ||
        DevicePathNodeLength(Node.DevPath) != sizeof(PCI_DEVICE_PATH)) {
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

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgeIo,
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
        &gEfiPciRootBridgeIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return EFI_SUCCESS;
}

/**
  Start this driver on ControllerHandle and enumerate Pci bus and start
  all device under PCI bus.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiIncompatiblePciDeviceSupportProtocolGuid,
                  NULL,
                  (VOID **) &gEfiIncompatiblePciDeviceSupport
                  );

  //
  // If PCI Platform protocol is available, get it now.
  // If the platform implements this, it must be installed before BDS phase
  //
  gPciPlatformProtocol = NULL;
  gBS->LocateProtocol (
        &gEfiPciPlatformProtocolGuid,
        NULL,
        (VOID **) &gPciPlatformProtocol
        );

  gFullEnumeration = (BOOLEAN) ((SearchHostBridgeHandle (Controller) ? FALSE : TRUE));

  //
  // Enumerate the entire host bridge
  // After enumeration, a database that records all the device information will be created
  //
  //
  Status = PciEnumerator (Controller);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Start all the devices under the entire host bridge.
  //
  StartPciDevices (Controller);

  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
PciBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  BOOLEAN     AllChildrenStopped;

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    gBS->CloseProtocol (
          Controller,
          &gEfiDevicePathProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
    gBS->CloseProtocol (
          Controller,
          &gEfiPciRootBridgeIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    DestroyRootBridgeByHandle (
      Controller
      );

    return EFI_SUCCESS;
  }

  //
  // Stop all the children
  //

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    //
    // De register all the pci device
    //
    Status = DeRegisterPciDevice (Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

