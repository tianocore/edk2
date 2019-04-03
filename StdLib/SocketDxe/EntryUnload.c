/** @file
  Implement the entry and unload for the socket driver.

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Socket.h"


/**
  The following GUID values are only used by the SocketDxe driver.  An
  alternative set of values exists in EfiSocketLib\UseEfiSocketLib.c
  which an application uses when it links against EfiSocketLib.  These
  two sets of values allow the SocketDxe driver to coexist with socket
  applications.

  Tag GUID - IPv4 in use by SocketDxe
**/
CONST EFI_GUID mEslIp4ServiceGuid = {
  0x4e3a82e6, 0xe43f, 0x460a, { 0x86, 0x6e, 0x9b, 0x5a, 0xab, 0x80, 0x44, 0x48 }
};


/**
  Tag GUID - IPv6 in use by SocketDxe
**/
CONST EFI_GUID mEslIp6ServiceGuid = {
  0x2fc3b2d3, 0x6eba, 0x42b0, { 0xa4, 0xa7, 0x14, 0xc7, 0xa8, 0x4b, 0x5d, 0x22 }
};


/**
  Tag GUID - TCPv4 in use by SocketDxe
**/
CONST EFI_GUID mEslTcp4ServiceGuid = {
  0x4dcaab0a, 0x1990, 0x4352, { 0x8d, 0x2f, 0x2d, 0x8f, 0x13, 0x55, 0x98, 0xa5 }
};


/**
  Tag GUID - TCPv6 in use by SocketDxe
**/
CONST EFI_GUID mEslTcp6ServiceGuid = {
  0xdd455a69, 0xec75, 0x456c, { 0x84, 0xd2, 0x95, 0xca, 0xe7, 0xd3, 0xc6, 0xd3 }
};


/**
  Tag GUID - UDPv4 in use by SocketDxe
**/
CONST EFI_GUID mEslUdp4ServiceGuid = {
  0x43a110ce, 0x9ccd, 0x402b, { 0x8c, 0x29, 0x4a, 0x6d, 0x8a, 0xf7, 0x79, 0x90 }
};


/**
  Tag GUID - UDPv6 in use by SocketDxe
**/
CONST EFI_GUID mEslUdp6ServiceGuid = {
  0x32ff59cd, 0xc33, 0x48d0, { 0xa2, 0x44, 0x4b, 0xb8, 0x11, 0x33, 0x64, 0x3 }
};


/**
  Socket driver unload routine.

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
        Status = DriverStop ( &mDriverBinding,
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
  //  Done with the socket layer
  //
  if ( !EFI_ERROR ( Status )) {
    Status = EslDxeUninstall ( ImageHandle );
    if ( !EFI_ERROR ( Status )) {
      //
      //  Remove the protocols installed by the EntryPoint routine.
      //
      Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &mDriverBinding,
                  &gEfiComponentNameProtocolGuid,
                  &mComponentName,
                  &gEfiComponentName2ProtocolGuid,
                  &mComponentName2,
                  NULL
                  );
      if ( !EFI_ERROR ( Status )) {
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
      else {
        DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                    "ERROR - Failed to remove gEfiDriverBindingProtocolGuid from 0x%08x, Status: %r\r\n",
                    ImageHandle,
                    Status ));
      }
    }
  }

  //
  //  Disconnect the network services
  //
  if ( !EFI_ERROR ( Status )) {
    EslServiceUnload ( );
  }

  //
  //  Return the unload status
  //
  return Status;
}


/**
Socket driver entry point.

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
  EFI_LOADED_IMAGE_PROTOCOL * pLoadedImage;
  EFI_STATUS    Status;

  DBG_ENTER ( );

  //
  //  Display the image handle
  //
  DEBUG (( DEBUG_INFO,
            "ImageHandle: 0x%08x\r\n",
            ImageHandle ));

  //
  //  Enable unload support
  //
  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&pLoadedImage
                  );
  if (!EFI_ERROR (Status)) {
    pLoadedImage->Unload = DriverUnload;

    //
    //  Add the driver to the list of drivers
    //
    Status = EfiLibInstallDriverBindingComponentName2 (
               ImageHandle,
               pSystemTable,
               &mDriverBinding,
               ImageHandle,
               &mComponentName,
               &mComponentName2
               );
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

      //
      //  Initialize the service layer
      //
      EslServiceLoad ( ImageHandle );

      //
      //  Make the socket serivces available to other drivers
      //  and applications
      //
      Status = EslDxeInstall ( &ImageHandle );
      if ( EFI_ERROR ( Status )) {
        //
        //  Disconnect from the network
        //
        EslServiceUnload ( );

        //
        //  Remove the driver bindings
        //
        gBS->UninstallMultipleProtocolInterfaces (
                ImageHandle,
                &gEfiDriverBindingProtocolGuid,
                &mDriverBinding,
                &gEfiComponentNameProtocolGuid,
                &mComponentName,
                &gEfiComponentName2ProtocolGuid,
                &mComponentName2,
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
    }
    else  {
      DEBUG (( DEBUG_ERROR | DEBUG_POOL | DEBUG_INIT,
                "ERROR - EfiLibInstallDriverBindingComponentName2 failed, Status: %r\r\n",
                Status ));
    }
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Socket layer's service binding protocol delcaration.
**/
CONST EFI_SERVICE_BINDING_PROTOCOL mEfiServiceBinding = {
  EslDxeCreateChild,
  EslDxeDestroyChild
};


/**
  The following entries disable the constructor and destructor
  for the SocketDxe driver.  Note that socket applications linking
  against EfiSocketLib use different redirection.
**/
PFN_ESL_xSTRUCTOR mpfnEslConstructor = NULL;  ///<  No EfiSocketLib constructor needed for SocketDxe
PFN_ESL_xSTRUCTOR mpfnEslDestructor = NULL;   ///<  No EfiSocketLib destructor needed for SocketDxe
