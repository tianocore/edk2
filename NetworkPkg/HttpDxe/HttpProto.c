/** @file
  Miscellaneous routines for HttpDxe driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
    return ;
  }

  *((BOOLEAN *) Context) = TRUE;
}

/**
  The notify function associated with TxToken for Tcp4->Transmit().

  @param[in]  Context The context.

**/
VOID
EFIAPI
HttpTcpTransmitNotifyDpc (
  IN VOID       *Context
  )
{
  HTTP_TOKEN_WRAP          *Wrap;

  if (Context == NULL) {
    return ;
  }

  Wrap = (HTTP_TOKEN_WRAP *) Context;
  Wrap->HttpToken->Status = Wrap->TcpWrap.TxToken.CompletionToken.Status;
  gBS->SignalEvent (Wrap->HttpToken->Event);

  //
  // Free resources.
  //
  if (Wrap->TcpWrap.TxToken.Packet.TxData->FragmentTable[0].FragmentBuffer != NULL) {
    FreePool (Wrap->TcpWrap.TxToken.Packet.TxData->FragmentTable[0].FragmentBuffer);
  }

  if (Wrap->TcpWrap.TxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Wrap->TcpWrap.TxToken.CompletionToken.Event);
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
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // Request HttpTcpTransmitNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpTcpTransmitNotifyDpc, Context);
}


/**
  The notify function associated with RxToken for Tcp4->Receive ().

  @param[in]  Context The context.

**/
VOID
EFIAPI
HttpTcpReceiveNotifyDpc (
  IN VOID       *Context
  )
{
  HTTP_TOKEN_WRAP          *Wrap;
  NET_MAP_ITEM             *Item;
  UINTN                    Length;
  EFI_STATUS               Status;
  HTTP_PROTOCOL            *HttpInstance;

  if (Context == NULL) {
    return ;
  }

  Wrap = (HTTP_TOKEN_WRAP *) Context;
  gBS->CloseEvent (Wrap->TcpWrap.RxToken.CompletionToken.Event);
  if (EFI_ERROR (Wrap->TcpWrap.RxToken.CompletionToken.Status)) {
    return ;
  }

  HttpInstance = Wrap->HttpInstance;

  //
  // Check whether we receive a complete HTTP message.
  //
  ASSERT (HttpInstance->MsgParser != NULL);

  Length = (UINTN) Wrap->TcpWrap.RxData.FragmentTable[0].FragmentLength;
  Status = HttpParseMessageBody (
             HttpInstance->MsgParser,
             Length,
             Wrap->HttpToken->Message->Body
             );
  if (EFI_ERROR (Status)) {
    return ;
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
                                           (CHAR8 *) Wrap->HttpToken->Message->Body;
    HttpInstance->CacheLen = Length - Wrap->HttpToken->Message->BodyLength;
    if (HttpInstance->CacheLen != 0) {
      HttpInstance->CacheBody = AllocateZeroPool (HttpInstance->CacheLen);
      if (HttpInstance->CacheBody == NULL) {
        return ;
      }
      CopyMem (HttpInstance->CacheBody, HttpInstance->NextMsg, HttpInstance->CacheLen);
      HttpInstance->NextMsg = HttpInstance->CacheBody;
      HttpInstance->CacheOffset = 0;
    }
  }

  Item = NetMapFindKey (&Wrap->HttpInstance->RxTokens, Wrap->HttpToken);
  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->HttpInstance->RxTokens, Item, NULL);
  }


  Wrap->TcpWrap.IsRxDone = TRUE;
  Wrap->HttpToken->Status = Wrap->TcpWrap.RxToken.CompletionToken.Status;

  gBS->SignalEvent (Wrap->HttpToken->Event);

  //
  // Check pending RxTokens and receive the HTTP message.
  //
  NetMapIterate (&Wrap->HttpInstance->RxTokens, HttpTcpReceive, NULL);
  
  FreePool (Wrap);
}

/**
  Request HttpTcpReceiveNotifyDpc as a DPC at TPL_CALLBACK.

  @param  Event                 The receive event delivered to TCP for receive.
  @param  Context               Context for the callback.

**/
VOID
EFIAPI
HttpTcpReceiveNotify (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // Request HttpTcpTransmitNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpTcpReceiveNotifyDpc, Context);
}


/**
  Create events for the TCP4 connection token and TCP4 close token.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4ConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  EFI_STATUS               Status;
    //
    // Create events for variuos asynchronous operations.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsConnDone,
                    &HttpInstance->ConnToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

    //
    // Initialize CloseToken
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    HttpCommonNotify,
                    &HttpInstance->IsCloseDone,
                    &HttpInstance->CloseToken.CompletionToken.Event
                    );
    if (EFI_ERROR (Status)) {
      goto ERROR;
    }

 
  return EFI_SUCCESS;

ERROR:
  //
  // Error handling
  //
  HttpCloseTcp4ConnCloseEvent (HttpInstance);

  return Status;
}


/**
  Close events in the TCP4 connection token and TCP4 close token.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
HttpCloseTcp4ConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  ASSERT (HttpInstance != NULL);

  if (NULL != HttpInstance->ConnToken.CompletionToken.Event) {
    gBS->CloseEvent (HttpInstance->ConnToken.CompletionToken.Event);
    HttpInstance->ConnToken.CompletionToken.Event = NULL;
  }

  if (NULL != HttpInstance->CloseToken.CompletionToken.Event) {
    gBS->CloseEvent(HttpInstance->CloseToken.CompletionToken.Event);
    HttpInstance->CloseToken.CompletionToken.Event = NULL;
  }  
}

/**
  Create event for the TCP4 transmit token.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4TxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap
  )
{
  EFI_STATUS               Status;
  HTTP_PROTOCOL            *HttpInstance;
  HTTP_TCP_TOKEN_WRAP      *TcpWrap;

  HttpInstance = Wrap->HttpInstance;
  TcpWrap      = &Wrap->TcpWrap;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpTcpTransmitNotify,
                  Wrap,
                  &TcpWrap->TxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TcpWrap->TxData.Push = TRUE;
  TcpWrap->TxData.Urgent = FALSE;
  TcpWrap->TxData.FragmentCount = 1;
  TcpWrap->TxToken.Packet.TxData = &Wrap->TcpWrap.TxData;
  TcpWrap->TxToken.CompletionToken.Status = EFI_NOT_READY;

  return EFI_SUCCESS;
}

/**
  Create event for the TCP4 receive token which is used to receive HTTP header.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4RxEventForHeader (
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  EFI_STATUS               Status;


  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpCommonNotify,
                  &HttpInstance->IsRxDone,
                  &HttpInstance->RxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HttpInstance->RxData.FragmentCount = 1;
  HttpInstance->RxToken.Packet.RxData = &HttpInstance->RxData;
  HttpInstance->RxToken.CompletionToken.Status = EFI_NOT_READY;

  return EFI_SUCCESS;
}

/**
  Create event for the TCP4 receive token which is used to receive HTTP body.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4RxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap 
  )
{
  EFI_STATUS               Status;
  HTTP_PROTOCOL            *HttpInstance;
  HTTP_TCP_TOKEN_WRAP      *TcpWrap;

  HttpInstance = Wrap->HttpInstance;
  TcpWrap      = &Wrap->TcpWrap;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpTcpReceiveNotify,
                  Wrap,
                  &TcpWrap->RxToken.CompletionToken.Event
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TcpWrap->RxData.FragmentCount = 1;
  TcpWrap->RxToken.Packet.RxData = &Wrap->TcpWrap.RxData;
  TcpWrap->RxToken.CompletionToken.Status = EFI_NOT_READY;

  return EFI_SUCCESS;
}

/**
  Intiialize the HTTP_PROTOCOL structure to the unconfigured state.

  @param[in]       HttpSb               The HTTP service private instance.
  @param[in, out]  HttpInstance         Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS       HTTP_PROTOCOL structure is initialized successfully.                                          
  @retval Others            Other error as indicated.

**/
EFI_STATUS
HttpInitProtocol (
  IN     HTTP_SERVICE            *HttpSb,
  IN OUT HTTP_PROTOCOL           *HttpInstance
  )
{
  EFI_STATUS                     Status;
  VOID                           *Interface;

  ASSERT ((HttpSb != NULL) && (HttpInstance != NULL));

  HttpInstance->Signature = HTTP_PROTOCOL_SIGNATURE;
  CopyMem (&HttpInstance->Http, &mEfiHttpTemplate, sizeof (HttpInstance->Http));
  HttpInstance->Service = HttpSb;

  //
  // Create TCP child.
  //
  Status = NetLibCreateServiceChild (
             HttpInstance->Service->ControllerHandle,
             HttpInstance->Service->ImageHandle,
             &gEfiTcp4ServiceBindingProtocolGuid,
             &HttpInstance->TcpChildHandle
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  HttpInstance->TcpChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  (VOID **) &Interface,
                  HttpInstance->Service->ImageHandle,
                  HttpInstance->Service->ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
                  
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  HttpInstance->TcpChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  (VOID **) &HttpInstance->Tcp4,
                  HttpInstance->Service->ImageHandle,
                  HttpInstance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR(Status)) {
    goto ON_ERROR;
  }

  HttpInstance->Url = AllocateZeroPool (HTTP_URL_BUFFER_LEN);
  if (HttpInstance->Url == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  NetMapInit (&HttpInstance->TxTokens);
  NetMapInit (&HttpInstance->RxTokens);

  return EFI_SUCCESS;

ON_ERROR:
  
  if (HttpInstance->TcpChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->TcpChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->ImageHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->TcpChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->ImageHandle,
           HttpInstance->Handle
           );
    
    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->ImageHandle,
      &gEfiTcp4ServiceBindingProtocolGuid,
      HttpInstance->TcpChildHandle
      );
  }

  return Status;
  
}

/**
  Clean up the HTTP child, release all the resources used by it.

  @param[in]  HttpInstance       The HTTP child to clean up.

**/
VOID
HttpCleanProtocol (
  IN  HTTP_PROTOCOL          *HttpInstance
  )
{
  HttpCloseConnection (HttpInstance);
  
  HttpCloseTcp4ConnCloseEvent (HttpInstance);

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

  if (HttpInstance->TcpChildHandle != NULL) {
    gBS->CloseProtocol (
           HttpInstance->TcpChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->ImageHandle,
           HttpInstance->Service->ControllerHandle
           );

    gBS->CloseProtocol (
           HttpInstance->TcpChildHandle,
           &gEfiTcp4ProtocolGuid,
           HttpInstance->Service->ImageHandle,
           HttpInstance->Handle
           );
    
    NetLibDestroyServiceChild (
      HttpInstance->Service->ControllerHandle,
      HttpInstance->Service->ImageHandle,
      &gEfiTcp4ServiceBindingProtocolGuid,
      HttpInstance->TcpChildHandle
      );
  }
}

/**
  Establish TCP connection with HTTP server.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateConnection (
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  EFI_STATUS                    Status;

  //
  // Create events for variuos asynchronous operations.
  //
  HttpInstance->IsConnDone = FALSE;

  //
  // Connect to Http server
  //
  HttpInstance->ConnToken.CompletionToken.Status = EFI_NOT_READY;
  Status = HttpInstance->Tcp4->Connect (HttpInstance->Tcp4, &HttpInstance->ConnToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpCreateConnection: Tcp4->Connect() = %r\n", Status));
    return Status;
  }

  while (!HttpInstance->IsConnDone) {
    HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
  }

  Status = HttpInstance->ConnToken.CompletionToken.Status;

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
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  EFI_STATUS                Status;

  if (HttpInstance->State == HTTP_STATE_TCP_CONNECTED) {
    HttpInstance->CloseToken.AbortOnClose = TRUE;
    HttpInstance->IsCloseDone             = FALSE;
    
    Status = HttpInstance->Tcp4->Close (HttpInstance->Tcp4, &HttpInstance->CloseToken);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    while (!HttpInstance->IsCloseDone) {
      HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
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
  IN  HTTP_PROTOCOL        *HttpInstance,
  IN  HTTP_TOKEN_WRAP      *Wrap
  )
{
  EFI_STATUS                 Status;
  EFI_TCP4_CONFIG_DATA       *Tcp4CfgData;
  EFI_TCP4_ACCESS_POINT      *Tcp4AP;
  EFI_TCP4_OPTION            *Tcp4Option;
  HTTP_TCP_TOKEN_WRAP        *TcpWrap;

  ASSERT (HttpInstance != NULL);
  TcpWrap = &Wrap->TcpWrap;


  Tcp4CfgData = &HttpInstance->Tcp4CfgData;
  ZeroMem (Tcp4CfgData, sizeof (EFI_TCP4_CONFIG_DATA));
  
  Tcp4CfgData->TypeOfService = HTTP_TOS_DEAULT;
  Tcp4CfgData->TimeToLive    = HTTP_TTL_DEAULT;
  Tcp4CfgData->ControlOption = &HttpInstance->Tcp4Option;

  Tcp4AP = &Tcp4CfgData->AccessPoint;
  Tcp4AP->UseDefaultAddress = HttpInstance->IPv4Node.UseDefaultAddress;
  if (!Tcp4AP->UseDefaultAddress) {
    IP4_COPY_ADDRESS (&Tcp4AP->StationAddress, &HttpInstance->IPv4Node.LocalAddress);
    IP4_COPY_ADDRESS (&Tcp4AP->SubnetMask, &HttpInstance->IPv4Node.LocalSubnet);
  }
  
  Tcp4AP->StationPort = HttpInstance->IPv4Node.LocalPort;
  Tcp4AP->RemotePort  = HttpInstance->RemotePort;
  Tcp4AP->ActiveFlag  = TRUE;
  IP4_COPY_ADDRESS (&Tcp4AP->RemoteAddress, &HttpInstance->RemoteAddr);

  Tcp4Option = Tcp4CfgData->ControlOption;
  Tcp4Option->ReceiveBufferSize      = HTTP_BUFFER_SIZE_DEAULT;
  Tcp4Option->SendBufferSize         = HTTP_BUFFER_SIZE_DEAULT;
  Tcp4Option->MaxSynBackLog          = HTTP_MAX_SYN_BACK_LOG;
  Tcp4Option->ConnectionTimeout      = HTTP_CONNECTION_TIMEOUT;
  Tcp4Option->DataRetries            = HTTP_DATA_RETRIES;
  Tcp4Option->FinTimeout             = HTTP_FIN_TIMEOUT;
  Tcp4Option->KeepAliveProbes        = HTTP_KEEP_ALIVE_PROBES;
  Tcp4Option->KeepAliveTime          = HTTP_KEEP_ALIVE_TIME;
  Tcp4Option->KeepAliveInterval      = HTTP_KEEP_ALIVE_INTERVAL;
  Tcp4Option->EnableNagle            = TRUE;
  Tcp4CfgData->ControlOption         = Tcp4Option;

  Status = HttpInstance->Tcp4->Configure (HttpInstance->Tcp4, Tcp4CfgData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "HttpConfigureTcp4 - %r\n", Status));
    return Status;
  }

  Status = HttpCreateTcp4ConnCloseEvent (HttpInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HttpCreateTcp4TxEvent (Wrap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HttpInstance->State = HTTP_STATE_TCP_CONFIGED;

  return EFI_SUCCESS;
}

/**
  Check existing TCP connection, if in error state, receover TCP4 connection.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval EFI_NOT_READY          TCP4 protocol child is not created or configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConnectTcp4 (
  IN  HTTP_PROTOCOL        *HttpInstance
  )
{
  EFI_STATUS                Status;
  EFI_TCP4_CONNECTION_STATE Tcp4State;


  if (HttpInstance->State != HTTP_STATE_TCP_CONFIGED || HttpInstance->Tcp4 == NULL) {
    return EFI_NOT_READY;
  }

  Status = HttpInstance->Tcp4->GetModeData(
                                 HttpInstance->Tcp4, 
                                 &Tcp4State, 
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL
                                 );
  if (EFI_ERROR(Status)){
    DEBUG ((EFI_D_ERROR, "Tcp4 GetModeData fail - %x\n", Status));
    return Status;
  }

  if (Tcp4State > Tcp4StateEstablished) {
    HttpCloseConnection(HttpInstance);
  }  

  return HttpCreateConnection (HttpInstance);
}

/**
  Send the HTTP message through TCP4.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.
  @param[in]  TxString           Buffer containing the HTTP message string.
  @param[in]  TxStringLen        Length of the HTTP message string in bytes.

  @retval EFI_SUCCESS            The HTTP message is queued into TCP transmit queue.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpTransmitTcp4 (
  IN  HTTP_PROTOCOL    *HttpInstance,
  IN  HTTP_TOKEN_WRAP  *Wrap,
  IN  UINT8            *TxString,
  IN  UINTN            TxStringLen
  )
{
  EFI_STATUS                    Status;
  EFI_TCP4_IO_TOKEN             *TxToken;
  EFI_TCP4_PROTOCOL             *Tcp4;
  
  Tcp4 = HttpInstance->Tcp4;
  TxToken = &Wrap->TcpWrap.TxToken;

  TxToken->Packet.TxData->DataLength = (UINT32) TxStringLen;
  TxToken->Packet.TxData->FragmentTable[0].FragmentLength = (UINT32) TxStringLen;
  TxToken->Packet.TxData->FragmentTable[0].FragmentBuffer = (VOID *) TxString;
  TxToken->CompletionToken.Status = EFI_NOT_READY;  

  Wrap->TcpWrap.IsTxDone = FALSE;
  Status  = Tcp4->Transmit (Tcp4, TxToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Transmit failed: %r\n", Status));
    return Status;
  }

  return Status;
}

/**
  Translate the status code in HTTP message to EFI_HTTP_STATUS_CODE defined 
  in UEFI 2.5 specification.

  @param[in]  StatusCode         The status code value in HTTP message.

  @return                        Value defined in EFI_HTTP_STATUS_CODE .

**/
EFI_HTTP_STATUS_CODE
HttpMappingToStatusCode (
  IN UINTN                  StatusCode
  )  
{
  switch (StatusCode) {
  case 100:
    return HTTP_STATUS_100_CONTINUE;
  case 101:
    return HTTP_STATUS_101_SWITCHING_PROTOCOLS;
  case 200:
    return HTTP_STATUS_200_OK;
  case 201:
    return HTTP_STATUS_201_CREATED;
  case 202:
    return HTTP_STATUS_202_ACCEPTED;
  case 203:
    return HTTP_STATUS_203_NON_AUTHORITATIVE_INFORMATION;
  case 204:
    return HTTP_STATUS_204_NO_CONTENT;
  case 205:
    return HTTP_STATUS_205_RESET_CONTENT;
  case 206:
    return HTTP_STATUS_206_PARTIAL_CONTENT;
  case 300:
    return HTTP_STATUS_300_MULTIPLE_CHIOCES;
  case 301:
    return HTTP_STATUS_301_MOVED_PERMANENTLY;
  case 302:
    return HTTP_STATUS_302_FOUND;
  case 303:
    return HTTP_STATUS_303_SEE_OTHER;
  case 304:
    return HTTP_STATUS_304_NOT_MODIFIED;
  case 305:
    return HTTP_STATUS_305_USE_PROXY;
  case 307:
    return HTTP_STATUS_307_TEMPORARY_REDIRECT;
  case 400:
    return HTTP_STATUS_400_BAD_REQUEST;
  case 401:
    return HTTP_STATUS_401_UNAUTHORIZED;
  case 402:
    return HTTP_STATUS_402_PAYMENT_REQUIRED;
  case 403:
    return HTTP_STATUS_403_FORBIDDEN;
  case 404:
    return HTTP_STATUS_404_NOT_FOUND;
  case 405:
    return HTTP_STATUS_405_METHOD_NOT_ALLOWED;
  case 406:
    return HTTP_STATUS_406_NOT_ACCEPTABLE;
  case 407:
    return HTTP_STATUS_407_PROXY_AUTHENTICATION_REQUIRED;
  case 408:
    return HTTP_STATUS_408_REQUEST_TIME_OUT;
  case 409:
    return HTTP_STATUS_409_CONFLICT;
  case 410:
    return HTTP_STATUS_410_GONE;
  case 411:
    return HTTP_STATUS_411_LENGTH_REQUIRED;
  case 412:
    return HTTP_STATUS_412_PRECONDITION_FAILED;
  case 413:
    return HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE;
  case 414:
    return HTTP_STATUS_414_REQUEST_URI_TOO_LARGE;
  case 415:
    return HTTP_STATUS_415_UNSUPPORTED_MEDIA_TYPE;
  case 416:
    return HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED;
  case 417:
    return HTTP_STATUS_417_EXPECTATION_FAILED;
  case 500:
    return HTTP_STATUS_500_INTERNAL_SERVER_ERROR;
  case 501:
    return HTTP_STATUS_501_NOT_IMPLEMENTED;
  case 502:
    return HTTP_STATUS_502_BAD_GATEWAY;
  case 503:
    return HTTP_STATUS_503_SERVICE_UNAVAILABLE;
  case 504:
    return HTTP_STATUS_504_GATEWAY_TIME_OUT;
  case 505:
    return HTTP_STATUS_505_HTTP_VERSION_NOT_SUPPORTED;

  default:
    return HTTP_STATUS_UNSUPPORTED_STATUS;
  }
}

/**
  Check whether the user's token or event has already
  been enqueue on HTTP TxToken or RxToken list.

  @param[in]  Map                The container of either user's transmit or receive
                                 token.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check againist.

  @retval EFI_ACCESS_DENIED      The token or event has already been enqueued in IP
  @retval EFI_SUCCESS            The current item isn't the same token/event as the
                                 context.

**/
EFI_STATUS
EFIAPI
HttpTokenExist (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  EFI_HTTP_TOKEN            *Token;
  EFI_HTTP_TOKEN            *TokenInItem;

  Token       = (EFI_HTTP_TOKEN *) Context;
  TokenInItem = (EFI_HTTP_TOKEN *) Item->Key;

  if (Token == TokenInItem || Token->Event == TokenInItem->Event) {
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Check whether the HTTP message associated with TxToken is already sent out.

  @param[in]  Map                The container of TxToken.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check againist.

  @retval EFI_NOT_READY          The HTTP message is still queued in the list.
  @retval EFI_SUCCESS            The HTTP message has been sent out.

**/
EFI_STATUS
EFIAPI
HttpTcpNotReady (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  HTTP_TOKEN_WRAP           *ValueInItem;

  ValueInItem = (HTTP_TOKEN_WRAP *) Item->Value;

  if (!ValueInItem->TcpWrap.IsTxDone) {
    return EFI_NOT_READY;
  }
  
  return EFI_SUCCESS;
}

/**
  Transmit the HTTP mssage by processing the associated HTTP token.

  @param[in]  Map                The container of TxToken.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check againist.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The HTTP message is queued into TCP transmit
                                 queue.

**/
EFI_STATUS
EFIAPI
HttpTcpTransmit (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  HTTP_TOKEN_WRAP           *ValueInItem;
  EFI_STATUS                Status;
  CHAR8                     *RequestStr;
  CHAR8                     *Url;

  ValueInItem = (HTTP_TOKEN_WRAP *) Item->Value;
  if (ValueInItem->TcpWrap.IsTxDone) {
    return EFI_SUCCESS;
  }

  //
  // Parse the URI of the remote host.
  //
  Url = AllocatePool (StrLen (ValueInItem->HttpToken->Message->Data.Request->Url) + 1);
  if (Url == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStr (ValueInItem->HttpToken->Message->Data.Request->Url, Url);

  //
  // Create request message.
  //
  RequestStr = HttpGenRequestString (
                 ValueInItem->HttpInstance,
                 ValueInItem->HttpToken->Message,
                 Url
                 );
  FreePool (Url);
  if (RequestStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Transmit the request message.
  //
  Status = HttpTransmitTcp4 (
             ValueInItem->HttpInstance,
             ValueInItem,
             (UINT8*) RequestStr,
             AsciiStrLen (RequestStr)
             );
  FreePool (RequestStr);
  return Status;
}

/**
  Receive the HTTP response by processing the associated HTTP token.

  @param[in]  Map                The container of RxToken.
  @param[in]  Item               Current item to check against.
  @param[in]  Context            The Token to check againist.

  @retval EFI_SUCCESS            The HTTP response is queued into TCP receive
                                 queue.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
EFIAPI
HttpTcpReceive (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{
  //
  // Process the queued HTTP response.
  //
  return HttpResponseWorker ((HTTP_TOKEN_WRAP *) Item->Value);
}

/**
  Generate HTTP request string.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.
  @param[in]  Message            Pointer to storage containing HTTP message data.
  @param[in]  Url                The URL of a remote host.

  @return     Pointer to the created HTTP request string.
  @return     NULL if any error occured.

**/
CHAR8 *
HttpGenRequestString (
  IN  HTTP_PROTOCOL        *HttpInstance,
  IN  EFI_HTTP_MESSAGE     *Message,
  IN  CHAR8                *Url
  )
{
  EFI_STATUS                  Status;
  UINTN                       StrLength;
  UINT8                       *Request;
  UINT8                       *RequestPtr;
  UINTN                       HttpHdrSize;
  UINTN                       MsgSize;
  BOOLEAN                     Success;
  VOID                        *HttpHdr;
  EFI_HTTP_HEADER             **AppendList; 
  UINTN                       Index;
  
  ASSERT (HttpInstance != NULL);
  ASSERT (Message != NULL);

  DEBUG ((EFI_D_ERROR, "HttpMethod - %x\n", Message->Data.Request->Method));

  Request = NULL;
  Success = FALSE;
  HttpHdr = NULL;
  AppendList = NULL;

  //
  // Build AppendList
  //
  AppendList = AllocateZeroPool (sizeof (EFI_HTTP_HEADER *) * (Message->HeaderCount));
  if (AppendList == NULL) {
    return NULL;
  }

  for(Index = 0; Index < Message->HeaderCount; Index++){
    AppendList[Index] = &Message->Headers[Index];
  }

  //
  // Check whether the EFI_HTTP_UTILITIES_PROTOCOL is available.
  //
  if (mHttpUtilities == NULL) {
    return NULL;
  }

  //
  // Build raw unformatted HTTP headers.
  //
  Status = mHttpUtilities->Build (
                             mHttpUtilities,
                             0,
                             NULL,
                             0,
                             NULL,
                             Message->HeaderCount,
                             AppendList,
                             &HttpHdrSize,
                             &HttpHdr
                             );
  FreePool (AppendList);
  if (EFI_ERROR (Status) || HttpHdr == NULL) {
    return NULL;
  }

  //
  // Calculate HTTP message length.
  //
  MsgSize = Message->BodyLength + HTTP_MAXIMUM_METHOD_LEN + AsciiStrLen (Url) + 
            AsciiStrLen (HTTP_VERSION_CRLF_STR) + HttpHdrSize;
  Request = AllocateZeroPool (MsgSize);
  if (Request == NULL) {
    goto Exit;
  }  

  RequestPtr = Request;
  //
  // Construct header request
  //
  switch (Message->Data.Request->Method) {
  case HttpMethodGet:
    StrLength = sizeof (HTTP_GET_STR) - 1;
    CopyMem (RequestPtr, HTTP_GET_STR, StrLength);
    RequestPtr += StrLength;
    break;
  case HttpMethodHead:
    StrLength = sizeof (HTTP_HEAD_STR) - 1;
    CopyMem (RequestPtr, HTTP_HEAD_STR, StrLength);
    RequestPtr += StrLength;
    break;
  default:
    ASSERT (FALSE);
    goto Exit;
  }

  StrLength = AsciiStrLen (Url);
  CopyMem (RequestPtr, Url, StrLength);
  RequestPtr += StrLength;

  StrLength = sizeof (HTTP_VERSION_CRLF_STR) - 1;
  CopyMem (RequestPtr, HTTP_VERSION_CRLF_STR, StrLength);
  RequestPtr += StrLength;

  //
  // Construct header
  //
  CopyMem (RequestPtr, HttpHdr, HttpHdrSize);
  RequestPtr += HttpHdrSize;

  //
  // Construct body
  //
  if (Message->Body != NULL) {
    CopyMem (RequestPtr, Message->Body, Message->BodyLength);
    RequestPtr += Message->BodyLength;
  }

  //
  // Done
  //
  *RequestPtr = 0;
  Success     = TRUE;
  
Exit:

  if (!Success) {
    if (Request != NULL) {
      FreePool (Request);
    }

    Request = NULL;
  }

  if (HttpHdr != NULL) {
    FreePool (HttpHdr);
  }

  return (CHAR8*) Request;
}
