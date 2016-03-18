/** @file
  Implement the driver binding protocol for the socket layer.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


  \section NetworkAdapterManagement Network Adapter Management
  Network adapters may come and go over the life if a system running
  UEFI.  The SocketDxe driver uses the driver binding API to manage
  the connections to network adapters.

  The ::DriverSupported routine selects network adapters that the
  socket layer is not using.  This determination by the lack of the
  tag GUID associated with the network protocol in the
  ::cEslSocketBinding array.  The selected network adapters are 
  passed to the ::DriverStart routine.

  The ::DriverStart routine calls the ::EslServiceConnect routine
  to create an ::ESL_SERVICE structure to manage the network adapter
  for the socket layer.  EslServiceConnect also installs the tag
  GUID on the network adapter to prevent future calls from
  ::DriverSupported.  EslService also calls the network specific
  initialization routine listed in ESL_SOCKET_BINDING::pfnInitialize
  field of the ::cEslSocketBinding entry.

  The ::DriverStop routine calls the ::EslServiceDisconnect routine
  to undo the work done by ::DriverStart.  The socket layer must break
  the active network connections, then remove the tag GUIDs from the
  controller handle and free ::ESL_SERVICE structure.

**/

#include "Socket.h"

/**
  Verify the controller type

  This routine walks the cEslSocketBinding array to determines if
  the controller is a network adapter by supporting any of the
  network protocols required by the sockets layer.  If so, the
  routine verifies that the socket layer is not already using the
  support by looking for the tag GUID listed in the corresponding
  array entry.  The controller handle is passed to the ::DriverStart
  routine if sockets can use the network adapter.
  See the \ref NetworkAdapterManagement section.

  This routine is called by the UEFI driver framework during connect
  processing.

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
  CONST ESL_SOCKET_BINDING * pEnd;
  VOID * pInterface;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
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
  Connect to a network adapter

  This routine calls ::EslServiceConnect to connect the socket
  layer to the network adapters.  See the \ref NetworkAdapterManagement
  section.

  This routine is called by the UEFI driver framework during connect
  processing if the controller passes the tests in ::DriverSupported.

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
  Disconnect from a network adapter

  This routine calls ::EslServiceDisconnect to disconnect the socket
  layer from the network adapters.  See the \ref NetworkAdapterManagement
  section.

  This routine is called by ::DriverUnload when the socket layer
  is being unloaded.  This routine should also called by the UEFI
  driver framework when a network adapter is being unloaded from
  the system.
  
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
  Driver binding protocol for the SocketDxe driver.
**/
EFI_DRIVER_BINDING_PROTOCOL  mDriverBinding = {
  DriverSupported,
  DriverStart,
  DriverStop,
  0xa,
  NULL,
  NULL
};
