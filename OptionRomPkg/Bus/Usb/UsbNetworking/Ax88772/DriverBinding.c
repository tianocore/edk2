/** @file
  Implement the driver binding protocol for Asix AX88772 Ethernet driver.

  Copyright (c) 2011-2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ax88772.h"

/**
  Verify the controller type

  @param [in] pThis                Protocol instance pointer.
  @param [in] Controller           Handle of device to test.
  @param [in] pRemainingDevicePath Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
DriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE Controller,
  IN EFI_DEVICE_PATH_PROTOCOL * pRemainingDevicePath
  )
{
  EFI_USB_DEVICE_DESCRIPTOR Device;
  EFI_USB_IO_PROTOCOL * pUsbIo;
  EFI_STATUS Status;

  //
  //  Connect to the USB stack
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &pUsbIo,
                  pThis->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR ( Status )) {

    //
    //  Get the interface descriptor to check the USB class and find a transport
    //  protocol handler.
    //
    Status = pUsbIo->UsbGetDeviceDescriptor ( pUsbIo, &Device );
    if (!EFI_ERROR ( Status )) {

      //
      //  Validate the adapter
      //
      if (( VENDOR_ID != Device.IdVendor )
        || ( PRODUCT_ID != Device.IdProduct )) {
        Status = EFI_UNSUPPORTED;
      }
    }

    //
    //  Done with the USB stack
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiUsbIoProtocolGuid,
           pThis->DriverBindingHandle,
           Controller
           );
  }

  //
  //  Return the device supported status
  //
  return Status;
}


/**
  Start this driver on Controller by opening UsbIo and DevicePath protocols.
  Initialize PXE structures, create a copy of the Controller Device Path with the
  NIC's MAC address appended to it, install the NetworkInterfaceIdentifier protocol
  on the newly created Device Path.

  @param [in] pThis                Protocol instance pointer.
  @param [in] Controller           Handle of device to work with.
  @param [in] pRemainingDevicePath Not used, always produce all possible children.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
DriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE Controller,
  IN EFI_DEVICE_PATH_PROTOCOL * pRemainingDevicePath
  )
{
  EFI_STATUS Status;
  NIC_DEVICE * pNicDevice;
  UINTN LengthInBytes;

  DBG_ENTER ( );

  //
  //  Allocate the device structure
  //
  LengthInBytes = sizeof ( *pNicDevice );
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  LengthInBytes,
                  (VOID **) &pNicDevice
                  );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Allocate pNicDevice, %d bytes\r\n",
              pNicDevice,
              sizeof ( *pNicDevice )));

    //
    //  Set the structure signature
    //
    ZeroMem ( pNicDevice, LengthInBytes );
    pNicDevice->Signature = DEV_SIGNATURE;

    //
    //  Connect to the USB I/O protocol
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &pNicDevice->pUsbIo,
                    pThis->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

    if ( !EFI_ERROR ( Status )) {
      //
      //  Allocate the necessary events
      //
      Status = gBS->CreateEvent ( EVT_TIMER,
                                  TPL_AX88772,
                                  (EFI_EVENT_NOTIFY)Ax88772Timer,
                                  pNicDevice,
                                  (VOID **)&pNicDevice->Timer );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                  "0x%08x: Allocated timer\r\n",
                  pNicDevice->Timer ));

        //
        //  Initialize the simple network protocol
        //
        pNicDevice->Controller = Controller;
        SN_Setup ( pNicDevice );

        //
        //  Start the timer
        //
        Status = gBS->SetTimer ( pNicDevice->Timer,
                                 TimerPeriodic,
                                 TIMER_MSEC );
        if ( !EFI_ERROR ( Status )) {
          //
          //  Install both the simple network and device path protocols.
          //
          Status = gBS->InstallMultipleProtocolInterfaces (
                          &Controller,
                          &gEfiCallerIdGuid,
                          pNicDevice,
                          &gEfiSimpleNetworkProtocolGuid,
                          &pNicDevice->SimpleNetwork,
                          NULL
                          );

          if ( !EFI_ERROR ( Status )) {
            DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                      "Installed: gEfiCallerIdGuid on   0x%08x\r\n",
                      Controller ));
            DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                      "Installed: gEfiSimpleNetworkProtocolGuid on   0x%08x\r\n",
                      Controller ));
            DBG_EXIT_STATUS ( Status );
            return Status;
          }
          DEBUG (( DEBUG_ERROR | DEBUG_INIT | DEBUG_INFO,
                    "ERROR - Failed to install gEfiSimpleNetworkProtocol on 0x%08x\r\n",
                    Controller ));
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_INIT | DEBUG_INFO,
                    "ERROR - Failed to start the timer, Status: %r\r\n",
                    Status ));
        }
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_INIT | DEBUG_INFO,
                  "ERROR - Failed to create timer event, Status: %r\r\n",
                  Status ));
      }

      //
      //  Done with the USB stack
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiUsbIoProtocolGuid,
             pThis->DriverBindingHandle,
             Controller
             );
    }

    //
    //  Done with the device
    //
    gBS->FreePool ( pNicDevice );
  }

  //
  //  Display the driver start status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Stop this driver on Controller by removing NetworkInterfaceIdentifier protocol and
  closing the DevicePath and PciIo protocols on Controller.

  @param [in] pThis                Protocol instance pointer.
  @param [in] Controller           Handle of device to stop driver on.
  @param [in] NumberOfChildren     How many children need to be stopped.
  @param [in] pChildHandleBuffer   Not used.

  @retval EFI_SUCCESS          This driver is removed Controller.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
DriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL * pThis,
  IN  EFI_HANDLE Controller,
  IN  UINTN NumberOfChildren,
  IN  EFI_HANDLE * pChildHandleBuffer
  )
{
  NIC_DEVICE * pNicDevice;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Determine if this driver is already attached
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiCallerIdGuid,
                  (VOID **) &pNicDevice,
                  pThis->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if ( !EFI_ERROR ( Status )) {
    //
    //  AX88772 driver is no longer running on this device
    //
    gBS->UninstallMultipleProtocolInterfaces (
              Controller,
              &gEfiSimpleNetworkProtocolGuid,
              &pNicDevice->SimpleNetwork,
              &gEfiCallerIdGuid,
              pNicDevice,
              NULL );
    DEBUG (( DEBUG_POOL | DEBUG_INIT,
                "Removed:   gEfiSimpleNetworkProtocolGuid from 0x%08x\r\n",
                Controller ));
    DEBUG (( DEBUG_POOL | DEBUG_INIT,
                "Removed:   gEfiCallerIdGuid from 0x%08x\r\n",
                Controller ));

    //
    //  Stop the timer
    //
    if ( NULL != pNicDevice->Timer ) {
      gBS->SetTimer ( pNicDevice->Timer, TimerCancel, 0 );
      gBS->CloseEvent ( pNicDevice->Timer );
      DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                "0x%08x: Released timer\r\n",
                pNicDevice->Timer ));
    }

    //
    //  Done with the device context
    //
    DEBUG (( DEBUG_POOL | DEBUG_INIT,
              "0x%08x: Free pNicDevice, %d bytes\r\n",
              pNicDevice,
              sizeof ( *pNicDevice )));
    gBS->FreePool ( pNicDevice );
  }

  //
  //  Return the shutdown status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Driver binding protocol declaration
**/
EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  DriverSupported,
  DriverStart,
  DriverStop,
  0xa,
  NULL,
  NULL
};


/**
  Ax88772 driver unload routine.

  @param [in] ImageHandle       Handle for the image.

  @retval EFI_SUCCESS           Image may be unloaded

**/
EFI_STATUS
EFIAPI
DriverUnload (
  IN EFI_HANDLE ImageHandle
  )
{
  UINTN BufferSize;
  UINTN Index;
  UINTN Max;
  EFI_HANDLE * pHandle;
  EFI_STATUS Status;

  //
  //  Determine which devices are using this driver
  //
  BufferSize = 0;
  pHandle = NULL;
  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiCallerIdGuid,
                  NULL,
                  &BufferSize,
                  NULL );
  if ( EFI_BUFFER_TOO_SMALL == Status ) {
    for ( ; ; ) {
      //
      //  One or more block IO devices are present
      //
      Status = gBS->AllocatePool (
                      EfiRuntimeServicesData,
                      BufferSize,
                      (VOID **) &pHandle
                      );
      if ( EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                  "Insufficient memory, failed handle buffer allocation\r\n" ));
        break;
      }

      //
      //  Locate the block IO devices
      //
      Status = gBS->LocateHandle (
                      ByProtocol,
                      &gEfiCallerIdGuid,
                      NULL,
                      &BufferSize,
                      pHandle );
      if ( EFI_ERROR ( Status )) {
        //
        //  Error getting handles
        //
        DEBUG (( DEBUG_ERROR | DEBUG_INIT | DEBUG_INFO,
                "Failure getting Telnet handles\r\n" ));
        break;
      }
      
      //
      //  Remove any use of the driver
      //
      Max = BufferSize / sizeof ( pHandle[ 0 ]);
      for ( Index = 0; Max > Index; Index++ ) {
        Status = DriverStop ( &gDriverBinding,
                              pHandle[ Index ],
                              0,
                              NULL );
        if ( EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_WARN | DEBUG_INIT | DEBUG_INFO,
                    "WARNING - Failed to shutdown the driver on handle %08x\r\n", pHandle[ Index ]));
          break;
        }
      }
      break;
    }
  }
  else {
    if ( EFI_NOT_FOUND == Status ) {
      //
      //  No devices were found
      //
      Status = EFI_SUCCESS;
    }
  }

  //
  //  Free the handle array
  //
  if ( NULL != pHandle ) {
    gBS->FreePool ( pHandle );
  }

  //
  //  Remove the protocols installed by the EntryPoint routine.
  //
  if ( !EFI_ERROR ( Status )) {
    gBS->UninstallMultipleProtocolInterfaces (
            ImageHandle,
            &gEfiDriverBindingProtocolGuid,
            &gDriverBinding,
            &gEfiComponentNameProtocolGuid,
            &gComponentName,
            &gEfiComponentName2ProtocolGuid,
            &gComponentName2,
            NULL
            );
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
            "Removed:   gEfiComponentName2ProtocolGuid from 0x%08x\r\n",
            ImageHandle ));
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Removed:   gEfiComponentNameProtocolGuid from 0x%08x\r\n",
              ImageHandle ));
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Removed:   gEfiDriverBindingProtocolGuid from 0x%08x\r\n",
              ImageHandle ));
  }

  //
  //  Return the unload status
  //
  return Status;
}


/**
Ax88772 driver entry point.

@param [in] ImageHandle       Handle for the image.
@param [in] pSystemTable      Address of the system table.

@retval EFI_SUCCESS           Image successfully loaded.

**/
EFI_STATUS
EFIAPI
EntryPoint (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE * pSystemTable
  )
{
  EFI_STATUS    Status;

  DBG_ENTER ( );

  //
  //  Add the driver to the list of drivers
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             pSystemTable,
             &gDriverBinding,
             ImageHandle,
             &gComponentName,
             &gComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Installed: gEfiDriverBindingProtocolGuid on   0x%08x\r\n",
              ImageHandle ));
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Installed: gEfiComponentNameProtocolGuid on   0x%08x\r\n",
              ImageHandle ));
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Installed: gEfiComponentName2ProtocolGuid on   0x%08x\r\n",
              ImageHandle ));
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}
