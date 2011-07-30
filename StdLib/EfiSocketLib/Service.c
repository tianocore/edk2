/** @file
  Connect to and disconnect from the various network layers

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Socket.h"

EFI_TCP4_PROTOCOL * mpEfiTcpClose4 [ 1024 ];


/**
  Connect to the network service bindings

  Walk the network service protocols on the controller handle and
  locate any that are not in use.  Create service structures to
  manage the service binding for the socket driver.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller       Handle of device to work with.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
EslServiceConnect (
  IN EFI_HANDLE BindingHandle,
  IN EFI_HANDLE Controller
  )
{
  BOOLEAN bInUse;
  UINTN LengthInBytes;
  CONST DT_SOCKET_BINDING * pEnd;
  VOID * pJunk;
  VOID * pInterface;
  DT_SERVICE * pService;
  CONST DT_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume the list is empty
  //
  Status = EFI_UNSUPPORTED;
  bInUse = FALSE;

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
                    BindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Determine if the socket layer is already connected
      //
      Status = gBS->OpenProtocol (
                      Controller,
                      (EFI_GUID *)pSocketBinding->pTagGuid,
                      &pJunk,
                      BindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if ( EFI_UNSUPPORTED == Status ) {
        //
        //  Allocate a service structure since the tag is not present
        //
        LengthInBytes = sizeof ( *pService );
        Status = gBS->AllocatePool (
                        EfiRuntimeServicesData,
                        LengthInBytes,
                        (VOID **) &pService
                        );
        if ( !EFI_ERROR ( Status )) {
          DEBUG (( DEBUG_POOL | DEBUG_INIT,
                    "0x%08x: Allocate pService, %d bytes\r\n",
                    pService,
                    LengthInBytes ));

          //
          //  Set the structure signature and service binding
          //
          ZeroMem ( pService, LengthInBytes );
          pService->Signature = SERVICE_SIGNATURE;
          pService->pSocketBinding = pSocketBinding;
          pService->Controller = Controller;
          pService->pInterface = pInterface;

          //
          //  Mark the controller in use
          //
          if ( !bInUse ) {
            Status = gBS->InstallMultipleProtocolInterfaces (
                            &Controller,
                            &gEfiCallerIdGuid,
                            NULL,
                            NULL
                            );
            if ( !EFI_ERROR ( Status )) {
              DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                        "Installed: gEfiCallerIdGuid on   0x%08x\r\n",
                        Controller ));
              bInUse = TRUE;
            }
            else {
              if ( EFI_INVALID_PARAMETER == Status ) {
                Status = EFI_SUCCESS;
              }
            }
          }
          if ( !EFI_ERROR ( Status )) {
            //
            //  Mark the network service protocol in use
            //
            Status = gBS->InstallMultipleProtocolInterfaces (
                            &Controller,
                            pSocketBinding->pTagGuid,
                            pService,
                            NULL
                            );
            if ( !EFI_ERROR ( Status )) {
              DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                        "Installed: %s TagGuid on   0x%08x\r\n",
                        pSocketBinding->pName,
                        Controller ));

              //
              //  Synchronize with the socket layer
              //
              RAISE_TPL ( TplPrevious, TPL_SOCKETS );

              //
              //  Initialize the service
              //
              Status = pSocketBinding->pfnInitialize ( pService );

              //
              //  Release the socket layer synchronization
              //
              RESTORE_TPL ( TplPrevious );

              //
              //  Determine if the initialization was successful
              //
              if ( EFI_ERROR ( Status )) {
                DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                          "ERROR - Failed to initialize service %s on 0x%08x, Status: %r\r\n",
                          pSocketBinding->pName,
                          Controller,
                          Status ));

                //
                //  Free the network service binding if necessary
                //
                gBS->UninstallMultipleProtocolInterfaces (
                          Controller,
                          pSocketBinding->pTagGuid,
                          pService,
                          NULL );
                DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                            "Removed:   %s TagGuid from 0x%08x\r\n",
                            pSocketBinding->pName,
                            Controller ));
              }
            }
            else {
              DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                        "ERROR - Failed to install %s TagGuid on 0x%08x, Status: %r\r\n",
                        pSocketBinding->pName,
                        Controller,
                        Status ));
            }

            if ( EFI_ERROR ( Status )) {
              //
              //  The controller is no longer in use
              //
              if ( bInUse ) {
                gBS->UninstallMultipleProtocolInterfaces (
                          Controller,
                          &gEfiCallerIdGuid,
                          NULL,
                          NULL );
                DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
                            "Removed:   gEfiCallerIdGuid from 0x%08x\r\n",
                            Controller ));
              }
            }
          }
          else {
            DEBUG (( DEBUG_ERROR | DEBUG_INIT,
                      "ERROR - Failed to install gEfiCallerIdGuid on 0x%08x, Status: %r\r\n",
                      Controller,
                      Status ));
          }

          //
          //  Release the service if necessary
          //
          if ( EFI_ERROR ( Status )) {
            gBS->FreePool ( pService );
            DEBUG (( DEBUG_POOL | DEBUG_INIT,
                      "0x%08x: Free pService, %d bytes\r\n",
                      pService,
                      sizeof ( *pService )));
            pService = NULL;
          }
        }
        else {
          DEBUG (( DEBUG_ERROR | DEBUG_INIT,
                    "ERROR - Failed service allocation, Status: %r\r\n",
                    Status ));
        }
      }
    }
  
    //
    //  Set the next network protocol
    //
    pSocketBinding += 1;
  }
  
  //
  //  Display the driver start status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Shutdown the network connections to this controller by removing
  NetworkInterfaceIdentifier protocol and closing the DevicePath
  and PciIo protocols on Controller.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller           Handle of device to stop driver on.

  @retval EFI_SUCCESS          This driver is removed Controller.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
EslServiceDisconnect (
  IN  EFI_HANDLE BindingHandle,
  IN  EFI_HANDLE Controller
  )
{
  CONST DT_SOCKET_BINDING * pEnd;
  DT_SERVICE * pService;
  CONST DT_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;
  
  DBG_ENTER ( );

  //
  //  Walk the list of network connection points in reverse order
  //
  pEnd = &cEslSocketBinding[0];
  pSocketBinding = &pEnd[ cEslSocketBindingEntries ];
  while ( pEnd < pSocketBinding ) {
    //
    //  Set the next network protocol
    //
    pSocketBinding -= 1;

    //
    //  Determine if the driver connected
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    (EFI_GUID *)pSocketBinding->pTagGuid,
                    (VOID **)&pService,
                    BindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if ( !EFI_ERROR ( Status )) {

      //
      //  Synchronize with the socket layer
      //
      RAISE_TPL ( TplPrevious, TPL_SOCKETS );

      //
      //  Shutdown the service
      //
      pSocketBinding->pfnShutdown ( pService );

      //
      //  Release the socket layer synchronization
      //
      RESTORE_TPL ( TplPrevious );

      //
      //  Break the driver connection
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                Controller,
                pSocketBinding->pTagGuid,
                pService,
                NULL );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL | DEBUG_INIT,
                    "Removed:   %s TagGuid from 0x%08x\r\n",
                    pSocketBinding->pName,
                    Controller ));
      }
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                    "ERROR - Failed to removed %s TagGuid from 0x%08x, Status: %r\r\n",
                    pSocketBinding->pName,
                    Controller,
                    Status ));
      }

      //
      //  Free the service structure
      //
      Status = gBS->FreePool ( pService );
      if ( !EFI_ERROR ( Status )) {
        DEBUG (( DEBUG_POOL | DEBUG_INIT,
                  "0x%08x: Free pService, %d bytes\r\n",
                  pService,
                  sizeof ( *pService )));
      }
      else {
        DEBUG (( DEBUG_POOL | DEBUG_INIT,
                  "ERROR - Failed to free pService 0x%08x, Status: %r\r\n",
                  pService,
                  Status ));
      }
      pService = NULL;
    }
  }

  //
  //  The controller is no longer in use
  //
  gBS->UninstallMultipleProtocolInterfaces (
            Controller,
            &gEfiCallerIdGuid,
            NULL,
            NULL );
  DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Removed:   gEfiCallerIdGuid from 0x%08x\r\n",
              Controller ));

  //
  //  The driver is disconnected from the network controller
  //
  Status = EFI_SUCCESS;

  //
  //  Display the driver start status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}



/**
Install the socket service

@param [in] pImageHandle      Address of the image handle

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslServiceInstall (
  IN EFI_HANDLE * pImageHandle
  )
{
  EFI_STATUS Status;

  //
  //  Install the socket service binding protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  pImageHandle,
                  &gEfiSocketServiceBindingProtocolGuid,
                  &mEslLayer.ServiceBinding,
                  NULL
                  );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_POOL | DEBUG_INIT | DEBUG_INFO,
              "Installed: gEfiSocketServiceBindingProtocolGuid on   0x%08x\r\n",
              *pImageHandle ));
  }
  else {
    DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
              "ERROR - InstallMultipleProtocolInterfaces failed, Status: %r\r\n",
              Status ));
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
Initialize the service layer

@param [in] ImageHandle       Handle for the image.

**/
VOID
EFIAPI
EslServiceLoad (
  IN EFI_HANDLE ImageHandle
  )
{
  DT_LAYER * pLayer;

  //
  //  Save the image handle
  //
  pLayer = &mEslLayer;
  pLayer->Signature = LAYER_SIGNATURE;
  pLayer->ImageHandle = ImageHandle;

  //
  //  Initialize the TCP4 close
  //
  pLayer->TcpCloseMax4 = DIM ( mpEfiTcpClose4 );
  pLayer->ppTcpClose4 = mpEfiTcpClose4;

  //
  //  Connect the service binding protocol to the image handle
  //
  pLayer->ServiceBinding.CreateChild = EslSocketCreateChild;
  pLayer->ServiceBinding.DestroyChild = EslSocketDestroyChild;
}


/**
Uninstall the socket service

@param [in] ImageHandle       Handle for the image.

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslServiceUninstall (
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;

  //
  //  Install the socket service binding protocol
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
              ImageHandle,
              &gEfiSocketServiceBindingProtocolGuid,
              &mEslLayer.ServiceBinding,
              NULL
              );
  if ( !EFI_ERROR ( Status )) {
    DEBUG (( DEBUG_POOL | DEBUG_INIT,
                "Removed:   gEfiSocketServiceBindingProtocolGuid from 0x%08x\r\n",
                ImageHandle ));
  }
  else {
    DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                "ERROR - Failed to remove gEfiSocketServiceBindingProtocolGuid from 0x%08x, Status: %r\r\n",
                ImageHandle,
                Status ));
  }

  //
  //  Return the operation status
  //
  return Status;
}


/**
  Shutdown the service layer

**/
VOID
EFIAPI
EslServiceUnload (
  VOID
  )
{
  DT_LAYER * pLayer;

  //
  //  Undo the work by ServiceLoad
  //
  pLayer = &mEslLayer;
  pLayer->ImageHandle = NULL;
  pLayer->ServiceBinding.CreateChild = NULL;
  pLayer->ServiceBinding.DestroyChild = NULL;
}
