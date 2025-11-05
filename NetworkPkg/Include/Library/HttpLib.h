/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to parse the HTTP message byte stream.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HTTP_LIB_H_
#define _HTTP_LIB_H_

#include <Protocol/Http.h>

/**
  Decode a percent-encoded URI component to the ASCII character.

  Decode the input component in Buffer according to RFC 3986. The caller is responsible to make
  sure ResultBuffer points to a buffer with size equal or greater than ((AsciiStrSize (Buffer))
  in bytes.

  @param[in]    Buffer           The pointer to a percent-encoded URI component.
  @param[in]    BufferLength     Length of Buffer in bytes.
  @param[out]   ResultBuffer     Point to the buffer to store the decode result.
  @param[out]   ResultLength     Length of decoded string in ResultBuffer in bytes.

  @retval EFI_SUCCESS            Successfully decoded the URI.
  @retval EFI_INVALID_PARAMETER  Buffer is not a valid percent-encoded string.

**/
EFI_STATUS
EFIAPI
UriPercentDecode (
  IN      CHAR8   *Buffer,
  IN      UINT32  BufferLength,
  OUT  CHAR8      *ResultBuffer,
  OUT  UINT32     *ResultLength
  );

/**
  Create a URL parser for the input URL string.

  This function will parse and dereference the input HTTP URL into it components. The original
  content of the URL won't be modified and the result will be returned in UrlParser, which can
  be used in other functions like NetHttpUrlGetHostName(). It is the caller's responsibility to
  free the buffer returned in *UrlParser by HttpUrlFreeParser().

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    Length             Length of Url in bytes.
  @param[in]    IsConnectMethod    Whether the Url is used in HTTP CONNECT method or not.
  @param[out]   UrlParser          Pointer to the returned buffer to store the parse result.

  @retval EFI_SUCCESS              Successfully dereferenced the HTTP URL.
  @retval EFI_INVALID_PARAMETER    UrlParser is NULL or Url is not a valid HTTP URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpParseUrl (
  IN      CHAR8    *Url,
  IN      UINT32   Length,
  IN      BOOLEAN  IsConnectMethod,
  OUT  VOID        **UrlParser
  );

/**
  Get the Hostname from a HTTP URL.

  This function will return the HostName according to the Url and previous parse result ,and
  it is the caller's responsibility to free the buffer returned in *HostName.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   HostName           Pointer to a buffer to store the HostName.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or HostName is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No hostName component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpUrlGetHostName (
  IN      CHAR8  *Url,
  IN      VOID   *UrlParser,
  OUT  CHAR8     **HostName
  );

/**
  Get the IPv4 address from a HTTP URL.

  This function will return the IPv4 address according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Ip4Address         Pointer to a buffer to store the IP address.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Ip4Address is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No IPv4 address component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpUrlGetIp4 (
  IN      CHAR8          *Url,
  IN      VOID           *UrlParser,
  OUT  EFI_IPv4_ADDRESS  *Ip4Address
  );

/**
  Get the IPv6 address from a HTTP URL.

  This function will return the IPv6 address according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Ip6Address         Pointer to a buffer to store the IP address.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Ip6Address is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No IPv6 address component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpUrlGetIp6 (
  IN      CHAR8          *Url,
  IN      VOID           *UrlParser,
  OUT  EFI_IPv6_ADDRESS  *Ip6Address
  );

/**
  Get the port number from a HTTP URL.

  This function will return the port number according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Port               Pointer to a buffer to store the port number.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Port is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No port number in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpUrlGetPort (
  IN      CHAR8  *Url,
  IN      VOID   *UrlParser,
  OUT  UINT16    *Port
  );

/**
  Get the Path from a HTTP URL.

  This function will return the Path according to the Url and previous parse result,and
  it is the caller's responsibility to free the buffer returned in *Path.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Path               Pointer to a buffer to store the Path.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or HostName is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No hostName component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpUrlGetPath (
  IN      CHAR8  *Url,
  IN      VOID   *UrlParser,
  OUT  CHAR8     **Path
  );

/**
  Release the resource of the URL parser.

  @param[in]    UrlParser            Pointer to the parser.

**/
VOID
EFIAPI
HttpUrlFreeParser (
  IN      VOID  *UrlParser
  );

//
// HTTP body parser interface.
//

typedef enum {
  //
  // Part of entity data.
  // Length of entity body in Data.
  //
  BodyParseEventOnData,
  //
  // End of message body.
  // Length is 0 and Data points to next byte after the end of the message.
  //
  BodyParseEventOnComplete
} HTTP_BODY_PARSE_EVENT;

/**
  A callback function to intercept events during message parser.

  This function will be invoked during HttpParseMessageBody() with various events type. An error
  return status of the callback function will cause the HttpParseMessageBody() aborted.

  @param[in]    EventType          Event type of this callback call.
  @param[in]    Data               A pointer to data buffer.
  @param[in]    Length             Length in bytes of the Data.
  @param[in]    Context            Callback context set by HttpInitMsgParser().

  @retval EFI_SUCCESS              Continue to parser the message body.
  @retval Others                   Abort the parse.

**/
typedef
EFI_STATUS
(EFIAPI *HTTP_BODY_PARSER_CALLBACK)(
  IN HTTP_BODY_PARSE_EVENT      EventType,
  IN CHAR8                      *Data,
  IN UINTN                      Length,
  IN VOID                       *Context
  );

/**
  Initialize a HTTP message-body parser.

  This function will create and initialize a HTTP message parser according to caller provided HTTP message
  header information. It is the caller's responsibility to free the buffer returned in *UrlParser by HttpFreeMsgParser().

  @param[in]    Method             The HTTP method (e.g. GET, POST) for this HTTP message.
  @param[in]    StatusCode         Response status code returned by the remote host.
  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[in]    Callback           Callback function that is invoked when parsing the HTTP message-body,
                                   set to NULL to ignore all events.
  @param[in]    Context            Pointer to the context that will be passed to Callback.
  @param[out]   MsgParser          Pointer to the returned buffer to store the message parser.

  @retval EFI_SUCCESS              Successfully initialized the parser.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  @retval EFI_INVALID_PARAMETER    MsgParser is NULL or HeaderCount is not NULL but Headers is NULL.
  @retval Others                   Failed to initialize the parser.

**/
EFI_STATUS
EFIAPI
HttpInitMsgParser (
  IN     EFI_HTTP_METHOD            Method,
  IN     EFI_HTTP_STATUS_CODE       StatusCode,
  IN     UINTN                      HeaderCount,
  IN     EFI_HTTP_HEADER            *Headers,
  IN     HTTP_BODY_PARSER_CALLBACK  Callback,
  IN     VOID                       *Context,
  OUT  VOID                         **MsgParser
  );

/**
  Parse message body.

  Parse BodyLength of message-body. This function can be called repeatedly to parse the message-body partially.

  @param[in, out]    MsgParser            Pointer to the message parser.
  @param[in]         BodyLength           Length in bytes of the Body.
  @param[in]         Body                 Pointer to the buffer of the message-body to be parsed.

  @retval EFI_SUCCESS                Successfully parse the message-body.
  @retval EFI_INVALID_PARAMETER      MsgParser is NULL or Body is NULL or BodyLength is 0.
  @retval EFI_ABORTED                Operation aborted.
  @retval Other                      Error happened while parsing message body.

**/
EFI_STATUS
EFIAPI
HttpParseMessageBody (
  IN OUT VOID   *MsgParser,
  IN     UINTN  BodyLength,
  IN     CHAR8  *Body
  );

/**
  Check whether the message-body is complete or not.

  @param[in]    MsgParser            Pointer to the message parser.

  @retval TRUE                       Message-body is complete.
  @retval FALSE                      Message-body is not complete.

**/
BOOLEAN
EFIAPI
HttpIsMessageComplete (
  IN VOID  *MsgParser
  );

/**
  Get the content length of the entity.

  Note that in trunk transfer, the entity length is not valid until the whole message body is received.

  @param[in]    MsgParser            Pointer to the message parser.
  @param[out]   ContentLength        Pointer to store the length of the entity.

  @retval EFI_SUCCESS                Successfully to get the entity length.
  @retval EFI_NOT_READY              Entity length is not valid yet.
  @retval EFI_INVALID_PARAMETER      MsgParser is NULL or ContentLength is NULL.

**/
EFI_STATUS
EFIAPI
HttpGetEntityLength (
  IN  VOID   *MsgParser,
  OUT UINTN  *ContentLength
  );

/**
  Release the resource of the message parser.

  @param[in]    MsgParser            Pointer to the message parser.

**/
VOID
EFIAPI
HttpFreeMsgParser (
  IN  VOID  *MsgParser
  );

/**
  Find a specified header field according to the field name.

  @param[in]   HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   FieldName        Null terminated string which describes a field name.

  @return    Pointer to the found header or NULL.

**/
EFI_HTTP_HEADER *
EFIAPI
HttpFindHeader (
  IN  UINTN            HeaderCount,
  IN  EFI_HTTP_HEADER  *Headers,
  IN  CHAR8            *FieldName
  );

/**
  Set FieldName and FieldValue into specified HttpHeader.

  @param[in,out]  HttpHeader          Specified HttpHeader.
  @param[in]      FieldName           FieldName of this HttpHeader, a NULL terminated ASCII string.
  @param[in]      FieldValue          FieldValue of this HttpHeader, a NULL terminated ASCII string.


  @retval EFI_SUCCESS             The FieldName and FieldValue are set into HttpHeader successfully.
  @retval EFI_INVALID_PARAMETER   The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
HttpSetFieldNameAndValue (
  IN  OUT   EFI_HTTP_HEADER  *HttpHeader,
  IN  CONST CHAR8            *FieldName,
  IN  CONST CHAR8            *FieldValue
  );

/**
  Get one key/value header pair from the raw string.

  @param[in]  String             Pointer to the raw string.
  @param[out] FieldName          Points directly to field name within 'HttpHeader'.
  @param[out] FieldValue         Points directly to field value within 'HttpHeader'.

  @return     Pointer to the next raw string.
  @return     NULL if no key/value header pair from this raw string.

**/
CHAR8 *
EFIAPI
HttpGetFieldNameAndValue (
  IN     CHAR8  *String,
  OUT CHAR8     **FieldName,
  OUT CHAR8     **FieldValue
  );

/**
  Free existing HeaderFields.

  @param[in]  HeaderFields       Pointer to array of key/value header pairs waiting for free.
  @param[in]  FieldCount         The number of header pairs in HeaderFields.

**/
VOID
EFIAPI
HttpFreeHeaderFields (
  IN  EFI_HTTP_HEADER  *HeaderFields,
  IN  UINTN            FieldCount
  );

/**
  Generate HTTP request message.

  This function will allocate memory for the whole HTTP message and generate a
  well formatted HTTP Request message in it, include the Request-Line, header
  fields and also the message body. It is the caller's responsibility to free
  the buffer returned in *RequestMsg.

  @param[in]   Message            Pointer to the EFI_HTTP_MESSAGE structure which
                                  contains the required information to generate
                                  the HTTP request message.
  @param[in]   Url                The URL of a remote host.
  @param[out]  RequestMsg         Pointer to the created HTTP request message.
                                  NULL if any error occurred.
  @param[out]  RequestMsgSize     Size of the RequestMsg (in bytes).

  @retval EFI_SUCCESS             If HTTP request string was created successfully.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate resources.
  @retval EFI_INVALID_PARAMETER   The input arguments are invalid.

**/
EFI_STATUS
EFIAPI
HttpGenRequestMessage (
  IN     CONST EFI_HTTP_MESSAGE  *Message,
  IN     CONST CHAR8             *Url,
  OUT CHAR8                      **RequestMsg,
  OUT UINTN                      *RequestMsgSize
  );

/**
  Translate the status code in HTTP message to EFI_HTTP_STATUS_CODE defined
  in UEFI 2.5 specification.

  The official HTTP status codes can be found here:
  https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml

  @param[in]  StatusCode         The status code value in HTTP message.

  @return                        Value defined in EFI_HTTP_STATUS_CODE .

**/
EFI_HTTP_STATUS_CODE
EFIAPI
HttpMappingToStatusCode (
  IN UINTN  StatusCode
  );

/**
  Check whether header field called FieldName is in DeleteList.

  @param[in]  DeleteList        Pointer to array of key/value header pairs.
  @param[in]  DeleteCount       The number of header pairs.
  @param[in]  FieldName         Pointer to header field's name.

  @return     TRUE if FieldName is not in DeleteList, that means this header field is valid.
  @return     FALSE if FieldName is in DeleteList, that means this header field is invalid.

**/
BOOLEAN
EFIAPI
HttpIsValidHttpHeader (
  IN  CHAR8  *DeleteList[],
  IN  UINTN  DeleteCount,
  IN  CHAR8  *FieldName
  );

//
// A wrapper structure to hold the HTTP headers.
//
typedef struct {
  UINTN              MaxHeaderCount;
  UINTN              HeaderCount;
  EFI_HTTP_HEADER    *Headers;
} HTTP_IO_HEADER;

/**
  Create a HTTP_IO_HEADER to hold the HTTP header items.

  @param[in]  MaxHeaderCount         The maximun number of HTTP header in this holder.

  @return    A pointer of the HTTP header holder or NULL if failed.

**/
HTTP_IO_HEADER *
HttpIoCreateHeader (
  UINTN  MaxHeaderCount
  );

/**
  Destroy the HTTP_IO_HEADER and release the resources.

  @param[in]  HttpIoHeader       Point to the HTTP header holder to be destroyed.

**/
VOID
HttpIoFreeHeader (
  IN  HTTP_IO_HEADER  *HttpIoHeader
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
HttpIoSetHeader (
  IN  HTTP_IO_HEADER  *HttpIoHeader,
  IN  CHAR8           *FieldName,
  IN  CHAR8           *FieldValue
  );

#endif
