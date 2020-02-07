/** @file
  The function declaration that provided for Socket Interface.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SOCK_IMPL_H_
#define _SOCK_IMPL_H_

#include "Socket.h"
#include "TcpMain.h"

/**
  Signal a event with the given status.

  @param[in] Token        The token's event is to be signaled.
  @param[in] TokenStatus  The status to be sent with the event.

**/
#define SIGNAL_TOKEN(Token, TokenStatus) \
  do { \
    (Token)->Status = (TokenStatus); \
    gBS->SignalEvent ((Token)->Event); \
  } while (0)

#define SOCK_HEADER_SPACE (60 + 60 + 72)

/**
  Process the TCP send data, buffer the tcp txdata and append
  the buffer to socket send buffer, then try to send it.

  @param[in]  Sock              Pointer to the socket.
  @param[in]  TcpTxData         Pointer to the application provided send buffer.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Failed due to resource limits.

**/
EFI_STATUS
SockProcessTcpSndData (
  IN SOCKET   *Sock,
  IN VOID     *TcpTxData
  );

/**
  Get received data from the socket layer to the receive token.

  @param[in, out]  Sock       Pointer to the socket.
  @param[in, out]  RcvToken   Pointer to the application provided receive token.

  @return The length of data received in this token.

**/
UINT32
SockProcessRcvToken (
  IN OUT SOCKET        *Sock,
  IN OUT SOCK_IO_TOKEN *RcvToken
  );

/**
  Flush the sndBuffer and rcvBuffer of socket.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockConnFlush (
  IN OUT SOCKET *Sock
  );

/**
  Cancel the tokens in the specific token list.

  @param[in]       Token                 Pointer to the Token. If NULL, all tokens
                                         in SpecifiedTokenList will be canceled.
  @param[in, out]  SpecifiedTokenList    Pointer to the token list to be checked.

  @retval EFI_SUCCESS          Cancel the tokens in the specific token listsuccessfully.
  @retval EFI_NOT_FOUND        The Token is not found in SpecifiedTokenList.

**/
EFI_STATUS
SockCancelToken (
  IN     SOCK_COMPLETION_TOKEN  *Token,
  IN OUT LIST_ENTRY             *SpecifiedTokenList
  );

/**
  Create a socket with initial data SockInitData.

  @param[in]  SockInitData          Pointer to the initial data of the socket.

  @return Pointer to the newly created socket, return NULL when exception occurred.

**/
SOCKET *
SockCreate (
  IN SOCK_INIT_DATA *SockInitData
  );

/**
  Destroy a socket.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockDestroy (
  IN OUT SOCKET *Sock
  );

#endif
