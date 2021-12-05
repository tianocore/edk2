/** @file
  Miscellaneous routines for HttpDxe driver.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpDriver.h"

/**
  The common notify function used in HTTP driver.

  @param[in]  Event   The event signaled.
  @param[in]  Context The context.

**/
VOID
EFIAPI
HttpCommonNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if ((Event == NULL) || (Context == NULL)) {
    return;
  }

  *((BOOLEAN *)Context) = TRUE;
}

/**
  The notify function associated with Tx4Token for Tcp4->Transmit() or Tx6Token for Tcp6->Transmit().

  @param[in]  Context The context.

**/
VOID
EFIAPI
HttpTcpTransmitNotifyDpc (
  IN VOID  *Context
  )
{
  HTTP_TOKEN_WRAP  *Wrap;
  HTTP_PROTOCOL    *HttpInstance;

  if (Context == NULL) {
    return;
  }

  Wrap         = (HTTP_TOKEN_WRAP *)Context;
  HttpInstance = Wrap->HttpInstance;

  if (!HttpInstance->LocalAddressIsIPv6) {
    Wrap->HttpToken->Status = Wrap->TcpWrap.Tx4Token.CompletionToken.Status;
    gBS->SignalEvent (Wrap->HttpToken->Event);

    //
    // Free resources.
    //
    if (Wrap->TcpWrap.Tx4Token.Packet.TxData->FragmentTable[0].FragmentBuffer != NULL) {
      FreePool (Wrap->TcpWrap.Tx4Token.Packet.TxData->FragmentTable[0].FragmentBuffer);
    }

    if (Wrap->TcpWrap.Tx4Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Tx4Token.CompletionToken.Event);
    }
  } else {
    Wrap->HttpToken->Status = Wrap->TcpWrap.Tx6Token.CompletionToken.Status;
    gBS->SignalEvent (Wrap->HttpToken->Event);

    //
    // Free resources.
    //
    if (Wrap->TcpWrap.Tx6Token.Packet.TxData->FragmentTable[0].FragmentBuffer != NULL) {
      FreePool (Wrap->TcpWrap.Tx6Token.Packet.TxData->FragmentTable[0].FragmentBuffer);
    }

    if (Wrap->TcpWrap.Tx6Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Tx6Token.CompletionToken.Event);
    }
  }

  Wrap->TcpWrap.IsTxDone = TRUE;

  //
  // Check pending TxTokens and sent out.
  //
  NetMapIterate (&Wrap->HttpInstance->TxTokens, HttpTcpTransmit, NULL);
}

/**
  Request HttpTcpTransmitNotifyDpc as a DPC at TPL_CALLBACK.

  @param  Event                 The receive event delivered to TCP for transmit.
  @param  Context               Context for the callback.

**/
VOID
EFIAPI
HttpTcpTransmitNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request HttpTcpTransmitNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpTcpTransmitNotifyDpc, Context);
}

/**
  The notify function associated with Rx4Token for Tcp4->Receive () or Rx6Token for Tcp6->Receive().

  @param[in]  Context The context.

**/
VOID
EFIAPI
HttpTcpReceiveNotifyDpc (
  IN VOID  *Context
  )
{
  HTTP_TOKEN_WRAP  *Wrap;
  NET_MAP_ITEM     *Item;
  UINTN            Length;
  EFI_STATUS       Status;
  HTTP_PROTOCOL    *HttpInstance;
  BOOLEAN          UsingIpv6;

  if (Context == NULL) {
    return;
  }

  Wrap         = (HTTP_TOKEN_WRAP *)Context;
  HttpInstance = Wrap->HttpInstance;
  UsingIpv6    = HttpInstance->LocalAddressIsIPv6;

  if (UsingIpv6) {
    gBS->CloseEvent (Wrap->TcpWrap.Rx6Token.CompletionToken.Event);
    Wrap->TcpWrap.Rx6Token.CompletionToken.Event = NULL;

    if (EFI_ERROR (Wrap->TcpWrap.Rx6Token.CompletionToken.Status)) {
      DEBUG ((DEBUG_ERROR, "HttpTcpReceiveNotifyDpc: %r!\n", Wrap->TcpWrap.Rx6Token.CompletionToken.Status));
      Wrap->HttpToken->Status = Wrap->TcpWrap.Rx6Token.CompletionToken.Status;
      gBS->SignalEvent (Wrap->HttpToken->Event);

      Item = NetMapFindKey (&HttpInstance->RxTokens, Wrap->HttpToken);
      if (Item != NULL) {
        NetMapRemoveItem (&HttpInstance->RxTokens, Item, NULL);
      }

      FreePool (Wrap);
      Wrap = NULL;

      return;
    }
  } else {
    gBS->CloseEvent (Wrap->TcpWrap.Rx4Token.CompletionToken.Event);
    Wrap->TcpWrap.Rx4Token.CompletionToken.Event = NULL;

    if (EFI_ERROR (Wrap->TcpWrap.Rx4Token.CompletionToken.Status)) {
      DEBUG ((DEBUG_ERROR, "HttpTcpReceiveNotifyDpc: %r!\n", Wrap->TcpWrap.Rx4Token.CompletionToken.Status));
      Wrap->HttpToken->Status = Wrap->TcpWrap.Rx4Token.CompletionToken.Status;
      gBS->SignalEvent (Wrap->HttpToken->Event);

      Item = NetMapFindKey (&HttpInstance->RxTokens, Wrap->HttpToken);
      if (Item != NULL) {
        NetMapRemoveItem (&HttpInstance->RxTokens, Item, NULL);
      }

      FreePool (Wrap);
      Wrap = NULL;

      return;
    }
  }

  //
  // Check whether we receive a complete HTTP message.
  //
  ASSERT (HttpInstance->MsgParser != NULL);
  if (UsingIpv6) {
    Length = (UINTN)Wrap->TcpWrap.Rx6Data.FragmentTable[0].FragmentLength;
  } else {
    Length = (UINTN)Wrap->TcpWrap.Rx4Data.FragmentTable[0].FragmentLength;
  }

  //
  // Record the CallbackData data.
  //
  HttpInstance->CallbackData.Wrap            = (VOID *)Wrap;
  HttpInstance->CallbackData.ParseData       = Wrap->HttpToken->Message->Body;
  HttpInstance->CallbackData.ParseDataLength = Length;

  //
  // Parse Body with CallbackData data.
  //
  Status = HttpParseMessageBody (
             HttpInstance->MsgParser,
             Length,
             Wrap->HttpToken->Message->Body
             );
  if (EFI_ERROR (Status)) {
    return;
  }

  if (HttpIsMessageComplete (HttpInstance->MsgParser)) {
    //
    // Free the MsgParse since we already have a full HTTP message.
    //
    HttpFreeMsgParser (HttpInstance->MsgParser);
    HttpInstance->MsgParser = NULL;
  }

  Wrap->HttpToken->Message->BodyLength = Length;
  ASSERT (HttpInstance->CacheBody == NULL);
  //
  // We receive part of header of next HTTP msg.
  //
  if (HttpInstance->NextMsg != NULL) {
    Wrap->HttpToken->Message->BodyLength = HttpInstance->NextMsg -
                                           (CHAR8 *)Wrap->HttpToken->Message->Body;
    HttpInstance->CacheLen = Length - Wrap->HttpToken->Message->BodyLength;
    if (HttpInstance->CacheLen != 0) {
      HttpInstance->CacheBody = AllocateZeroPool (HttpInstance->CacheLen);
      if (HttpInstance->CacheBody == NULL) {
        return;
      }

      CopyMem (HttpInstance->CacheBody, HttpInstance->NextMsg, HttpInstance->CacheLen);
      HttpInstance->NextMsg     = HttpInstance->CacheBody;
      HttpInstance->CacheOffset = 0;
    }
  }

  Item = NetMapFindKey (&Wrap->HttpInstance->RxTokens, Wrap->HttpToken);
  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->HttpInstance->RxTokens, Item, NULL);
  }

  Wrap->TcpWrap.IsRxDone = TRUE;
  if (UsingIpv6) {
    Wrap->HttpToken->Status = Wrap->TcpWrap.Rx6Token.CompletionToken.Status;
  } else {
    Wrap->HttpToken->Status = Wrap->TcpWrap.Rx4Token.CompletionToken.Status;
  }

  gBS->SignalEvent (Wrap->HttpToken->Event);

  //
  // Check pending RxTokens and receive the HTTP message.
  //
  NetMapIterate (&Wrap->HttpInstance->RxTokens, HttpTcpReceive, NULL);

  FreePool (Wrap);
  Wrap = NULL;
}

/**
  Request HttpTcpReceiveNotifyDpc as a DPC at TPL_CALLBACK.

  @param  Event                 The receive event delivered to TCP for receive.
  @param  Context               Context for the callback.

**/
VOID
EFIAPI
HttpTcpReceiveNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request HttpTcpTransmitNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpTcpReceiveNotifyDpc, Context);
}

/**
  Create events for the TCP connection token and TCP close token.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpConnCloseEvent (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  if (!HttpInstance->LocalAddressIsIPv6) {
    //
    // Create events for various asynchronous operations.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsTcp4ConnDone,
                    &HttpInstance->Tcp4ConnToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    //
    // Initialize Tcp4CloseToken
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsTcp4CloseDone,
                    &HttpInstance->Tcp4CloseToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }
  } else {
    //
    // Create events for various asynchronous operations.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsTcp6ConnDone,
                    &HttpInstance->Tcp6ConnToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    //
    // Initialize Tcp6CloseToken
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsTcp6CloseDone,
                    &HttpInstance->Tcp6CloseToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }
  }

  return EFI_SUCCESS;

ERROR:
  //
  // Error handling
  //
  HttpCloseTcpConnCloseEvent (HttpInstance);

  return Status;
}

/**
  Close events in the TCP connection token and TCP close token.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
HttpCloseTcpConnCloseEvent (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->LocalAddressIsIPv6) {
    if (NULL != HttpInstance->Tcp6ConnToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp6ConnToken.CompletionToken.Event);
      HttpInstance->Tcp6ConnToken.CompletionToken.Event = NULL;
    }

    if (NULL != HttpInstance->Tcp6CloseToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp6CloseToken.CompletionToken.Event);
      HttpInstance->Tcp6CloseToken.CompletionToken.Event = NULL;
    }
  } else {
    if (NULL != HttpInstance->Tcp4ConnToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp4ConnToken.CompletionToken.Event);
      HttpInstance->Tcp4ConnToken.CompletionToken.Event = NULL;
    }

    if (NULL != HttpInstance->Tcp4CloseToken.CompletionToken.Event) {
      gBS->CloseEvent (HttpInstance->Tcp4CloseToken.CompletionToken.Event);
      HttpInstance->Tcp4CloseToken.CompletionToken.Event = NULL;
    }
  }
}

/**
  Create event for the TCP transmit token.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpTxEvent (
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  EFI_STATUS           Status;
  HTTP_PROTOCOL        *HttpInstance;
  HTTP_TCP_TOKEN_WRAP  *TcpWrap;

  HttpInstance = Wrap->HttpInstance;
  TcpWrap      = &Wrap->TcpWrap;

  if (!HttpInstance->LocalAddressIsIPv6) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpTcpTransmitNotify,
                    Wrap,
                    &TcpWrap->Tx4Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TcpWrap->Tx4Data.Push                    = TRUE;
    TcpWrap->Tx4Data.Urgent                  = FALSE;
    TcpWrap->Tx4Data.FragmentCount           = 1;
    TcpWrap->Tx4Token.Packet.TxData          = &Wrap->TcpWrap.Tx4Data;
    TcpWrap->Tx4Token.CompletionToken.Status = EFI_NOT_READY;
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpTcpTransmitNotify,
                    Wrap,
                    &TcpWrap->Tx6Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TcpWrap->Tx6Data.Push                    = TRUE;
    TcpWrap->Tx6Data.Urgent                  = FALSE;
    TcpWrap->Tx6Data.FragmentCount           = 1;
    TcpWrap->Tx6Token.Packet.TxData          = &Wrap->TcpWrap.Tx6Data;
    TcpWrap->Tx6Token.CompletionToken.Status = EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Create event for the TCP receive token which is used to receive HTTP header.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpRxEventForHeader (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  if (!HttpInstance->LocalAddressIsIPv6) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsRxDone,
                    &HttpInstance->Rx4Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HttpInstance->Rx4Data.FragmentCount           = 1;
    HttpInstance->Rx4Token.Packet.RxData          = &HttpInstance->Rx4Data;
    HttpInstance->Rx4Token.CompletionToken.Status = EFI_NOT_READY;
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsRxDone,
                    &HttpInstance->Rx6Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HttpInstance->Rx6Data.FragmentCount           = 1;
    HttpInstance->Rx6Token.Packet.RxData          = &HttpInstance->Rx6Data;
    HttpInstance->Rx6Token.CompletionToken.Status = EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Create event for the TCP receive token which is used to receive HTTP body.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpRxEvent (
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  EFI_STATUS           Status;
  HTTP_PROTOCOL        *HttpInstance;
  HTTP_TCP_TOKEN_WRAP  *TcpWrap;

  HttpInstance = Wrap->HttpInstance;
  TcpWrap      = &Wrap->TcpWrap;
  if (!HttpInstance->LocalAddressIsIPv6) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpTcpReceiveNotify,
                    Wrap,
                    &TcpWrap->Rx4Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TcpWrap->Rx4Data.FragmentCount           = 1;
    TcpWrap->Rx4Token.Packet.RxData          = &Wrap->TcpWrap.Rx4Data;
    TcpWrap->Rx4Token.CompletionToken.Status = EFI_NOT_READY;
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpTcpReceiveNotify,
                    Wrap,
                    &TcpWrap->Rx6Token.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    TcpWrap->Rx6Data.FragmentCount           = 1;
    TcpWrap->Rx6Token.Packet.RxData          = &Wrap->TcpWrap.Rx6Data;
    TcpWrap->Rx6Token.CompletionToken.Status = EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Close Events for Tcp Receive Tokens for HTTP body and HTTP header.

  @param[in]  Wrap               Pointer to HTTP token's wrap data.

**/
VOID
HttpCloseTcpRxEvent (
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  HTTP_PROTOCOL  *HttpInstance;

  ASSERT (Wrap != NULL);
  HttpInstance = Wrap->HttpInstance;

  if (HttpInstance->LocalAddressIsIPv6) {
    if (Wrap->TcpWrap.Rx6Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Rx6Token.CompletionToken.Event);
    }

    if (HttpInstance->Rx6Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (HttpInstance->Rx6Token.CompletionToken.Event);
      HttpInstance->Rx6Token.CompletionToken.Event = NULL;
    }
  } else {
    if (Wrap->TcpWrap.Rx4Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Rx4Token.CompletionToken.Event);
    }

    if (HttpInstance->Rx4Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (HttpInstance->Rx4Token.CompletionToken.Event);
      HttpInstance->Rx4Token.CompletionToken.Event = NULL;
    }
  }
}

/**
  Initialize the HTTP_PROTOCOL structure to the unconfigured state.

  @param[in, out]  HttpInstance         Pointer to HTTP_PROTOCOL structure.
  @param[in]       IpVersion            Indicate us TCP4 protocol or TCP6 protocol.

  @retval EFI_SUCCESS       HTTP_PROTOCOL structure is initialized successfully.
  @retval Others            Other error as indicated.

**/
EFI_STATUS
HttpInitProtocol (
  IN OUT HTTP_PROTOCOL  *HttpInstance,
  IN     BOOLEAN        IpVersion
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;
  BOOLEAN     UsingIpv6;

  ASSERT (HttpInstance != NULL);
  UsingIpv6 = IpVersion;

  if (!UsingIpv6) {
    //
    // Create TCP4 child.
    //
    Status = NetLibCreateServiceChild (
               HttpInstance->Service->ControllerHandle,
               HttpInstance->Service->Ip4DriverBindingHandle,
               &gEfiTcp4ServiceBindingProtocolGuid,
               &HttpInstance->Tcp4ChildHandle
               );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Tcp4ChildHandle,
                    &gEfiTcp4ProtocolGuid,
                    (VOID **)&Interface,
                    HttpInstance->Service->Ip4DriverBindingHandle,
                    HttpInstance->Service->ControllerHandle,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Tcp4ChildHandle,
                    &gEfiTcp4ProtocolGuid,
                    (VOID **)&HttpInstance->Tcp4,
                    HttpInstance->Service->Ip4DriverBindingHandle,
                    HttpInstance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Service->Tcp4ChildHandle,
                    &gEfiTcp4ProtocolGuid,
                    (VOID **)&Interface,
                    HttpInstance->Service->Ip4DriverBindingHandle,
                    HttpInstance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  } else {
    //
    // Create TCP6 Child.
    //
    Status = NetLibCreateServiceChild (
               HttpInstance->Service->ControllerHandle,
               HttpInstance->Service->Ip6DriverBindingHandle,
               &gEfiTcp6ServiceBindingProtocolGuid,
               &HttpInstance->Tcp6ChildHandle
               );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Tcp6ChildHandle,
                    &gEfiTcp6ProtocolGuid,
                    (VOID **)&Interface,
                    HttpInstance->Service->Ip6DriverBindingHandle,
                    HttpInstance->Service->ControllerHandle,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Tcp6ChildHandle,
                    &gEfiTcp6ProtocolGuid,
                    (VOID **)&HttpInstance->Tcp6,
                    HttpInstance->Service->Ip6DriverBindingHandle,
                    HttpInstance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    Status = gBS->OpenProtocol (
                    HttpInstance->Service->Tcp6ChildHandle,
                    &gEfiTcp6ProtocolGuid,
                    (VOID **)&Interface,
                    HttpInstance->Service->Ip6DriverBindingHandle,
                    HttpInstance->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  HttpInstance->Url = AllocateZeroPool (HTTP_URL_BUFFER_LEN);
  if (HttpInstance->Url == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (HttpInstance->Tcp4ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Handle
           );

    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->Ip4DriverBindingHandle,
      &gEfiTcp4ServiceBindingProtocolGuid,
      HttpInstance->Tcp4ChildHandle
      );
  }

  if (HttpInstance->Service->Tcp4ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Service->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Handle
           );
  }

  if (HttpInstance->Tcp6ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Handle
           );

    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->Ip6DriverBindingHandle,
      &gEfiTcp6ServiceBindingProtocolGuid,
      HttpInstance->Tcp6ChildHandle
      );
  }

  if (HttpInstance->Service->Tcp6ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Service->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Handle
           );
  }

  return EFI_UNSUPPORTED;
}

/**
  Clean up the HTTP child, release all the resources used by it.

  @param[in]  HttpInstance       The HTTP child to clean up.

**/
VOID
HttpCleanProtocol (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  HttpCloseConnection (HttpInstance);

  HttpCloseTcpConnCloseEvent (HttpInstance);

  if (HttpInstance->TimeoutEvent != NULL) {
    gBS->CloseEvent (HttpInstance->TimeoutEvent);
    HttpInstance->TimeoutEvent = NULL;
  }

  if (HttpInstance->CacheBody != NULL) {
    FreePool (HttpInstance->CacheBody);
    HttpInstance->CacheBody = NULL;
    HttpInstance->NextMsg   = NULL;
  }

  if (HttpInstance->RemoteHost != NULL) {
    FreePool (HttpInstance->RemoteHost);
    HttpInstance->RemoteHost = NULL;
  }

  if (HttpInstance->MsgParser != NULL) {
    HttpFreeMsgParser (HttpInstance->MsgParser);
    HttpInstance->MsgParser = NULL;
  }

  if (HttpInstance->Url != NULL) {
    FreePool (HttpInstance->Url);
    HttpInstance->Url = NULL;
  }

  NetMapClean (&HttpInstance->TxTokens);
  NetMapClean (&HttpInstance->RxTokens);

  if ((HttpInstance->TlsSb != NULL) && (HttpInstance->TlsChildHandle != NULL)) {
    //
    // Destroy the TLS instance.
    //
    HttpInstance->TlsSb->DestroyChild (HttpInstance->TlsSb, HttpInstance->TlsChildHandle);
    HttpInstance->TlsChildHandle = NULL;
  }

  if (HttpInstance->Tcp4ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Handle
           );

    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->Ip4DriverBindingHandle,
      &gEfiTcp4ServiceBindingProtocolGuid,
      HttpInstance->Tcp4ChildHandle
      );
  }

  if (HttpInstance->Service->Tcp4ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Service->Tcp4ChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->Ip4DriverBindingHandle,
           HttpInstance->Handle
           );
  }

  if (HttpInstance->Tcp6ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Handle
           );

    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->Ip6DriverBindingHandle,
      &gEfiTcp6ServiceBindingProtocolGuid,
      HttpInstance->Tcp6ChildHandle
      );
  }

  if (HttpInstance->Service->Tcp6ChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->Service->Tcp6ChildHandle,
           &gEfiTcp6ProtocolGuid,
           HttpInstance->Service->Ip6DriverBindingHandle,
           HttpInstance->Handle
           );
  }

  TlsCloseTxRxEvent (HttpInstance);
}

/**
  Establish TCP connection with HTTP server.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateConnection (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  //
  // Connect to Http server
  //
  if (!HttpInstance->LocalAddressIsIPv6) {
    HttpInstance->IsTcp4ConnDone                       = FALSE;
    HttpInstance->Tcp4ConnToken.CompletionToken.Status = EFI_NOT_READY;
    Status                                             = HttpInstance->Tcp4->Connect (HttpInstance->Tcp4, &HttpInstance->Tcp4ConnToken);
    HttpNotify (HttpEventConnectTcp, Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HttpCreateConnection: Tcp4->Connect() = %r\n", Status));
      return Status;
    }

    while (!HttpInstance->IsTcp4ConnDone) {
      HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
    }

    Status = HttpInstance->Tcp4ConnToken.CompletionToken.Status;
  } else {
    HttpInstance->IsTcp6ConnDone                       = FALSE;
    HttpInstance->Tcp6ConnToken.CompletionToken.Status = EFI_NOT_READY;
    Status                                             = HttpInstance->Tcp6->Connect (HttpInstance->Tcp6, &HttpInstance->Tcp6ConnToken);
    HttpNotify (HttpEventConnectTcp, Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HttpCreateConnection: Tcp6->Connect() = %r\n", Status));
      return Status;
    }

    while (!HttpInstance->IsTcp6ConnDone) {
      HttpInstance->Tcp6->Poll (HttpInstance->Tcp6);
    }

    Status = HttpInstance->Tcp6ConnToken.CompletionToken.Status;
  }

  if (!EFI_ERROR (Status)) {
    HttpInstance->State = HTTP_STATE_TCP_CONNECTED;
  }

  return Status;
}

/**
  Close existing TCP connection.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is closed.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCloseConnection (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS  Status;

  if (HttpInstance->State == HTTP_STATE_TCP_CONNECTED) {
    if (HttpInstance->LocalAddressIsIPv6) {
      HttpInstance->Tcp6CloseToken.AbortOnClose = TRUE;
      HttpInstance->IsTcp6CloseDone             = FALSE;
      Status                                    = HttpInstance->Tcp6->Close (HttpInstance->Tcp6, &HttpInstance->Tcp6CloseToken);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      while (!HttpInstance->IsTcp6CloseDone) {
        HttpInstance->Tcp6->Poll (HttpInstance->Tcp6);
      }
    } else {
      HttpInstance->Tcp4CloseToken.AbortOnClose = TRUE;
      HttpInstance->IsTcp4CloseDone             = FALSE;
      Status                                    = HttpInstance->Tcp4->Close (HttpInstance->Tcp4, &HttpInstance->Tcp4CloseToken);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      while (!HttpInstance->IsTcp4CloseDone) {
        HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
      }
    }
  }

  HttpInstance->State = HTTP_STATE_TCP_CLOSED;
  return EFI_SUCCESS;
}

/**
  Configure TCP4 protocol child.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.

  @retval EFI_SUCCESS            The TCP4 protocol child is configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConfigureTcp4 (
  IN  HTTP_PROTOCOL    *HttpInstance,
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  EFI_STATUS             Status;
  EFI_TCP4_CONFIG_DATA   *Tcp4CfgData;
  EFI_TCP4_ACCESS_POINT  *Tcp4AP;
  EFI_TCP4_OPTION        *Tcp4Option;

  ASSERT (HttpInstance != NULL);

  Tcp4CfgData = &HttpInstance->Tcp4CfgData;
  ZeroMem (Tcp4CfgData, sizeof (EFI_TCP4_CONFIG_DATA));

  Tcp4CfgData->TypeOfService = HTTP_TOS_DEAULT;
  Tcp4CfgData->TimeToLive    = HTTP_TTL_DEAULT;
  Tcp4CfgData->ControlOption = &HttpInstance->Tcp4Option;

  Tcp4AP                    = &Tcp4CfgData->AccessPoint;
  Tcp4AP->UseDefaultAddress = HttpInstance->IPv4Node.UseDefaultAddress;
  if (!Tcp4AP->UseDefaultAddress) {
    IP4_COPY_ADDRESS (&Tcp4AP->StationAddress, &HttpInstance->IPv4Node.LocalAddress);
    IP4_COPY_ADDRESS (&Tcp4AP->SubnetMask, &HttpInstance->IPv4Node.LocalSubnet);
  }

  Tcp4AP->StationPort = HttpInstance->IPv4Node.LocalPort;
  Tcp4AP->RemotePort  = HttpInstance->RemotePort;
  Tcp4AP->ActiveFlag  = TRUE;
  IP4_COPY_ADDRESS (&Tcp4AP->RemoteAddress, &HttpInstance->RemoteAddr);

  Tcp4Option                    = Tcp4CfgData->ControlOption;
  Tcp4Option->ReceiveBufferSize = HTTP_BUFFER_SIZE_DEAULT;
  Tcp4Option->SendBufferSize    = HTTP_BUFFER_SIZE_DEAULT;
  Tcp4Option->MaxSynBackLog     = HTTP_MAX_SYN_BACK_LOG;
  Tcp4Option->ConnectionTimeout = HTTP_CONNECTION_TIMEOUT;
  Tcp4Option->DataRetries       = HTTP_DATA_RETRIES;
  Tcp4Option->FinTimeout        = HTTP_FIN_TIMEOUT;
  Tcp4Option->KeepAliveProbes   = HTTP_KEEP_ALIVE_PROBES;
  Tcp4Option->KeepAliveTime     = HTTP_KEEP_ALIVE_TIME;
  Tcp4Option->KeepAliveInterval = HTTP_KEEP_ALIVE_INTERVAL;
  Tcp4Option->EnableNagle       = TRUE;
  Tcp4CfgData->ControlOption    = Tcp4Option;

  Status = HttpInstance->Tcp4->Configure (HttpInstance->Tcp4, Tcp4CfgData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HttpConfigureTcp4 - %r\n", Status));
    return Status;
  }

  Status = HttpCreateTcpConnCloseEvent (HttpInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpCreateTcpTxEvent (Wrap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HttpInstance->State = HTTP_STATE_TCP_CONFIGED;

  return EFI_SUCCESS;
}

/**
  Configure TCP6 protocol child.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.

  @retval EFI_SUCCESS            The TCP6 protocol child is configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConfigureTcp6 (
  IN  HTTP_PROTOCOL    *HttpInstance,
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  EFI_STATUS             Status;
  EFI_TCP6_CONFIG_DATA   *Tcp6CfgData;
  EFI_TCP6_ACCESS_POINT  *Tcp6Ap;
  EFI_TCP6_OPTION        *Tcp6Option;

  ASSERT (HttpInstance != NULL);

  Tcp6CfgData = &HttpInstance->Tcp6CfgData;
  ZeroMem (Tcp6CfgData, sizeof (EFI_TCP6_CONFIG_DATA));

  Tcp6CfgData->TrafficClass  = 0;
  Tcp6CfgData->HopLimit      = 255;
  Tcp6CfgData->ControlOption = &HttpInstance->Tcp6Option;

  Tcp6Ap              = &Tcp6CfgData->AccessPoint;
  Tcp6Ap->ActiveFlag  = TRUE;
  Tcp6Ap->StationPort = HttpInstance->Ipv6Node.LocalPort;
  Tcp6Ap->RemotePort  = HttpInstance->RemotePort;
  IP6_COPY_ADDRESS (&Tcp6Ap->StationAddress, &HttpInstance->Ipv6Node.LocalAddress);
  IP6_COPY_ADDRESS (&Tcp6Ap->RemoteAddress, &HttpInstance->RemoteIpv6Addr);

  Tcp6Option                    = Tcp6CfgData->ControlOption;
  Tcp6Option->ReceiveBufferSize = HTTP_BUFFER_SIZE_DEAULT;
  Tcp6Option->SendBufferSize    = HTTP_BUFFER_SIZE_DEAULT;
  Tcp6Option->MaxSynBackLog     = HTTP_MAX_SYN_BACK_LOG;
  Tcp6Option->ConnectionTimeout = HTTP_CONNECTION_TIMEOUT;
  Tcp6Option->DataRetries       = HTTP_DATA_RETRIES;
  Tcp6Option->FinTimeout        = HTTP_FIN_TIMEOUT;
  Tcp6Option->KeepAliveProbes   = HTTP_KEEP_ALIVE_PROBES;
  Tcp6Option->KeepAliveTime     = HTTP_KEEP_ALIVE_TIME;
  Tcp6Option->KeepAliveInterval = HTTP_KEEP_ALIVE_INTERVAL;
  Tcp6Option->EnableNagle       = TRUE;

  Status = HttpInstance->Tcp6->Configure (HttpInstance->Tcp6, Tcp6CfgData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HttpConfigureTcp6 - %r\n", Status));
    return Status;
  }

  Status = HttpCreateTcpConnCloseEvent (HttpInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpCreateTcpTxEvent (Wrap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HttpInstance->State = HTTP_STATE_TCP_CONFIGED;

  return EFI_SUCCESS;
}

/**
  Check existing TCP connection, if in error state, recover TCP4 connection. Then,
  connect one TLS session if required.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval EFI_NOT_READY          TCP4 protocol child is not created or configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConnectTcp4 (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS                 Status;
  EFI_TCP4_CONNECTION_STATE  Tcp4State;

  if ((HttpInstance->State < HTTP_STATE_TCP_CONFIGED) || (HttpInstance->Tcp4 == NULL)) {
    return EFI_NOT_READY;
  }

  Status = HttpInstance->Tcp4->GetModeData (
                                 HttpInstance->Tcp4,
                                 &Tcp4State,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcp4 GetModeData fail - %x\n", Status));
    return Status;
  }

  if (Tcp4State == Tcp4StateEstablished) {
    return EFI_SUCCESS;
  } else if (Tcp4State > Tcp4StateEstablished ) {
    HttpCloseConnection (HttpInstance);
  }

  Status = HttpCreateConnection (HttpInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcp4 Connection fail - %x\n", Status));
    return Status;
  }

  //
  // Tls session connection.
  //
  if (HttpInstance->UseHttps) {
    if (HttpInstance->TimeoutEvent == NULL) {
      //
      // Create TimeoutEvent for TLS connection.
      //
      Status = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &HttpInstance->TimeoutEvent
                      );
      if (EFI_ERROR (Status)) {
        TlsCloseTxRxEvent (HttpInstance);
        return Status;
      }
    }

    //
    // Start the timer, and wait Timeout seconds for connection.
    //
    Status = gBS->SetTimer (HttpInstance->TimeoutEvent, TimerRelative, HTTP_CONNECTION_TIMEOUT * TICKS_PER_SECOND);
    if (EFI_ERROR (Status)) {
      TlsCloseTxRxEvent (HttpInstance);
      return Status;
    }

    Status = TlsConnectSession (HttpInstance, HttpInstance->TimeoutEvent);
    HttpNotify (HttpEventTlsConnectSession, Status);

    gBS->SetTimer (HttpInstance->TimeoutEvent, TimerCancel, 0);

    if (EFI_ERROR (Status)) {
      TlsCloseTxRxEvent (HttpInstance);
      return Status;
    }
  }

  return Status;
}

/**
  Check existing TCP connection, if in error state, recover TCP6 connection. Then,
  connect one TLS session if required.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval EFI_NOT_READY          TCP6 protocol child is not created or configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConnectTcp6 (
  IN  HTTP_PROTOCOL  *HttpInstance
  )
{
  EFI_STATUS                 Status;
  EFI_TCP6_CONNECTION_STATE  Tcp6State;

  if ((HttpInstance->State < HTTP_STATE_TCP_CONFIGED) || (HttpInstance->Tcp6 == NULL)) {
    return EFI_NOT_READY;
  }

  Status = HttpInstance->Tcp6->GetModeData (
                                 HttpInstance->Tcp6,
                                 &Tcp6State,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcp6 GetModeData fail - %x\n", Status));
    return Status;
  }

  if (Tcp6State == Tcp6StateEstablished) {
    return EFI_SUCCESS;
  } else if (Tcp6State > Tcp6StateEstablished ) {
    HttpCloseConnection (HttpInstance);
  }

  Status = HttpCreateConnection (HttpInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Tcp6 Connection fail - %x\n", Status));
    return Status;
  }

  //
  // Tls session connection.
  //
  if (HttpInstance->UseHttps) {
    if (HttpInstance->TimeoutEvent == NULL) {
      //
      // Create TimeoutEvent for TLS connection.
      //
      Status = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &HttpInstance->TimeoutEvent
                      );
      if (EFI_ERROR (Status)) {
        TlsCloseTxRxEvent (HttpInstance);
        return Status;
      }
    }

    //
    // Start the timer, and wait Timeout seconds for connection.
    //
    Status = gBS->SetTimer (HttpInstance->TimeoutEvent, TimerRelative, HTTP_CONNECTION_TIMEOUT * TICKS_PER_SECOND);
    if (EFI_ERROR (Status)) {
      TlsCloseTxRxEvent (HttpInstance);
      return Status;
    }

    Status = TlsConnectSession (HttpInstance, HttpInstance->TimeoutEvent);
    HttpNotify (HttpEventTlsConnectSession, Status);

    gBS->SetTimer (HttpInstance->TimeoutEvent, TimerCancel, 0);

    if (EFI_ERROR (Status)) {
      TlsCloseTxRxEvent (HttpInstance);
      return Status;
    }
  }

  return Status;
}

/**
  Initialize Http session.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.
  @param[in]  Configure          The Flag indicates whether need to initialize session.
  @param[in]  TlsConfigure       The Flag indicates whether it's the new Tls session.

  @retval EFI_SUCCESS            The initialization of session is done.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpInitSession (
  IN  HTTP_PROTOCOL    *HttpInstance,
  IN  HTTP_TOKEN_WRAP  *Wrap,
  IN  BOOLEAN          Configure,
  IN  BOOLEAN          TlsConfigure
  )
{
  EFI_STATUS  Status;

  ASSERT (HttpInstance != NULL);

  //
  // Configure Tls session.
  //
  if (TlsConfigure) {
    Status = TlsConfigureSession (HttpInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!HttpInstance->LocalAddressIsIPv6) {
    //
    // Configure TCP instance.
    //
    if (Configure) {
      Status = HttpConfigureTcp4 (HttpInstance, Wrap);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Connect TCP.
    //
    Status = HttpConnectTcp4 (HttpInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // Configure TCP instance.
    //
    if (Configure) {
      Status = HttpConfigureTcp6 (HttpInstance, Wrap);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Connect TCP.
    //
    Status = HttpConnectTcp6 (HttpInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Send the HTTP or HTTPS message through TCP4 or TCP6.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.
  @param[in]  TxString           Buffer containing the HTTP message string.
  @param[in]  TxStringLen        Length of the HTTP message string in bytes.

  @retval EFI_SUCCESS            The HTTP message is queued into TCP transmit queue.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpTransmitTcp (
  IN  HTTP_PROTOCOL    *HttpInstance,
  IN  HTTP_TOKEN_WRAP  *Wrap,
  IN  UINT8            *TxString,
  IN  UINTN            TxStringLen
  )
{
  EFI_STATUS         Status;
  EFI_TCP4_IO_TOKEN  *Tx4Token;
  EFI_TCP4_PROTOCOL  *Tcp4;
  EFI_TCP6_IO_TOKEN  *Tx6Token;
  EFI_TCP6_PROTOCOL  *Tcp6;
  UINT8              *TlsRecord;
  UINT16             PayloadSize;
  NET_FRAGMENT       TempFragment;
  NET_FRAGMENT       Fragment;
  UINTN              RecordCount;
  UINTN              RemainingLen;

  Status            = EFI_SUCCESS;
  TlsRecord         = NULL;
  PayloadSize       = 0;
  TempFragment.Len  = 0;
  TempFragment.Bulk = NULL;
  Fragment.Len      = 0;
  Fragment.Bulk     = NULL;
  RecordCount       = 0;
  RemainingLen      = 0;

  //
  // Need to encrypt data.
  //
  if (HttpInstance->UseHttps) {
    //
    // Allocate enough buffer for each TLS plaintext records.
    //
    TlsRecord = AllocateZeroPool (TLS_RECORD_HEADER_LENGTH + TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH);
    if (TlsRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      return Status;
    }

    //
    // Allocate enough buffer for all TLS ciphertext records.
    //
    RecordCount   = TxStringLen / TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH + 1;
    Fragment.Bulk = AllocateZeroPool (RecordCount * (TLS_RECORD_HEADER_LENGTH + TLS_CIPHERTEXT_RECORD_MAX_PAYLOAD_LENGTH));
    if (Fragment.Bulk == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_ERROR;
    }

    //
    // Encrypt each TLS plaintext records.
    //
    RemainingLen = TxStringLen;
    while (RemainingLen != 0) {
      PayloadSize = (UINT16)MIN (TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH, RemainingLen);

      ((TLS_RECORD_HEADER *)TlsRecord)->ContentType   = TlsContentTypeApplicationData;
      ((TLS_RECORD_HEADER *)TlsRecord)->Version.Major = HttpInstance->TlsConfigData.Version.Major;
      ((TLS_RECORD_HEADER *)TlsRecord)->Version.Minor = HttpInstance->TlsConfigData.Version.Minor;
      ((TLS_RECORD_HEADER *)TlsRecord)->Length        = PayloadSize;

      CopyMem (TlsRecord + TLS_RECORD_HEADER_LENGTH, TxString + (TxStringLen - RemainingLen), PayloadSize);

      Status = TlsProcessMessage (
                 HttpInstance,
                 TlsRecord,
                 TLS_RECORD_HEADER_LENGTH + PayloadSize,
                 EfiTlsEncrypt,
                 &TempFragment
                 );
      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      //
      // Record the processed/encrypted Packet.
      //
      CopyMem (Fragment.Bulk + Fragment.Len, TempFragment.Bulk, TempFragment.Len);
      Fragment.Len += TempFragment.Len;

      FreePool (TempFragment.Bulk);
      TempFragment.Len  = 0;
      TempFragment.Bulk = NULL;

      RemainingLen -= (UINTN)PayloadSize;
      ZeroMem (TlsRecord, TLS_RECORD_HEADER_LENGTH + TLS_PLAINTEXT_RECORD_MAX_PAYLOAD_LENGTH);
    }

    FreePool (TlsRecord);
    TlsRecord = NULL;
  }

  if (!HttpInstance->LocalAddressIsIPv6) {
    Tcp4     = HttpInstance->Tcp4;
    Tx4Token = &Wrap->TcpWrap.Tx4Token;

    if (HttpInstance->UseHttps) {
      Tx4Token->Packet.TxData->DataLength                      = Fragment.Len;
      Tx4Token->Packet.TxData->FragmentTable[0].FragmentLength = Fragment.Len;
      Tx4Token->Packet.TxData->FragmentTable[0].FragmentBuffer = (VOID *)Fragment.Bulk;
    } else {
      Tx4Token->Packet.TxData->DataLength                      = (UINT32)TxStringLen;
      Tx4Token->Packet.TxData->FragmentTable[0].FragmentLength = (UINT32)TxStringLen;
      Tx4Token->Packet.TxData->FragmentTable[0].FragmentBuffer = (VOID *)TxString;
    }

    Tx4Token->CompletionToken.Status = EFI_NOT_READY;

    Wrap->TcpWrap.IsTxDone = FALSE;
    Status                 = Tcp4->Transmit (Tcp4, Tx4Token);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Transmit failed: %r\n", Status));
      goto ON_ERROR;
    }
  } else {
    Tcp6     = HttpInstance->Tcp6;
    Tx6Token = &Wrap->TcpWrap.Tx6Token;

    if (HttpInstance->UseHttps) {
      Tx6Token->Packet.TxData->DataLength                      = Fragment.Len;
      Tx6Token->Packet.TxData->FragmentTable[0].FragmentLength = Fragment.Len;
      Tx6Token->Packet.TxData->FragmentTable[0].FragmentBuffer = (VOID *)Fragment.Bulk;
    } else {
      Tx6Token->Packet.TxData->DataLength                      = (UINT32)TxStringLen;
      Tx6Token->Packet.TxData->FragmentTable[0].FragmentLength = (UINT32)TxStringLen;
      Tx6Token->Packet.TxData->FragmentTable[0].FragmentBuffer = (VOID *)TxString;
    }

    Tx6Token->CompletionToken.Status = EFI_NOT_READY;

    Wrap->TcpWrap.IsTxDone = FALSE;
    Status                 = Tcp6->Transmit (Tcp6, Tx6Token);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Transmit failed: %r\n", Status));
      goto ON_ERROR;
    }
  }

  return Status;

ON_ERROR:

  if (HttpInstance->UseHttps) {
    if (TlsRecord != NULL) {
      FreePool (TlsRecord);
      TlsRecord = NULL;
    }

    if (Fragment.Bulk != NULL) {
      FreePool (Fragment.Bulk);
      Fragment.Bulk = NULL;
    }
  }

  return Status;
}

/**
  Check whether the user's token or event has already
  been enqueue on HTTP Tx or Rx Token list.

  @param[in]  Map                The container of either user's transmit or receive
                                 token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check against.

  @retval EFI_ACCESS_DENIED      The token or event has already been enqueued in IP
  @retval EFI_SUCCESS            The current item isn't the same token/event as the
                                 context.

**/
EFI_STATUS
EFIAPI
HttpTokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  EFI_HTTP_TOKEN  *Token;
  EFI_HTTP_TOKEN  *TokenInItem;

  Token       = (EFI_HTTP_TOKEN *)Context;
  TokenInItem = (EFI_HTTP_TOKEN *)Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Check whether the HTTP message associated with Tx4Token or Tx6Token is already sent out.

  @param[in]  Map                The container of Tx4Token or Tx6Token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check against.

  @retval EFI_NOT_READY          The HTTP message is still queued in the list.
  @retval EFI_SUCCESS            The HTTP message has been sent out.

**/
EFI_STATUS
EFIAPI
HttpTcpNotReady (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  HTTP_TOKEN_WRAP  *ValueInItem;

  ValueInItem = (HTTP_TOKEN_WRAP *)Item->Value;

  if (!ValueInItem->TcpWrap.IsTxDone) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

/**
  Transmit the HTTP or HTTPS message by processing the associated HTTP token.

  @param[in]  Map                The container of Tx4Token or Tx6Token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check against.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The HTTP message is queued into TCP transmit
                                 queue.

**/
EFI_STATUS
EFIAPI
HttpTcpTransmit (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  HTTP_TOKEN_WRAP  *ValueInItem;
  EFI_STATUS       Status;
  CHAR8            *RequestMsg;
  CHAR8            *Url;
  UINTN            UrlSize;
  UINTN            RequestMsgSize;

  RequestMsg = NULL;

  ValueInItem = (HTTP_TOKEN_WRAP *)Item->Value;
  if (ValueInItem->TcpWrap.IsTxDone) {
    return EFI_SUCCESS;
  }

  //
  // Parse the URI of the remote host.
  //
  UrlSize = StrLen (ValueInItem->HttpToken->Message->Data.Request->Url) + 1;
  Url     = AllocatePool (UrlSize);
  if (Url == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (ValueInItem->HttpToken->Message->Data.Request->Url, Url, UrlSize);

  //
  // Create request message.
  //
  Status = HttpGenRequestMessage (
             ValueInItem->HttpToken->Message,
             Url,
             &RequestMsg,
             &RequestMsgSize
             );
  FreePool (Url);

  if (EFI_ERROR (Status) || (NULL == RequestMsg)) {
    return Status;
  }

  ASSERT (RequestMsg != NULL);

  //
  // Transmit the request message.
  //
  Status = HttpTransmitTcp (
             ValueInItem->HttpInstance,
             ValueInItem,
             (UINT8 *)RequestMsg,
             RequestMsgSize
             );
  FreePool (RequestMsg);
  return Status;
}

/**
  Receive the HTTP response by processing the associated HTTP token.

  @param[in]  Map                The container of Rx4Token or Rx6Token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check against.

  @retval EFI_SUCCESS            The HTTP response is queued into TCP receive
                                 queue.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
HttpTcpReceive (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Context
  )
{
  //
  // Process the queued HTTP response.
  //
  return HttpResponseWorker ((HTTP_TOKEN_WRAP *)Item->Value);
}

/**
  Receive the HTTP header by processing the associated HTTP token.

  @param[in]       HttpInstance     The HTTP instance private data.
  @param[in, out]  SizeofHeaders    The HTTP header length.
  @param[in, out]  BufferSize       The size of buffer to cache the header message.
  @param[in]       Timeout          The time to wait for receiving the header packet.

  @retval EFI_SUCCESS               The HTTP header is received.
  @retval Others                    Other errors as indicated.

**/
EFI_STATUS
HttpTcpReceiveHeader (
  IN  HTTP_PROTOCOL  *HttpInstance,
  IN  OUT UINTN      *SizeofHeaders,
  IN  OUT UINTN      *BufferSize,
  IN  EFI_EVENT      Timeout
  )
{
  EFI_STATUS         Status;
  EFI_TCP4_IO_TOKEN  *Rx4Token;
  EFI_TCP4_PROTOCOL  *Tcp4;
  EFI_TCP6_IO_TOKEN  *Rx6Token;
  EFI_TCP6_PROTOCOL  *Tcp6;
  CHAR8              **EndofHeader;
  CHAR8              **HttpHeaders;
  CHAR8              *Buffer;
  NET_FRAGMENT       Fragment;

  ASSERT (HttpInstance != NULL);

  EndofHeader   = HttpInstance->EndofHeader;
  HttpHeaders   = HttpInstance->HttpHeaders;
  Tcp4          = HttpInstance->Tcp4;
  Tcp6          = HttpInstance->Tcp6;
  Buffer        = NULL;
  Rx4Token      = NULL;
  Rx6Token      = NULL;
  Fragment.Len  = 0;
  Fragment.Bulk = NULL;

  if (HttpInstance->LocalAddressIsIPv6) {
    ASSERT (Tcp6 != NULL);
  } else {
    ASSERT (Tcp4 != NULL);
  }

  if (!HttpInstance->UseHttps) {
    Status = HttpCreateTcpRxEventForHeader (HttpInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!HttpInstance->LocalAddressIsIPv6) {
    if (!HttpInstance->UseHttps) {
      Rx4Token                                                 = &HttpInstance->Rx4Token;
      Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer = AllocateZeroPool (DEF_BUF_LEN);
      if (Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }
    }

    //
    // Receive the HTTP headers only when EFI_HTTP_RESPONSE_DATA is not NULL.
    //
    while (*EndofHeader == NULL) {
      if (!HttpInstance->UseHttps) {
        HttpInstance->IsRxDone                                   = FALSE;
        Rx4Token->Packet.RxData->DataLength                      = DEF_BUF_LEN;
        Rx4Token->Packet.RxData->FragmentTable[0].FragmentLength = DEF_BUF_LEN;
        Status                                                   = Tcp4->Receive (Tcp4, Rx4Token);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Tcp4 receive failed: %r\n", Status));
          return Status;
        }

        while (!HttpInstance->IsRxDone && ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout)))) {
          Tcp4->Poll (Tcp4);
        }

        if (!HttpInstance->IsRxDone) {
          //
          // Cancel the Token before close its Event.
          //
          Tcp4->Cancel (HttpInstance->Tcp4, &Rx4Token->CompletionToken);
          gBS->CloseEvent (Rx4Token->CompletionToken.Event);
          Rx4Token->CompletionToken.Status = EFI_TIMEOUT;
        }

        Status = Rx4Token->CompletionToken.Status;
        if (EFI_ERROR (Status)) {
          return Status;
        }

        Fragment.Len  = Rx4Token->Packet.RxData->FragmentTable[0].FragmentLength;
        Fragment.Bulk = (UINT8 *)Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer;
      } else {
        if (Fragment.Bulk != NULL) {
          FreePool (Fragment.Bulk);
          Fragment.Bulk = NULL;
        }

        Status = HttpsReceive (HttpInstance, &Fragment, Timeout);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Tcp4 receive failed: %r\n", Status));
          return Status;
        }
      }

      //
      // Append the response string along with a Null-terminator.
      //
      *BufferSize = *SizeofHeaders + Fragment.Len;
      Buffer      = AllocatePool (*BufferSize + 1);
      if (Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      if (*HttpHeaders != NULL) {
        CopyMem (Buffer, *HttpHeaders, *SizeofHeaders);
        FreePool (*HttpHeaders);
      }

      CopyMem (
        Buffer + *SizeofHeaders,
        Fragment.Bulk,
        Fragment.Len
        );
      *(Buffer + *BufferSize) = '\0';
      *HttpHeaders            = Buffer;
      *SizeofHeaders          = *BufferSize;

      //
      // Check whether we received end of HTTP headers.
      //
      *EndofHeader = AsciiStrStr (*HttpHeaders, HTTP_END_OF_HDR_STR);
    }

    //
    // Free the buffer.
    //
    if ((Rx4Token != NULL) && (Rx4Token->Packet.RxData != NULL) && (Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL)) {
      FreePool (Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer);
      Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
      Fragment.Bulk                                            = NULL;
    }

    if (Fragment.Bulk != NULL) {
      FreePool (Fragment.Bulk);
      Fragment.Bulk = NULL;
    }
  } else {
    if (!HttpInstance->UseHttps) {
      Rx6Token                                                 = &HttpInstance->Rx6Token;
      Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer = AllocateZeroPool (DEF_BUF_LEN);
      if (Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }
    }

    //
    // Receive the HTTP headers only when EFI_HTTP_RESPONSE_DATA is not NULL.
    //
    while (*EndofHeader == NULL) {
      if (!HttpInstance->UseHttps) {
        HttpInstance->IsRxDone                                   = FALSE;
        Rx6Token->Packet.RxData->DataLength                      = DEF_BUF_LEN;
        Rx6Token->Packet.RxData->FragmentTable[0].FragmentLength = DEF_BUF_LEN;
        Status                                                   = Tcp6->Receive (Tcp6, Rx6Token);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Tcp6 receive failed: %r\n", Status));
          return Status;
        }

        while (!HttpInstance->IsRxDone && ((Timeout == NULL) || EFI_ERROR (gBS->CheckEvent (Timeout)))) {
          Tcp6->Poll (Tcp6);
        }

        if (!HttpInstance->IsRxDone) {
          //
          // Cancel the Token before close its Event.
          //
          Tcp6->Cancel (HttpInstance->Tcp6, &Rx6Token->CompletionToken);
          gBS->CloseEvent (Rx6Token->CompletionToken.Event);
          Rx6Token->CompletionToken.Status = EFI_TIMEOUT;
        }

        Status = Rx6Token->CompletionToken.Status;
        if (EFI_ERROR (Status)) {
          return Status;
        }

        Fragment.Len  = Rx6Token->Packet.RxData->FragmentTable[0].FragmentLength;
        Fragment.Bulk = (UINT8 *)Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer;
      } else {
        if (Fragment.Bulk != NULL) {
          FreePool (Fragment.Bulk);
          Fragment.Bulk = NULL;
        }

        Status = HttpsReceive (HttpInstance, &Fragment, Timeout);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "Tcp6 receive failed: %r\n", Status));
          return Status;
        }
      }

      //
      // Append the response string along with a Null-terminator.
      //
      *BufferSize = *SizeofHeaders + Fragment.Len;
      Buffer      = AllocatePool (*BufferSize + 1);
      if (Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      if (*HttpHeaders != NULL) {
        CopyMem (Buffer, *HttpHeaders, *SizeofHeaders);
        FreePool (*HttpHeaders);
      }

      CopyMem (
        Buffer + *SizeofHeaders,
        Fragment.Bulk,
        Fragment.Len
        );
      *(Buffer + *BufferSize) = '\0';
      *HttpHeaders            = Buffer;
      *SizeofHeaders          = *BufferSize;

      //
      // Check whether we received end of HTTP headers.
      //
      *EndofHeader = AsciiStrStr (*HttpHeaders, HTTP_END_OF_HDR_STR);
    }

    //
    // Free the buffer.
    //
    if ((Rx6Token != NULL) && (Rx6Token->Packet.RxData != NULL) && (Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL)) {
      FreePool (Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer);
      Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
      Fragment.Bulk                                            = NULL;
    }

    if (Fragment.Bulk != NULL) {
      FreePool (Fragment.Bulk);
      Fragment.Bulk = NULL;
    }
  }

  //
  // Skip the CRLF after the HTTP headers.
  //
  *EndofHeader = *EndofHeader + AsciiStrLen (HTTP_END_OF_HDR_STR);

  *SizeofHeaders = *EndofHeader - *HttpHeaders;

  return EFI_SUCCESS;
}

/**
  Receive the HTTP body by processing the associated HTTP token.

  @param[in]  Wrap               The HTTP token's wrap data.
  @param[in]  HttpMsg            The HTTP message data.

  @retval EFI_SUCCESS            The HTTP body is received.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpTcpReceiveBody (
  IN  HTTP_TOKEN_WRAP   *Wrap,
  IN  EFI_HTTP_MESSAGE  *HttpMsg
  )
{
  EFI_STATUS         Status;
  HTTP_PROTOCOL      *HttpInstance;
  EFI_TCP6_PROTOCOL  *Tcp6;
  EFI_TCP6_IO_TOKEN  *Rx6Token;
  EFI_TCP4_PROTOCOL  *Tcp4;
  EFI_TCP4_IO_TOKEN  *Rx4Token;

  HttpInstance = Wrap->HttpInstance;
  Tcp4         = HttpInstance->Tcp4;
  Tcp6         = HttpInstance->Tcp6;
  Rx4Token     = NULL;
  Rx6Token     = NULL;

  if (HttpInstance->LocalAddressIsIPv6) {
    ASSERT (Tcp6 != NULL);
  } else {
    ASSERT (Tcp4 != NULL);
  }

  if (HttpInstance->LocalAddressIsIPv6) {
    Rx6Token                                                 = &Wrap->TcpWrap.Rx6Token;
    Rx6Token->Packet.RxData->DataLength                      = (UINT32)MIN (MAX_UINT32, HttpMsg->BodyLength);
    Rx6Token->Packet.RxData->FragmentTable[0].FragmentLength = (UINT32)MIN (MAX_UINT32, HttpMsg->BodyLength);
    Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer = (VOID *)HttpMsg->Body;
    Rx6Token->CompletionToken.Status                         = EFI_NOT_READY;

    Status = Tcp6->Receive (Tcp6, Rx6Token);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tcp6 receive failed: %r\n", Status));
      return Status;
    }
  } else {
    Rx4Token                                                 = &Wrap->TcpWrap.Rx4Token;
    Rx4Token->Packet.RxData->DataLength                      = (UINT32)MIN (MAX_UINT32, HttpMsg->BodyLength);
    Rx4Token->Packet.RxData->FragmentTable[0].FragmentLength = (UINT32)MIN (MAX_UINT32, HttpMsg->BodyLength);
    Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer = (VOID *)HttpMsg->Body;

    Rx4Token->CompletionToken.Status = EFI_NOT_READY;
    Status                           = Tcp4->Receive (Tcp4, Rx4Token);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tcp4 receive failed: %r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Clean up Tcp Tokens while the Tcp transmission error occurs.

  @param[in]  Wrap               Pointer to HTTP token's wrap data.

**/
VOID
HttpTcpTokenCleanup (
  IN  HTTP_TOKEN_WRAP  *Wrap
  )
{
  HTTP_PROTOCOL      *HttpInstance;
  EFI_TCP4_IO_TOKEN  *Rx4Token;
  EFI_TCP6_IO_TOKEN  *Rx6Token;

  ASSERT (Wrap != NULL);
  HttpInstance = Wrap->HttpInstance;
  Rx4Token     = NULL;
  Rx6Token     = NULL;

  if (HttpInstance->LocalAddressIsIPv6) {
    Rx6Token = &Wrap->TcpWrap.Rx6Token;

    if (Rx6Token->CompletionToken.Event != NULL) {
      gBS->CloseEvent (Rx6Token->CompletionToken.Event);
      Rx6Token->CompletionToken.Event = NULL;
    }

    FreePool (Wrap);

    Rx6Token = &HttpInstance->Rx6Token;

    if (Rx6Token->CompletionToken.Event != NULL) {
      gBS->CloseEvent (Rx6Token->CompletionToken.Event);
      Rx6Token->CompletionToken.Event = NULL;
    }

    if (Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL) {
      FreePool (Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer);
      Rx6Token->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
    }
  } else {
    Rx4Token = &Wrap->TcpWrap.Rx4Token;

    if (Rx4Token->CompletionToken.Event != NULL) {
      gBS->CloseEvent (Rx4Token->CompletionToken.Event);
      Rx4Token->CompletionToken.Event = NULL;
    }

    FreePool (Wrap);

    Rx4Token = &HttpInstance->Rx4Token;

    if (Rx4Token->CompletionToken.Event != NULL) {
      gBS->CloseEvent (Rx4Token->CompletionToken.Event);
      Rx4Token->CompletionToken.Event = NULL;
    }

    if (Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL) {
      FreePool (Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer);
      Rx4Token->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
    }
  }
}

/**
  Send Events via EDKII_HTTP_CALLBACK_PROTOCOL.

  @param[in]  Event               The event that occurs in the current state.
  @param[in]  EventStatus         The Status of Event, EFI_SUCCESS or other errors.

**/
VOID
HttpNotify (
  IN  EDKII_HTTP_CALLBACK_EVENT  Event,
  IN  EFI_STATUS                 EventStatus
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *Handles;
  UINTN                         Index;
  UINTN                         HandleCount;
  EFI_HANDLE                    Handle;
  EDKII_HTTP_CALLBACK_PROTOCOL  *HttpCallback;

  DEBUG ((DEBUG_INFO, "HttpNotify: Event - %d, EventStatus - %r\n", Event, EventStatus));

  Handles     = NULL;
  HandleCount = 0;
  Status      = gBS->LocateHandleBuffer (
                       ByProtocol,
                       &gEdkiiHttpCallbackProtocolGuid,
                       NULL,
                       &HandleCount,
                       &Handles
                       );
  if (Status == EFI_SUCCESS) {
    for (Index = 0; Index < HandleCount; Index++) {
      Handle = Handles[Index];
      Status = gBS->HandleProtocol (
                      Handle,
                      &gEdkiiHttpCallbackProtocolGuid,
                      (VOID **)&HttpCallback
                      );
      if (Status == EFI_SUCCESS) {
        DEBUG ((DEBUG_INFO, "HttpNotify: Notifying %p\n", HttpCallback));
        HttpCallback->Callback (
                        HttpCallback,
                        Event,
                        EventStatus
                        );
      }
    }

    FreePool (Handles);
  }
}
