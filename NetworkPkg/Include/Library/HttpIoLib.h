/** @file
  HttpIoLib.h.

(C) Copyright 2020 Hewlett-Packard Development Company, L.P.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HTTP_IO_LIB_H_
#define HTTP_IO_LIB_H_

#include <IndustryStandard/Http11.h>

#include <Library/DpcLib.h>
#include <Library/HttpLib.h>
#include <Library/NetLib.h>

#define HTTP_IO_MAX_SEND_PAYLOAD                    1024
#define HTTP_IO_CHUNK_SIZE_STRING_LEN               50
#define HTTP_IO_CHUNKED_TRANSFER_CODING_DATA_LENGTH 256

///
/// HTTP_IO_CALLBACK_EVENT
///
typedef enum {
  HttpIoRequest,
  HttpIoResponse
} HTTP_IO_CALLBACK_EVENT;

/**
  HttpIo Callback function which will be invoked when specified HTTP_IO_CALLBACK_EVENT happened.

  @param[in]    EventType      Indicate the Event type that occurs in the current callback.
  @param[in]    Message        HTTP message which will be send to, or just received from HTTP server.
  @param[in]    Context        The Callback Context pointer.

  @retval EFI_SUCCESS          Tells the HttpIo to continue the HTTP process.
  @retval Others               Tells the HttpIo to abort the current HTTP process.
**/
typedef
EFI_STATUS
(EFIAPI * HTTP_IO_CALLBACK) (
  IN  HTTP_IO_CALLBACK_EVENT    EventType,
  IN  EFI_HTTP_MESSAGE          *Message,
  IN  VOID                      *Context
  );

///
/// A wrapper structure to hold the received HTTP response data.
///
typedef struct {
  EFI_HTTP_RESPONSE_DATA      Response;
  UINTN                       HeaderCount;
  EFI_HTTP_HEADER             *Headers;
  UINTN                       BodyLength;
  CHAR8                       *Body;
  EFI_STATUS                  Status;
} HTTP_IO_RESPONSE_DATA;

///
/// HTTP_IO configuration data for IPv4
///
typedef struct {
  EFI_HTTP_VERSION          HttpVersion;
  UINT32                    RequestTimeOut;  ///< In milliseconds.
  UINT32                    ResponseTimeOut; ///< In milliseconds.
  BOOLEAN                   UseDefaultAddress;
  EFI_IPv4_ADDRESS          LocalIp;
  EFI_IPv4_ADDRESS          SubnetMask;
  UINT16                    LocalPort;
} HTTP4_IO_CONFIG_DATA;

///
/// HTTP_IO configuration data for IPv6
///
typedef struct {
  EFI_HTTP_VERSION          HttpVersion;
  UINT32                    RequestTimeOut;  ///< In milliseconds.
  BOOLEAN                   UseDefaultAddress;
  EFI_IPv6_ADDRESS          LocalIp;
  UINT16                    LocalPort;
} HTTP6_IO_CONFIG_DATA;

///
/// HTTP_IO configuration
///
typedef union {
  HTTP4_IO_CONFIG_DATA       Config4;
  HTTP6_IO_CONFIG_DATA       Config6;
} HTTP_IO_CONFIG_DATA;

///
/// HTTP_IO wrapper of the EFI HTTP service.
///
typedef struct {
  UINT8                     IpVersion;
  EFI_HANDLE                Image;
  EFI_HANDLE                Controller;
  EFI_HANDLE                Handle;

  EFI_HTTP_PROTOCOL         *Http;

  HTTP_IO_CALLBACK          Callback;
  VOID                      *Context;

  EFI_HTTP_TOKEN            ReqToken;
  EFI_HTTP_MESSAGE          ReqMessage;
  EFI_HTTP_TOKEN            RspToken;
  EFI_HTTP_MESSAGE          RspMessage;

  BOOLEAN                   IsTxDone;
  BOOLEAN                   IsRxDone;

  EFI_EVENT                 TimeoutEvent;
  UINT32                    Timeout;
} HTTP_IO;

///
/// Process code of HTTP chunk transfer.
///
typedef enum  {
  HttpIoSendChunkNone = 0,
  HttpIoSendChunkHeaderZeroContent,
  HttpIoSendChunkContent,
  HttpIoSendChunkEndChunk,
  HttpIoSendChunkFinish
} HTTP_IO_SEND_CHUNK_PROCESS;

///
/// Process code of HTTP non chunk transfer.
///
typedef enum  {
  HttpIoSendNonChunkNone = 0,
  HttpIoSendNonChunkHeaderZeroContent,
  HttpIoSendNonChunkContent,
  HttpIoSendNonChunkFinish
} HTTP_IO_SEND_NON_CHUNK_PROCESS;

///
/// Chunk links for HTTP chunked transfer coding.
///
typedef struct {
  LIST_ENTRY  NextChunk;
  UINTN       Length;
  CHAR8       *Data;
} HTTP_IO_CHUNKS;

/**
  Notify the callback function when an event is triggered.

  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotifyDpc (
  IN VOID                *Context
  );

/**
  Request HttpIoNotifyDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotify (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  Destroy the HTTP_IO and release the resources.

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO                *HttpIo
  );

/**
  Create a HTTP_IO to access the HTTP service. It will create and configure
  a HTTP child handle.

  @param[in]  Image          The handle of the driver image.
  @param[in]  Controller     The handle of the controller.
  @param[in]  IpVersion      IP_VERSION_4 or IP_VERSION_6.
  @param[in]  ConfigData     The HTTP_IO configuration data.
  @param[in]  Callback       Callback function which will be invoked when specified
                             HTTP_IO_CALLBACK_EVENT happened.
  @param[in]  Context        The Context data which will be passed to the Callback function.
  @param[out] HttpIo         The HTTP_IO.

  @retval EFI_SUCCESS            The HTTP_IO is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the HTTP_IO or configure it.

**/
EFI_STATUS
HttpIoCreateIo (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  IpVersion,
  IN HTTP_IO_CONFIG_DATA    *ConfigData,
  IN HTTP_IO_CALLBACK       Callback,
  IN VOID                   *Context,
  OUT HTTP_IO               *HttpIo
  );

/**
  Synchronously send a HTTP REQUEST message to the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   Request          A pointer to storage such data as URL and HTTP method.
  @param[in]   HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   BodyLength       Length in bytes of the HTTP body.
  @param[in]   Body             Body associated with the HTTP request.

  @retval EFI_SUCCESS            The HTTP request is transmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoSendRequest (
  IN  HTTP_IO                *HttpIo,
  IN  EFI_HTTP_REQUEST_DATA  *Request,      OPTIONAL
  IN  UINTN                  HeaderCount,
  IN  EFI_HTTP_HEADER        *Headers,      OPTIONAL
  IN  UINTN                  BodyLength,
  IN  VOID                   *Body          OPTIONAL
  );

/**
  Synchronously receive a HTTP RESPONSE message from the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   RecvMsgHeader    TRUE to receive a new HTTP response (from message header).
                                FALSE to continue receive the previous response message.
  @param[out]  ResponseData     Point to a wrapper of the received response data.

  @retval EFI_SUCCESS            The HTTP response is received.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoRecvResponse (
  IN      HTTP_IO                  *HttpIo,
  IN      BOOLEAN                  RecvMsgHeader,
  OUT     HTTP_IO_RESPONSE_DATA    *ResponseData
  );

/**
  Get the value of the content length if there is a "Content-Length" header.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[out]   ContentLength      Pointer to save the value of the content length.

  @retval EFI_SUCCESS              Successfully get the content length.
  @retval EFI_NOT_FOUND            No "Content-Length" header in the Headers.

**/
EFI_STATUS
HttpIoGetContentLength (
  IN     UINTN                HeaderCount,
  IN     EFI_HTTP_HEADER      *Headers,
  OUT    UINTN                *ContentLength
  );

/**
  Synchronously receive a HTTP RESPONSE message from the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   HeaderCount      Number of headers in Headers.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[out]  ChunkListHead    A pointer to receivce list head of chunked data.
                                Caller has to release memory of ChunkListHead
                                and all list entries.
  @param[out]  ContentLength    Total content length

  @retval EFI_SUCCESS            The HTTP chunked transfer is received.
  @retval EFI_NOT_FOUND          No chunked transfer coding header found.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER  Improper parameters.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoGetChunkedTransferContent (
  IN     HTTP_IO              *HttpIo,
  IN     UINTN                HeaderCount,
  IN     EFI_HTTP_HEADER      *Headers,
  OUT    LIST_ENTRY           **ChunkListHead,
  OUT    UINTN                *ContentLength
  );

/**
  Send HTTP request in chunks.

  @param[in]   HttpIo             The HttpIo wrapping the HTTP service.
  @param[in]   SendChunkProcess   Pointer to current chunk process status.
  @param[out]  RequestMessage     Request to send.

  @retval EFI_SUCCESS             Successfully to send chunk data according to SendChunkProcess.
  @retval Other                   Other errors.

**/
EFI_STATUS
HttpIoSendChunkedTransfer (
  IN  HTTP_IO                    *HttpIo,
  IN  HTTP_IO_SEND_CHUNK_PROCESS *SendChunkProcess,
  IN  EFI_HTTP_MESSAGE           *RequestMessage
);
#endif
