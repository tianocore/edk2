/** @file
  Connect to and disconnect from the various network layers

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Socket.h"


/**
  Connect to the network service bindings

  Walk the network service protocols on the controller handle and
  locate any that are not in use.  Create ::ESL_SERVICE structures to
  manage the network layer interfaces for the socket driver.  Tag
  each of the network interfaces that are being used.  Finally, this
  routine calls ESL_SOCKET_BINDING::pfnInitialize to prepare the network
  interface for use by the socket layer.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller       Handle of device to work with.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval EFI_OUT_OF_RESOURCES No more memory available.
  @retval EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
EslServiceConnect (
  IN EFI_HANDLE BindingHandle,
  IN EFI_HANDLE Controller
  )
{
  BOOLEAN bInUse;
  EFI_STATUS ExitStatus;
  UINTN LengthInBytes;
  UINT8 * pBuffer;
  CONST ESL_SOCKET_BINDING * pEnd;
  VOID * pJunk;
  ESL_SERVICE ** ppServiceListHead;
  ESL_SERVICE * pService;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  EFI_STATUS Status;
  EFI_TPL TplPrevious;

  DBG_ENTER ( );

  //
  //  Assume the list is empty
  //
  ExitStatus = EFI_UNSUPPORTED;
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
                    (VOID**)&pServiceBinding,
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
          pService->pServiceBinding = pServiceBinding;

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
              //  Connect the service to the list
              //
              pBuffer = (UINT8 *)&mEslLayer;
              pBuffer = &pBuffer[ pSocketBinding->ServiceListOffset ];
              ppServiceListHead = (ESL_SERVICE **)pBuffer;
              pService->pNext = *ppServiceListHead;
              *ppServiceListHead = pService;

              //
              //  Release the socket layer synchronization
              //
              RESTORE_TPL ( TplPrevious );

              //
              //  At least one service was made available
              //
              ExitStatus = EFI_SUCCESS;
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
          ExitStatus = EFI_OUT_OF_RESOURCES;
          break;
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
  DBG_EXIT_STATUS ( ExitStatus );
  return ExitStatus;
}


/**
  Shutdown the connections to the network layer by locating the
  tags on the network interfaces established by ::EslServiceConnect.
  This routine shutdowns any activity on the network interface and
  then frees the ::ESL_SERVICE structures.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller       Handle of device to stop driver on.

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
  UINT8 * pBuffer;
  CONST ESL_SOCKET_BINDING * pEnd;
  ESL_PORT * pPort;
  ESL_SERVICE * pPreviousService;
  ESL_SERVICE * pService;
  ESL_SERVICE ** ppServiceListHead;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
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
      //  Walk the list of ports
      //
      pPort = pService->pPortList;
      while ( NULL != pPort ) {
        //
        //  Remove the port from the port list
        //
        pPort->pService = NULL;
        pService->pPortList = pPort->pLinkService;
  
        //
        //  Close the port
        //
        EslSocketPortCloseStart ( pPort,
                                  TRUE,
                                  DEBUG_POOL | DEBUG_INIT );

        //
        //  Set the next port
        //
        pPort = pService->pPortList;
      }
    
      //
      //  Remove the service from the service list
      //
      pBuffer = (UINT8 *)&mEslLayer;
      pBuffer = &pBuffer[ pService->pSocketBinding->ServiceListOffset ];
      ppServiceListHead = (ESL_SERVICE **)pBuffer;
      pPreviousService = *ppServiceListHead;
      if ( pService == pPreviousService ) {
        //
        //  Remove the service from the beginning of the list
        //
        *ppServiceListHead = pService->pNext;
      }
      else {
        //
        //  Remove the service from the middle of the list
        //
        while ( NULL != pPreviousService ) {
          if ( pService == pPreviousService->pNext ) {
            pPreviousService->pNext = pService->pNext;
            break;
          }
          pPreviousService = pPreviousService->pNext;
        }
      }

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
Initialize the service layer

@param [in] ImageHandle       Handle for the image.

**/
VOID
EFIAPI
EslServiceLoad (
  IN EFI_HANDLE ImageHandle
  )
{
  ESL_LAYER * pLayer;

  //
  //  Save the image handle
  //
  pLayer = &mEslLayer;
  ZeroMem ( pLayer, sizeof ( *pLayer ));
  pLayer->Signature = LAYER_SIGNATURE;
  pLayer->ImageHandle = ImageHandle;

  //
  //  Connect the service binding protocol to the image handle
  //
  pLayer->pServiceBinding = &mEfiServiceBinding;
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
  ESL_LAYER * pLayer;

  //
  //  Undo the work by ServiceLoad
  //
  pLayer = &mEslLayer;
  pLayer->ImageHandle = NULL;
  pLayer->pServiceBinding = NULL;
}
