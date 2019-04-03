/** @file
  Implement the connection to the socket driver

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiSocket.h>
#include <Protocol/ServiceBinding.h>


/**
  Free the socket resources

  This releases the socket resources allocated by calling
  EslServiceGetProtocol.

  This routine is called from the ::close routine in BsdSocketLib
  to release the socket resources.

  @param [in] pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL
                                structure

  @return       Value for ::errno, zero (0) indicates success.

 **/
int
EslServiceFreeProtocol (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol
  )
{
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  int RetVal;
  EFI_STATUS Status;

  //
  //  Assume success
  //
  RetVal = 0;

  //
  //  Locate the socket protocol
  //
  Status = gBS->LocateProtocol ( &gEfiSocketServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **) &pServiceBinding );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Release the handle
    //
    Status = pServiceBinding->DestroyChild ( pServiceBinding,
                                             pSocketProtocol->SocketHandle );
  }
  if ( EFI_ERROR ( Status )) {
    RetVal = EIO;
  }

  //
  //  Return the operation status
  //
  return RetVal;
}


/**
  Connect to the EFI socket library

  This routine establishes a connection to the socket driver
  and returns the API (::EFI_SOCKET_PROTOCOL address) to the
  socket file system layer in BsdSocketLib.  This routine looks for
  the gEfiSocketServiceBindingProtocolGuid to locate the socket
  driver.  This routine then creates a child handle and locates
  the gEfiSocketProtocolGuid protocol on that handle to get the
  ::EFI_SOCKET_PROTOCOL structure address.

  This routine is called from the ::socket routine in BsdSocketLib
  to create the data structure and initialize the API for a socket.
  Note that this implementation is only used by socket applications
  that link directly to UseSocketDxe.

  @param [in] ppSocketProtocol  Address to receive the ::EFI_SOCKET_PROTOCOL
                                structure address

  @return       Value for ::errno, zero (0) indicates success.

 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  )
{
  EFI_SERVICE_BINDING_PROTOCOL * pServiceBinding;
  int RetVal;
  EFI_HANDLE SocketHandle;
  EFI_STATUS Status;

  //
  //  Locate the socket protocol
  //
  Status = gBS->LocateProtocol ( &gEfiSocketServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&pServiceBinding );
  if ( !EFI_ERROR ( Status )) {
    //
    //  Create a new socket
    //
    SocketHandle = NULL;
    Status = pServiceBinding->CreateChild ( pServiceBinding,
                                            &SocketHandle );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Get the socket protocol
      //
      Status = gBS->OpenProtocol ( SocketHandle,
                                   &gEfiSocketProtocolGuid,
                                   (VOID **)ppSocketProtocol,
                                   NULL,
                                   NULL,
                                   EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL );
      if ( !EFI_ERROR ( Status )) {
        //
        //  Success!
        //
        RetVal = 0;
      }
      else {
        DEBUG (( DEBUG_ERROR,
                  "ERROR - No socket protocol on 0x%08x, Status: %r\r\n",
                  SocketHandle,
                  Status ));
        RetVal = ENODEV;
      }
    }
    else {
      //
      //  Translate the error
      //
      DEBUG (( DEBUG_ERROR,
                "ERROR - CreateChild failed, Status: %r\r\n",
                Status ));
      switch ( Status ) {
      case EFI_SUCCESS:
        RetVal = 0;
        break;

      case EFI_ACCESS_DENIED:
      case EFI_WRITE_PROTECTED:
        RetVal = EACCES;
        break;

      case EFI_NO_RESPONSE:
        RetVal = EHOSTUNREACH;
        break;

      case EFI_BAD_BUFFER_SIZE:
      case EFI_BUFFER_TOO_SMALL:
      case EFI_INVALID_PARAMETER:
        RetVal = EINVAL;
        break;

      case EFI_DEVICE_ERROR:
      case EFI_MEDIA_CHANGED:
      case EFI_NO_MEDIA:
      case EFI_VOLUME_CORRUPTED:
        RetVal = EIO;
        break;

      case EFI_NOT_FOUND:
        RetVal = ENOENT;
        break;

      default:
      case EFI_OUT_OF_RESOURCES:
        RetVal = ENOMEM;
        break;

      case EFI_VOLUME_FULL:
        RetVal = ENOSPC;
        break;

      case EFI_UNSUPPORTED:
        RetVal = ENOSYS;
        break;

      case EFI_NO_MAPPING:
        RetVal = ENXIO;
        break;

      case EFI_LOAD_ERROR:
        RetVal = ESRCH;
        break;

      case EFI_TIMEOUT:
        RetVal = ETIMEDOUT;
        break;

      case EFI_NOT_READY:
        RetVal = EWOULDBLOCK;
        break;
      }
    }
  }
  else {
    DEBUG (( DEBUG_ERROR,
              "ERROR - Socket driver not loaded, Status: %r\r\n",
              Status ));
    RetVal = ENODEV;
  }

  //
  //  Return the operation status
  //
  return RetVal;
}
