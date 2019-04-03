/** @file
  Support functions declaration for UEFI HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_HTTP_BOOT_SUPPORT_H__
#define __EFI_HTTP_BOOT_SUPPORT_H__

/**
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp4Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  Get the Nic handle using any child handle in the IPv6 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv6.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp6Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  This function is to convert UINTN to ASCII string with the required formatting.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  Length         The length of the required format.

**/
VOID
HttpBootUintnToAscDecWithFormat (
  IN UINTN                       Number,
  IN UINT8                       *Buffer,
  IN INTN                        Length
  );


/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
HttpBootShowIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  );

/**
  This function is to display the IPv6 address.

  @param[in]  Ip        The pointer to the IPv6 address.

**/
VOID
HttpBootShowIp6Addr (
  IN EFI_IPv6_ADDRESS   *Ip
  );

/**
  This function is to display the HTTP error status.

  @param[in]      StatusCode      The status code value in HTTP message.

**/
VOID
HttpBootPrintErrorMessage (
  EFI_HTTP_STATUS_CODE            StatusCode
  );

//
// A wrapper structure to hold the HTTP headers.
//
typedef struct {
  UINTN                       MaxHeaderCount;
  UINTN                       HeaderCount;
  EFI_HTTP_HEADER             *Headers;
} HTTP_IO_HEADER;

/**
  Create a HTTP_IO_HEADER to hold the HTTP header items.

  @param[in]  MaxHeaderCount         The maximun number of HTTP header in this holder.

  @return    A pointer of the HTTP header holder or NULL if failed.

**/
HTTP_IO_HEADER *
HttpBootCreateHeader (
  IN  UINTN                MaxHeaderCount
  );

/**
  Destroy the HTTP_IO_HEADER and release the resouces.

  @param[in]  HttpIoHeader       Point to the HTTP header holder to be destroyed.

**/
VOID
HttpBootFreeHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader
  );

/**
  Set or update a HTTP header with the field name and corresponding value.

  @param[in]  HttpIoHeader       Point to the HTTP header holder.
  @param[in]  FieldName          Null terminated string which describes a field name.
  @param[in]  FieldValue         Null terminated string which describes the corresponding field value.

  @retval  EFI_SUCCESS           The HTTP header has been set or updated.
  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES  Insufficient resource to complete the operation.
  @retval  Other                 Unexpected error happened.

**/
EFI_STATUS
HttpBootSetHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader,
  IN  CHAR8                *FieldName,
  IN  CHAR8                *FieldValue
  );

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

//
// HTTP_IO configuration data for IPv4
//
typedef struct {
  EFI_HTTP_VERSION          HttpVersion;
  UINT32                    RequestTimeOut;  // In milliseconds.
  UINT32                    ResponseTimeOut; // In milliseconds.
  BOOLEAN                   UseDefaultAddress;
  EFI_IPv4_ADDRESS          LocalIp;
  EFI_IPv4_ADDRESS          SubnetMask;
  UINT16                    LocalPort;
} HTTP4_IO_CONFIG_DATA;

//
// HTTP_IO configuration data for IPv6
//
typedef struct {
  EFI_HTTP_VERSION          HttpVersion;
  UINT32                    RequestTimeOut;  // In milliseconds.
  BOOLEAN                   UseDefaultAddress;
  EFI_IPv6_ADDRESS          LocalIp;
  UINT16                    LocalPort;
} HTTP6_IO_CONFIG_DATA;


//
// HTTP_IO configuration
//
typedef union {
  HTTP4_IO_CONFIG_DATA       Config4;
  HTTP6_IO_CONFIG_DATA       Config6;
} HTTP_IO_CONFIG_DATA;

//
// HTTP_IO wrapper of the EFI HTTP service.
//
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
} HTTP_IO;

//
// A wrapper structure to hold the received HTTP response data.
//
typedef struct {
  EFI_HTTP_RESPONSE_DATA      Response;
  UINTN                       HeaderCount;
  EFI_HTTP_HEADER             *Headers;
  UINTN                       BodyLength;
  CHAR8                       *Body;
  EFI_STATUS                  Status;
} HTTP_IO_RESPONSE_DATA;

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  Private             The pointer to the driver's private data.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
HttpBootDns (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv6_ADDRESS         *IpAddress
  );

/**
  Notify the callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpBootCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
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
  Destroy the HTTP_IO and release the resouces.

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO                *HttpIo
  );

/**
  Synchronously send a HTTP REQUEST message to the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   Request          A pointer to storage such data as URL and HTTP method.
  @param[in]   HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   BodyLength       Length in bytes of the HTTP body.
  @param[in]   Body             Body associated with the HTTP request.

  @retval EFI_SUCCESS            The HTTP request is trasmitted.
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
     OUT  HTTP_IO_RESPONSE_DATA    *ResponseData
  );

/**
  This function checks the HTTP(S) URI scheme.

  @param[in]    Uri              The pointer to the URI string.

  @retval EFI_SUCCESS            The URI scheme is valid.
  @retval EFI_INVALID_PARAMETER  The URI scheme is not HTTP or HTTPS.
  @retval EFI_ACCESS_DENIED      HTTP is disabled and the URI is HTTP.

**/
EFI_STATUS
HttpBootCheckUriScheme (
  IN      CHAR8                  *Uri
  );

/**
  Get the URI address string from the input device path.

  Caller need to free the buffer in the UriAddress pointer.

  @param[in]   FilePath         Pointer to the device path which contains a URI device path node.
  @param[out]  UriAddress       The URI address string extract from the device path.

  @retval EFI_SUCCESS            The URI string is returned.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HttpBootParseFilePath (
  IN     EFI_DEVICE_PATH_PROTOCOL     *FilePath,
     OUT CHAR8                        **UriAddress
  );

/**
  This function returns the image type according to server replied HTTP message
  and also the image's URI info.

  @param[in]    Uri              The pointer to the image's URI string.
  @param[in]    UriParser        URI Parse result returned by NetHttpParseUrl().
  @param[in]    HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]    Headers          Array containing list of HTTP headers.
  @param[out]   ImageType        The image type of the downloaded file.

  @retval EFI_SUCCESS            The image type is returned in ImageType.
  @retval EFI_INVALID_PARAMETER  ImageType, Uri or UriParser is NULL.
  @retval EFI_INVALID_PARAMETER  HeaderCount is not zero, and Headers is NULL.
  @retval EFI_NOT_FOUND          Failed to identify the image type.
  @retval Others                 Unexpect error happened.

**/
EFI_STATUS
HttpBootCheckImageType (
  IN      CHAR8                  *Uri,
  IN      VOID                   *UriParser,
  IN      UINTN                  HeaderCount,
  IN      EFI_HTTP_HEADER        *Headers,
     OUT  HTTP_BOOT_IMAGE_TYPE   *ImageType
  );

/**
  This function register the RAM disk info to the system.

  @param[in]       Private         The pointer to the driver's private data.
  @param[in]       BufferSize      The size of Buffer in bytes.
  @param[in]       Buffer          The base address of the RAM disk.
  @param[in]       ImageType       The image type of the file in Buffer.

  @retval EFI_SUCCESS              The RAM disk has been registered.
  @retval EFI_NOT_FOUND            No RAM disk protocol instances were found.
  @retval EFI_UNSUPPORTED          The ImageType is not supported.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpBootRegisterRamDisk (
  IN  HTTP_BOOT_PRIVATE_DATA       *Private,
  IN  UINTN                        BufferSize,
  IN  VOID                         *Buffer,
  IN  HTTP_BOOT_IMAGE_TYPE         ImageType
  );

/**
  Indicate if the HTTP status code indicates a redirection.

  @param[in]  StatusCode      HTTP status code from server.

  @return                     TRUE if it's redirection.

**/
BOOLEAN
HttpBootIsHttpRedirectStatusCode (
  IN   EFI_HTTP_STATUS_CODE        StatusCode
  );
#endif
