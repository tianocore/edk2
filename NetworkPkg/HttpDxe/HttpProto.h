/** @file
  The header files of miscellaneous routines for HttpDxe driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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
#define HTTP_RESPONSE_TIMEOUT        5
#define HTTP_DATA_RETRIES            12
#define HTTP_FIN_TIMEOUT             2
#define HTTP_KEEP_ALIVE_PROBES       6
#define HTTP_KEEP_ALIVE_TIME         7200
#define HTTP_KEEP_ALIVE_INTERVAL     30

#define HTTP_URL_BUFFER_LEN          4096

typedef struct _HTTP_SERVICE {
  UINT32                        Signature;
  EFI_SERVICE_BINDING_PROTOCOL  ServiceBinding;
  EFI_HANDLE                    Ip4DriverBindingHandle;
  EFI_HANDLE                    Ip6DriverBindingHandle;
  EFI_HANDLE                    ControllerHandle;
  EFI_HANDLE                    Tcp4ChildHandle;
  EFI_HANDLE                    Tcp6ChildHandle;
  LIST_ENTRY                    ChildrenList;
  UINTN                         ChildrenNumber;
  INTN                          State;
} HTTP_SERVICE;

typedef struct {
  EFI_TCP4_IO_TOKEN             Tx4Token;
  EFI_TCP4_TRANSMIT_DATA        Tx4Data;
  EFI_TCP6_IO_TOKEN             Tx6Token;
  EFI_TCP6_TRANSMIT_DATA        Tx6Data;
  EFI_TCP4_IO_TOKEN             Rx4Token;
  EFI_TCP4_RECEIVE_DATA         Rx4Data;
  EFI_TCP6_IO_TOKEN             Rx6Token;
  EFI_TCP6_RECEIVE_DATA         Rx6Data;
  BOOLEAN                       IsTxDone;
  BOOLEAN                       IsRxDone;
  UINTN                         BodyLen;
  EFI_HTTP_METHOD               Method;
} HTTP_TCP_TOKEN_WRAP;

typedef struct {
  EFI_TLS_VERSION               Version;
  EFI_TLS_CONNECTION_END        ConnectionEnd;
  EFI_TLS_VERIFY                VerifyMethod;
  EFI_TLS_SESSION_STATE         SessionState;
} TLS_CONFIG_DATA;

//
// Callback data for HTTP_PARSER_CALLBACK()
//
typedef struct {
  UINTN                         ParseDataLength;
  VOID                          *ParseData;
  VOID                          *Wrap;
} HTTP_CALLBACK_DATA;

typedef struct _HTTP_PROTOCOL {
  UINT32                        Signature;
  EFI_HTTP_PROTOCOL             Http;
  EFI_HANDLE                    Handle;
  HTTP_SERVICE                  *Service;
  LIST_ENTRY                    Link;   // Link to all HTTP instance from the service.
  BOOLEAN                       InDestroy;
  INTN                          State;
  EFI_HTTP_METHOD               Method;

  UINTN                         StatusCode;

  EFI_EVENT                     TimeoutEvent;

  EFI_HANDLE                    Tcp4ChildHandle;
  EFI_TCP4_PROTOCOL             *Tcp4;
  EFI_TCP4_CONFIG_DATA          Tcp4CfgData;
  EFI_TCP4_OPTION               Tcp4Option;

  EFI_TCP4_CONNECTION_TOKEN     Tcp4ConnToken;
  BOOLEAN                       IsTcp4ConnDone;
  EFI_TCP4_CLOSE_TOKEN          Tcp4CloseToken;
  BOOLEAN                       IsTcp4CloseDone;
  CHAR8                         *RemoteHost;
  UINT16                        RemotePort;
  EFI_IPv4_ADDRESS              RemoteAddr;

  EFI_HANDLE                    Tcp6ChildHandle;
  EFI_TCP6_PROTOCOL             *Tcp6;
  EFI_TCP6_CONFIG_DATA          Tcp6CfgData;
  EFI_TCP6_OPTION               Tcp6Option;

  EFI_TCP6_CONNECTION_TOKEN     Tcp6ConnToken;
  BOOLEAN                       IsTcp6ConnDone;
  EFI_TCP6_CLOSE_TOKEN          Tcp6CloseToken;
  BOOLEAN                       IsTcp6CloseDone;
  EFI_IPv6_ADDRESS              RemoteIpv6Addr;

  //
  // Rx4Token or Rx6Token used for receiving HTTP header.
  //
  EFI_TCP4_IO_TOKEN             Rx4Token;
  EFI_TCP4_RECEIVE_DATA         Rx4Data;
  EFI_TCP6_IO_TOKEN             Rx6Token;
  EFI_TCP6_RECEIVE_DATA         Rx6Data;
  BOOLEAN                       IsRxDone;

  CHAR8                         **EndofHeader;
  CHAR8                         **HttpHeaders;
  CHAR8                         *CacheBody;
  CHAR8                         *NextMsg;
  UINTN                         CacheLen;
  UINTN                         CacheOffset;

  //
  // HTTP message-body parser.
  //
  VOID                          *MsgParser;
  HTTP_CALLBACK_DATA            CallbackData;

  EFI_HTTP_VERSION              HttpVersion;
  UINT32                        TimeOutMillisec;
  BOOLEAN                       LocalAddressIsIPv6;

  EFI_HTTPv4_ACCESS_POINT       IPv4Node;
  EFI_HTTPv6_ACCESS_POINT       Ipv6Node;

  NET_MAP                       TxTokens;
  NET_MAP                       RxTokens;

  CHAR8                         *Url;

  //
  // Https Support
  //
  BOOLEAN                          UseHttps;

  EFI_SERVICE_BINDING_PROTOCOL     *TlsSb;
  EFI_HANDLE                       TlsChildHandle; /// Tls ChildHandle
  TLS_CONFIG_DATA                  TlsConfigData;
  EFI_TLS_PROTOCOL                 *Tls;
  EFI_TLS_CONFIGURATION_PROTOCOL   *TlsConfiguration;
  EFI_TLS_SESSION_STATE            TlsSessionState;

  //
  // TlsTxData used for transmitting TLS related messages.
  //
  EFI_TCP4_IO_TOKEN                Tcp4TlsTxToken;
  EFI_TCP4_TRANSMIT_DATA           Tcp4TlsTxData;
  EFI_TCP6_IO_TOKEN                Tcp6TlsTxToken;
  EFI_TCP6_TRANSMIT_DATA           Tcp6TlsTxData;
  BOOLEAN                          TlsIsTxDone;

  //
  // TlsRxData used for receiving TLS related messages.
  //
  EFI_TCP4_IO_TOKEN                Tcp4TlsRxToken;
  EFI_TCP4_RECEIVE_DATA            Tcp4TlsRxData;
  EFI_TCP6_IO_TOKEN                Tcp6TlsRxToken;
  EFI_TCP6_RECEIVE_DATA            Tcp6TlsRxData;
  BOOLEAN                          TlsIsRxDone;
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
  Create events for the TCP connection token and TCP close token.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events are created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Close events in the TCP connection token and TCP close token.

  @param[in]  HttpInstance   Pointer to HTTP_PROTOCOL structure.

**/
VOID
HttpCloseTcpConnCloseEvent (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Create event for the TCP transmit token.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpTxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap
  );

/**
  Create event for the TCP receive token which is used to receive HTTP header.

  @param[in]  HttpInstance       Pointer to HTTP_PROTOCOL structure.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpRxEventForHeader (
  IN  HTTP_PROTOCOL        *HttpInstance
  );

/**
  Create event for the TCP receive token which is used to receive HTTP body.

  @param[in]  Wrap               Point to HTTP token's wrap data.

  @retval EFI_SUCCESS            The events is created successfully.
  @retval others                 Other error as indicated.

**/
EFI_STATUS
HttpCreateTcpRxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap
  );

/**
  Close Events for Tcp Receive Tokens for HTTP body and HTTP header.

  @param[in]  Wrap               Pointer to HTTP token's wrap data.

**/
VOID
HttpCloseTcpRxEvent (
  IN  HTTP_TOKEN_WRAP      *Wrap
  );

/**
  Intiialize the HTTP_PROTOCOL structure to the unconfigured state.

  @param[in, out]  HttpInstance         Pointer to HTTP_PROTOCOL structure.
  @param[in]       IpVersion            Indicate us TCP4 protocol or TCP6 protocol.

  @retval EFI_SUCCESS       HTTP_PROTOCOL structure is initialized successfully.
  @retval Others            Other error as indicated.

**/
EFI_STATUS
HttpInitProtocol (
  IN OUT HTTP_PROTOCOL           *HttpInstance,
  IN     BOOLEAN                 IpVersion
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
  Configure TCP6 protocol child.

  @param[in]  HttpInstance       The HTTP instance private data.
  @param[in]  Wrap               The HTTP token's wrap data.

  @retval EFI_SUCCESS            The TCP6 protocol child is configured.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpConfigureTcp6 (
  IN  HTTP_PROTOCOL        *HttpInstance,
  IN  HTTP_TOKEN_WRAP      *Wrap
  );

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
  IN  HTTP_PROTOCOL        *HttpInstance
  );

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
  IN  HTTP_PROTOCOL        *HttpInstance
  );

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
  );

/**
  Check whether the user's token or event has already
  been enqueue on HTTP Tx or Rx Token list.

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
  Check whether the HTTP message associated with TxToken or Tx6Token is already sent out.

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
  );

/**
  Transmit the HTTP or HTTPS mssage by processing the associated HTTP token.

  @param[in]  Map                The container of TxToken or Tx6Token.
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

  @param[in]  Map                The container of Rx4Token or Rx6Token.
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
  Receive the HTTP header by processing the associated HTTP token.

  @param[in]       HttpInstance    The HTTP instance private data.
  @param[in, out]  SizeofHeaders   The HTTP header length.
  @param[in, out]  BufferSize      The size of buffer to cacahe the header message.
  @param[in]       Timeout         The time to wait for receiving the header packet.

  @retval EFI_SUCCESS              The HTTP header is received.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
HttpTcpReceiveHeader (
  IN  HTTP_PROTOCOL         *HttpInstance,
  IN  OUT UINTN             *SizeofHeaders,
  IN  OUT UINTN             *BufferSize,
  IN  EFI_EVENT             Timeout
  );

/**
  Receive the HTTP body by processing the associated HTTP token.

  @param[in]  Wrap               The HTTP token's wrap data.
  @param[in]  HttpMsg            The HTTP message data.

  @retval EFI_SUCCESS            The HTTP body is received.
  @retval Others                 Other error as indicated.

**/
EFI_STATUS
HttpTcpReceiveBody (
  IN  HTTP_TOKEN_WRAP       *Wrap,
  IN  EFI_HTTP_MESSAGE      *HttpMsg
  );

/**
  Clean up Tcp Tokens while the Tcp transmission error occurs.

  @param[in]  Wrap               Pointer to HTTP token's wrap data.

**/
VOID
HttpTcpTokenCleanup (
  IN  HTTP_TOKEN_WRAP      *Wrap
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
