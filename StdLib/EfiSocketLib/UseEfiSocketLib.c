/** @file
  Implement the connection to the EFI socket library

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Socket.h"


CONST EFI_GUID mEslRawServiceGuid = {
  0xc31bf4a5, 0x2c7, 0x49d2, { 0xa5, 0x58, 0xfe, 0x62, 0x6f, 0x7e, 0xd4, 0x77 }
};

CONST EFI_GUID mEslTcp4ServiceGuid = {
  0xffc659c2, 0x4ef2, 0x4532, { 0xb8, 0x75, 0xcd, 0x9a, 0xa4, 0x27, 0x4c, 0xde }
};

CONST EFI_GUID mEslUdp4ServiceGuid = {
  0x44e03a55, 0x8d97, 0x4511, { 0xbf, 0xef, 0xa, 0x8b, 0xc6, 0x2c, 0x25, 0xae }
};


/**
  Connect to the EFI socket library

  @param [in] ppSocketProtocol  Address to receive the socket protocol address

  @retval 0             Successfully returned the socket protocol
  @retval other         Value for errno
 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  )
{
  EFI_HANDLE ChildHandle;
  DT_SOCKET * pSocket;
  int RetVal;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  RetVal = 0;

  //
  //  Locate the socket protocol
  //
  ChildHandle = NULL;
  Status = EslSocketAllocate ( &ChildHandle,
                               DEBUG_SOCKET,
                               &pSocket );
  if ( !EFI_ERROR ( Status )) {
    *ppSocketProtocol = &pSocket->SocketProtocol;
  }
  else {
    //
    //  No resources
    //
    RetVal = ENOMEM;
  }

  //
  //  Return the operation status
  //
  DBG_EXIT_DEC ( RetVal );
  return RetVal;
}


/**
  Connect to the network layer

  @retval EFI_SUCCESS   Successfully connected to the network layer

 **/
EFI_STATUS
EslServiceNetworkConnect (
  VOID
  )
{
  UINTN HandleCount;
  EFI_HANDLE * pHandles;
  UINTN Index;
  CONST DT_SOCKET_BINDING * pSocketBinding;
  CONST DT_SOCKET_BINDING * pEnd;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Initialize the socket layer
  //
  Status = EFI_SUCCESS;
  EslServiceLoad ( gImageHandle );

  //
  //  Connect the network devices
  //
  pSocketBinding = &cEslSocketBinding [0];
  pEnd = &pSocketBinding [ cEslSocketBindingEntries ];
  while ( pEnd > pSocketBinding ) {
    //
    //  Attempt to locate the network adapters
    //
    HandleCount = 0;
    pHandles = NULL;
    Status = gBS->LocateHandleBuffer ( ByProtocol,
                                       pSocketBinding->pNetworkBinding,
                                       NULL,
                                       &HandleCount,
                                       &pHandles );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pHandles ) {
      //
      //  Attempt to connect to this network adapter
      //
      for ( Index = 0; HandleCount > Index; Index++ ) {
        Status = EslServiceConnect ( gImageHandle,
                                     pHandles [ Index ]);
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }

      //
      //  Done with the handles
      //
      gBS->FreePool ( pHandles );
    }

    //
    //  Set the next network protocol
    //
    pSocketBinding += 1;
  }

  //
  //  Return the network connection status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Disconnect from the network layer

  @retval EFI_SUCCESS   Successfully disconnected from the network layer

 **/
EFI_STATUS
EslServiceNetworkDisconnect (
  VOID
  )
{
  UINTN HandleCount;
  EFI_HANDLE * pHandles;
  UINTN Index;
  CONST DT_SOCKET_BINDING * pSocketBinding;
  CONST DT_SOCKET_BINDING * pEnd;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Disconnect the network devices
  //
  pSocketBinding = &cEslSocketBinding [0];
  pEnd = &pSocketBinding [ cEslSocketBindingEntries ];
  while ( pEnd > pSocketBinding ) {
    //
    //  Attempt to locate the network adapters
    //
    HandleCount = 0;
    pHandles = NULL;
    Status = gBS->LocateHandleBuffer ( ByProtocol,
                                       pSocketBinding->pNetworkBinding,
                                       NULL,
                                       &HandleCount,
                                       &pHandles );
    if ( EFI_ERROR ( Status )) {
      break;
    }
    if ( NULL != pHandles ) {
      //
      //  Attempt to disconnect from this network adapter
      //
      for ( Index = 0; HandleCount > Index; Index++ ) {
        Status = EslServiceDisconnect ( gImageHandle,
                                        pHandles [ Index ]);
        if ( EFI_ERROR ( Status )) {
          break;
        }
      }

      //
      //  Done with the handles
      //
      gBS->FreePool ( pHandles );
    }

    //
    //  Set the next network protocol
    //
    pSocketBinding += 1;
  }

  //
  //  Finish the disconnect operation
  //
  if ( !EFI_ERROR ( Status )) {
    EslServiceUnload ( );
  }

  //
  //  Return the network connection status
  //
  DBG_EXIT_STATUS ( Status );
  return Status;
}


PFN_ESL_xSTRUCTOR mpfnEslConstructor = EslServiceNetworkConnect;
PFN_ESL_xSTRUCTOR mpfnEslDestructor = EslServiceNetworkDisconnect;
