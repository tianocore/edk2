/** @file
  Implement the socket API.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <SocketInternals.h>


const struct fileops SocketOperations = {
  BslSocketClose,     //  close
  BslSocketRead,      //  read
  BslSocketWrite,     //  write

  //
  //  Not supported
  //
  fnullop_fcntl,      //  fcntl
  BslSocketPoll,      //  poll
  fnullop_flush,      //  flush

  fbadop_stat,        //  stat
  fbadop_ioctl,       //  ioctl
  fbadop_delete,      //  delete
  fbadop_rmdir,       //  rmdir
  fbadop_mkdir,       //  mkdir
  fbadop_rename,      //  rename

  NULL                //  lseek
};


/**
  Translate from the socket file descriptor to the socket protocol.

  @param [in] s             Socket file descriptor returned from ::socket.

  @param [in] ppDescriptor  Address to receive the descriptor structure
                            address for the file
  @param [in] pErrno        Address of the errno variable

  @return   A pointer to the socket protocol structure or NULL if
            an invalid file descriptor was passed in.

 **/
EFI_SOCKET_PROTOCOL *
BslFdToSocketProtocol (
  int s,
  struct __filedes ** ppDescriptor,
  int * pErrno
  )
{
  struct __filedes * pDescriptor;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //
  //  Assume failure
  //
  pSocketProtocol = NULL;

  //
  //  Validate the file descriptor
  //
  if ( !ValidateFD ( s, TRUE )) {
    //
    //  Bad file descriptor
    //
    *pErrno = EBADF;
  }
  else {
    //
    //  Get the descriptor for the file
    //
    pDescriptor = &gMD->fdarray [ s ];

    //
    //  Validate that the descriptor is associated with sockets
    //
    pSocketProtocol = BslValidateSocketFd ( pDescriptor, pErrno );
    if (( NULL != ppDescriptor ) && ( NULL != pSocketProtocol )) {
      *ppDescriptor = pDescriptor;
    }
  }

  //
  //  Return the socket protocol
  //
  return pSocketProtocol;
}


/**
  Build a file descriptor for a socket.

  @param [in] pSocketProtocol   Socket protocol structure address

  @param [in] pErrno            Address of the errno variable

  @return  The file descriptor for the socket or -1 if an error occurs.

 **/
int
BslSocketProtocolToFd (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  )
{
  int FileDescriptor;
  struct __filedes * pDescriptor;

  //
  //  Assume failure
  //
  FileDescriptor = -1;

  //
  //  Locate a file descriptor
  //
  FileDescriptor = FindFreeFD ( VALID_CLOSED );
  if( FileDescriptor < 0 ) {
    //
    // All available FDs are in use
    //
    errno = EMFILE;
  }
  else {
    //
    //  Initialize the file descriptor
    //
    pDescriptor = &gMD->fdarray [ FileDescriptor ];
    pDescriptor->f_offset = 0;
    pDescriptor->f_flag = 0;
    pDescriptor->f_iflags = DTYPE_SOCKET;
    pDescriptor->MyFD = (UINT16)FileDescriptor;
    pDescriptor->Oflags = 0;
    pDescriptor->Omode = S_ACC_READ | S_ACC_WRITE;
    pDescriptor->RefCount = 1;
    FILE_SET_MATURE ( pDescriptor );

    //
    //  Socket specific file descriptor initialization
    //
    pDescriptor->devdata = pSocketProtocol;
    pDescriptor->f_ops = &SocketOperations;
  }

  //
  //  Return the socket's file descriptor
  //
  return FileDescriptor;
}


/**
  Creates an endpoint for network communication.

  The ::Socket routine initializes the communication endpoint by providing
  the support for the socket library function ::socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html">POSIX</a>
  documentation for the socket routine is available online for reference.

  @param [in] domain    Select the family of protocols for the client or server
                        application.

  @param [in] type      Specifies how to make the network connection.  The following values
                        are supported:
                        <ul>
                          <li>
                            SOCK_STREAM - Connect to TCP, provides a byte stream
                            that is manipluated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_SEQPACKET - Connect to TCP, provides sequenced packet stream
                            that is manipulated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_DGRAM - Connect to UDP, provides a datagram service that is
                            manipulated by recvfrom and sendto.
                          </li>
                        </ul>

  @param [in] protocol  Specifies the lower layer protocol to use.  The following
                        values are supported:
                        <ul>
                          <li>IPPROTO_TCP</li> - This value must be combined with SOCK_STREAM.</li>
                          <li>IPPROTO_UDP</li> - This value must be combined with SOCK_DGRAM.</li>
                        </ul>

  @return  This routine returns a file descriptor for the socket.

 **/
INT32
socket (
  IN INT32 domain,
  IN INT32 type,
  IN INT32 protocol
  )
{
  INT32 FileDescriptor;
  EFI_SOCKET_PROTOCOL * pSocketProtocol;
  EFI_STATUS Status;

  //
  //  Assume failure
  //
  FileDescriptor = -1;

  //
  //  Locate the socket protocol
  //
  errno = EslServiceGetProtocol ( &pSocketProtocol );
  if ( 0 == errno ) {
    //
    //  Initialize the socket
    //
    Status = pSocketProtocol->pfnSocket ( pSocketProtocol,
                                          domain,
                                          type,
                                          protocol,
                                          &errno );
    if ( !EFI_ERROR ( Status ))
    {
      //
      //  Build the file descriptor for the socket
      //
      FileDescriptor = BslSocketProtocolToFd ( pSocketProtocol,
                                               &errno );
    }
  }

  //
  //  Return the socket's file descriptor
  //
  return FileDescriptor;
}


/**
  Validate the socket's file descriptor

  @param [in] pDescriptor Descriptor for the file

  @param [in] pErrno      Address of the errno variable

  @return   A pointer to the socket protocol structure or NULL if
            an invalid file descriptor was passed in.

 **/
EFI_SOCKET_PROTOCOL *
BslValidateSocketFd (
  struct __filedes * pDescriptor,
  int * pErrno
  )
{
  EFI_SOCKET_PROTOCOL * pSocketProtocol;

  //
  //  Assume failure
  //
  *pErrno = ENOTSOCK;
  pSocketProtocol = NULL;

  //
  //  Validate that the descriptor is associated with sockets
  //
  if ( DTYPE_SOCKET == ( pDescriptor->f_iflags & DTYPE_MASK )) {
    //
    //  Locate the socket protocol
    //
    pSocketProtocol = pDescriptor->devdata;
    *pErrno = 0;
  }

  //
  //  Return the socket protocol
  //
  return pSocketProtocol;
}
