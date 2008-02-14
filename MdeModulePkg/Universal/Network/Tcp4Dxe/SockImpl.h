/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SockImpl.h

Abstract:


**/

#ifndef _SOCK_IMPL_H_
#define _SOCK_IMPL_H_

#include "Socket.h"


#define SOCK_TRIM_RCV_BUFF(Sock, Len) \
  (NetbufQueTrim ((Sock)->RcvBuffer.DataQueue, (Len)))

#define SIGNAL_TOKEN(Token, TokenStatus) \
  do { \
    (Token)->Status = (TokenStatus); \
    gBS->SignalEvent ((Token)->Event); \
  } while (0)

#define SOCK_HEADER_SPACE (60 + 60 + 72)

//
// Supporting function for both SockImpl and SockInterface
//
VOID
SockFreeFoo (
  IN EFI_EVENT Event
  );

EFI_STATUS
SockProcessTcpSndData (
  IN SOCKET *Sock,
  IN VOID   *TcpTxData
  );

VOID
SockSetTcpRxData (
  IN SOCKET         *Sock,
  IN VOID           *TcpRxData,
  IN UINT32         RcvdBytes,
  IN BOOLEAN        IsOOB
  );

UINT32
SockProcessRcvToken (
  IN SOCKET        *Sock,
  IN SOCK_IO_TOKEN *RcvToken
  );

VOID
SockConnFlush (
  IN SOCKET *Sock
  );

SOCKET  *
SockCreate (
  IN SOCK_INIT_DATA *SockInitData
  );

VOID
SockDestroy (
  IN SOCKET *Sock
  );

#endif
