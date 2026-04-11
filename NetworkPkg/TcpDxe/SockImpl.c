/** @file
  Implementation of the Socket.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SockImpl.h"

/**
  Get the first buffer block in the specific socket buffer.

  @param[in]  Sockbuf               Pointer to the socket buffer.

  @return Pointer to the first buffer in the queue. NULL if the queue is empty.

**/
NET_BUF *
SockBufFirst (
  IN SOCK_BUFFER  *Sockbuf
  )
{
  LIST_ENTRY  *NetbufList;

  NetbufList = &(Sockbuf->DataQueue->BufList);

  if (IsListEmpty (NetbufList)) {
    return NULL;
  }

  return NET_LIST_HEAD (NetbufList, NET_BUF, List);
}

/**
  Get the next buffer block in the specific socket buffer.

  @param[in]  Sockbuf     Pointer to the socket buffer.
  @param[in]  SockEntry   Pointer to the buffer block prior to the required one.

  @return Pointer to the buffer block next to SockEntry. NULL if SockEntry is
          the tail or head entry.

**/
NET_BUF *
SockBufNext (
  IN SOCK_BUFFER  *Sockbuf,
  IN NET_BUF      *SockEntry
  )
{
  LIST_ENTRY  *NetbufList;

  NetbufList = &(Sockbuf->DataQueue->BufList);

  if ((SockEntry->List.ForwardLink == NetbufList) ||
      (SockEntry->List.BackLink == &SockEntry->List) ||
      (SockEntry->List.ForwardLink == &SockEntry->List)
      )
  {
    return NULL;
  }

  return NET_LIST_USER_STRUCT (SockEntry->List.ForwardLink, NET_BUF, List);
}

/**
  User provided callback function for NetbufFromExt.

  @param[in] Arg      The Arg parameter forwarded by NetbufFromExt(). Ignored.

**/
VOID
EFIAPI
SockFreeFoo (
  IN VOID  *Arg
  )
{
  return;
}

/**
  Get the length of the data that can be retrieved from the socket
  receive buffer.

  @param[in]  SockBuffer            Pointer to the socket receive buffer.
  @param[out] IsUrg                 Pointer to a BOOLEAN variable.
                                    If TRUE the data is OOB.
  @param[in]  BufLen                The maximum length of the data buffer to
                                    store the received data in the socket layer.

  @return The length of the data can be retrieved.

**/
UINT32
SockTcpDataToRcv (
  IN  SOCK_BUFFER  *SockBuffer,
  OUT BOOLEAN      *IsUrg,
  IN  UINT32       BufLen
  )
{
  NET_BUF       *RcvBufEntry;
  UINT32        DataLen;
  TCP_RSV_DATA  *TcpRsvData;
  BOOLEAN       Urg;

  ASSERT ((SockBuffer != NULL) && (IsUrg != NULL) && (BufLen > 0));

  //
  // Get the first socket receive buffer
  //
  RcvBufEntry = SockBufFirst (SockBuffer);
  ASSERT (RcvBufEntry != NULL);

  TcpRsvData = (TCP_RSV_DATA *)RcvBufEntry->ProtoData;

  //
  // Check whether the receive data is out of bound. If yes, calculate the maximum
  // allowed length of the urgent data and output it.
  //
  *IsUrg = (BOOLEAN)((TcpRsvData->UrgLen > 0) ? TRUE : FALSE);

  if (*IsUrg && (TcpRsvData->UrgLen < RcvBufEntry->TotalSize)) {
    DataLen = MIN (TcpRsvData->UrgLen, BufLen);

    if (DataLen < TcpRsvData->UrgLen) {
      TcpRsvData->UrgLen = TcpRsvData->UrgLen - DataLen;
    } else {
      TcpRsvData->UrgLen = 0;
    }

    return DataLen;
  }

  //
  // Process the next socket receive buffer to get the maximum allowed length
  // of the received data.
  //
  DataLen = RcvBufEntry->TotalSize;

  RcvBufEntry = SockBufNext (SockBuffer, RcvBufEntry);

  while ((BufLen > DataLen) && (RcvBufEntry != NULL)) {
    TcpRsvData = (TCP_RSV_DATA *)RcvBufEntry->ProtoData;

    Urg = (BOOLEAN)((TcpRsvData->UrgLen > 0) ? TRUE : FALSE);

    if (*IsUrg != Urg) {
      break;
    }

    if (*IsUrg && (TcpRsvData->UrgLen < RcvBufEntry->TotalSize)) {
      if (TcpRsvData->UrgLen + DataLen < BufLen) {
        TcpRsvData->UrgLen = 0;
      } else {
        TcpRsvData->UrgLen = TcpRsvData->UrgLen - (BufLen - DataLen);
      }

      return MIN (TcpRsvData->UrgLen + DataLen, BufLen);
    }

    DataLen += RcvBufEntry->TotalSize;

    RcvBufEntry = SockBufNext (SockBuffer, RcvBufEntry);
  }

  DataLen = MIN (BufLen, DataLen);
  return DataLen;
}

/**
  Copy data from socket buffer to an application provided receive buffer.

  @param[in]  Sock        Pointer to the socket.
  @param[in]  TcpRxData   Pointer to the application provided receive buffer.
  @param[in]  RcvdBytes   The maximum length of the data can be copied.
  @param[in]  IsUrg       If TRUE the data is Out of Bound, FALSE the data is normal.

**/
VOID
SockSetTcpRxData (
  IN SOCKET   *Sock,
  IN VOID     *TcpRxData,
  IN UINT32   RcvdBytes,
  IN BOOLEAN  IsUrg
  )
{
  UINT32                  Index;
  UINT32                  CopyBytes;
  UINT32                  OffSet;
  EFI_TCP4_RECEIVE_DATA   *RxData;
  EFI_TCP4_FRAGMENT_DATA  *Fragment;

  RxData = (EFI_TCP4_RECEIVE_DATA *)TcpRxData;

  OffSet = 0;

  ASSERT (RxData->DataLength >= RcvdBytes);

  RxData->DataLength = RcvdBytes;
  RxData->UrgentFlag = IsUrg;

  //
  // Copy the CopyBytes data from socket receive buffer to RxData.
  //
  for (Index = 0; (Index < RxData->FragmentCount) && (RcvdBytes > 0); Index++) {
    Fragment  = &RxData->FragmentTable[Index];
    CopyBytes = MIN ((UINT32)(Fragment->FragmentLength), RcvdBytes);

    NetbufQueCopy (
      Sock->RcvBuffer.DataQueue,
      OffSet,
      CopyBytes,
      Fragment->FragmentBuffer
      );

    Fragment->FragmentLength = CopyBytes;
    RcvdBytes               -= CopyBytes;
    OffSet                  += CopyBytes;
  }
}

/**
  Process the send token.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockProcessSndToken (
  IN OUT SOCKET  *Sock
  )
{
  UINT32                  FreeSpace;
  SOCK_TOKEN              *SockToken;
  UINT32                  DataLen;
  SOCK_IO_TOKEN           *SndToken;
  EFI_TCP4_TRANSMIT_DATA  *TxData;
  EFI_STATUS              Status;

  ASSERT ((Sock != NULL) && (SockStream == Sock->Type));

  FreeSpace = SockGetFreeSpace (Sock, SOCK_SND_BUF);

  //
  // to determine if process a send token using
  // socket layer flow control policy
  //
  while ((FreeSpace >= Sock->SndBuffer.LowWater) && !IsListEmpty (&Sock->SndTokenList)) {
    SockToken = NET_LIST_HEAD (
                  &(Sock->SndTokenList),
                  SOCK_TOKEN,
                  TokenList
                  );

    //
    // process this token
    //
    RemoveEntryList (&(SockToken->TokenList));
    InsertTailList (
      &(Sock->ProcessingSndTokenList),
      &(SockToken->TokenList)
      );

    //
    // Process it in the light of SockType
    //
    SndToken = (SOCK_IO_TOKEN *)SockToken->Token;
    TxData   = SndToken->Packet.TxData;

    DataLen = TxData->DataLength;
    Status  = SockProcessTcpSndData (Sock, TxData);

    if (EFI_ERROR (Status)) {
      goto OnError;
    }

    if (DataLen >= FreeSpace) {
      FreeSpace = 0;
    } else {
      FreeSpace -= DataLen;
    }
  }

  return;

OnError:

  RemoveEntryList (&SockToken->TokenList);
  SIGNAL_TOKEN (SockToken->Token, Status);
  FreePool (SockToken);
}

/**
  Get received data from the socket layer to the receive token.

  @param[in, out]  Sock       Pointer to the socket.
  @param[in, out]  RcvToken   Pointer to the application provided receive token.

  @return The length of data received in this token.

**/
UINT32
SockProcessRcvToken (
  IN OUT SOCKET         *Sock,
  IN OUT SOCK_IO_TOKEN  *RcvToken
  )
{
  UINT32                 TokenRcvdBytes;
  EFI_TCP4_RECEIVE_DATA  *RxData;
  BOOLEAN                IsUrg;

  ASSERT (Sock != NULL);

  ASSERT (SockStream == Sock->Type);

  RxData = RcvToken->Packet.RxData;

  TokenRcvdBytes = SockTcpDataToRcv (
                     &Sock->RcvBuffer,
                     &IsUrg,
                     RxData->DataLength
                     );

  //
  // Copy data from RcvBuffer of socket to user
  // provided RxData and set the fields in TCP RxData
  //
  SockSetTcpRxData (Sock, RxData, TokenRcvdBytes, IsUrg);

  NetbufQueTrim (Sock->RcvBuffer.DataQueue, TokenRcvdBytes);
  SIGNAL_TOKEN (&(RcvToken->Token), EFI_SUCCESS);

  return TokenRcvdBytes;
}

/**
  Process the TCP send data, buffer the tcp txdata, and append
  the buffer to socket send buffer, then try to send it.

  @param[in]  Sock              Pointer to the socket.
  @param[in]  TcpTxData         Pointer to the application provided send buffer.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Failed due to resource limits.

**/
EFI_STATUS
SockProcessTcpSndData (
  IN SOCKET  *Sock,
  IN VOID    *TcpTxData
  )
{
  NET_BUF                 *SndData;
  EFI_STATUS              Status;
  EFI_TCP4_TRANSMIT_DATA  *TxData;

  TxData = (EFI_TCP4_TRANSMIT_DATA *)TcpTxData;

  //
  // transform this TxData into a NET_BUFFER
  // and insert it into Sock->SndBuffer
  //
  SndData = NetbufFromExt (
              (NET_FRAGMENT *)TxData->FragmentTable,
              TxData->FragmentCount,
              0,
              0,
              SockFreeFoo,
              NULL
              );

  if (NULL == SndData) {
    DEBUG (
      (DEBUG_ERROR,
       "SockKProcessSndData: Failed to call NetBufferFromExt\n")
      );

    return EFI_OUT_OF_RESOURCES;
  }

  NetbufQueAppend (Sock->SndBuffer.DataQueue, SndData);

  //
  // notify the low layer protocol to handle this send token
  //
  if (TxData->Urgent) {
    Status = Sock->ProtoHandler (Sock, SOCK_SNDURG, NULL);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (TxData->Push) {
    Status = Sock->ProtoHandler (Sock, SOCK_SNDPUSH, NULL);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // low layer protocol should really handle the sending
  // process when catching SOCK_SND request
  //
  Status = Sock->ProtoHandler (Sock, SOCK_SND, NULL);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Flush the tokens in the specific token list.

  @param[in]       Sock                  Pointer to the socket.
  @param[in, out]  PendingTokenList      Pointer to the token list to be flushed.

**/
VOID
SockFlushPendingToken (
  IN     SOCKET      *Sock,
  IN OUT LIST_ENTRY  *PendingTokenList
  )
{
  SOCK_TOKEN             *SockToken;
  SOCK_COMPLETION_TOKEN  *Token;

  ASSERT ((Sock != NULL) && (PendingTokenList != NULL));

  while (!IsListEmpty (PendingTokenList)) {
    SockToken = NET_LIST_HEAD (
                  PendingTokenList,
                  SOCK_TOKEN,
                  TokenList
                  );

    Token = SockToken->Token;
    SIGNAL_TOKEN (Token, Sock->SockError);

    RemoveEntryList (&(SockToken->TokenList));
    FreePool (SockToken);
  }
}

/**
  Wake up the connection token while the connection is successfully established,
  then try to process any pending send token.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockWakeConnToken (
  IN OUT SOCKET  *Sock
  )
{
  ASSERT (Sock->ConnectionToken != NULL);

  SIGNAL_TOKEN (Sock->ConnectionToken, EFI_SUCCESS);
  Sock->ConnectionToken = NULL;

  //
  // check to see if some pending send token existed?
  //
  SockProcessSndToken (Sock);
}

/**
  Wake up the listen token while the connection is established successfully.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockWakeListenToken (
  IN OUT SOCKET  *Sock
  )
{
  SOCKET                 *Parent;
  SOCK_TOKEN             *SockToken;
  EFI_TCP4_LISTEN_TOKEN  *ListenToken;

  Parent = Sock->Parent;

  ASSERT ((Parent != NULL) && SOCK_IS_LISTENING (Parent) && SOCK_IS_CONNECTED (Sock));

  if (!IsListEmpty (&Parent->ListenTokenList)) {
    SockToken = NET_LIST_HEAD (
                  &Parent->ListenTokenList,
                  SOCK_TOKEN,
                  TokenList
                  );

    ListenToken                 = (EFI_TCP4_LISTEN_TOKEN *)SockToken->Token;
    ListenToken->NewChildHandle = Sock->SockHandle;

    SIGNAL_TOKEN (&(ListenToken->CompletionToken), EFI_SUCCESS);

    RemoveEntryList (&SockToken->TokenList);
    FreePool (SockToken);

    RemoveEntryList (&Sock->ConnectionList);

    Parent->ConnCnt--;
    DEBUG (
      (DEBUG_NET,
       "SockWakeListenToken: accept a socket, now conncnt is %d",
       Parent->ConnCnt)
      );

    Sock->Parent = NULL;
  }
}

/**
  Wake up the receive token while some data is received.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockWakeRcvToken (
  IN OUT SOCKET  *Sock
  )
{
  UINT32         RcvdBytes;
  UINT32         TokenRcvdBytes;
  SOCK_TOKEN     *SockToken;
  SOCK_IO_TOKEN  *RcvToken;

  ASSERT (Sock->RcvBuffer.DataQueue != NULL);

  RcvdBytes = (Sock->RcvBuffer.DataQueue)->BufSize;

  ASSERT (RcvdBytes > 0);

  while (RcvdBytes > 0 && !IsListEmpty (&Sock->RcvTokenList)) {
    SockToken = NET_LIST_HEAD (
                  &Sock->RcvTokenList,
                  SOCK_TOKEN,
                  TokenList
                  );

    RcvToken       = (SOCK_IO_TOKEN *)SockToken->Token;
    TokenRcvdBytes = SockProcessRcvToken (Sock, RcvToken);

    if (0 == TokenRcvdBytes) {
      return;
    }

    RemoveEntryList (&(SockToken->TokenList));
    FreePool (SockToken);
    RcvdBytes -= TokenRcvdBytes;
  }
}

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
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  *Entry;
  SOCK_TOKEN  *SockToken;

  Status    = EFI_SUCCESS;
  Entry     = NULL;
  SockToken = NULL;

  if (IsListEmpty (SpecifiedTokenList) && (Token != NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Iterate through the SpecifiedTokenList.
  //
  Entry = SpecifiedTokenList->ForwardLink;
  while (Entry != SpecifiedTokenList) {
    SockToken = NET_LIST_USER_STRUCT (Entry, SOCK_TOKEN, TokenList);

    if (Token == NULL) {
      SIGNAL_TOKEN (SockToken->Token, EFI_ABORTED);
      RemoveEntryList (&SockToken->TokenList);
      FreePool (SockToken);

      Entry  = SpecifiedTokenList->ForwardLink;
      Status = EFI_SUCCESS;
    } else {
      if (Token == (VOID *)SockToken->Token) {
        SIGNAL_TOKEN (Token, EFI_ABORTED);
        RemoveEntryList (&(SockToken->TokenList));
        FreePool (SockToken);

        return EFI_SUCCESS;
      }

      Status = EFI_NOT_FOUND;

      Entry = Entry->ForwardLink;
    }
  }

  ASSERT (IsListEmpty (SpecifiedTokenList) || Token != NULL);

  return Status;
}

/**
  Create a socket with initial data SockInitData.

  @param[in]  SockInitData          Pointer to the initial data of the socket.

  @return Pointer to the newly created socket, return NULL when an exception occurs.

**/
SOCKET *
SockCreate (
  IN SOCK_INIT_DATA  *SockInitData
  )
{
  SOCKET      *Sock;
  SOCKET      *Parent;
  EFI_STATUS  Status;
  EFI_GUID    *TcpProtocolGuid;
  UINTN       ProtocolLength;

  ASSERT ((SockInitData != NULL) && (SockInitData->ProtoHandler != NULL));
  ASSERT (SockInitData->Type == SockStream);
  ASSERT ((SockInitData->ProtoData != NULL) && (SockInitData->DataSize <= PROTO_RESERVED_LEN));

  if (SockInitData->IpVersion == IP_VERSION_4) {
    TcpProtocolGuid = &gEfiTcp4ProtocolGuid;
    ProtocolLength  = sizeof (EFI_TCP4_PROTOCOL);
  } else {
    TcpProtocolGuid = &gEfiTcp6ProtocolGuid;
    ProtocolLength  = sizeof (EFI_TCP6_PROTOCOL);
  }

  Parent = SockInitData->Parent;

  if ((Parent != NULL) && (Parent->ConnCnt == Parent->BackLog)) {
    DEBUG (
      (DEBUG_ERROR,
       "SockCreate: Socket parent has reached its connection limit with %d ConnCnt and %d BackLog\n",
       Parent->ConnCnt,
       Parent->BackLog)
      );

    return NULL;
  }

  Sock = AllocateZeroPool (sizeof (SOCKET));
  if (NULL == Sock) {
    DEBUG ((DEBUG_ERROR, "SockCreate: No resource to create a new socket\n"));
    return NULL;
  }

  InitializeListHead (&Sock->Link);
  InitializeListHead (&Sock->ConnectionList);
  InitializeListHead (&Sock->ListenTokenList);
  InitializeListHead (&Sock->RcvTokenList);
  InitializeListHead (&Sock->SndTokenList);
  InitializeListHead (&Sock->ProcessingSndTokenList);

  EfiInitializeLock (&(Sock->Lock), TPL_CALLBACK);

  Sock->SndBuffer.DataQueue = NetbufQueAlloc ();
  if (NULL == Sock->SndBuffer.DataQueue) {
    DEBUG (
      (DEBUG_ERROR,
       "SockCreate: No resource to allocate SndBuffer for new socket\n")
      );

    goto OnError;
  }

  Sock->RcvBuffer.DataQueue = NetbufQueAlloc ();
  if (NULL == Sock->RcvBuffer.DataQueue) {
    DEBUG (
      (DEBUG_ERROR,
       "SockCreate: No resource to allocate RcvBuffer for new socket\n")
      );

    goto OnError;
  }

  Sock->Signature = SOCK_SIGNATURE;

  Sock->Parent              = Parent;
  Sock->BackLog             = SockInitData->BackLog;
  Sock->ProtoHandler        = SockInitData->ProtoHandler;
  Sock->SndBuffer.HighWater = SockInitData->SndBufferSize;
  Sock->RcvBuffer.HighWater = SockInitData->RcvBufferSize;
  Sock->Type                = SockInitData->Type;
  Sock->DriverBinding       = SockInitData->DriverBinding;
  Sock->State               = SockInitData->State;
  Sock->CreateCallback      = SockInitData->CreateCallback;
  Sock->DestroyCallback     = SockInitData->DestroyCallback;
  Sock->Context             = SockInitData->Context;

  Sock->SockError          = EFI_ABORTED;
  Sock->SndBuffer.LowWater = SOCK_BUFF_LOW_WATER;
  Sock->RcvBuffer.LowWater = SOCK_BUFF_LOW_WATER;

  Sock->IpVersion = SockInitData->IpVersion;

  //
  // Install protocol on Sock->SockHandle
  //
  CopyMem (&Sock->NetProtocol, SockInitData->Protocol, ProtocolLength);

  //
  // copy the protodata into socket
  //
  CopyMem (Sock->ProtoReserved, SockInitData->ProtoData, SockInitData->DataSize);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Sock->SockHandle,
                  TcpProtocolGuid,
                  &Sock->NetProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG (
      (DEBUG_ERROR,
       "SockCreate: Install TCP protocol in socket failed with %r\n",
       Status)
      );

    goto OnError;
  }

  if (Parent != NULL) {
    ASSERT (Parent->BackLog > 0);
    ASSERT (SOCK_IS_LISTENING (Parent));

    //
    // need to add it into Parent->ConnectionList
    // if the Parent->ConnCnt < Parent->BackLog
    //
    Parent->ConnCnt++;

    DEBUG (
      (DEBUG_NET,
       "SockCreate: Create a new socket and add to parent, now conncnt is %d\n",
       Parent->ConnCnt)
      );

    InsertTailList (&Parent->ConnectionList, &Sock->ConnectionList);
  }

  if (Sock->CreateCallback != NULL) {
    Status = Sock->CreateCallback (Sock, Sock->Context);
    if (EFI_ERROR (Status)) {
      goto OnError;
    }
  }

  return Sock;

OnError:

  if (Sock->SockHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Sock->SockHandle,
           TcpProtocolGuid,
           &Sock->NetProtocol,
           NULL
           );
  }

  if (NULL != Sock->SndBuffer.DataQueue) {
    NetbufQueFree (Sock->SndBuffer.DataQueue);
  }

  if (NULL != Sock->RcvBuffer.DataQueue) {
    NetbufQueFree (Sock->RcvBuffer.DataQueue);
  }

  FreePool (Sock);

  return NULL;
}

/**
  Destroy a socket.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockDestroy (
  IN OUT SOCKET  *Sock
  )
{
  ASSERT (SockStream == Sock->Type);

  //
  // Flush the completion token buffered
  // by sock and rcv, snd buffer
  //
  if (!SOCK_IS_UNCONFIGURED (Sock)) {
    SockConnFlush (Sock);
    SockSetState (Sock, SO_CLOSED);
    Sock->ConfigureState = SO_UNCONFIGURED;
  }

  //
  // Destroy the RcvBuffer Queue and SendBuffer Queue
  //
  NetbufQueFree (Sock->RcvBuffer.DataQueue);
  NetbufQueFree (Sock->SndBuffer.DataQueue);

  //
  // Remove it from parent connection list if needed
  //
  if (Sock->Parent != NULL) {
    RemoveEntryList (&(Sock->ConnectionList));
    (Sock->Parent->ConnCnt)--;

    DEBUG (
      (DEBUG_WARN,
       "SockDestroy: Delete a unaccepted socket from parent now conncnt is %d\n",
       Sock->Parent->ConnCnt)
      );

    Sock->Parent = NULL;
  }

  FreePool (Sock);
}

/**
  Flush the sndBuffer and rcvBuffer of socket.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockConnFlush (
  IN OUT SOCKET  *Sock
  )
{
  SOCKET  *Child;

  ASSERT (Sock != NULL);

  //
  // Clear the flag in this socket
  //
  Sock->Flag = 0;

  //
  // Flush the SndBuffer and RcvBuffer of Sock
  //
  NetbufQueFlush (Sock->SndBuffer.DataQueue);
  NetbufQueFlush (Sock->RcvBuffer.DataQueue);

  //
  // Signal the pending token
  //
  if (Sock->ConnectionToken != NULL) {
    SIGNAL_TOKEN (Sock->ConnectionToken, Sock->SockError);
    Sock->ConnectionToken = NULL;
  }

  if (Sock->CloseToken != NULL) {
    SIGNAL_TOKEN (Sock->CloseToken, Sock->SockError);
    Sock->CloseToken = NULL;
  }

  SockFlushPendingToken (Sock, &(Sock->ListenTokenList));
  SockFlushPendingToken (Sock, &(Sock->RcvTokenList));
  SockFlushPendingToken (Sock, &(Sock->SndTokenList));
  SockFlushPendingToken (Sock, &(Sock->ProcessingSndTokenList));

  //
  // Destroy the pending connection, if it is a listening socket
  //
  if (SOCK_IS_LISTENING (Sock)) {
    while (!IsListEmpty (&Sock->ConnectionList)) {
      Child = NET_LIST_HEAD (
                &Sock->ConnectionList,
                SOCKET,
                ConnectionList
                );

      SockDestroyChild (Child);
    }

    Sock->ConnCnt = 0;
  }
}

/**
  Set the state of the socket.

  @param[in, out]  Sock                  Pointer to the socket.
  @param[in]       State                 The new socket state to be set.

**/
VOID
SockSetState (
  IN OUT SOCKET  *Sock,
  IN     UINT8   State
  )
{
  Sock->State = State;
}

/**
  Clone a new socket, including its associated protocol control block.

  @param[in]  Sock                  Pointer to the socket to be cloned.

  @return Pointer to the newly cloned socket. If NULL, an error condition occurred.

**/
SOCKET *
SockClone (
  IN SOCKET  *Sock
  )
{
  SOCKET          *ClonedSock;
  SOCK_INIT_DATA  InitData;

  InitData.BackLog         = Sock->BackLog;
  InitData.Parent          = Sock;
  InitData.State           = Sock->State;
  InitData.ProtoHandler    = Sock->ProtoHandler;
  InitData.Type            = Sock->Type;
  InitData.RcvBufferSize   = Sock->RcvBuffer.HighWater;
  InitData.SndBufferSize   = Sock->SndBuffer.HighWater;
  InitData.DriverBinding   = Sock->DriverBinding;
  InitData.IpVersion       = Sock->IpVersion;
  InitData.Protocol        = &(Sock->NetProtocol);
  InitData.CreateCallback  = Sock->CreateCallback;
  InitData.DestroyCallback = Sock->DestroyCallback;
  InitData.Context         = Sock->Context;
  InitData.ProtoData       = Sock->ProtoReserved;
  InitData.DataSize        = sizeof (Sock->ProtoReserved);

  ClonedSock = SockCreate (&InitData);

  if (NULL == ClonedSock) {
    DEBUG ((DEBUG_ERROR, "SockClone: no resource to create a cloned sock\n"));
    return NULL;
  }

  SockSetState (ClonedSock, SO_CONNECTING);
  ClonedSock->ConfigureState = Sock->ConfigureState;

  return ClonedSock;
}

/**
  Called by the low layer protocol to indicate the socket a connection is
  established.

  This function just changes the socket's state to SO_CONNECTED
  and signals the token used for connection establishment.

  @param[in, out]  Sock         Pointer to the socket associated with the
                                established connection.

**/
VOID
SockConnEstablished (
  IN OUT SOCKET  *Sock
  )
{
  ASSERT (SO_CONNECTING == Sock->State);

  SockSetState (Sock, SO_CONNECTED);

  if (NULL == Sock->Parent) {
    SockWakeConnToken (Sock);
  } else {
    SockWakeListenToken (Sock);
  }
}

/**
  Called by the low layer protocol to indicate the connection is closed.

  This function flushes the socket, sets the state to SO_CLOSED, and signals
  the close token.

  @param[in, out]  Sock         Pointer to the socket associated with the closed
                                connection.

**/
VOID
SockConnClosed (
  IN OUT SOCKET  *Sock
  )
{
  if (Sock->CloseToken != NULL) {
    SIGNAL_TOKEN (Sock->CloseToken, EFI_SUCCESS);
    Sock->CloseToken = NULL;
  }

  SockConnFlush (Sock);
  SockSetState (Sock, SO_CLOSED);

  if (Sock->Parent != NULL) {
    SockDestroyChild (Sock);
  }
}

/**
  Called by low layer protocol to indicate that some data was sent or processed.

  This function trims the sent data in the socket send buffer, and signals the data
  token if proper.

  @param[in, out]  Sock      Pointer to the socket.
  @param[in]       Count     The length of the data processed or sent, in bytes.

**/
VOID
SockDataSent (
  IN OUT SOCKET  *Sock,
  IN     UINT32  Count
  )
{
  SOCK_TOKEN             *SockToken;
  SOCK_COMPLETION_TOKEN  *SndToken;

  ASSERT (!IsListEmpty (&Sock->ProcessingSndTokenList));
  ASSERT (Count <= (Sock->SndBuffer.DataQueue)->BufSize);

  NetbufQueTrim (Sock->SndBuffer.DataQueue, Count);

  //
  // To check if we can signal some snd token in this socket
  //
  while (Count > 0) {
    SockToken = NET_LIST_HEAD (
                  &(Sock->ProcessingSndTokenList),
                  SOCK_TOKEN,
                  TokenList
                  );

    SndToken = SockToken->Token;

    if (SockToken->RemainDataLen <= Count) {
      RemoveEntryList (&(SockToken->TokenList));
      SIGNAL_TOKEN (SndToken, EFI_SUCCESS);
      Count -= SockToken->RemainDataLen;
      FreePool (SockToken);
    } else {
      SockToken->RemainDataLen -= Count;
      Count                     = 0;
    }
  }

  //
  // to judge if we can process some send token in
  // Sock->SndTokenList, if so process those send token
  //
  SockProcessSndToken (Sock);
}

/**
  Called by the low layer protocol to copy some data in the socket send
  buffer starting from the specific offset to a buffer provided by
  the caller.

  @param[in]  Sock                  Pointer to the socket.
  @param[in]  Offset                The start point of the data to be copied.
  @param[in]  Len                   The length of the data to be copied.
  @param[out] Dest                  Pointer to the destination to copy the data.

  @return The data size copied.

**/
UINT32
SockGetDataToSend (
  IN  SOCKET  *Sock,
  IN  UINT32  Offset,
  IN  UINT32  Len,
  OUT UINT8   *Dest
  )
{
  ASSERT ((Sock != NULL) && SockStream == Sock->Type);

  return NetbufQueCopy (
           Sock->SndBuffer.DataQueue,
           Offset,
           Len,
           Dest
           );
}

/**
  Called by the low layer protocol to deliver received data to socket layer.

  This function will append the data to the socket receive buffer, set the
  urgent data length, and then check if any receive token can be signaled.

  @param[in, out]  Sock       Pointer to the socket.
  @param[in, out]  NetBuffer  Pointer to the buffer that contains the received data.
  @param[in]       UrgLen     The length of the urgent data in the received data.

**/
VOID
SockDataRcvd (
  IN OUT SOCKET   *Sock,
  IN OUT NET_BUF  *NetBuffer,
  IN     UINT32   UrgLen
  )
{
  ASSERT (
    (Sock != NULL) && (Sock->RcvBuffer.DataQueue != NULL) &&
    UrgLen <= NetBuffer->TotalSize
    );

  NET_GET_REF (NetBuffer);

  ((TCP_RSV_DATA *)(NetBuffer->ProtoData))->UrgLen = UrgLen;

  NetbufQueAppend (Sock->RcvBuffer.DataQueue, NetBuffer);

  SockWakeRcvToken (Sock);
}

/**
  Get the length of the free space of the specific socket buffer.

  @param[in]  Sock              Pointer to the socket.
  @param[in]  Which             Flag to indicate which socket buffer to check:
                                either send buffer or receive buffer.

  @return The length of the free space, in bytes.

**/
UINT32
SockGetFreeSpace (
  IN SOCKET  *Sock,
  IN UINT32  Which
  )
{
  UINT32       BufferCC;
  SOCK_BUFFER  *SockBuffer;

  ASSERT (Sock != NULL && ((SOCK_SND_BUF == Which) || (SOCK_RCV_BUF == Which)));

  if (SOCK_SND_BUF == Which) {
    SockBuffer = &(Sock->SndBuffer);
  } else {
    SockBuffer = &(Sock->RcvBuffer);
  }

  BufferCC = (SockBuffer->DataQueue)->BufSize;

  if (BufferCC >= SockBuffer->HighWater) {
    return 0;
  }

  return SockBuffer->HighWater - BufferCC;
}

/**
  Called by the low layer protocol to indicate that there will be no more data
  from the communication peer.

  This function sets the socket's state to SO_NO_MORE_DATA and signals all queued
  IO tokens with the error status EFI_CONNECTION_FIN.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockNoMoreData (
  IN OUT SOCKET  *Sock
  )
{
  EFI_STATUS  Err;

  SOCK_NO_MORE_DATA (Sock);

  if (!IsListEmpty (&Sock->RcvTokenList)) {
    ASSERT (0 == GET_RCV_DATASIZE (Sock));

    Err = Sock->SockError;

    SOCK_ERROR (Sock, EFI_CONNECTION_FIN);

    SockFlushPendingToken (Sock, &Sock->RcvTokenList);

    SOCK_ERROR (Sock, Err);
  }
}
