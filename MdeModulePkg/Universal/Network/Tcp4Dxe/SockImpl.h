/** @file
  Socket implementation header file.

Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SOCK_IMPL_H_
#define _SOCK_IMPL_H_

#include "Socket.h"
#include "Tcp4Main.h"

/**
  Signal a event with the given status.
  
  @param Token        The token's event is to be signaled.
  @param TokenStatus  The status to be sent with the event.
  
**/
#define SIGNAL_TOKEN(Token, TokenStatus) \
  do { \
    (Token)->Status = (TokenStatus); \
    gBS->SignalEvent ((Token)->Event); \
  } while (0)


/**
  Supporting function for both SockImpl and SockInterface.

  @param Event  The Event this notify function registered to, ignored.
  
**/
VOID
EFIAPI
SockFreeFoo (
  IN EFI_EVENT Event
  );

/**
  Process the TCP send data, buffer the tcp txdata and append
  the buffer to socket send buffer,then try to send it.

  @param  Sock                  Pointer to the socket.
  @param  TcpTxData             Pointer to the tcp txdata.

  @retval EFI_SUCCESS           The operation is completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Failed due to resource limit.

**/
EFI_STATUS
SockProcessTcpSndData (
  IN SOCKET   *Sock,
  IN VOID     *TcpTxData
  );

/**
  Copy data from socket buffer to application provided receive buffer.

  @param  Sock                  Pointer to the socket.
  @param  TcpRxData             Pointer to the application provided receive buffer.
  @param  RcvdBytes             The maximum length of the data can be copied.
  @param  IsOOB                 If TRUE the data is OOB, else the data is normal.

**/
VOID
SockSetTcpRxData (
  IN SOCKET     *Sock,
  IN VOID       *TcpRxData,
  IN UINT32     RcvdBytes,
  IN BOOLEAN    IsOOB
  );

/**
  Get received data from the socket layer to the receive token.

  @param  Sock                  Pointer to the socket.
  @param  RcvToken              Pointer to the application provided receive token.

  @return The length of data received in this token.

**/
UINT32
SockProcessRcvToken (
  IN     SOCKET        *Sock,
  IN OUT SOCK_IO_TOKEN *RcvToken
  );

/**
  Flush the socket.

  @param  Sock                  Pointer to the socket.

**/
VOID
SockConnFlush (
  IN OUT SOCKET *Sock
  );

/**
  Create a socket with initial data SockInitData.

  @param  SockInitData          Pointer to the initial data of the socket.

  @return Pointer to the newly created socket.

**/
SOCKET *
SockCreate (
  IN SOCK_INIT_DATA *SockInitData
  );

/**
  Destroy a socket.

  @param  Sock                  Pointer to the socket.

**/
VOID
SockDestroy (
  IN OUT SOCKET *Sock
  );

#endif
