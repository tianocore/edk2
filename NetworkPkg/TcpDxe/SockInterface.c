/** @file
  Interface function of the Socket.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SockImpl.h"

/**
  Check whether the Event is in the List.

  @param[in]  List             Pointer to the token list to be searched.
  @param[in]  Event            The event to be checked.

  @retval  TRUE                The specific Event exists in the List.
  @retval  FALSE               The specific Event is not in the List.

**/
BOOLEAN
SockTokenExistedInList (
  IN LIST_ENTRY     *List,
  IN EFI_EVENT      Event
  )
{
  LIST_ENTRY      *ListEntry;
  SOCK_TOKEN      *SockToken;

  NET_LIST_FOR_EACH (ListEntry, List) {
    SockToken = NET_LIST_USER_STRUCT (
                  ListEntry,
                  SOCK_TOKEN,
                  TokenList
                  );

    if (Event == SockToken->Token->Event) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Call SockTokenExistedInList() to check whether the Event is
  in the related socket's lists.

  @param[in]  Sock             Pointer to the instance's socket.
  @param[in]  Event            The event to be checked.

  @retval  TRUE                The Event exists in related socket's lists.
  @retval  FALSE               The Event is not in related socket's lists.

**/
BOOLEAN
SockTokenExisted (
  IN SOCKET    *Sock,
  IN EFI_EVENT Event
  )
{

  if (SockTokenExistedInList (&Sock->SndTokenList, Event) ||
      SockTokenExistedInList (&Sock->ProcessingSndTokenList, Event) ||
      SockTokenExistedInList (&Sock->RcvTokenList, Event) ||
      SockTokenExistedInList (&Sock->ListenTokenList, Event)
        ) {

    return TRUE;
  }

  if ((Sock->ConnectionToken != NULL) && (Sock->ConnectionToken->Event == Event)) {

    return TRUE;
  }

  if ((Sock->CloseToken != NULL) && (Sock->CloseToken->Event == Event)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Buffer a token into the specific list of the socket Sock.

  @param[in]  Sock             Pointer to the instance's socket.
  @param[in]  List             Pointer to the list to store the token.
  @param[in]  Token            Pointer to the token to be buffered.
  @param[in]  DataLen          The data length of the buffer contained in Token.

  @return Pointer to the token that wraps Token. If NULL, an error condition occurred.

**/
SOCK_TOKEN *
SockBufferToken (
  IN SOCKET         *Sock,
  IN LIST_ENTRY     *List,
  IN VOID           *Token,
  IN UINT32         DataLen
  )
{
  SOCK_TOKEN  *SockToken;

  SockToken = AllocateZeroPool (sizeof (SOCK_TOKEN));
  if (NULL == SockToken) {

    DEBUG (
      (EFI_D_ERROR,
      "SockBufferIOToken: No Memory to allocate SockToken\n")
      );

    return NULL;
  }

  SockToken->Sock           = Sock;
  SockToken->Token          = (SOCK_COMPLETION_TOKEN *) Token;
  SockToken->RemainDataLen  = DataLen;
  InsertTailList (List, &SockToken->TokenList);

  return SockToken;
}

/**
  Destroy the socket Sock and its associated protocol control block.

  @param[in, out]  Sock                 The socket to be destroyed.

  @retval EFI_SUCCESS          The socket Sock was destroyed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockDestroyChild (
  IN OUT SOCKET *Sock
  )
{
  EFI_STATUS  Status;

  ASSERT ((Sock != NULL) && (Sock->ProtoHandler != NULL));

  if (Sock->InDestroy) {
    return EFI_SUCCESS;
  }

  Sock->InDestroy = TRUE;

  Status            = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockDestroyChild: Get the lock to access socket failed with %r\n",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  //
  // force protocol layer to detach the PCB
  //
  Status = Sock->ProtoHandler (Sock, SOCK_DETACH, NULL);

  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockDestroyChild: Protocol detach socket failed with %r\n",
      Status)
      );

    Sock->InDestroy = FALSE;
  } else if (SOCK_IS_CONFIGURED (Sock)) {

    SockConnFlush (Sock);
    SockSetState (Sock, SO_CLOSED);

    Sock->ConfigureState = SO_UNCONFIGURED;
  }

  EfiReleaseLock (&(Sock->Lock));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  SockDestroy (Sock);
  return EFI_SUCCESS;
}

/**
  Create a socket and its associated protocol control block
  with the intial data SockInitData and protocol specific
  data ProtoData.

  @param[in]  SockInitData         Inital data to setting the socket.

  @return Pointer to the newly created socket. If NULL, an error condition occured.

**/
SOCKET *
SockCreateChild (
  IN SOCK_INIT_DATA *SockInitData
  )
{
  SOCKET      *Sock;
  EFI_STATUS  Status;

  //
  // create a new socket
  //
  Sock = SockCreate (SockInitData);
  if (NULL == Sock) {

    DEBUG (
      (EFI_D_ERROR,
      "SockCreateChild: No resource to create a new socket\n")
      );

    return NULL;
  }

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockCreateChild: Get the lock to access socket failed with %r\n",
      Status)
      );

    SockDestroy (Sock);
    return NULL;
  }
  //
  // inform the protocol layer to attach the socket
  // with a new protocol control block
  //
  Status = Sock->ProtoHandler (Sock, SOCK_ATTACH, NULL);
  EfiReleaseLock (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockCreateChild: Protocol failed to attach a socket with %r\n",
      Status)
      );

    SockDestroy (Sock);
    Sock = NULL;
  }

  return Sock;
}

/**
  Configure the specific socket Sock using configuration data ConfigData.

  @param[in]  Sock             Pointer to the socket to be configured.
  @param[in]  ConfigData       Pointer to the configuration data.

  @retval EFI_SUCCESS          The socket configured successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is already configured.

**/
EFI_STATUS
SockConfigure (
  IN SOCKET *Sock,
  IN VOID   *ConfigData
  )
{
  EFI_STATUS  Status;

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockConfigure: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_CONFIGURED (Sock)) {
    Status = EFI_ACCESS_DENIED;
    goto OnExit;
  }

  ASSERT (Sock->State == SO_CLOSED);

  Status = Sock->ProtoHandler (Sock, SOCK_CONFIGURE, ConfigData);

OnExit:
  EfiReleaseLock (&(Sock->Lock));

  return Status;
}

/**
  Initiate a connection establishment process.

  @param[in]  Sock             Pointer to the socket to initiate the initate the
                               connection.
  @param[in]  Token            Pointer to the token used for the connection
                               operation.

  @retval EFI_SUCCESS          The connection initialized successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not configured to
                               be an active one, or the token is already in one of
                               this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockConnect (
  IN SOCKET *Sock,
  IN VOID   *Token
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockConnect: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {
    Status = EFI_NO_MAPPING;
    goto OnExit;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {

    Status = EFI_NOT_STARTED;
    goto OnExit;
  }

  if (!SOCK_IS_CLOSED (Sock) || !SOCK_IS_CONFIGURED_ACTIVE (Sock)) {

    Status = EFI_ACCESS_DENIED;
    goto OnExit;
  }

  Event = ((SOCK_COMPLETION_TOKEN *) Token)->Event;

  if (SockTokenExisted (Sock, Event)) {

    Status = EFI_ACCESS_DENIED;
    goto OnExit;
  }

  Sock->ConnectionToken = (SOCK_COMPLETION_TOKEN *) Token;
  SockSetState (Sock, SO_CONNECTING);
  Status = Sock->ProtoHandler (Sock, SOCK_CONNECT, NULL);

OnExit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Issue a listen token to get an existed connected network instance
  or wait for a connection if there is none.

  @param[in]  Sock             Pointer to the socket to accept connections.
  @param[in]  Token            The token to accept a connection.

  @retval EFI_SUCCESS          Either a connection is accpeted or the Token is
                               buffered for further acception.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not configured to
                               be a passive one, or the token is already in one of
                               this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the Token due to memory limits.

**/
EFI_STATUS
SockAccept (
  IN SOCKET *Sock,
  IN VOID   *Token
  )
{
  EFI_TCP4_LISTEN_TOKEN *ListenToken;
  LIST_ENTRY            *ListEntry;
  EFI_STATUS            Status;
  SOCKET                *Socket;
  EFI_EVENT             Event;

  ASSERT (SockStream == Sock->Type);

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockAccept: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {

    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  if (!SOCK_IS_LISTENING (Sock)) {

    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  Event = ((SOCK_COMPLETION_TOKEN *) Token)->Event;

  if (SockTokenExisted (Sock, Event)) {

    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  ListenToken = (EFI_TCP4_LISTEN_TOKEN *) Token;

  //
  // Check if a connection has already in this Sock->ConnectionList
  //
  NET_LIST_FOR_EACH (ListEntry, &Sock->ConnectionList) {

    Socket = NET_LIST_USER_STRUCT (ListEntry, SOCKET, ConnectionList);

    if (SOCK_IS_CONNECTED (Socket)) {
      ListenToken->NewChildHandle = Socket->SockHandle;
      SIGNAL_TOKEN (&(ListenToken->CompletionToken), EFI_SUCCESS);

      RemoveEntryList (ListEntry);

      ASSERT (Socket->Parent != NULL);

      Socket->Parent->ConnCnt--;

      DEBUG (
        (EFI_D_INFO,
        "SockAccept: Accept a socket, now conncount is %d",
        Socket->Parent->ConnCnt)
        );
      Socket->Parent = NULL;

      goto Exit;
    }
  }

  //
  // Buffer this token for latter incoming connection request
  //
  if (NULL == SockBufferToken (Sock, &(Sock->ListenTokenList), Token, 0)) {

    Status = EFI_OUT_OF_RESOURCES;
  }

Exit:
  EfiReleaseLock (&(Sock->Lock));

  return Status;
}

/**
  Issue a token with data to the socket to send out.

  @param[in]  Sock             Pointer to the socket to process the token with
                               data.
  @param[in]  Token            The token with data that needs to send out.

  @retval EFI_SUCCESS          The token processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to memory limits.

**/
EFI_STATUS
SockSend (
  IN SOCKET *Sock,
  IN VOID   *Token
  )
{
  SOCK_IO_TOKEN           *SndToken;
  EFI_EVENT               Event;
  UINT32                  FreeSpace;
  EFI_TCP4_TRANSMIT_DATA  *TxData;
  EFI_STATUS              Status;
  SOCK_TOKEN              *SockToken;
  UINT32                  DataLen;

  ASSERT (SockStream == Sock->Type);

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockSend: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  SndToken  = (SOCK_IO_TOKEN *) Token;
  TxData    = (EFI_TCP4_TRANSMIT_DATA *) SndToken->Packet.TxData;

  if (SOCK_IS_UNCONFIGURED (Sock)) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  if (!(SOCK_IS_CONNECTING (Sock) || SOCK_IS_CONNECTED (Sock))) {

    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  //
  // check if a token is already in the token buffer
  //
  Event = SndToken->Token.Event;

  if (SockTokenExisted (Sock, Event)) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  DataLen = TxData->DataLength;

  //
  // process this sending token now or buffer it only?
  //
  FreeSpace = SockGetFreeSpace (Sock, SOCK_SND_BUF);

  if ((FreeSpace < Sock->SndBuffer.LowWater) || !SOCK_IS_CONNECTED (Sock)) {

    SockToken = SockBufferToken (
                  Sock,
                  &Sock->SndTokenList,
                  SndToken,
                  DataLen
                  );

    if (NULL == SockToken) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {

    SockToken = SockBufferToken (
                  Sock,
                  &Sock->ProcessingSndTokenList,
                  SndToken,
                  DataLen
                  );

    if (NULL == SockToken) {
      DEBUG (
        (EFI_D_ERROR,
        "SockSend: Failed to buffer IO token into socket processing SndToken List\n",
        Status)
        );

      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    Status = SockProcessTcpSndData (Sock, TxData);

    if (EFI_ERROR (Status)) {
      DEBUG (
        (EFI_D_ERROR,
        "SockSend: Failed to process Snd Data\n",
        Status)
        );

      RemoveEntryList (&(SockToken->TokenList));
      FreePool (SockToken);
    }
  }

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Issue a token to get data from the socket.

  @param[in]  Sock             Pointer to the socket to get data from.
  @param[in]  Token            The token to store the received data from the
                               socket.

  @retval EFI_SUCCESS          The token processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_CONNECTION_FIN   The connection is closed and there is no more data.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to memory limit.

**/
EFI_STATUS
SockRcv (
  IN SOCKET *Sock,
  IN VOID   *Token
  )
{
  SOCK_IO_TOKEN *RcvToken;
  UINT32        RcvdBytes;
  EFI_STATUS    Status;
  EFI_EVENT     Event;

  ASSERT (SockStream == Sock->Type);

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockRcv: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {

    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {

    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  if (!(SOCK_IS_CONNECTED (Sock) || SOCK_IS_CONNECTING (Sock))) {

    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  RcvToken = (SOCK_IO_TOKEN *) Token;

  //
  // check if a token is already in the token buffer of this socket
  //
  Event = RcvToken->Token.Event;
  if (SockTokenExisted (Sock, Event)) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  RcvToken  = (SOCK_IO_TOKEN *) Token;
  RcvdBytes = GET_RCV_DATASIZE (Sock);

  //
  // check whether an error has happened before
  //
  if (EFI_ABORTED != Sock->SockError) {

    SIGNAL_TOKEN (&(RcvToken->Token), Sock->SockError);
    Sock->SockError = EFI_ABORTED;
    goto Exit;
  }

  //
  // check whether can not receive and there is no any
  // data buffered in Sock->RcvBuffer
  //
  if (SOCK_IS_NO_MORE_DATA (Sock) && (0 == RcvdBytes)) {

    Status = EFI_CONNECTION_FIN;
    goto Exit;
  }

  if (RcvdBytes != 0) {
    Status = SockProcessRcvToken (Sock, RcvToken);

    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    Status = Sock->ProtoHandler (Sock, SOCK_CONSUMED, NULL);
  } else {

    if (NULL == SockBufferToken (Sock, &Sock->RcvTokenList, RcvToken, 0)) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Reset the socket and its associated protocol control block.

  @param[in, out]  Sock        Pointer to the socket to be flushed.

  @retval EFI_SUCCESS          The socket is flushed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockFlush (
  IN OUT SOCKET *Sock
  )
{
  EFI_STATUS  Status;

  ASSERT (SockStream == Sock->Type);

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockFlush: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (!SOCK_IS_CONFIGURED (Sock)) {

    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  Status = Sock->ProtoHandler (Sock, SOCK_FLUSH, NULL);
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockFlush: Protocol failed handling SOCK_FLUSH with %r",
      Status)
      );

    goto Exit;
  }

  SOCK_ERROR (Sock, EFI_ABORTED);
  SockConnFlush (Sock);
  SockSetState (Sock, SO_CLOSED);

  Sock->ConfigureState = SO_UNCONFIGURED;

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Close or abort the socket associated connection.

  @param[in, out]  Sock        Pointer to the socket of the connection to close
                               or abort.
  @param[in]  Token            The token for a close operation.
  @param[in]  OnAbort          TRUE for aborting the connection; FALSE to close it.

  @retval EFI_SUCCESS          The close or abort operation initialized
                               successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockClose (
  IN OUT SOCKET  *Sock,
  IN     VOID    *Token,
  IN     BOOLEAN OnAbort
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

  ASSERT (SockStream == Sock->Type);

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {
    DEBUG (
      (EFI_D_ERROR,
      "SockClose: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  if (SOCK_IS_DISCONNECTING (Sock)) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  Event = ((SOCK_COMPLETION_TOKEN *) Token)->Event;

  if (SockTokenExisted (Sock, Event)) {
    Status = EFI_ACCESS_DENIED;
    goto Exit;
  }

  Sock->CloseToken = Token;
  SockSetState (Sock, SO_DISCONNECTING);

  if (OnAbort) {
    Status = Sock->ProtoHandler (Sock, SOCK_ABORT, NULL);
  } else {
    Status = Sock->ProtoHandler (Sock, SOCK_CLOSE, NULL);
  }

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Get the mode data of the low layer protocol.

  @param[in]       Sock        Pointer to the socket to get mode data from.
  @param[in, out]  Mode        Pointer to the data to store the low layer mode
                               information.

  @retval EFI_SUCCESS          The mode data was obtained successfully.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockGetMode (
  IN     SOCKET *Sock,
  IN OUT VOID   *Mode
  )
{
  return Sock->ProtoHandler (Sock, SOCK_MODE, Mode);
}

/**
  Configure the low level protocol to join a multicast group for
  this socket's connection.

  @param[in]  Sock             Pointer to the socket of the connection to join the
                               specific multicast group.
  @param[in]  GroupInfo        Pointer to the multicast group info.

  @retval EFI_SUCCESS          The configuration completed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockGroup (
  IN SOCKET *Sock,
  IN VOID   *GroupInfo
  )
{
  EFI_STATUS  Status;

  Status = EfiAcquireLockOrFail (&(Sock->Lock));

  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "SockGroup: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  Status = Sock->ProtoHandler (Sock, SOCK_GROUP, GroupInfo);

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

/**
  Add or remove route information in IP route table associated
  with this socket.

  @param[in]  Sock             Pointer to the socket associated with the IP route
                               table to operate on.
  @param[in]  RouteInfo        Pointer to the route information to be processed.

  @retval EFI_SUCCESS          The route table updated successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.
  @retval EFI_NO_MAPPING       The IP address configuration operation is  not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockRoute (
  IN SOCKET    *Sock,
  IN VOID      *RouteInfo
  )
{
  EFI_STATUS  Status;

  Status = EfiAcquireLockOrFail (&(Sock->Lock));
  if (EFI_ERROR (Status)) {
    DEBUG (
      (EFI_D_ERROR,
      "SockRoute: Get the access for socket failed with %r",
      Status)
      );

    return EFI_ACCESS_DENIED;
  }

  if (SOCK_IS_NO_MAPPING (Sock)) {
    Status = EFI_NO_MAPPING;
    goto Exit;
  }

  if (SOCK_IS_UNCONFIGURED (Sock)) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  Status = Sock->ProtoHandler (Sock, SOCK_ROUTE, RouteInfo);

Exit:
  EfiReleaseLock (&(Sock->Lock));
  return Status;
}

