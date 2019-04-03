/** @file
  Implementation of EFI_HTTP_PROTOCOL protocol interfaces.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
                                  HTTP instance. It is the responsibility of the caller
                                  to allocate the memory for HttpConfigData and
                                  HttpConfigData->AccessPoint.IPv6Node/IPv4Node. In fact,
                                  it is recommended to allocate sufficient memory to record
                                  IPv6Node since it is big enough for all possibilities.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  HttpConfigData is NULL.
                                  HttpConfigData->AccessPoint.IPv4Node or
                                  HttpConfigData->AccessPoint.IPv6Node is NULL.
  @retval EFI_NOT_STARTED         This EFI HTTP Protocol instance has not been started.

**/
EFI_STATUS
EFIAPI
EfiHttpGetModeData (
  IN  EFI_HTTP_PROTOCOL         *This,
  OUT EFI_HTTP_CONFIG_DATA      *HttpConfigData
  )
{
  HTTP_PROTOCOL                 *HttpInstance;

  //
  // Check input parameters.
  //
  if ((This == NULL) || (HttpConfigData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);

  if ((HttpConfigData->AccessPoint.IPv6Node == NULL) ||
      (HttpConfigData->AccessPoint.IPv4Node == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HttpInstance->State < HTTP_STATE_HTTP_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  HttpConfigData->HttpVersion        = HttpInstance->HttpVersion;
  HttpConfigData->TimeOutMillisec    = HttpInstance->TimeOutMillisec;
  HttpConfigData->LocalAddressIsIPv6 = HttpInstance->LocalAddressIsIPv6;

  if (HttpInstance->LocalAddressIsIPv6) {
    CopyMem (
      HttpConfigData->AccessPoint.IPv6Node,
      &HttpInstance->Ipv6Node,
      sizeof (HttpInstance->Ipv6Node)
    );
  } else {
    CopyMem (
      HttpConfigData->AccessPoint.IPv4Node,
      &HttpInstance->IPv4Node,
      sizeof (HttpInstance->IPv4Node)
      );
  }

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

  No other EFI HTTP function can be executed by this instance until the Configure()
  function is executed and returns successfully.

  @param[in]  This                Pointer to EFI_HTTP_PROTOCOL instance.
  @param[in]  HttpConfigData      Pointer to the configure data to configure the instance.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   One or more of the following conditions is TRUE:
                                  This is NULL.
                                  HttpConfigData->LocalAddressIsIPv6 is FALSE and
                                  HttpConfigData->AccessPoint.IPv4Node is NULL.
                                  HttpConfigData->LocalAddressIsIPv6 is TRUE and
                                  HttpConfigData->AccessPoint.IPv6Node is NULL.
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
  IN  EFI_HTTP_CONFIG_DATA      *HttpConfigData OPTIONAL
  )
{
  HTTP_PROTOCOL                 *HttpInstance;
  EFI_STATUS                    Status;

  //
  // Check input parameters.
  //
  if (This == NULL ||
      (HttpConfigData != NULL &&
       ((HttpConfigData->LocalAddressIsIPv6 && HttpConfigData->AccessPoint.IPv6Node == NULL) ||
        (!HttpConfigData->LocalAddressIsIPv6 && HttpConfigData->AccessPoint.IPv4Node == NULL)))) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);
  ASSERT (HttpInstance->Service != NULL);

  if (HttpConfigData != NULL) {

    if (HttpConfigData->HttpVersion >= HttpVersionUnsupported) {
      return EFI_UNSUPPORTED;
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
      CopyMem (
        &HttpInstance->Ipv6Node,
        HttpConfigData->AccessPoint.IPv6Node,
        sizeof (HttpInstance->Ipv6Node)
        );
    } else {
      CopyMem (
        &HttpInstance->IPv4Node,
        HttpConfigData->AccessPoint.IPv4Node,
        sizeof (HttpInstance->IPv4Node)
        );
    }

    //
    // Creat Tcp child
    //
    Status = HttpInitProtocol (HttpInstance, HttpInstance->LocalAddressIsIPv6);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    HttpInstance->State = HTTP_STATE_HTTP_CONFIGED;
    return EFI_SUCCESS;

  } else {
    //
    // Reset all the resources related to HttpInsance.
    //
    HttpCleanProtocol (HttpInstance);
    HttpInstance->State = HTTP_STATE_UNCONFIGED;
    return EFI_SUCCESS;
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
                                  Token is NULL.
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
  UINTN                         HostNameSize;
  UINT16                        RemotePort;
  HTTP_PROTOCOL                 *HttpInstance;
  BOOLEAN                       Configure;
  BOOLEAN                       ReConfigure;
  BOOLEAN                       TlsConfigure;
  CHAR8                         *RequestMsg;
  CHAR8                         *Url;
  UINTN                         UrlLen;
  CHAR16                        *HostNameStr;
  HTTP_TOKEN_WRAP               *Wrap;
  CHAR8                         *FileUrl;
  UINTN                         RequestMsgSize;
  EFI_HANDLE                    ImageHandle;

  //
  // Initializations
  //
  Url = NULL;
  UrlParser = NULL;
  RemotePort = 0;
  HostName = NULL;
  RequestMsg = NULL;
  HostNameStr = NULL;
  Wrap = NULL;
  FileUrl = NULL;
  TlsConfigure = FALSE;

  if ((This == NULL) || (Token == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpMsg = Token->Message;
  if (HttpMsg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Request = HttpMsg->Data.Request;

  //
  // Only support GET, HEAD, DELETE, PATCH, PUT and POST method in current implementation.
  //
  if ((Request != NULL) && (Request->Method != HttpMethodGet) &&
      (Request->Method != HttpMethodHead) && (Request->Method != HttpMethodDelete) &&
      (Request->Method != HttpMethodPut) && (Request->Method != HttpMethodPost) &&
      (Request->Method != HttpMethodPatch)) {
    return EFI_UNSUPPORTED;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);

  //
  // Capture the method into HttpInstance.
  //
  if (Request != NULL) {
    HttpInstance->Method = Request->Method;
  }

  if (HttpInstance->State < HTTP_STATE_HTTP_CONFIGED) {
    return EFI_NOT_STARTED;
  }

  if (Request == NULL) {
    //
    // Request would be NULL only for PUT/POST/PATCH operation (in the current implementation)
    //
    if ((HttpInstance->Method != HttpMethodPut) &&
        (HttpInstance->Method != HttpMethodPost) &&
        (HttpInstance->Method != HttpMethodPatch)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // For PUT/POST/PATCH, we need to have the TCP already configured. Bail out if it is not!
    //
    if (HttpInstance->State < HTTP_STATE_TCP_CONFIGED) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // We need to have the Message Body for sending the HTTP message across in these cases.
    //
    if (HttpMsg->Body == NULL || HttpMsg->BodyLength == 0) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Use existing TCP instance to transmit the packet.
    //
    Configure   = FALSE;
    ReConfigure = FALSE;
  } else {
    //
    // Check whether the token already existed.
    //
    if (EFI_ERROR (NetMapIterate (&HttpInstance->TxTokens, HttpTokenExist, Token))) {
      return EFI_ACCESS_DENIED;
    }

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


    UnicodeStrToAsciiStrS (Request->Url, Url, UrlLen);

    //
    // From the information in Url, the HTTP instance will
    // be able to determine whether to use http or https.
    //
    HttpInstance->UseHttps = IsHttpsUrl (Url);

    //
    // HTTP is disabled, return directly if the URI is not HTTPS.
    //
    if (!PcdGetBool (PcdAllowHttpConnections) && !(HttpInstance->UseHttps)) {

      DEBUG ((EFI_D_ERROR, "EfiHttpRequest: HTTP is disabled.\n"));

      return EFI_ACCESS_DENIED;
    }

    //
    // Check whether we need to create Tls child and open the TLS protocol.
    //
    if (HttpInstance->UseHttps && HttpInstance->TlsChildHandle == NULL) {
      //
      // Use TlsSb to create Tls child and open the TLS protocol.
      //
      if (HttpInstance->LocalAddressIsIPv6) {
        ImageHandle = HttpInstance->Service->Ip6DriverBindingHandle;
      } else {
        ImageHandle = HttpInstance->Service->Ip4DriverBindingHandle;
      }

      HttpInstance->TlsChildHandle = TlsCreateChild (
                                       ImageHandle,
                                       &(HttpInstance->TlsSb),
                                       &(HttpInstance->Tls),
                                       &(HttpInstance->TlsConfiguration)
                                       );
      if (HttpInstance->TlsChildHandle == NULL) {
        return EFI_DEVICE_ERROR;
      }

      TlsConfigure = TRUE;
    }

    UrlParser = NULL;
    Status = HttpParseUrl (Url, (UINT32) AsciiStrLen (Url), FALSE, &UrlParser);
    if (EFI_ERROR (Status)) {
      goto Error1;
    }

    Status = HttpUrlGetHostName (Url, UrlParser, &HostName);
    if (EFI_ERROR (Status)) {
      goto Error1;
    }

    if (HttpInstance->LocalAddressIsIPv6) {
      HostNameSize = AsciiStrSize (HostName);

      if (HostNameSize > 2 && HostName[0] == '[' && HostName[HostNameSize - 2] == ']') {
        //
        // HostName format is expressed as IPv6, so, remove '[' and ']'.
        //
        HostNameSize -= 2;
        CopyMem (HostName, HostName + 1, HostNameSize - 1);
        HostName[HostNameSize - 1] = '\0';
      }
    }

    Status = HttpUrlGetPort (Url, UrlParser, &RemotePort);
    if (EFI_ERROR (Status)) {
      if (HttpInstance->UseHttps) {
        RemotePort = HTTPS_DEFAULT_PORT;
      } else {
        RemotePort = HTTP_DEFAULT_PORT;
      }
    }
    //
    // If Configure is TRUE, it indicates the first time to call Request();
    // If ReConfigure is TRUE, it indicates the request URL is not same
    // with the previous call to Request();
    //
    Configure   = TRUE;
    ReConfigure = TRUE;

    if (HttpInstance->RemoteHost == NULL) {
      //
      // Request() is called the first time.
      //
      ReConfigure = FALSE;
    } else {
      if ((HttpInstance->RemotePort == RemotePort) &&
          (AsciiStrCmp (HttpInstance->RemoteHost, HostName) == 0) &&
          (!HttpInstance->UseHttps || (HttpInstance->UseHttps &&
                                       !TlsConfigure &&
                                       HttpInstance->TlsSessionState == EfiTlsSessionDataTransferring))) {
        //
        // Host Name and port number of the request URL are the same with previous call to Request().
        // If Https protocol used, the corresponding SessionState is EfiTlsSessionDataTransferring.
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

          Status = HttpCreateTcpTxEvent (Wrap);
          if (EFI_ERROR (Status)) {
            goto Error1;
          }

          Status = NetMapInsertTail (&HttpInstance->TxTokens, Token, Wrap);
          if (EFI_ERROR (Status)) {
            goto Error1;
          }

          Wrap->TcpWrap.Method = Request->Method;

          FreePool (HostName);

          HttpUrlFreeParser (UrlParser);

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
  }

  if (Configure) {
    //
    // Parse Url for IPv4 or IPv6 address, if failed, perform DNS resolution.
    //
    if (!HttpInstance->LocalAddressIsIPv6) {
      Status = NetLibAsciiStrToIp4 (HostName, &HttpInstance->RemoteAddr);
    } else {
      Status = HttpUrlGetIp6 (Url, UrlParser, &HttpInstance->RemoteIpv6Addr);
    }

    if (EFI_ERROR (Status)) {
      HostNameSize = AsciiStrSize (HostName);
      HostNameStr = AllocateZeroPool (HostNameSize * sizeof (CHAR16));
      if (HostNameStr == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error1;
      }

      AsciiStrToUnicodeStrS (HostName, HostNameStr, HostNameSize);
      if (!HttpInstance->LocalAddressIsIPv6) {
        Status = HttpDns4 (HttpInstance, HostNameStr, &HttpInstance->RemoteAddr);
      } else {
        Status = HttpDns6 (HttpInstance, HostNameStr, &HttpInstance->RemoteIpv6Addr);
      }

      FreePool (HostNameStr);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Error: Could not retrieve the host address from DNS server.\n"));
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
    if (!HttpInstance->LocalAddressIsIPv6) {
      ASSERT (HttpInstance->Tcp4 != NULL);
    } else {
      ASSERT (HttpInstance->Tcp6 != NULL);
    }

    if (HttpInstance->UseHttps && !TlsConfigure) {
      Status = TlsCloseSession (HttpInstance);
      if (EFI_ERROR (Status)) {
        goto Error1;
      }

      TlsCloseTxRxEvent (HttpInstance);
    }

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
  if (Request != NULL) {
    Wrap->TcpWrap.Method = Request->Method;
  }

  Status = HttpInitSession (
             HttpInstance,
             Wrap,
             Configure || ReConfigure,
             TlsConfigure
             );
  if (EFI_ERROR (Status)) {
    goto Error2;
  }

  if (!Configure && !ReConfigure && !TlsConfigure) {
    //
    // For the new HTTP token, create TX TCP token events.
    //
    Status = HttpCreateTcpTxEvent (Wrap);
    if (EFI_ERROR (Status)) {
      goto Error1;
    }
  }

  //
  // Create request message.
  //
  FileUrl = Url;
  if (Url != NULL && *FileUrl != '/') {
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

  Status = HttpGenRequestMessage (HttpMsg, FileUrl, &RequestMsg, &RequestMsgSize);

  if (EFI_ERROR (Status) || NULL == RequestMsg) {
    goto Error3;
  }

  //
  // Every request we insert a TxToken and a response call would remove the TxToken.
  // In cases of PUT/POST/PATCH, after an initial request-response pair, we would do a
  // continuous request without a response call. So, in such cases, where Request
  // structure is NULL, we would not insert a TxToken.
  //
  if (Request != NULL) {
    Status = NetMapInsertTail (&HttpInstance->TxTokens, Token, Wrap);
    if (EFI_ERROR (Status)) {
      goto Error4;
    }
  }

  //
  // Transmit the request message.
  //
  Status = HttpTransmitTcp (
             HttpInstance,
             Wrap,
             (UINT8*) RequestMsg,
             RequestMsgSize
             );
  if (EFI_ERROR (Status)) {
    goto Error5;
  }

  DispatchDpc ();

  if (HostName != NULL) {
    FreePool (HostName);
  }

  if (UrlParser != NULL) {
    HttpUrlFreeParser (UrlParser);
  }

  return EFI_SUCCESS;

Error5:
  //
  // We would have inserted a TxToken only if Request structure is not NULL.
  // Hence check before we do a remove in this error case.
  //
  if (Request != NULL) {
    NetMapRemoveTail (&HttpInstance->TxTokens, NULL);
  }

Error4:
  if (RequestMsg != NULL) {
    FreePool (RequestMsg);
  }

Error3:
  if (HttpInstance->UseHttps) {
    TlsCloseSession (HttpInstance);
    TlsCloseTxRxEvent (HttpInstance);
  }

Error2:
  HttpCloseConnection (HttpInstance);

  HttpCloseTcpConnCloseEvent (HttpInstance);
  if (NULL != Wrap->TcpWrap.Tx4Token.CompletionToken.Event) {
    gBS->CloseEvent (Wrap->TcpWrap.Tx4Token.CompletionToken.Event);
    Wrap->TcpWrap.Tx4Token.CompletionToken.Event = NULL;
  }
  if (NULL != Wrap->TcpWrap.Tx6Token.CompletionToken.Event) {
    gBS->CloseEvent (Wrap->TcpWrap.Tx6Token.CompletionToken.Event);
    Wrap->TcpWrap.Tx6Token.CompletionToken.Event = NULL;
  }

Error1:
  if (HostName != NULL) {
    FreePool (HostName);
  }
  if (Wrap != NULL) {
    FreePool (Wrap);
  }
  if (UrlParser != NULL) {
    HttpUrlFreeParser (UrlParser);
  }

  return Status;

}

/**
  Cancel a user's Token.

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
  HTTP_PROTOCOL             *HttpInstance;

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
  HttpInstance = Wrap->HttpInstance;

  if (!HttpInstance->LocalAddressIsIPv6) {
    if (Wrap->TcpWrap.Rx4Token.CompletionToken.Event != NULL) {
      //
      // Cancle the Token before close its Event.
      //
      HttpInstance->Tcp4->Cancel (HttpInstance->Tcp4, &Wrap->TcpWrap.Rx4Token.CompletionToken);

      //
      // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
      //
      DispatchDpc ();
    }
  } else {
    if (Wrap->TcpWrap.Rx6Token.CompletionToken.Event != NULL) {
      //
      // Cancle the Token before close its Event.
      //
      HttpInstance->Tcp6->Cancel (HttpInstance->Tcp6, &Wrap->TcpWrap.Rx6Token.CompletionToken);

      //
      // Dispatch the DPC queued by the NotifyFunction of the canceled token's events.
      //
      DispatchDpc ();
    }
  }

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

  if (!HttpInstance->UseHttps) {
    //
    // Then check the tokens queued by EfiHttpResponse(), except for Https.
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
  } else {
    if (!HttpInstance->LocalAddressIsIPv6) {
      HttpInstance->Tcp4->Cancel (HttpInstance->Tcp4, &HttpInstance->Tcp4TlsRxToken.CompletionToken);
    } else {
      HttpInstance->Tcp6->Cancel (HttpInstance->Tcp6, &HttpInstance->Tcp6TlsRxToken.CompletionToken);
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
  HTTP_CALLBACK_DATA            *CallbackData;
  HTTP_TOKEN_WRAP               *Wrap;
  UINTN                         BodyLength;
  CHAR8                         *Body;

  if (EventType != BodyParseEventOnComplete) {
    return EFI_SUCCESS;
  }

  if (Data == NULL || Length != 0 || Context == NULL) {
    return EFI_SUCCESS;
  }

  CallbackData = (HTTP_CALLBACK_DATA *) Context;

  Wrap       = (HTTP_TOKEN_WRAP *) (CallbackData->Wrap);
  Body       = CallbackData->ParseData;
  BodyLength = CallbackData->ParseDataLength;

  if (Data < Body + BodyLength) {
    Wrap->HttpInstance->NextMsg = Data;
  } else {
    Wrap->HttpInstance->NextMsg = NULL;
  }

  return EFI_SUCCESS;
}

/**
  The work function of EfiHttpResponse().

  @param[in]  Wrap                Pointer to HTTP token's wrap data.

  @retval EFI_SUCCESS             Allocation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to complete the opration due to lack of resources.
  @retval EFI_NOT_READY           Can't find a corresponding Tx4Token/Tx6Token or
                                  the EFI_HTTP_UTILITIES_PROTOCOL is not available.

**/
EFI_STATUS
HttpResponseWorker (
  IN  HTTP_TOKEN_WRAP           *Wrap
  )
{
  EFI_STATUS                    Status;
  EFI_HTTP_MESSAGE              *HttpMsg;
  CHAR8                         *EndofHeader;
  CHAR8                         *HttpHeaders;
  UINTN                         SizeofHeaders;
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
  NET_FRAGMENT                  Fragment;

  if (Wrap == NULL || Wrap->HttpInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = Wrap->HttpInstance;
  Token = Wrap->HttpToken;
  HttpMsg = Token->Message;

  HttpInstance->EndofHeader = NULL;
  HttpInstance->HttpHeaders = NULL;
  HttpMsg->Headers          = NULL;
  HttpHeaders               = NULL;
  SizeofHeaders             = 0;
  BufferSize                = 0;
  EndofHeader               = NULL;
  ValueInItem               = NULL;
  Fragment.Len              = 0;
  Fragment.Bulk             = NULL;

  if (HttpMsg->Data.Response != NULL) {
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

    HttpInstance->EndofHeader = &EndofHeader;
    HttpInstance->HttpHeaders = &HttpHeaders;


    if (HttpInstance->TimeoutEvent == NULL) {
      //
      // Create TimeoutEvent for response
      //
      Status = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &HttpInstance->TimeoutEvent
                      );
      if (EFI_ERROR (Status)) {
        goto Error;
      }
    }

    //
    // Start the timer, and wait Timeout seconds to receive the header packet.
    //
    Status = gBS->SetTimer (HttpInstance->TimeoutEvent, TimerRelative, HTTP_RESPONSE_TIMEOUT * TICKS_PER_SECOND);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Status = HttpTcpReceiveHeader (HttpInstance, &SizeofHeaders, &BufferSize, HttpInstance->TimeoutEvent);

    gBS->SetTimer (HttpInstance->TimeoutEvent, TimerCancel, 0);

    if (EFI_ERROR (Status)) {
      goto Error;
    }

    ASSERT (HttpHeaders != NULL);

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

    //
    // Search for Status Code.
    //
    StatusCodeStr = HttpHeaders + AsciiStrLen (HTTP_VERSION_STR) + 1;
    if (StatusCodeStr == NULL) {
      Status = EFI_NOT_READY;
      goto Error;
    }

    StatusCode = AsciiStrDecimalToUintn (StatusCodeStr);

    //
    // Remove the first line of HTTP message, e.g. "HTTP/1.1 200 OK\r\n".
    //
    Tmp = AsciiStrStr (HttpHeaders, HTTP_CRLF_STR);
    if (Tmp == NULL) {
      Status = EFI_NOT_READY;
      goto Error;
    }

    //
    // We could have response with just a HTTP message and no headers. For Example,
    // "100 Continue". In such cases, we would not want to unnecessarily call a Parse
    // method. A "\r\n" following Tmp string again would indicate an end. Compare and
    // set SizeofHeaders to 0.
    //
    Tmp = Tmp + AsciiStrLen (HTTP_CRLF_STR);
    if (CompareMem (Tmp, HTTP_CRLF_STR, AsciiStrLen (HTTP_CRLF_STR)) == 0) {
      SizeofHeaders = 0;
    } else {
      SizeofHeaders = SizeofHeaders - (Tmp - HttpHeaders);
    }

    HttpMsg->Data.Response->StatusCode = HttpMappingToStatusCode (StatusCode);
    HttpInstance->StatusCode = StatusCode;

    Status = EFI_NOT_READY;
    ValueInItem = NULL;

    //
    // In cases of PUT/POST/PATCH, after an initial request-response pair, we would do a
    // continuous request without a response call. So, we would not do an insert of
    // TxToken. After we have sent the complete file, we will call a response to get
    // a final response from server. In such a case, we would not have any TxTokens.
    // Hence, check that case before doing a NetMapRemoveHead.
    //
    if (!NetMapIsEmpty (&HttpInstance->TxTokens)) {
      NetMapRemoveHead (&HttpInstance->TxTokens, (VOID**) &ValueInItem);
      if (ValueInItem == NULL)  {
        goto Error;
      }

      //
      // The first Tx Token not transmitted yet, insert back and return error.
      //
      if (!ValueInItem->TcpWrap.IsTxDone) {
        goto Error2;
      }
    }

    if (SizeofHeaders != 0) {
      HeaderTmp = AllocateZeroPool (SizeofHeaders);
      if (HeaderTmp == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error2;
      }

      CopyMem (HeaderTmp, Tmp, SizeofHeaders);
      FreePool (HttpHeaders);
      HttpHeaders = HeaderTmp;

      //
      // Check whether the EFI_HTTP_UTILITIES_PROTOCOL is available.
      //
      if (mHttpUtilities == NULL) {
        Status = EFI_NOT_READY;
        goto Error2;
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
        goto Error2;
      }

      FreePool (HttpHeaders);
      HttpHeaders = NULL;


      //
      // Init message-body parser by header information.
      //
      Status = HttpInitMsgParser (
                 HttpInstance->Method,
                 HttpMsg->Data.Response->StatusCode,
                 HttpMsg->HeaderCount,
                 HttpMsg->Headers,
                 HttpBodyParserCallback,
                 (VOID *) (&HttpInstance->CallbackData),
                 &HttpInstance->MsgParser
                 );
      if (EFI_ERROR (Status)) {
        goto Error2;
      }

      //
      // Check whether we received a complete HTTP message.
      //
      if (HttpInstance->CacheBody != NULL) {
        //
        // Record the CallbackData data.
        //
        HttpInstance->CallbackData.Wrap = (VOID *) Wrap;
        HttpInstance->CallbackData.ParseData = (VOID *) HttpInstance->CacheBody;
        HttpInstance->CallbackData.ParseDataLength = HttpInstance->CacheLen;

        //
        // Parse message with CallbackData data.
        //
        Status = HttpParseMessageBody (HttpInstance->MsgParser, HttpInstance->CacheLen, HttpInstance->CacheBody);
        if (EFI_ERROR (Status)) {
          goto Error2;
        }
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
  if (!HttpInstance->UseHttps) {
    Status = HttpTcpReceiveBody (Wrap, HttpMsg);

    if (EFI_ERROR (Status)) {
      goto Error2;
    }

  } else {
    if (HttpInstance->TimeoutEvent == NULL) {
      //
      // Create TimeoutEvent for response
      //
      Status = gBS->CreateEvent (
                      EVT_TIMER,
                      TPL_CALLBACK,
                      NULL,
                      NULL,
                      &HttpInstance->TimeoutEvent
                      );
      if (EFI_ERROR (Status)) {
        goto Error2;
      }
    }

    //
    // Start the timer, and wait Timeout seconds to receive the body packet.
    //
    Status = gBS->SetTimer (HttpInstance->TimeoutEvent, TimerRelative, HTTP_RESPONSE_TIMEOUT * TICKS_PER_SECOND);
    if (EFI_ERROR (Status)) {
      goto Error2;
    }

    Status = HttpsReceive (HttpInstance, &Fragment, HttpInstance->TimeoutEvent);

    gBS->SetTimer (HttpInstance->TimeoutEvent, TimerCancel, 0);

    if (EFI_ERROR (Status)) {
      goto Error2;
    }

    //
    // Process the received the body packet.
    //
    HttpMsg->BodyLength = MIN (Fragment.Len, (UINT32) HttpMsg->BodyLength);

    CopyMem (HttpMsg->Body, Fragment.Bulk, HttpMsg->BodyLength);

    //
    // Record the CallbackData data.
    //
    HttpInstance->CallbackData.Wrap = (VOID *) Wrap;
    HttpInstance->CallbackData.ParseData = HttpMsg->Body;
    HttpInstance->CallbackData.ParseDataLength = HttpMsg->BodyLength;

    //
    // Parse Body with CallbackData data.
    //
    Status = HttpParseMessageBody (
               HttpInstance->MsgParser,
               HttpMsg->BodyLength,
               HttpMsg->Body
               );
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

    //
    // Check whether there is the next message header in the HttpMsg->Body.
    //
    if (HttpInstance->NextMsg != NULL) {
      HttpMsg->BodyLength = HttpInstance->NextMsg - (CHAR8 *) HttpMsg->Body;
    }

    HttpInstance->CacheLen = Fragment.Len - HttpMsg->BodyLength;
    if (HttpInstance->CacheLen != 0) {
      if (HttpInstance->CacheBody != NULL) {
        FreePool (HttpInstance->CacheBody);
      }

      HttpInstance->CacheBody = AllocateZeroPool (HttpInstance->CacheLen);
      if (HttpInstance->CacheBody == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error2;
      }

      CopyMem (HttpInstance->CacheBody, Fragment.Bulk + HttpMsg->BodyLength, HttpInstance->CacheLen);
      HttpInstance->CacheOffset = 0;
      if (HttpInstance->NextMsg != NULL) {
        HttpInstance->NextMsg = HttpInstance->CacheBody;
      }
    }

    if (Fragment.Bulk != NULL) {
      FreePool (Fragment.Bulk);
      Fragment.Bulk = NULL;
    }

    goto Exit;
  }

  return Status;

Exit:
  Item = NetMapFindKey (&Wrap->HttpInstance->RxTokens, Wrap->HttpToken);
  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->HttpInstance->RxTokens, Item, NULL);
  }

  if (HttpInstance->StatusCode >= HTTP_ERROR_OR_NOT_SUPPORT_STATUS_CODE) {
    Token->Status = EFI_HTTP_ERROR;
  } else {
    Token->Status = Status;
  }

  gBS->SignalEvent (Token->Event);
  HttpCloseTcpRxEvent (Wrap);
  FreePool (Wrap);
  return Status;

Error2:
  if (ValueInItem != NULL) {
    NetMapInsertHead (&HttpInstance->TxTokens, ValueInItem->HttpToken, ValueInItem);
  }

Error:
  Item = NetMapFindKey (&Wrap->HttpInstance->RxTokens, Wrap->HttpToken);
  if (Item != NULL) {
    NetMapRemoveItem (&Wrap->HttpInstance->RxTokens, Item, NULL);
  }

  if (!HttpInstance->UseHttps) {
    HttpTcpTokenCleanup (Wrap);
  } else {
    FreePool (Wrap);
  }

  if (HttpHeaders != NULL) {
    FreePool (HttpHeaders);
    HttpHeaders = NULL;
  }

  if (Fragment.Bulk != NULL) {
    FreePool (Fragment.Bulk);
    Fragment.Bulk = NULL;
  }

  if (HttpMsg->Headers != NULL) {
    FreePool (HttpMsg->Headers);
    HttpMsg->Headers = NULL;
  }

  if (HttpInstance->CacheBody != NULL) {
    FreePool (HttpInstance->CacheBody);
    HttpInstance->CacheBody = NULL;
  }

  if (HttpInstance->StatusCode >= HTTP_ERROR_OR_NOT_SUPPORT_STATUS_CODE) {
    Token->Status = EFI_HTTP_ERROR;
  } else {
    Token->Status = Status;
  }

  gBS->SignalEvent (Token->Event);

  return Status;

}


/**
  The Response() function queues an HTTP response to this HTTP instance, similar to
  Receive() function in the EFI TCP driver. When the HTTP response is received successfully,
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

  if (HttpInstance->State != HTTP_STATE_TCP_CONNECTED) {
    return EFI_NOT_STARTED;
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

  //
  // Notes: For Https, receive token wrapped in HTTP_TOKEN_WRAP is not used to
  // receive the https response. A special TlsRxToken is used for receiving TLS
  // related messages. It should be a blocking response.
  //
  if (!HttpInstance->UseHttps) {
    Status = HttpCreateTcpRxEvent (Wrap);
    if (EFI_ERROR (Status)) {
      goto Error;
    }
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
    if (Wrap->TcpWrap.Rx4Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Rx4Token.CompletionToken.Event);
    }

    if (Wrap->TcpWrap.Rx6Token.CompletionToken.Event != NULL) {
      gBS->CloseEvent (Wrap->TcpWrap.Rx6Token.CompletionToken.Event);
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
  EFI_STATUS                    Status;
  HTTP_PROTOCOL                 *HttpInstance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (This);

  if (HttpInstance->State != HTTP_STATE_TCP_CONNECTED) {
    return EFI_NOT_STARTED;
  }

  if (HttpInstance->LocalAddressIsIPv6) {
    if (HttpInstance->Tcp6 == NULL) {
      return EFI_NOT_STARTED;
    }
    Status = HttpInstance->Tcp6->Poll (HttpInstance->Tcp6);
  } else {
    if (HttpInstance->Tcp4 == NULL) {
      return EFI_NOT_STARTED;
    }
    Status = HttpInstance->Tcp4->Poll (HttpInstance->Tcp4);
  }

  DispatchDpc ();

  return Status;
}
