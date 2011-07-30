/** @file
  Implement the driver binding protocol for the socket layer.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Socket.h"

/**
  Verify the controller type

  Determine if any of the network service binding protocols exist on
  the controller handle.  If so, verify that these protocols are not
  already in use.  Call ::DriverStart for any network service binding
  protocol that is not in use.

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
  CONST DT_SOCKET_BINDING * pEnd;
  VOID * pInterface;
  CONST DT_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;

  //
  //  Assume the list is empty
  //
  Status = EFI_UNSUPPORTED;

  //
  //  Walk the list of network connection points
  //
  pSocketBinding = &cEslSocketBinding[0];
  pEnd = &pSocketBinding[ cEslSocketBindingEntries ];
  while ( pEnd > pSocketBinding ) {
    //
    //  Determine if the controller supports the network protocol
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    pSocketBinding->pNetworkBinding,
                    &pInterface,
                    pThis->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Determine if the driver is already connected
      //
      Status = gBS->OpenProtocol (
                      Controller,
                      (EFI_GUID *)pSocketBinding->pTagGuid,
                      &pInterface,
                      pThis->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if ( !EFI_ERROR ( Status )) {
        Status = EFI_ALREADY_STARTED;
      }
      else {
        if ( EFI_UNSUPPORTED == Status ) {
          //
          //  Connect the driver since the tag is not present
          //
          Status = EFI_SUCCESS;
        }
      }
    }

    //
    //  Set the next network protocol
    //
    pSocketBinding += 1;
  }

  //
  //  Return the device supported status
  //
  return Status;
}


/**
  Connect to the network service bindings

  Walk the network service protocols on the controller handle and
  locate any that are not in use.  Create service structures to
  manage the service binding for the socket driver.

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

  DBG_ENTER ( );

  //
  //  Connect to this network adapter
  //
  Status = EslServiceConnect ( pThis->DriverBindingHandle,
                               Controller );

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
  EFI_STATUS Status;
  
  DBG_ENTER ( );

  //
  //  Disconnect the network adapters
  //
  Status = EslServiceDisconnect ( pThis->DriverBindingHandle,
                                  Controller );

  //
  //  Display the driver start status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Driver binding protocol definition
**/
EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  DriverSupported,
  DriverStart,
  DriverStop,
  0xa,
  NULL,
  NULL
};
