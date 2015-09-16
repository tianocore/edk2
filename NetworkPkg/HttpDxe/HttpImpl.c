/** @file
  Implementation of EFI_HTTP_PROTOCOL protocol interfaces.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpDriver.h"

EFI_HTTP_PROTOCOL  mEfiHttpTemplate = {
  EfiHttpGetModeData,
  EfiHttpConfigure,
  EfiHttpRequest,
  EfiHttpCancel,
  EfiHttpResponse,
  EfiHttpPoll
};

/**
  Returns the operational parameters for the current HTTP child instance.

  The GetModeData() function is used to read the current mode data (operational
  parameters) for this HTTP protocol instance.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[out] HttpConfigData      Point to buffer for operational parameters of this
                                  HTTP instance.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  HttpConfigData is NULL.
                                  HttpConfigData->AccessPoint is NULL.
  @retval EFI_NOT_STARTED         The HTTP instance is not configured.

**/
EFI_STATUS
EFIAPI
EfiHttpGetModeData (
  IN  EFI_HTTP_PROTOCOL         *This,
  OUT EFI_HTTP_CONFIG_DATA      *HttpConfigData
  )
{
  HTTP_PROTOCOL                 *HttpInstance;

  if ((This == NULL) || (HttpConfigData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->State < HTTP_STATE_HTTP_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (HttpConfigData->AccessPoint.IPv4Node == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpConfigData->HttpVersion        = HttpInstance->HttpVersion;
  HttpConfigData->TimeOutMillisec    = HttpInstance->TimeOutMillisec;
  HttpConfigData->LocalAddressIsIPv6 = HttpInstance->LocalAddressIsIPv6;

  CopyMem (
    HttpConfigData->AccessPoint.IPv4Node,
    &HttpInstance->IPv4Node,
    sizeof (HttpInstance->IPv4Node)
    );

  return EFI_SUCCESS;
}

/**
  Initialize or brutally reset the operational parameters for this EFI HTTP instance.

  The Configure() function does the following:
  When HttpConfigData is not NULL Initialize this EFI HTTP instance by configuring
  timeout, local address, port, etc.
  When HttpConfigData is NULL, reset this EFI HTTP instance by closing all active
  connections with remote hosts, canceling all asynchronous tokens, and flush request
  and response buffers without informing the appropriate hosts.

  Except for GetModeData() and Configure(), No other EFI HTTP function can be executed
  by this instance until the Configure() function is executed and returns successfully.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[in]  HttpConfigData      Pointer to the configure data to configure the instance.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  HttpConfigData->LocalAddressIsIPv6 is FALSE and
                                  HttpConfigData->IPv4Node is NULL.
                                  HttpConfigData->LocalAddressIsIPv6 is TRUE and
                                  HttpConfigData->IPv6Node is NULL.
  @retval EFI_ALREADY_STARTED     Reinitialize this HTTP instance without calling
                                  Configure() with NULL to reset it.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate enough system resources when
                                  executing Configure().
  @retval EFI_UNSUPPORTED         One or more options in HttpConfigData are not supported
                                  in the implementation.
**/
EFI_STATUS
EFIAPI
EfiHttpConfigure (
  IN  EFI_HTTP_PROTOCOL         *This,
  IN  EFI_HTTP_CONFIG_DATA      *HttpConfigData
  ) 
{
  HTTP_PROTOCOL                 *HttpInstance;
  EFI_STATUS                    Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL && HttpInstance->Service != NULL);

  if (HttpConfigData != NULL) {
    //
    // Check input parameters.
    //
    if (HttpConfigData->LocalAddressIsIPv6) {
      if (HttpConfigData->AccessPoint.IPv6Node == NULL) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      if (HttpConfigData->AccessPoint.IPv4Node == NULL) {
        return EFI_INVALID_PARAMETER;
      }
    }
    //
    // Now configure this HTTP instance.
    //
    if (HttpInstance->State != HTTP_STATE_UNCONFIGED) {
      return EFI_ALREADY_STARTED;
    }

    HttpInstance->HttpVersion        = HttpConfigData->HttpVersion;
    HttpInstance->TimeOutMillisec    = HttpConfigData->TimeOutMillisec;
    HttpInstance->LocalAddressIsIPv6 = HttpConfigData->LocalAddressIsIPv6;

    if (HttpConfigData->LocalAddressIsIPv6) {
      return EFI_UNSUPPORTED;
    } else {
      CopyMem (
        &HttpInstance->IPv4Node,
        HttpConfigData->AccessPoint.IPv4Node,
        sizeof (HttpInstance->IPv4Node)
        );

      HttpInstance->State = HTTP_STATE_HTTP_CONFIGED;
      return EFI_SUCCESS;
    }

  } else {
    if (HttpInstance->LocalAddressIsIPv6) {
      return EFI_UNSUPPORTED;
    } else {
      HttpCleanProtocol (HttpInstance);
      Status = HttpInitProtocol (HttpInstance->Service, HttpInstance);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      HttpInstance->State = HTTP_STATE_UNCONFIGED;
      return EFI_SUCCESS;
    }
  }
}
 

/**
  The Request() function queues an HTTP request to this HTTP instance.

  Similar to Transmit() function in the EFI TCP driver. When the HTTP request is sent
  successfully, or if there is an error, Status in token will be updated and Event will
  be signaled.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[in]  Token               Pointer to storage containing HTTP request token.

  @retval EFI_SUCCESS             Outgoing data was processed.
  @retval EFI_NOT_STARTED         This EFI HTTP Protocol instance has not been started.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_TIMEOUT             Data was dropped out of the transmit or receive queue.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate enough system resources.
  @retval EFI_UNSUPPORTED         The HTTP method is not supported in current
                                  implementation.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token->Message is NULL.
                                  Token->Message->Body is not NULL,
                                  Token->Message->BodyLength is non-zero, and
                                  Token->Message->Data is NULL, but a previous call to
                                  Request()has not been completed successfully.
**/
EFI_STATUS
EFIAPI
EfiHttpRequest (
  IN  EFI_HTTP_PROTOCOL         *This,
  IN  EFI_HTTP_TOKEN            *Token
  )
{
  EFI_HTTP_MESSAGE              *HttpMsg;
  EFI_HTTP_REQUEST_DATA         *Request;
  VOID                          *UrlParser;
  EFI_STATUS                    Status;
  CHAR8                         *HostName;
  UINT16                        RemotePort;
  HTTP_PROTOCOL                 *HttpInstance;
  BOOLEAN                       Configure;
  BOOLEAN                       ReConfigure;
  CHAR8                         *RequestStr;
  CHAR8                         *Url;
  UINTN                         UrlLen;
  CHAR16                        *HostNameStr;
  HTTP_TOKEN_WRAP               *Wrap;
  HTTP_TCP_TOKEN_WRAP           *TcpWrap;
  CHAR8                         *FileUrl;
  
  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpMsg = Token->Message;
  if ((HttpMsg == NULL) || (HttpMsg->Headers == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Current implementation does not support POST/PUT method.
  // If future version supports these two methods, Request could be NULL for a special case that to send large amounts
  // of data. For this case, the implementation need check whether previous call to Request() has been completed or not.
  // 
  //
  Request = HttpMsg->Data.Request;
  if ((Request == NULL) || (Request->Url == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only support GET and HEAD method in current implementation.
  //
  if ((Request->Method != HttpMethodGet) && (Request->Method != HttpMethodHead)) {
    return EFI_UNSUPPORTED;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->State < HTTP_STATE_HTTP_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (HttpInstance->LocalAddressIsIPv6) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether the token already existed.
  //
  if (EFI_ERROR (NetMapIterate (&HttpInstance->TxTokens, HttpTokenExist, Token))) {
    return EFI_ACCESS_DENIED;   
  }  

  HostName    = NULL;
  Wrap        = NULL;
  HostNameStr = NULL;
  TcpWrap     = NULL;

  //
  // Parse the URI of the remote host.
  //
  Url = HttpInstance->Url;
  UrlLen = StrLen (Request->Url) + 1;
  if (UrlLen > HTTP_URL_BUFFER_LEN) {
    Url = AllocateZeroPool (UrlLen);
    if (Url == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    FreePool (HttpInstance->Url);
    HttpInstance->Url = Url;    
  }  

  UnicodeStrToAsciiStr (Request->Url, Url);
  UrlParser = NULL;
  Status = HttpParseUrl (Url, (UINT32) AsciiStrLen (Url), FALSE, &UrlParser);
  if (EFI_ERROR (Status)) {
    goto Error1;
  }

  RequestStr = NULL;
  HostName   = NULL;
  Status     = HttpUrlGetHostName (Url, UrlParser, &HostName);
  if (EFI_ERROR (Status)) {
    goto Error1;
  }

  Status = HttpUrlGetPort (Url, UrlParser, &RemotePort);
  if (EFI_ERROR (Status)) {
    RemotePort = HTTP_DEFAULT_PORT;
  }

  Configure   = TRUE;
  ReConfigure = TRUE;  

  if (HttpInstance->RemoteHost == NULL) {
    //
    // Request() is called the first time. 
    //
    ReConfigure = FALSE;
  } else {
    if ((HttpInstance->RemotePort == RemotePort) &&
        (AsciiStrCmp (HttpInstance->RemoteHost, HostName) == 0)) {
      //
      // Host Name and port number of the request URL are the same with previous call to Request().
      // Check whether previous TCP packet sent out.
      //
      if (EFI_ERROR (NetMapIterate (&HttpInstance->TxTokens, HttpTcpNotReady, NULL))) {
        //
        // Wrap the HTTP token in HTTP_TOKEN_WRAP
        //
        Wrap = AllocateZeroPool (sizeof (HTTP_TOKEN_WRAP));
        if (Wrap == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Error1;
        }

        Wrap->HttpToken    = Token;
        Wrap->HttpInstance = HttpInstance;

        Status = HttpCreateTcp4TxEvent (Wrap);
        if (EFI_ERROR (Status)) {
          goto Error1;
        }

        Status = NetMapInsertTail (&HttpInstance->TxTokens, Token, Wrap);
        if (EFI_ERROR (Status)) {
          goto Error1;
        }

        Wrap->TcpWrap.Method = Request->Method;

        FreePool (HostName);
        
        //
        // Queue the HTTP token and return.
        //
        return EFI_SUCCESS;
      } else {
        //
        // Use existing TCP instance to transmit the packet.
        //
        Configure   = FALSE;
        ReConfigure = FALSE;
      }
    } else {
      //
      // Need close existing TCP instance and create a new TCP instance for data transmit.
      //
      if (HttpInstance->RemoteHost != NULL) {
        FreePool (HttpInstance->RemoteHost);
        HttpInstance->RemoteHost = NULL;
        HttpInstance->RemotePort = 0;
      }
    }
  } 

  if (Configure) {
    //
    // Parse Url for IPv4 address, if failed, perform DNS resolution.
    //
    Status = NetLibAsciiStrToIp4 (HostName, &HttpInstance->RemoteAddr);
    if (EFI_ERROR (Status)) {
      HostNameStr = AllocateZeroPool ((AsciiStrLen (HostName) + 1) * sizeof (UINT16));
      if (HostNameStr == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error1;
      }

      AsciiStrToUnicodeStr (HostName, HostNameStr);
      Status = HttpDns4 (HttpInstance, HostNameStr, &HttpInstance->RemoteAddr);
      FreePool (HostNameStr);
      if (EFI_ERROR (Status)) {
        goto Error1;
      }
    }

    //
    // Save the RemotePort and RemoteHost.
    //
    ASSERT (HttpInstance->RemoteHost == NULL);
    HttpInstance->RemotePort = RemotePort;
    HttpInstance->RemoteHost = HostName;
    HostName = NULL;
  }

  if (ReConfigure) {
    //
    // The request URL is different from previous calls to Request(), close existing TCP instance.
    //
    ASSERT (HttpInstance->Tcp4 != NULL);
    HttpCloseConnection (HttpInstance);
    EfiHttpCancel (This, NULL);
  }

  //
  // Wrap the HTTP token in HTTP_TOKEN_WRAP
  //
  Wrap = AllocateZeroPool (sizeof (HTTP_TOKEN_WRAP));
  if (Wrap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error1;
  }

  Wrap->HttpToken      = Token;
  Wrap->HttpInstance   = HttpInstance;
  Wrap->TcpWrap.Method = Request->Method;

  if (Configure) {
    //
    // Configure TCP instance.
    //
    Status = HttpConfigureTcp4 (HttpInstance, Wrap);
    if (EFI_ERROR (Status)) {
      goto Error1;
    }
    //
    // Connect TCP.
    //
    Status = HttpConnectTcp4 (HttpInstance);
    if (EFI_ERROR (Status)) {
      goto Error2;
    }
  } else {
    //
    // For the new HTTP token, create TX TCP token events.    
    //
    Status = HttpCreateTcp4TxEvent (Wrap);
    if (EFI_ERROR (Status)) {
      goto Error1;
    }
  }

  //
  // Create request message.
  //
  FileUrl = Url;
  if (*FileUrl != '/') {
    //
    // Convert the absolute-URI to the absolute-path
    //
    while (*FileUrl != ':') {
      FileUrl++;
    }
    if ((*(FileUrl+1) == '/') && (*(FileUrl+2) == '/')) {
      FileUrl += 3;
      while (*FileUrl != '/') {
        FileUrl++;
      }
    } else {
      Status = EFI_INVALID_PARAMETER;
      goto Error3;
    }
  }
  RequestStr = HttpGenRequestString (HttpInstance, HttpMsg, FileUrl);
  if (RequestStr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error3;
  }

  Status = NetMapInsertTail (&HttpInstance->TxTokens, Token, Wrap);
  if (EFI_ERROR (Status)) {
    goto Error4;
  }

  if (HostName != NULL) {
    FreePool (HostName);
  }

  //
  // Transmit the request message.
  //
  Status = HttpTransmitTcp4 (
             HttpInstance,
             Wrap,
             (UINT8*) RequestStr,
             AsciiStrLen (RequestStr)
             );
  if (EFI_ERROR (Status)) {
    goto Error5;    
  }

  DispatchDpc ();

  return EFI_SUCCESS;

Error5:
    NetMapRemoveTail (&HttpInstance->TxTokens, NULL);

Error4:
  if (RequestStr != NULL) {
    FreePool (RequestStr);
  }  

Error3:
  HttpCloseConnection (HttpInstance);


Error2:
  HttpCloseTcp4ConnCloseEvent (HttpInstance);
  if (NULL != Wrap->TcpWrap.TxToken.CompletionToken.Event) {
    gBS->CloseEvent (Wrap->TcpWrap.TxToken.CompletionToken.Event);
    Wrap->TcpWrap.TxToken.CompletionToken.Event = NULL;
  }

Error1:
  if (HostName != NULL) {
    FreePool (HostName);
  }
  if (Wrap != NULL) {
    FreePool (Wrap);
  }
  if (UrlParser!= NULL) {
    HttpUrlFreeParser (UrlParser);
  }

  return Status;
  
}

/**
  Cancel a TxToken or RxToken. 
 
  @param[in]  Map                The HTTP instance's token queue.
  @param[in]  Item               Object container for one HTTP token and token's wrap.
  @param[in]  Context            The user's token to cancel.

  @retval EFI_SUCCESS            Continue to check the next Item.
  @retval EFI_ABORTED            The user's Token (Token != NULL) is cancelled.

**/
EFI_STATUS
EFIAPI
HttpCancelTokens (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Context
  )
{

  EFI_HTTP_TOKEN            *Token;
  HTTP_TOKEN_WRAP           *Wrap;

  Token = (EFI_HTTP_TOKEN *) Context;

  //
  // Return EFI_SUCCESS to check the next item in the map if
  // this one doesn't match.
  //
  if ((Token != NULL) && (Token != Item->Key)) {
    return EFI_SUCCESS;
  }

  Wrap = (HTTP_TOKEN_WRAP *) Item->Value;
  ASSERT (Wrap != NULL);

  //
  // Free resources.
  //
  NetMapRemoveItem (Map, Item, NULL); 
  
  if (Wrap->TcpWrap.TxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Wrap->TcpWrap.TxToken.CompletionToken.Event);
  }

  if (Wrap->TcpWrap.RxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (Wrap->TcpWrap.RxToken.CompletionToken.Event);
  }

  if (Wrap->TcpWrap.RxToken.Packet.RxData->FragmentTable[0].FragmentBuffer != NULL) {
    FreePool (Wrap->TcpWrap.RxToken.Packet.RxData->FragmentTable[0].FragmentBuffer);
  }

  FreePool (Wrap);

  //
  // If only one item is to be cancel, return EFI_ABORTED to stop
  // iterating the map any more.
  //
  if (Token != NULL) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS; 
}

/**
  Cancel the user's receive/transmit request. It is the worker function of
  EfiHttpCancel API. If a matching token is found, it will call HttpCancelTokens to cancel the
  token.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.
  @param[in]  Token              The token to cancel. If NULL, all token will be
                                 cancelled.

  @retval EFI_SUCCESS            The token is cancelled.
  @retval EFI_NOT_FOUND          The asynchronous request or response token is not found.                                 
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCancel (
  IN  HTTP_PROTOCOL             *HttpInstance,
  IN  EFI_HTTP_TOKEN            *Token
  )
{
  EFI_STATUS                    Status;

  //
  // First check the tokens queued by EfiHttpRequest().
  //
  Status = NetMapIterate (&HttpInstance->TxTokens, HttpCancelTokens, Token);
  if (EFI_ERROR (Status)) {
    if (Token != NULL) {
      if (Status == EFI_ABORTED) {
        return EFI_SUCCESS;
      } 
    } else {
      return Status;
    }
  }

  //
  // Then check the tokens queued by EfiHttpResponse().
  //
  Status = NetMapIterate (&HttpInstance->RxTokens, HttpCancelTokens, Token);
  if (EFI_ERROR (Status)) {
    if (Token != NULL) {
      if (Status == EFI_ABORTED) {
        return EFI_SUCCESS;
      } else {
        return EFI_NOT_FOUND;
      }
    } else {
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
  Abort an asynchronous HTTP request or response token.

  The Cancel() function aborts a pending HTTP request or response transaction. If
  Token is not NULL and the token is in transmit or receive queues when it is being
  cancelled, its Token->Status will be set to EFI_ABORTED and then Token->Event will
  be signaled. If the token is not in one of the queues, which usually means that the
  asynchronous operation has completed, EFI_NOT_FOUND is returned. If Token is NULL,
  all asynchronous tokens issued by Request() or Response() will be aborted.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[in]  Token               Point to storage containing HTTP request or response
                                  token.

  @retval EFI_SUCCESS             Request and Response queues are successfully flushed.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NOT_STARTED         This instance hasn't been configured.
  @retval EFI_NO_MAPPING          When using the default address, configuration (DHCP,
                                  BOOTP, RARP, etc.) hasn't finished yet.
  @retval EFI_NOT_FOUND           The asynchronous request or response token is not
                                  found.
  @retval EFI_UNSUPPORTED         The implementation does not support this function.

**/
EFI_STATUS
EFIAPI
EfiHttpCancel (
  IN  EFI_HTTP_PROTOCOL         *This,
  IN  EFI_HTTP_TOKEN            *Token
  )
{
  HTTP_PROTOCOL                 *HttpInstance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->State != HTTP_STATE_TCP_CONNECTED) {
    return EFI_NOT_STARTED;
  }

  return HttpCancel (HttpInstance, Token);

}

/**
  A callback function to intercept events during message parser.

  This function will be invoked during HttpParseMessageBody() with various events type. An error
  return status of the callback function will cause the HttpParseMessageBody() aborted.

  @param[in]    EventType          Event type of this callback call.
  @param[in]    Data               A pointer to data buffer.
  @param[in]    Length             Length in bytes of the Data.
  @param[in]    Context            Callback context set by HttpInitMsgParser().

  @retval EFI_SUCCESS              Continue to parser the message body.

**/
EFI_STATUS
EFIAPI
HttpBodyParserCallback (
  IN HTTP_BODY_PARSE_EVENT      EventType,
  IN CHAR8                      *Data,
  IN UINTN                      Length,
  IN VOID                       *Context
  )
{
  HTTP_TOKEN_WRAP               *Wrap;

  if (EventType != BodyParseEventOnComplete) {
    return EFI_SUCCESS;
  }

  if (Data == NULL || Length != 0 || Context == NULL) {
    return EFI_SUCCESS;
  }

  Wrap = (HTTP_TOKEN_WRAP *) Context;
  Wrap->HttpInstance->NextMsg = Data;

  //
  // Free TxToken since already received corrsponding HTTP response.
  //
  FreePool (Wrap);

  return EFI_SUCCESS;
}

/**
  The work function of EfiHttpResponse().

  @param[in]  Wrap                Pointer to HTTP token's wrap data.

  @retval EFI_SUCCESS             Allocation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to complete the opration due to lack of resources.
  @retval EFI_NOT_READY           Can't find a corresponding TxToken or 
                                  the EFI_HTTP_UTILITIES_PROTOCOL is not available.

**/
EFI_STATUS
HttpResponseWorker (
  IN  HTTP_TOKEN_WRAP           *Wrap
  )
{
  EFI_STATUS                    Status;
  EFI_HTTP_MESSAGE              *HttpMsg;
  EFI_TCP4_IO_TOKEN             *RxToken;
  EFI_TCP4_PROTOCOL             *Tcp4;
  CHAR8                         *EndofHeader;
  CHAR8                         *HttpHeaders;
  UINTN                         SizeofHeaders;
  CHAR8                         *Buffer;
  UINTN                         BufferSize;
  UINTN                         StatusCode;
  CHAR8                         *Tmp;
  CHAR8                         *HeaderTmp;
  CHAR8                         *StatusCodeStr;
  UINTN                         BodyLen;
  HTTP_PROTOCOL                 *HttpInstance;
  EFI_HTTP_TOKEN                *Token;
  NET_MAP_ITEM                  *Item;
  HTTP_TOKEN_WRAP               *ValueInItem;
  UINTN                         HdrLen;

  if (Wrap == NULL || Wrap->HttpInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  HttpInstance = Wrap->HttpInstance;
  Token = Wrap->HttpToken;

  HttpMsg = Token->Message;

  Tcp4 = HttpInstance->Tcp4;
  ASSERT (Tcp4 != NULL);
  HttpMsg->Headers = NULL;
  HttpHeaders   = NULL;
  SizeofHeaders = 0;
  Buffer        = NULL;
  BufferSize    = 0;
  EndofHeader   = NULL;
 
  if (HttpMsg->Data.Response != NULL) {
    //
    // Need receive the HTTP headers, prepare buffer.
    //
    Status = HttpCreateTcp4RxEventForHeader (HttpInstance);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    //
    // Check whether we have cached header from previous call.
    //
    if ((HttpInstance->CacheBody != NULL) && (HttpInstance->NextMsg != NULL)) {
      //
      // The data is stored at [NextMsg, CacheBody + CacheLen].
      //
      HdrLen = HttpInstance->CacheBody + HttpInstance->CacheLen - HttpInstance->NextMsg;
      HttpHeaders = AllocateZeroPool (HdrLen);
      if (HttpHeaders == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }

      CopyMem (HttpHeaders, HttpInstance->NextMsg, HdrLen);
      FreePool (HttpInstance->CacheBody);
      HttpInstance->CacheBody   = NULL;
      HttpInstance->NextMsg     = NULL;
      HttpInstance->CacheOffset = 0;
      SizeofHeaders = HdrLen;
      BufferSize = HttpInstance->CacheLen;

      //
      // Check whether we cached the whole HTTP headers.
      //
      EndofHeader = AsciiStrStr (HttpHeaders, HTTP_END_OF_HDR_STR); 
    }
    
    RxToken = &HttpInstance->RxToken;
    RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer = AllocateZeroPool (DEF_BUF_LEN);
    if (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    //
    // Receive the HTTP headers only when EFI_HTTP_RESPONSE_DATA is not NULL.
    //
    while (EndofHeader == NULL) {   
      HttpInstance->IsRxDone = FALSE;
      RxToken->Packet.RxData->DataLength = DEF_BUF_LEN;
      RxToken->Packet.RxData->FragmentTable[0].FragmentLength = DEF_BUF_LEN;
      Status = Tcp4->Receive (Tcp4, RxToken);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Tcp4 receive failed: %r\n", Status));
        goto Error;
      }
      
      while (!HttpInstance->IsRxDone) {
       Tcp4->Poll (Tcp4);
      }    

      Status = RxToken->CompletionToken.Status;
      if (EFI_ERROR (Status)) {
        goto Error;
      }

      //
      // Append the response string.
      //
      BufferSize = SizeofHeaders + RxToken->Packet.RxData->FragmentTable[0].FragmentLength;
      Buffer = AllocateZeroPool (BufferSize);
      if (Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }

      if (HttpHeaders != NULL) {
        CopyMem (Buffer, HttpHeaders, SizeofHeaders);
        FreePool (HttpHeaders);
      }

      CopyMem (
        Buffer + SizeofHeaders,
        RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer,
        RxToken->Packet.RxData->FragmentTable[0].FragmentLength
        );
      HttpHeaders   = Buffer;
      SizeofHeaders = BufferSize;

      //
      // Check whether we received end of HTTP headers.
      //
      EndofHeader = AsciiStrStr (HttpHeaders, HTTP_END_OF_HDR_STR); 
    };

    //
    // Skip the CRLF after the HTTP headers.
    //
    EndofHeader = EndofHeader + AsciiStrLen (HTTP_END_OF_HDR_STR);

    //
    // Cache the part of body.
    //
    BodyLen = BufferSize - (EndofHeader - HttpHeaders);
    if (BodyLen > 0) {
      if (HttpInstance->CacheBody != NULL) {
        FreePool (HttpInstance->CacheBody);
      }

      HttpInstance->CacheBody = AllocateZeroPool (BodyLen);
      if (HttpInstance->CacheBody == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error;
      }

      CopyMem (HttpInstance->CacheBody, EndofHeader, BodyLen);
      HttpInstance->CacheLen = BodyLen;
    }

    FreePool (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer);
    RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;

    //
    // Search for Status Code.
    //
    StatusCodeStr = HttpHeaders + AsciiStrLen (HTTP_VERSION_STR) + 1;
    if (StatusCodeStr == NULL) {
      goto Error;
    }

    StatusCode = AsciiStrDecimalToUintn (StatusCodeStr);

    //
    // Remove the first line of HTTP message, e.g. "HTTP/1.1 200 OK\r\n".
    //
    Tmp = AsciiStrStr (HttpHeaders, HTTP_CRLF_STR);
    if (Tmp == NULL) {
      goto Error;
    }

    Tmp = Tmp + AsciiStrLen (HTTP_CRLF_STR);
    SizeofHeaders = SizeofHeaders - (Tmp - HttpHeaders);
    HeaderTmp = AllocateZeroPool (SizeofHeaders);
    if (HeaderTmp == NULL) {
      goto Error;
    }

    CopyMem (HeaderTmp, Tmp, SizeofHeaders);
    FreePool (HttpHeaders);
    HttpHeaders = HeaderTmp;

    //
    // Check whether the EFI_HTTP_UTILITIES_PROTOCOL is available.
    //
    if (mHttpUtilities == NULL) {
      Status = EFI_NOT_READY;
      goto Error;
    }
    
    //
    // Parse the HTTP header into array of key/value pairs.
    //
    Status = mHttpUtilities->Parse (
                               mHttpUtilities, 
                               HttpHeaders, 
                               SizeofHeaders, 
                               &HttpMsg->Headers, 
                               &HttpMsg->HeaderCount
                               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    FreePool (HttpHeaders);
    HttpHeaders = NULL;
    
    HttpMsg->Data.Response->StatusCode = HttpMappingToStatusCode (StatusCode);

    //
    // Init message-body parser by header information.  
    //
    Status = EFI_NOT_READY;
    ValueInItem = NULL;
    NetMapRemoveHead (&HttpInstance->TxTokens, (VOID**) &ValueInItem);
    if (ValueInItem == NULL)  {
      goto Error;
    }

    //
    // The first TxToken not transmitted yet, insert back and return error.
    //
    if (!ValueInItem->TcpWrap.IsTxDone) {
      goto Error2;
    }

    Status = HttpInitMsgParser (
               ValueInItem->TcpWrap.Method,
               HttpMsg->Data.Response->StatusCode,
               HttpMsg->HeaderCount,
               HttpMsg->Headers,
               HttpBodyParserCallback,
               (VOID *) ValueInItem,
               &HttpInstance->MsgParser
               );
    if (EFI_ERROR (Status)) {       
      goto Error2;
    }

    //
    // Check whether we received a complete HTTP message.
    //
    if (HttpInstance->CacheBody != NULL) {
      Status = HttpParseMessageBody (HttpInstance->MsgParser, HttpInstance->CacheLen, HttpInstance->CacheBody);
      if (EFI_ERROR (Status)) {
        goto Error2;
      }

      if (HttpIsMessageComplete (HttpInstance->MsgParser)) {
        //
        // Free the MsgParse since we already have a full HTTP message.
        //
        HttpFreeMsgParser (HttpInstance->MsgParser);
        HttpInstance->MsgParser = NULL;
      }
    }

    if ((HttpMsg->Body == NULL) || (HttpMsg->BodyLength == 0)) {    
      Status = EFI_SUCCESS;
      goto Exit;
    }
  }  

  //
  // Receive the response body.
  //
  BodyLen = 0;

  //
  // First check whether we cached some data.
  //
  if (HttpInstance->CacheBody != NULL) {
    //
    // Calculate the length of the cached data.
    //
    if (HttpInstance->NextMsg != NULL) {
      //
      // We have a cached HTTP message which includes a part of HTTP header of next message.
      //
      BodyLen = HttpInstance->NextMsg - (HttpInstance->CacheBody + HttpInstance->CacheOffset);      
    } else {
      BodyLen = HttpInstance->CacheLen - HttpInstance->CacheOffset;
    }

    if (BodyLen > 0) {
      //
      // We have some cached data. Just copy the data and return.
      //
      if (HttpMsg->BodyLength < BodyLen) {
        CopyMem (HttpMsg->Body, HttpInstance->CacheBody + HttpInstance->CacheOffset, HttpMsg->BodyLength);
        HttpInstance->CacheOffset = HttpInstance->CacheOffset + HttpMsg->BodyLength;
      } else {
        //
        // Copy all cached data out.
        //
        CopyMem (HttpMsg->Body, HttpInstance->CacheBody + HttpInstance->CacheOffset, BodyLen);
        HttpInstance->CacheOffset = BodyLen + HttpInstance->CacheOffset;
        HttpMsg->BodyLength = BodyLen;

        if (HttpInstance->NextMsg == NULL) {
          //
          // There is no HTTP header of next message. Just free the cache buffer.
          //
          FreePool (HttpInstance->CacheBody);
          HttpInstance->CacheBody   = NULL;
          HttpInstance->NextMsg     = NULL;
          HttpInstance->CacheOffset = 0;
        }
      }
      //
      // Return since we aready received required data.
      //
      Status = EFI_SUCCESS;
      goto Exit;
    } 

    if (BodyLen == 0 && HttpInstance->MsgParser == NULL) {
      //
      // We received a complete HTTP message, and we don't have more data to return to caller.
      //
      HttpMsg->BodyLength = 0;
      Status = EFI_SUCCESS;
      goto Exit;      
    }    
  }

  ASSERT (HttpInstance->MsgParser != NULL);

  //
  // We still need receive more data when there is no cache data and MsgParser is not NULL;
  //
  RxToken = &Wrap->TcpWrap.RxToken;

  RxToken->Packet.RxData->DataLength = (UINT32) HttpMsg->BodyLength;
  RxToken->Packet.RxData->FragmentTable[0].FragmentLength = (UINT32) HttpMsg->BodyLength;
  RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer = (VOID *) HttpMsg->Body;

  RxToken->CompletionToken.Status = EFI_NOT_READY;
  Status = Tcp4->Receive (Tcp4, RxToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Tcp4 receive failed: %r\n", Status));
    goto Error;
  }

  return Status;

Exit:
  Item = NetMapFindKey (&Wrap->HttpInstance->RxTokens, Wrap->HttpToken);
  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->HttpInstance->RxTokens, Item, NULL);
  }
  Token->Status = Status;
  gBS->SignalEvent (Token->Event);
  FreePool (Wrap);
  return Status;

Error2:
  NetMapInsertHead (&HttpInstance->TxTokens, ValueInItem->HttpToken, ValueInItem);

Error:
  if (Wrap != NULL) {
    if (Wrap->TcpWrap.RxToken.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.RxToken.CompletionToken.Event);
    }
    RxToken = &Wrap->TcpWrap.RxToken;
    if (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL) {
      FreePool (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer);
      RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
    }
    FreePool (Wrap);
  }

  if (HttpInstance->RxToken.CompletionToken.Event != NULL) {
    gBS->CloseEvent (HttpInstance->RxToken.CompletionToken.Event);
    HttpInstance->RxToken.CompletionToken.Event = NULL;
  }

  RxToken = &HttpInstance->RxToken;
  if (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer != NULL) {
    FreePool (RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer);
    RxToken->Packet.RxData->FragmentTable[0].FragmentBuffer = NULL;
  }
  
  if (HttpHeaders != NULL) {
    FreePool (HttpHeaders);
  }

  if (HttpMsg->Headers != NULL) {
    FreePool (HttpMsg->Headers);
  }

  if (HttpInstance->CacheBody != NULL) {
    FreePool (HttpInstance->CacheBody);
    HttpInstance->CacheBody = NULL;
  }

  Token->Status = Status;
  gBS->SignalEvent (Token->Event);

  return Status;  

}


/**
  The Response() function queues an HTTP response to this HTTP instance, similar to
  Receive() function in the EFI TCP driver. When the HTTP request is sent successfully,
  or if there is an error, Status in token will be updated and Event will be signaled.

  The HTTP driver will queue a receive token to the underlying TCP instance. When data
  is received in the underlying TCP instance, the data will be parsed and Token will
  be populated with the response data. If the data received from the remote host
  contains an incomplete or invalid HTTP header, the HTTP driver will continue waiting
  (asynchronously) for more data to be sent from the remote host before signaling
  Event in Token.

  It is the responsibility of the caller to allocate a buffer for Body and specify the
  size in BodyLength. If the remote host provides a response that contains a content
  body, up to BodyLength bytes will be copied from the receive buffer into Body and
  BodyLength will be updated with the amount of bytes received and copied to Body. This
  allows the client to download a large file in chunks instead of into one contiguous
  block of memory. Similar to HTTP request, if Body is not NULL and BodyLength is
  non-zero and all other fields are NULL or 0, the HTTP driver will queue a receive
  token to underlying TCP instance. If data arrives in the receive buffer, up to
  BodyLength bytes of data will be copied to Body. The HTTP driver will then update
  BodyLength with the amount of bytes received and copied to Body.

  If the HTTP driver does not have an open underlying TCP connection with the host
  specified in the response URL, Request() will return EFI_ACCESS_DENIED. This is
  consistent with RFC 2616 recommendation that HTTP clients should attempt to maintain
  an open TCP connection between client and host.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[in]  Token               Pointer to storage containing HTTP response token.

  @retval EFI_SUCCESS             Allocation succeeded.
  @retval EFI_NOT_STARTED         This EFI HTTP Protocol instance has not been
                                  initialized.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  Token is NULL.
                                  Token->Message->Headers is NULL.
                                  Token->Message is NULL.
                                  Token->Message->Body is not NULL,
                                  Token->Message->BodyLength is non-zero, and
                                  Token->Message->Data is NULL, but a previous call to
                                  Response() has not been completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate enough system resources.
  @retval EFI_ACCESS_DENIED       An open TCP connection is not present with the host
                                  specified by response URL.
**/
EFI_STATUS
EFIAPI
EfiHttpResponse (
  IN  EFI_HTTP_PROTOCOL         *This,
  IN  EFI_HTTP_TOKEN            *Token
  )
{
  EFI_STATUS                    Status;
  EFI_HTTP_MESSAGE              *HttpMsg;
  HTTP_PROTOCOL                 *HttpInstance;
  HTTP_TOKEN_WRAP               *Wrap;

  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpMsg = Token->Message;
  if (HttpMsg == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->State != HTTP_STATE_TCP_CONNECTED) {
    return EFI_NOT_STARTED;
  }

  if (HttpInstance->LocalAddressIsIPv6) {
    return EFI_UNSUPPORTED;
  }  

  //
  // Check whether the token already existed.
  //
  if (EFI_ERROR (NetMapIterate (&HttpInstance->RxTokens, HttpTokenExist, Token))) {
    return EFI_ACCESS_DENIED;   
  }

  Wrap = AllocateZeroPool (sizeof (HTTP_TOKEN_WRAP));
  if (Wrap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Wrap->HttpInstance = HttpInstance;
  Wrap->HttpToken    = Token;

  Status = HttpCreateTcp4RxEvent (Wrap);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Status = NetMapInsertTail (&HttpInstance->RxTokens, Token, Wrap);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // If already have pending RxTokens, return directly.
  //
  if (NetMapGetCount (&HttpInstance->RxTokens) > 1) {
    return EFI_SUCCESS;
  }

  return HttpResponseWorker (Wrap);

Error:
  if (Wrap != NULL) {
    if (Wrap->TcpWrap.RxToken.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.RxToken.CompletionToken.Event);
    }
    FreePool (Wrap);
  }  

  return Status;  
}

/**
  The Poll() function can be used by network drivers and applications to increase the
  rate that data packets are moved between the communication devices and the transmit
  and receive queues.

  In some systems, the periodic timer event in the managed network driver may not poll
  the underlying communications device fast enough to transmit and/or receive all data
  packets without missing incoming packets or dropping outgoing packets. Drivers and
  applications that are experiencing packet loss should try calling the Poll() function
  more often.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.

  @retval EFI_SUCCESS             Incoming or outgoing data was processed.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_INVALID_PARAMETER   This is NULL.
  @retval EFI_NOT_READY           No incoming or outgoing data is processed.
  @retval EFI_NOT_STARTED         This EFI HTTP Protocol instance has not been started.

**/
EFI_STATUS
EFIAPI
EfiHttpPoll (
  IN  EFI_HTTP_PROTOCOL         *This
  )
{
  HTTP_PROTOCOL                 *HttpInstance;
  EFI_STATUS                    Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance != NULL);

  if (HttpInstance->LocalAddressIsIPv6) {
    return EFI_UNSUPPORTED;
  }

  if (HttpInstance->Tcp4 == NULL || HttpInstance->State != HTTP_STATE_TCP_CONNECTED) {
    return EFI_NOT_STARTED;
  }

  Status = HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);

  DispatchDpc ();

  return Status;
}
