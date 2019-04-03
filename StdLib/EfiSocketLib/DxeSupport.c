/** @file
  SocketDxe support routines

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Socket.h"


/**
  Creates a child handle and installs gEfiSocketProtocolGuid.

  This routine creates a child handle for the socket driver and
  installs the ::gEfiSocketProtocolGuid on that handle with a pointer
  to the ::EFI_SOCKET_PROTOCOL structure address.

  This routine is called by ::EslServiceGetProtocol in UseSocketDxe
  when the socket application is linked with UseSocketDxe.

  @param [in] pThis        Address of the EFI_SERVICE_BINDING_PROTOCOL structure.
  @param [in] pChildHandle Pointer to the handle of the child to create. If it is NULL,
                           then a new handle is created. If it is a pointer to an existing UEFI handle, 
                           then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
EslDxeCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN OUT EFI_HANDLE * pChildHandle
  )
{
  ESL_SOCKET * pSocket;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Create a socket structure
  //
  Status = EslSocketAllocate ( pChildHandle,
                               DEBUG_SOCKET,
                               &pSocket );

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Removes gEfiSocketProtocolGuid and destroys the child handle.

  This routine uninstalls ::gEfiSocketProtocolGuid from the child handle
  and destroys the child handle if necessary.

  This routine is called from ???.
  
  @param [in] pThis       Address of the EFI_SERVICE_BINDING_PROTOCOL structure.
  @param [in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is not a valid UEFI Handle.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
EslDxeDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE ChildHandle
  )
{
  ESL_LAYER * pLayer;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Locate the socket control structure
  //
  pLayer = &mEslLayer;
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiSocketProtocolGuid,
                  (VOID **)&pSocketProtocol,
                  pLayer->ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Free the socket resources
    //
    Status = EslSocketFree ( pSocketProtocol, NULL );
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Failed to open socket protocol on 0x%08x, Status; %r\r\n",
              ChildHandle,
              Status ));
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
Install the socket service

This routine installs the ::gEfiSocketServiceBindingProtocolGuid
on the SocketDxe image handle to announce the availability
of the socket layer to the rest of EFI.

SocketDxe's EntryPoint routine calls this routine to
make the socket layer available.

@param [in] pImageHandle      Address of the image handle

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslDxeInstall (
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
                  mEslLayer.pServiceBinding,
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
Uninstall the socket service

This routine removes the gEfiSocketServiceBindingProtocolGuid from
the SocketDxe image handle to notify EFI that the socket layer
is no longer available.

SocketDxe's DriverUnload routine calls this routine to remove the
socket layer.

@param [in] ImageHandle       Handle for the image.

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslDxeUninstall (
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
              mEslLayer.pServiceBinding,
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
