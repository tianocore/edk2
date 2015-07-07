/** @file
  The header files of implementation of EFI_HTTP_PROTOCOL protocol interfaces.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_HTTP_IMPL_H__
#define __EFI_HTTP_IMPL_H__

#define HTTP_DEFAULT_PORT        80
#define HTTP_END_OF_HDR_STR      "\r\n\r\n"
#define HTTP_CRLF_STR            "\r\n"
#define HTTP_VERSION_STR         "HTTP/1.1"
#define HTTP_VERSION_CRLF_STR    " HTTP/1.1\r\n"
#define HTTP_GET_STR             "GET "
#define HTTP_HEAD_STR            "HEAD "
//
// Connect method has maximum length according to EFI_HTTP_METHOD defined in
// UEFI2.5 spec so use this.
//
#define HTTP_MAXIMUM_METHOD_LEN  sizeof ("CONNECT")

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
  );

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
  @retval EFI_UNSUPPORTED         One or more options in ConfigData are not supported
                                  in the implementation.
**/
EFI_STATUS
EFIAPI
EfiHttpConfigure (
  IN  EFI_HTTP_PROTOCOL         *This,
  IN  EFI_HTTP_CONFIG_DATA      *HttpConfigData
  );

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
  );

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
  );

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
  );

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
  );

extern EFI_HTTP_PROTOCOL  mEfiHttpTemplate;

#endif
