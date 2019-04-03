/** @file
  Implement the connection to the EFI socket library

  Copyright (c) 2011, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Socket.h"


/**
  The following GUID values are only used when an application links
  against EfiSocketLib.  An alternative set of values exists in
  SocketDxe\EntryUnload.c which the SocketDxe driver uses to coexist
  with socket applications.
  
  Tag GUID - IPv4 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslIp4ServiceGuid __attribute__((weak)) = {
  0x9c756011, 0x5d44, 0x4ee0, { 0xbc, 0xe7, 0xc3, 0x82, 0x18, 0xfe, 0x39, 0x8d }
};


/**
  Tag GUID - IPv6 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslIp6ServiceGuid __attribute__((weak)) = {
  0xc51b2761, 0xc476, 0x45fe, { 0xbe, 0x61, 0xba, 0x4b, 0xcc, 0x32, 0xf2, 0x34 }
};


/**
  Tag GUID - TCPv4 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslTcp4ServiceGuid __attribute__((weak)) = {
  0xffc659c2, 0x4ef2, 0x4532, { 0xb8, 0x75, 0xcd, 0x9a, 0xa4, 0x27, 0x4c, 0xde }
};


/**
  Tag GUID - TCPv6 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslTcp6ServiceGuid __attribute__((weak)) = {
  0x279858a4, 0x4e9e, 0x4e53, { 0x93, 0x22, 0xf2, 0x54, 0xe0, 0x7e, 0xef, 0xd4 }
};


/**
  Tag GUID - UDPv4 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslUdp4ServiceGuid __attribute__((weak)) = {
  0x44e03a55, 0x8d97, 0x4511, { 0xbf, 0xef, 0xa, 0x8b, 0xc6, 0x2c, 0x25, 0xae }
};


/**
  Tag GUID - UDPv6 in use by an application using EfiSocketLib
**/
CONST EFI_GUID mEslUdp6ServiceGuid __attribute__((weak)) = {
  0xaa4af677, 0x6efe, 0x477c, { 0x96, 0x68, 0xe8, 0x13, 0x9d, 0x2, 0xfd, 0x9b }
};


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
  int RetVal;

  //
  //  Release the socket resources
  //
  EslSocketFree ( pSocketProtocol, &RetVal );

  //
  //  Return the operation status
  //
  return RetVal;
}


/**
  Connect to the EFI socket library

  This routine creates the ::ESL_SOCKET structure and returns
  the API (::EFI_SOCKET_PROTOCOL address) to the socket file
  system layer in BsdSocketLib.

  This routine is called from the ::socket routine in BsdSocketLib
  to create the data structure and initialize the API for a socket.
  Note that this implementation is only used by socket applications
  that link directly to EslSocketLib.

  @param [in] ppSocketProtocol  Address to receive the ::EFI_SOCKET_PROTOCOL
                                structure address

  @return       Value for ::errno, zero (0) indicates success.

 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  )
{
  EFI_HANDLE ChildHandle;
  ESL_SOCKET * pSocket;
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

  This routine is the constructor for the EfiSocketLib when the
  library is linked directly to an application.  This routine
  walks the ::cEslSocketBinding table to create ::ESL_SERVICE
  structures, associated with the network adapters, which this
  routine links to the ::ESL_LAYER structure.

  This routine is called from ::EslConstructor as a result of the
  constructor redirection in ::mpfnEslConstructor at the end of this
  file.

  @retval EFI_SUCCESS   Successfully connected to the network layer

 **/
EFI_STATUS
EslServiceNetworkConnect (
  VOID
  )
{
  BOOLEAN bSomethingFound;
  UINTN HandleCount;
  UINTN Index;
  CONST ESL_SOCKET_BINDING * pEnd;
  EFI_HANDLE * pHandles;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Initialize the socket layer
  //
  Status = EFI_SUCCESS;
  bSomethingFound = FALSE;
  EslServiceLoad ( gImageHandle );

  //
  //  Connect the network devices
  //
  pSocketBinding = &cEslSocketBinding[0];
  pEnd = &pSocketBinding[ cEslSocketBindingEntries ];
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
      DEBUG (( DEBUG_ERROR,
               "ERROR with %s layer, Status: %r\r\n",
               pSocketBinding->pName,
               Status ));
    }
    else {
      if ( NULL != pHandles ) {
        //
        //  Attempt to connect to this network adapter
        //
        for ( Index = 0; HandleCount > Index; Index++ ) {
          Status = EslServiceConnect ( gImageHandle,
                                       pHandles[ Index ]);
          if ( !EFI_ERROR ( Status )) {
            bSomethingFound = TRUE;
          }
          else {
            if ( EFI_OUT_OF_RESOURCES == Status ) {
              //
              //  Pointless to continue without memory
              //
              break;
            }
          }
        }

        //
        //  Done with the handles
        //
        gBS->FreePool ( pHandles );
      }
    }

    //
    //  Set the next network protocol
    //
    pSocketBinding += 1;
  }

  //
  //  Return the network connection status
  //
  if ( bSomethingFound ) {
    Status = EFI_SUCCESS;
  }
  DBG_EXIT_STATUS ( Status );
  return Status;
}


/**
  Disconnect from the network layer

  Destructor for the EfiSocketLib when the library is linked
  directly to an application.  This routine walks the
  ::cEslSocketBinding table to remove the ::ESL_SERVICE
  structures (network connections) from the ::ESL_LAYER structure.

  This routine is called from ::EslDestructor as a result of the
  destructor redirection in ::mpfnEslDestructor at the end of this
  file.

  @retval EFI_SUCCESS   Successfully disconnected from the network layer

 **/
EFI_STATUS
EslServiceNetworkDisconnect (
  VOID
  )
{
  UINTN HandleCount;
  UINTN Index;
  CONST ESL_SOCKET_BINDING * pEnd;
  EFI_HANDLE * pHandles;
  CONST ESL_SOCKET_BINDING * pSocketBinding;
  EFI_STATUS Status;

  DBG_ENTER ( );

  //
  //  Assume success
  //
  Status = EFI_SUCCESS;

  //
  //  Disconnect the network devices
  //
  pSocketBinding = &cEslSocketBinding[0];
  pEnd = &pSocketBinding[ cEslSocketBindingEntries ];
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
    if (( !EFI_ERROR ( Status ))
      && ( NULL != pHandles )) {
      //
      //  Attempt to disconnect from this network adapter
      //
      for ( Index = 0; HandleCount > Index; Index++ ) {
        Status = EslServiceDisconnect ( gImageHandle,
                                        pHandles[ Index ]);
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
    Status = EFI_SUCCESS;
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


/**
  Socket layer's service binding protocol delcaration.
**/
CONST EFI_SERVICE_BINDING_PROTOCOL mEfiServiceBinding __attribute__((weak)) = {
  NULL,
  NULL
};


/**
  The following entries redirect the constructor and destructor
  for any socket application that links against the EfiSocketLib.
  Note that the SocketDxe driver uses different redirection.
**/
PFN_ESL_xSTRUCTOR mpfnEslConstructor __attribute__((weak)) = EslServiceNetworkConnect;    ///<  Constructor for EfiSocketLib
PFN_ESL_xSTRUCTOR mpfnEslDestructor __attribute__((weak)) = EslServiceNetworkDisconnect;  ///<  Destructor for EfiSocketLib
