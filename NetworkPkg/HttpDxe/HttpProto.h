/** @file
  The header files of miscellaneous routines for HttpDxe driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_HTTP_PROTO_H__
#define __EFI_HTTP_PROTO_H__

#define DEF_BUF_LEN                         2048

#define HTTP_SERVICE_SIGNATURE  SIGNATURE_32('H', 't', 't', 'S')

#define HTTP_SERVICE_FROM_PROTOCOL(a) \
  CR ( \
  (a), \
  HTTP_SERVICE, \
  ServiceBinding, \
  HTTP_SERVICE_SIGNATURE \
  )

//
// The state of HTTP protocol. It starts from UNCONFIGED.
//
#define HTTP_STATE_UNCONFIGED        0
#define HTTP_STATE_HTTP_CONFIGED     1
#define HTTP_STATE_TCP_CONFIGED      2
#define HTTP_STATE_TCP_UNCONFIGED    3
#define HTTP_STATE_TCP_CONNECTED     4
#define HTTP_STATE_TCP_CLOSED        5

//
// TCP configured data.
//
#define HTTP_TOS_DEAULT              8
#define HTTP_TTL_DEAULT              255
#define HTTP_BUFFER_SIZE_DEAULT      65535
#define HTTP_MAX_SYN_BACK_LOG        5
#define HTTP_CONNECTION_TIMEOUT      60
#define HTTP_DATA_RETRIES            12
#define HTTP_FIN_TIMEOUT             2
#define HTTP_KEEP_ALIVE_PROBES       6
#define HTTP_KEEP_ALIVE_TIME         7200
#define HTTP_KEEP_ALIVE_INTERVAL     30

#define HTTP_URL_BUFFER_LEN          4096

typedef struct _HTTP_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    ImageHandle;
  EFI_HANDLE                    ControllerHandle;
  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  EFI_HANDLE                    TcpChildHandle;
  INTN                          State;
} HTTP_SERVICE;

typedef struct {
  EFI_TCP4_IO_TOKEN             TxToken;
  EFI_TCP4_TRANSMIT_DATA        TxData;
  BOOLEAN                       IsTxDone;
  EFI_TCP4_IO_TOKEN             RxToken;
  EFI_TCP4_RECEIVE_DATA         RxData;
  BOOLEAN                       IsRxDone;
  UINTN                         BodyLen;
  EFI_HTTP_METHOD               Method;
} HTTP_TCP_TOKEN_WRAP;

typedef struct _HTTP_PROTOCOL {
  UINT32                        Signature;
  EFI_HTTP_PROTOCOL             Http;
  EFI_HANDLE                    Handle;
  HTTP_SERVICE                  *Service;
  LIST_ENTRY                    Link;   // Link to all HTTP instance from the service.
  BOOLEAN                       InDestroy;
  INTN                          State;

  EFI_HANDLE                    TcpChildHandle;
  EFI_TCP4_PROTOCOL             *Tcp4;
  EFI_TCP4_CONFIG_DATA          Tcp4CfgData;
  EFI_TCP4_OPTION               Tcp4Option;

  EFI_TCP4_CONNECTION_TOKEN     ConnToken;
  BOOLEAN                       IsConnDone;
  EFI_TCP4_CLOSE_TOKEN          CloseToken;
  BOOLEAN                       IsCloseDone;

  CHAR8                         *RemoteHost;
  UINT16                        RemotePort;
  EFI_IPv4_ADDRESS              RemoteAddr;
  //
  // RxToken used for receiving HTTP header.
  //
  EFI_TCP4_IO_TOKEN             RxToken;
  EFI_TCP4_RECEIVE_DATA         RxData;
  BOOLEAN                       IsRxDone;

  CHAR8                         *CacheBody;
  CHAR8                         *NextMsg;
  UINTN                         CacheLen;
  UINTN                         CacheOffset;

  //
  // HTTP message-body parser.
  //
  VOID                          *MsgParser;
  
  EFI_HTTP_VERSION              HttpVersion;
  UINT32                        TimeOutMillisec;
  BOOLEAN                       LocalAddressIsIPv6;

  EFI_HTTPv4_ACCESS_POINT       IPv4Node;

  NET_MAP                       TxTokens;
  NET_MAP                       RxTokens;

  CHAR8                         *Url;
} HTTP_PROTOCOL;

typedef struct {
  EFI_HTTP_TOKEN                *HttpToken;
  HTTP_PROTOCOL                 *HttpInstance;
  HTTP_TCP_TOKEN_WRAP           TcpWrap;
} HTTP_TOKEN_WRAP;


#define HTTP_PROTOCOL_SIGNATURE  SIGNATURE_32('H', 't', 't', 'P')

#define HTTP_INSTANCE_FROM_PROTOCOL(a) \
  CR ( \
  (a), \
  HTTP_PROTOCOL, \
  Http, \
  HTTP_PROTOCOL_SIGNATURE \
  )

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
  );

/**
  Create events for the TCP4 connection token and TCP4 close token.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4ConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Close events in the TCP4 connection token and TCP4 close token.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
HttpCloseTcp4ConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Create event for the TCP4 transmit token.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4TxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap
  );

/**
  Create event for the TCP4 receive token which is used to receive HTTP header.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4RxEventForHeader (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Create event for the TCP4 receive token which is used to receive HTTP body.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcp4RxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap 
  );

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
  );

/**
  Clean up the HTTP child, release all the resources used by it.

  @param[in]  HttpInstance       The HTTP child to clean up.

**/
VOID
HttpCleanProtocol (
  IN  HTTP_PROTOCOL          *HttpInstance
  );

/**
  Establish TCP connection with HTTP server.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is established.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateConnection (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Close existing TCP connection.

  @param[in]  HttpInstance       The HTTP instance private data.

  @retval EFI_SUCCESS            The TCP connection is closed.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpCloseConnection (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

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
  );

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
  );

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
  );

/**
  Translate the status code in HTTP message to EFI_HTTP_STATUS_CODE defined 
  in UEFI 2.5 specification.

  @param[in]  StatusCode         The status code value in HTTP message.

  @return                        Value defined in EFI_HTTP_STATUS_CODE .

**/
EFI_HTTP_STATUS_CODE
HttpMappingToStatusCode (
  IN UINTN                  StatusCode
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  The work function of EfiHttpResponse().

  @param[in]  Wrap                Pointer to HTTP token's wrap data.

  @retval EFI_SUCCESS             Allocation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to complete the opration due to lack of resources.
  @retval EFI_NOT_READY           Can't find a corresponding TxToken.

**/
EFI_STATUS
HttpResponseWorker (
  IN  HTTP_TOKEN_WRAP           *Wrap
  );

#endif
