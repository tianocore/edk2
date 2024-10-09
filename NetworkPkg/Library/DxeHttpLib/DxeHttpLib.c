/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to parse the HTTP message byte stream.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 - 2020  Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeHttpLib.h"

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
  )
{
  UINTN  Index;
  UINTN  Offset;
  CHAR8  HexStr[3];

  if ((Buffer == NULL) || (BufferLength == 0) || (ResultBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Index     = 0;
  Offset    = 0;
  HexStr[2] = '\0';
  while (Index < BufferLength) {
    if (Buffer[Index] == '%') {
      if ((Index + 1 >= BufferLength) || (Index + 2 >= BufferLength) ||
          !NET_IS_HEX_CHAR (Buffer[Index+1]) || !NET_IS_HEX_CHAR (Buffer[Index+2]))
      {
        return EFI_INVALID_PARAMETER;
      }

      HexStr[0]            = Buffer[Index+1];
      HexStr[1]            = Buffer[Index+2];
      ResultBuffer[Offset] = (CHAR8)AsciiStrHexToUintn (HexStr);
      Index               += 3;
    } else {
      ResultBuffer[Offset] = Buffer[Index];
      Index++;
    }

    Offset++;
  }

  *ResultLength = (UINT32)Offset;

  return EFI_SUCCESS;
}

/**
  This function return the updated state according to the input state and next character of
  the authority.

  @param[in]       Char           Next character.
  @param[in]       State          Current value of the parser state machine.
  @param[in]       IsRightBracket TRUE if there is an sign ']' in the authority component and
                                  indicates the next part is ':' before Port.

  @return          Updated state value.
**/
HTTP_URL_PARSE_STATE
NetHttpParseAuthorityChar (
  IN  CHAR8                 Char,
  IN  HTTP_URL_PARSE_STATE  State,
  IN  BOOLEAN               *IsRightBracket
  )
{
  //
  // RFC 3986:
  // The authority component is preceded by a double slash ("//") and is
  // terminated by the next slash ("/"), question mark ("?"), or number
  // sign ("#") character, or by the end of the URI.
  //
  if ((Char == ' ') || (Char == '\r') || (Char == '\n')) {
    return UrlParserStateMax;
  }

  //
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  switch (State) {
    case UrlParserUserInfo:
      if (Char == '@') {
        return UrlParserHostStart;
      }

      break;

    case UrlParserHost:
    case UrlParserHostStart:
      if (Char == '[') {
        return UrlParserHostIpv6;
      }

      if (Char == ':') {
        return UrlParserPortStart;
      }

      return UrlParserHost;

    case UrlParserHostIpv6:
      if (Char == ']') {
        *IsRightBracket = TRUE;
      }

      if ((Char == ':') && *IsRightBracket) {
        return UrlParserPortStart;
      }

      return UrlParserHostIpv6;

    case UrlParserPort:
    case UrlParserPortStart:
      return UrlParserPort;

    default:
      break;
  }

  return State;
}

/**
  This function parse the authority component of the input URL and update the parser.

  @param[in]       Url            The pointer to a HTTP URL string.
  @param[in]       FoundAt        TRUE if there is an at sign ('@') in the authority component.
  @param[in, out]  UrlParser      Pointer to the buffer of the parse result.

  @retval EFI_SUCCESS             Successfully parse the authority.
  @retval EFI_INVALID_PARAMETER   The Url is invalid to parse the authority component.

**/
EFI_STATUS
NetHttpParseAuthority (
  IN      CHAR8            *Url,
  IN      BOOLEAN          FoundAt,
  IN OUT  HTTP_URL_PARSER  *UrlParser
  )
{
  CHAR8                 *Char;
  CHAR8                 *Authority;
  UINT32                Length;
  HTTP_URL_PARSE_STATE  State;
  UINT32                Field;
  UINT32                OldField;
  BOOLEAN               IsrightBracket;

  ASSERT ((UrlParser->FieldBitMap & BIT (HTTP_URI_FIELD_AUTHORITY)) != 0);

  //
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  if (FoundAt) {
    State = UrlParserUserInfo;
  } else {
    State = UrlParserHost;
  }

  IsrightBracket = FALSE;
  Field          = HTTP_URI_FIELD_MAX;
  OldField       = Field;
  Authority      = Url + UrlParser->FieldData[HTTP_URI_FIELD_AUTHORITY].Offset;
  Length         = UrlParser->FieldData[HTTP_URI_FIELD_AUTHORITY].Length;
  for (Char = Authority; Char < Authority + Length; Char++) {
    State = NetHttpParseAuthorityChar (*Char, State, &IsrightBracket);
    switch (State) {
      case UrlParserStateMax:
        return EFI_INVALID_PARAMETER;

      case UrlParserHostStart:
      case UrlParserPortStart:
        continue;

      case UrlParserUserInfo:
        Field = HTTP_URI_FIELD_USERINFO;
        break;

      case UrlParserHost:
        Field = HTTP_URI_FIELD_HOST;
        break;

      case UrlParserHostIpv6:
        Field = HTTP_URI_FIELD_HOST;
        break;

      case UrlParserPort:
        Field = HTTP_URI_FIELD_PORT;
        break;

      default:
        ASSERT (FALSE);
    }

    //
    // Field not changed, count the length.
    //
    ASSERT (Field < HTTP_URI_FIELD_MAX);
    if (Field == OldField) {
      UrlParser->FieldData[Field].Length++;
      continue;
    }

    //
    // New field start
    //
    UrlParser->FieldBitMap            |= BIT (Field);
    UrlParser->FieldData[Field].Offset = (UINT32)(Char - Url);
    UrlParser->FieldData[Field].Length = 1;
    OldField                           = Field;
  }

  return EFI_SUCCESS;
}

/**
  This function return the updated state according to the input state and next character of a URL.

  @param[in]       Char           Next character.
  @param[in]       State          Current value of the parser state machine.

  @return          Updated state value.

**/
HTTP_URL_PARSE_STATE
NetHttpParseUrlChar (
  IN  CHAR8                 Char,
  IN  HTTP_URL_PARSE_STATE  State
  )
{
  if ((Char == ' ') || (Char == '\r') || (Char == '\n')) {
    return UrlParserStateMax;
  }

  //
  // http_URL = "http:" "//" host [ ":" port ] [ abs_path [ "?" query ]]
  //
  // Request-URI    = "*" | absolute-URI | path-absolute | authority
  //
  // absolute-URI  = scheme ":" hier-part [ "?" query ]
  // path-absolute = "/" [ segment-nz *( "/" segment ) ]
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  switch (State) {
    case UrlParserUrlStart:
      if ((Char == '*') || (Char == '/')) {
        return UrlParserPath;
      }

      return UrlParserScheme;

    case UrlParserScheme:
      if (Char == ':') {
        return UrlParserSchemeColon;
      }

      break;

    case UrlParserSchemeColon:
      if (Char == '/') {
        return UrlParserSchemeColonSlash;
      }

      break;

    case UrlParserSchemeColonSlash:
      if (Char == '/') {
        return UrlParserSchemeColonSlashSlash;
      }

      break;

    case UrlParserAtInAuthority:
      if (Char == '@') {
        return UrlParserStateMax;
      }

    case UrlParserAuthority:
    case UrlParserSchemeColonSlashSlash:
      if (Char == '@') {
        return UrlParserAtInAuthority;
      }

      if (Char == '/') {
        return UrlParserPath;
      }

      if (Char == '?') {
        return UrlParserQueryStart;
      }

      if (Char == '#') {
        return UrlParserFragmentStart;
      }

      return UrlParserAuthority;

    case UrlParserPath:
      if (Char == '?') {
        return UrlParserQueryStart;
      }

      if (Char == '#') {
        return UrlParserFragmentStart;
      }

      break;

    case UrlParserQuery:
    case UrlParserQueryStart:
      if (Char == '#') {
        return UrlParserFragmentStart;
      }

      return UrlParserQuery;

    case UrlParserFragmentStart:
      return UrlParserFragment;

    default:
      break;
  }

  return State;
}

/**
  Create a URL parser for the input URL string.

  This function will parse and dereference the input HTTP URL into it components. The original
  content of the URL won't be modified and the result will be returned in UrlParser, which can
  be used in other functions like NetHttpUrlGetHostName().

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
  )
{
  HTTP_URL_PARSE_STATE  State;
  CHAR8                 *Char;
  UINT32                Field;
  UINT32                OldField;
  BOOLEAN               FoundAt;
  EFI_STATUS            Status;
  HTTP_URL_PARSER       *Parser;

  Parser = NULL;

  if ((Url == NULL) || (Length == 0) || (UrlParser == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = AllocateZeroPool (sizeof (HTTP_URL_PARSER));
  if (Parser == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IsConnectMethod) {
    //
    // According to RFC 2616, the authority form is only used by the CONNECT method.
    //
    State = UrlParserAuthority;
  } else {
    State = UrlParserUrlStart;
  }

  Field    = HTTP_URI_FIELD_MAX;
  OldField = Field;
  FoundAt  = FALSE;
  for (Char = Url; Char < Url + Length; Char++) {
    //
    // Update state machine according to next char.
    //
    State = NetHttpParseUrlChar (*Char, State);

    switch (State) {
      case UrlParserStateMax:
        FreePool (Parser);
        return EFI_INVALID_PARAMETER;

      case UrlParserSchemeColon:
      case UrlParserSchemeColonSlash:
      case UrlParserSchemeColonSlashSlash:
      case UrlParserQueryStart:
      case UrlParserFragmentStart:
        //
        // Skip all the delimiting char: "://" "?" "@"
        //
        continue;

      case UrlParserScheme:
        Field = HTTP_URI_FIELD_SCHEME;
        break;

      case UrlParserAtInAuthority:
        FoundAt = TRUE;
      case UrlParserAuthority:
        Field = HTTP_URI_FIELD_AUTHORITY;
        break;

      case UrlParserPath:
        Field = HTTP_URI_FIELD_PATH;
        break;

      case UrlParserQuery:
        Field = HTTP_URI_FIELD_QUERY;
        break;

      case UrlParserFragment:
        Field = HTTP_URI_FIELD_FRAGMENT;
        break;

      default:
        ASSERT (FALSE);
    }

    //
    // Field not changed, count the length.
    //
    ASSERT (Field < HTTP_URI_FIELD_MAX);
    if (Field == OldField) {
      Parser->FieldData[Field].Length++;
      continue;
    }

    //
    // New field start
    //
    Parser->FieldBitMap            |= BIT (Field);
    Parser->FieldData[Field].Offset = (UINT32)(Char - Url);
    Parser->FieldData[Field].Length = 1;
    OldField                        = Field;
  }

  //
  // If has authority component, continue to parse the username, host and port.
  //
  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_AUTHORITY)) != 0) {
    Status = NetHttpParseAuthority (Url, FoundAt, Parser);
    if (EFI_ERROR (Status)) {
      FreePool (Parser);
      return Status;
    }
  }

  *UrlParser = Parser;
  return EFI_SUCCESS;
}

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
  )
{
  CHAR8            *Name;
  EFI_STATUS       Status;
  UINT32           ResultLength;
  HTTP_URL_PARSER  *Parser;

  if ((Url == NULL) || (UrlParser == NULL) || (HostName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER *)UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_NOT_FOUND;
  }

  Name = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_HOST].Length + 1);
  if (Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset,
             Parser->FieldData[HTTP_URI_FIELD_HOST].Length,
             Name,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    FreePool (Name);
    return Status;
  }

  Name[ResultLength] = '\0';
  *HostName          = Name;
  return EFI_SUCCESS;
}

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
  )
{
  CHAR8            *Ip4String;
  EFI_STATUS       Status;
  UINT32           ResultLength;
  HTTP_URL_PARSER  *Parser;

  if ((Url == NULL) || (UrlParser == NULL) || (Ip4Address == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER *)UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_NOT_FOUND;
  }

  Ip4String = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_HOST].Length + 1);
  if (Ip4String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset,
             Parser->FieldData[HTTP_URI_FIELD_HOST].Length,
             Ip4String,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    FreePool (Ip4String);
    return Status;
  }

  Ip4String[ResultLength] = '\0';
  Status                  = NetLibAsciiStrToIp4 (Ip4String, Ip4Address);
  FreePool (Ip4String);

  return Status;
}

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
  )
{
  CHAR8            *Ip6String;
  CHAR8            *Ptr;
  UINT32           Length;
  EFI_STATUS       Status;
  UINT32           ResultLength;
  HTTP_URL_PARSER  *Parser;

  if ((Url == NULL) || (UrlParser == NULL) || (Ip6Address == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER *)UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
  //
  Length = Parser->FieldData[HTTP_URI_FIELD_HOST].Length;
  if (Length < 2) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr = Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset;
  if ((Ptr[0] != '[') || (Ptr[Length - 1] != ']')) {
    return EFI_INVALID_PARAMETER;
  }

  Ip6String = AllocatePool (Length);
  if (Ip6String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Ptr + 1,
             Length - 2,
             Ip6String,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    FreePool (Ip6String);
    return Status;
  }

  Ip6String[ResultLength] = '\0';
  Status                  = NetLibAsciiStrToIp6 (Ip6String, Ip6Address);
  FreePool (Ip6String);

  return Status;
}

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
  )
{
  CHAR8            *PortString;
  EFI_STATUS       Status;
  UINTN            Index;
  UINTN            Data;
  UINT32           ResultLength;
  HTTP_URL_PARSER  *Parser;

  if ((Url == NULL) || (UrlParser == NULL) || (Port == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Port = 0;
  Index = 0;

  Parser = (HTTP_URL_PARSER *)UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_PORT)) == 0) {
    return EFI_NOT_FOUND;
  }

  PortString = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_PORT].Length + 1);
  if (PortString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_PORT].Offset,
             Parser->FieldData[HTTP_URI_FIELD_PORT].Length,
             PortString,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  PortString[ResultLength] = '\0';

  while (Index < ResultLength) {
    if (!NET_IS_DIGIT (PortString[Index])) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    Index++;
  }

  Status =  AsciiStrDecimalToUintnS (Url + Parser->FieldData[HTTP_URI_FIELD_PORT].Offset, (CHAR8 **)NULL, &Data);

  if (EFI_ERROR (Status) || (Data > HTTP_URI_PORT_MAX_NUM)) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  *Port = (UINT16)Data;

ON_EXIT:
  FreePool (PortString);
  return Status;
}

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
  )
{
  CHAR8            *PathStr;
  EFI_STATUS       Status;
  UINT32           ResultLength;
  HTTP_URL_PARSER  *Parser;

  if ((Url == NULL) || (UrlParser == NULL) || (Path == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER *)UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_PATH)) == 0) {
    return EFI_NOT_FOUND;
  }

  PathStr = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_PATH].Length + 1);
  if (PathStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_PATH].Offset,
             Parser->FieldData[HTTP_URI_FIELD_PATH].Length,
             PathStr,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    FreePool (PathStr);
    return Status;
  }

  PathStr[ResultLength] = '\0';
  *Path                 = PathStr;
  return EFI_SUCCESS;
}

/**
  Release the resource of the URL parser.

  @param[in]    UrlParser            Pointer to the parser.

**/
VOID
EFIAPI
HttpUrlFreeParser (
  IN      VOID  *UrlParser
  )
{
  FreePool (UrlParser);
}

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
  )
{
  UINTN  Index;

  if ((HeaderCount == 0) || (Headers == NULL) || (FieldName == NULL)) {
    return NULL;
  }

  for (Index = 0; Index < HeaderCount; Index++) {
    //
    // Field names are case-insensitive (RFC 2616).
    //
    if (AsciiStriCmp (Headers[Index].FieldName, FieldName) == 0) {
      return &Headers[Index];
    }
  }

  return NULL;
}

typedef enum {
  BodyParserBodyStart,
  BodyParserBodyIdentity,
  BodyParserChunkSizeStart,
  BodyParserChunkSize,
  BodyParserChunkSizeEndCR,
  BodyParserChunkExtStart,
  BodyParserChunkDataStart,
  BodyParserChunkDataEnd,
  BodyParserChunkDataEndCR,
  BodyParserTrailer,
  BodyParserLastCRLF,
  BodyParserLastCRLFEnd,
  BodyParserComplete,
  BodyParserStateMax
} HTTP_BODY_PARSE_STATE;

typedef struct {
  BOOLEAN                      IgnoreBody;     // "MUST NOT" include a message-body
  BOOLEAN                      IsChunked;      // "chunked" transfer-coding.
  BOOLEAN                      ContentLengthIsValid;
  UINTN                        ContentLength;  // Entity length (not the message-body length), invalid until ContentLengthIsValid is TRUE

  HTTP_BODY_PARSER_CALLBACK    Callback;
  VOID                         *Context;
  UINTN                        ParsedBodyLength;
  HTTP_BODY_PARSE_STATE        State;
  UINTN                        CurrentChunkSize;
  UINTN                        CurrentChunkParsedSize;
} HTTP_BODY_PARSER;

/**
  Convert an hexadecimal char to a value of type UINTN.

  @param[in]       Char           Ascii character.

  @return          Value translated from Char.

**/
UINTN
HttpIoHexCharToUintn (
  IN CHAR8  Char
  )
{
  if ((Char >= '0') && (Char <= '9')) {
    return Char - '0';
  }

  return (10 + AsciiCharToUpper (Char) - 'A');
}

/**
  Get the value of the content length if there is a "Content-Length" header.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[out]   ContentLength      Pointer to save the value of the content length.

  @retval EFI_SUCCESS              Successfully get the content length.
  @retval EFI_NOT_FOUND            No "Content-Length" header in the Headers.

**/
EFI_STATUS
HttpIoParseContentLengthHeader (
  IN     UINTN            HeaderCount,
  IN     EFI_HTTP_HEADER  *Headers,
  OUT UINTN               *ContentLength
  )
{
  EFI_HTTP_HEADER  *Header;

  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_CONTENT_LENGTH);
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }

  return AsciiStrDecimalToUintnS (Header->FieldValue, (CHAR8 **)NULL, ContentLength);
}

/**

  Check whether the HTTP message is using the "chunked" transfer-coding.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.

  @return       The message is "chunked" transfer-coding (TRUE) or not (FALSE).

**/
BOOLEAN
HttpIoIsChunked (
  IN   UINTN            HeaderCount,
  IN   EFI_HTTP_HEADER  *Headers
  )
{
  EFI_HTTP_HEADER  *Header;

  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_TRANSFER_ENCODING);
  if (Header == NULL) {
    return FALSE;
  }

  if (AsciiStriCmp (Header->FieldValue, "identity") != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check whether the HTTP message should have a message-body.

  @param[in]    Method             The HTTP method (e.g. GET, POST) for this HTTP message.
  @param[in]    StatusCode         Response status code returned by the remote host.

  @return       The message should have a message-body (FALSE) or not (TRUE).

**/
BOOLEAN
HttpIoNoMessageBody (
  IN   EFI_HTTP_METHOD       Method,
  IN   EFI_HTTP_STATUS_CODE  StatusCode
  )
{
  //
  // RFC 2616:
  // All responses to the HEAD request method
  // MUST NOT include a message-body, even though the presence of entity-
  // header fields might lead one to believe they do. All 1xx
  // (informational), 204 (no content), and 304 (not modified) responses
  // MUST NOT include a message-body. All other responses do include a
  // message-body, although it MAY be of zero length.
  //
  if (Method == HttpMethodHead) {
    return TRUE;
  }

  if ((StatusCode == HTTP_STATUS_100_CONTINUE) ||
      (StatusCode == HTTP_STATUS_101_SWITCHING_PROTOCOLS) ||
      (StatusCode == HTTP_STATUS_204_NO_CONTENT) ||
      (StatusCode == HTTP_STATUS_304_NOT_MODIFIED))
  {
    return TRUE;
  }

  return FALSE;
}

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
  )
{
  EFI_STATUS        Status;
  HTTP_BODY_PARSER  *Parser;

  if ((HeaderCount != 0) && (Headers == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MsgParser == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = AllocateZeroPool (sizeof (HTTP_BODY_PARSER));
  if (Parser == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Parser->State = BodyParserBodyStart;

  //
  // Determine the message length according to RFC 2616.
  // 1. Check whether the message "MUST NOT" have a message-body.
  //
  Parser->IgnoreBody = HttpIoNoMessageBody (Method, StatusCode);
  //
  // 2. Check whether the message using "chunked" transfer-coding.
  //
  Parser->IsChunked = HttpIoIsChunked (HeaderCount, Headers);
  //
  // 3. Check whether the message has a Content-Length header field.
  //
  Status = HttpIoParseContentLengthHeader (HeaderCount, Headers, &Parser->ContentLength);
  if (!EFI_ERROR (Status)) {
    Parser->ContentLengthIsValid = TRUE;
  }

  //
  // 4. Range header is not supported now, so we won't meet media type "multipart/byteranges".
  // 5. By server closing the connection
  //

  //
  // Set state to skip body parser if the message shouldn't have a message body.
  //
  if (Parser->IgnoreBody) {
    Parser->State = BodyParserComplete;
  } else {
    Parser->Callback = Callback;
    Parser->Context  = Context;
  }

  *MsgParser = Parser;
  return EFI_SUCCESS;
}

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
  )
{
  CHAR8             *Char;
  UINTN             RemainderLengthInThis;
  UINTN             LengthForCallback;
  UINTN             PortionLength;
  EFI_STATUS        Status;
  HTTP_BODY_PARSER  *Parser;

  if ((BodyLength == 0) || (Body == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MsgParser == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_BODY_PARSER *)MsgParser;

  if (Parser->IgnoreBody) {
    Parser->State = BodyParserComplete;
    if (Parser->Callback != NULL) {
      Status = Parser->Callback (
                         BodyParseEventOnComplete,
                         Body,
                         0,
                         Parser->Context
                         );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    return EFI_SUCCESS;
  }

  if (Parser->State == BodyParserBodyStart) {
    Parser->ParsedBodyLength = 0;
    if (Parser->IsChunked) {
      Parser->State = BodyParserChunkSizeStart;
    } else {
      Parser->State = BodyParserBodyIdentity;
    }
  }

  //
  // The message body might be truncated in anywhere, so we need to parse is byte-by-byte.
  //
  for (Char = Body; Char < Body + BodyLength; ) {
    switch (Parser->State) {
      case BodyParserStateMax:
        return EFI_ABORTED;

      case BodyParserBodyIdentity:
        //
        // Identity transfer-coding, just notify user to save the body data.
        //
        PortionLength = MIN (
                          BodyLength,
                          Parser->ContentLength - Parser->ParsedBodyLength
                          );
        if (PortionLength == 0) {
          //
          // Got BodyLength, but no ContentLength. Use BodyLength.
          //
          PortionLength         = BodyLength;
          Parser->ContentLength = PortionLength;
        }

        if (Parser->Callback != NULL) {
          Status = Parser->Callback (
                             BodyParseEventOnData,
                             Char,
                             PortionLength,
                             Parser->Context
                             );
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        Char                     += PortionLength;
        Parser->ParsedBodyLength += PortionLength;
        if (Parser->ParsedBodyLength == Parser->ContentLength) {
          Parser->State = BodyParserComplete;
          if (Parser->Callback != NULL) {
            Status = Parser->Callback (
                               BodyParseEventOnComplete,
                               Char,
                               0,
                               Parser->Context
                               );
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }
        }

        break;

      case BodyParserChunkSizeStart:
        //
        // First byte of chunk-size, the chunk-size might be truncated.
        //
        Parser->CurrentChunkSize = 0;
        Parser->State            = BodyParserChunkSize;
      case BodyParserChunkSize:
        if (!NET_IS_HEX_CHAR (*Char)) {
          if (*Char == ';') {
            Parser->State = BodyParserChunkExtStart;
            Char++;
          } else if (*Char == '\r') {
            Parser->State = BodyParserChunkSizeEndCR;
            Char++;
          } else {
            Parser->State = BodyParserStateMax;
          }

          break;
        }

        if (Parser->CurrentChunkSize > (((~((UINTN)0)) - 16) / 16)) {
          return EFI_INVALID_PARAMETER;
        }

        Parser->CurrentChunkSize = Parser->CurrentChunkSize * 16 + HttpIoHexCharToUintn (*Char);
        Char++;
        break;

      case BodyParserChunkExtStart:
        //
        // Ignore all the chunk extensions.
        //
        if (*Char == '\r') {
          Parser->State = BodyParserChunkSizeEndCR;
        }

        Char++;
        break;

      case BodyParserChunkSizeEndCR:
        if (*Char != '\n') {
          Parser->State = BodyParserStateMax;
          break;
        }

        Char++;
        if (Parser->CurrentChunkSize == 0) {
          //
          // The last chunk has been parsed and now assumed the state
          // of HttpBodyParse is ParserLastCRLF. So it need to decide
          // whether the rest message is trailer or last CRLF in the next round.
          //
          Parser->ContentLengthIsValid = TRUE;
          Parser->State                = BodyParserLastCRLF;
          break;
        }

        Parser->State                  = BodyParserChunkDataStart;
        Parser->CurrentChunkParsedSize = 0;
        break;

      case BodyParserLastCRLF:
        //
        // Judge the byte is belong to the Last CRLF or trailer, and then
        // configure the state of HttpBodyParse to corresponding state.
        //
        if (*Char == '\r') {
          Char++;
          Parser->State = BodyParserLastCRLFEnd;
          break;
        } else {
          Parser->State = BodyParserTrailer;
          break;
        }

      case BodyParserLastCRLFEnd:
        if (*Char == '\n') {
          Parser->State = BodyParserComplete;
          Char++;
          if (Parser->Callback != NULL) {
            Status = Parser->Callback (
                               BodyParseEventOnComplete,
                               Char,
                               0,
                               Parser->Context
                               );
            if (EFI_ERROR (Status)) {
              return Status;
            }
          }

          break;
        } else {
          Parser->State = BodyParserStateMax;
          break;
        }

      case BodyParserTrailer:
        if (*Char == '\r') {
          Parser->State = BodyParserChunkSizeEndCR;
        }

        Char++;
        break;

      case BodyParserChunkDataStart:
        //
        // First byte of chunk-data, the chunk data also might be truncated.
        //
        RemainderLengthInThis = BodyLength - (Char - Body);
        LengthForCallback     = MIN (Parser->CurrentChunkSize - Parser->CurrentChunkParsedSize, RemainderLengthInThis);
        if (Parser->Callback != NULL) {
          Status = Parser->Callback (
                             BodyParseEventOnData,
                             Char,
                             LengthForCallback,
                             Parser->Context
                             );
          if (EFI_ERROR (Status)) {
            return Status;
          }
        }

        Char                           += LengthForCallback;
        Parser->ContentLength          += LengthForCallback;
        Parser->CurrentChunkParsedSize += LengthForCallback;
        if (Parser->CurrentChunkParsedSize == Parser->CurrentChunkSize) {
          Parser->State = BodyParserChunkDataEnd;
        }

        break;

      case BodyParserChunkDataEnd:
        if (*Char == '\r') {
          Parser->State = BodyParserChunkDataEndCR;
        } else {
          Parser->State = BodyParserStateMax;
        }

        Char++;
        break;

      case BodyParserChunkDataEndCR:
        if (*Char != '\n') {
          Parser->State = BodyParserStateMax;
          break;
        }

        Char++;
        Parser->State = BodyParserChunkSizeStart;
        break;

      default:
        break;
    }
  }

  if (Parser->State == BodyParserStateMax) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

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
  )
{
  HTTP_BODY_PARSER  *Parser;

  if (MsgParser == NULL) {
    return FALSE;
  }

  Parser = (HTTP_BODY_PARSER *)MsgParser;

  if (Parser->State == BodyParserComplete) {
    return TRUE;
  }

  return FALSE;
}

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
  )
{
  HTTP_BODY_PARSER  *Parser;

  if ((MsgParser == NULL) || (ContentLength == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_BODY_PARSER *)MsgParser;

  if (!Parser->ContentLengthIsValid) {
    return EFI_NOT_READY;
  }

  *ContentLength = Parser->ContentLength;
  return EFI_SUCCESS;
}

/**
  Release the resource of the message parser.

  @param[in]    MsgParser            Pointer to the message parser.

**/
VOID
EFIAPI
HttpFreeMsgParser (
  IN  VOID  *MsgParser
  )
{
  FreePool (MsgParser);
}

/**
  Get the next string, which is distinguished by specified separator.

  @param[in]  String             Pointer to the string.
  @param[in]  Separator          Specified separator used to distinguish where is the beginning
                                 of next string.

  @return     Pointer to the next string.
  @return     NULL if not find or String is NULL.

**/
CHAR8 *
AsciiStrGetNextToken (
  IN CONST CHAR8  *String,
  IN       CHAR8  Separator
  )
{
  CONST CHAR8  *Token;

  Token = String;
  while (TRUE) {
    if (*Token == 0) {
      return NULL;
    }

    if (*Token == Separator) {
      return (CHAR8 *)(Token + 1);
    }

    Token++;
  }
}

/**
  Set FieldName and FieldValue into specified HttpHeader.

  @param[in,out]  HttpHeader      Specified HttpHeader.
  @param[in]  FieldName           FieldName of this HttpHeader, a NULL terminated ASCII string.
  @param[in]  FieldValue          FieldValue of this HttpHeader, a NULL terminated ASCII string.


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
  )
{
  UINTN  FieldNameSize;
  UINTN  FieldValueSize;

  if ((HttpHeader == NULL) || (FieldName == NULL) || (FieldValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HttpHeader->FieldName != NULL) {
    FreePool (HttpHeader->FieldName);
  }

  if (HttpHeader->FieldValue != NULL) {
    FreePool (HttpHeader->FieldValue);
  }

  FieldNameSize         = AsciiStrSize (FieldName);
  HttpHeader->FieldName = AllocateZeroPool (FieldNameSize);
  if (HttpHeader->FieldName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (HttpHeader->FieldName, FieldName, FieldNameSize);
  HttpHeader->FieldName[FieldNameSize - 1] = 0;

  FieldValueSize         = AsciiStrSize (FieldValue);
  HttpHeader->FieldValue = AllocateZeroPool (FieldValueSize);
  if (HttpHeader->FieldValue == NULL) {
    FreePool (HttpHeader->FieldName);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (HttpHeader->FieldValue, FieldValue, FieldValueSize);
  HttpHeader->FieldValue[FieldValueSize - 1] = 0;

  return EFI_SUCCESS;
}

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
  )
{
  CHAR8  *FieldNameStr;
  CHAR8  *FieldValueStr;
  CHAR8  *StrPtr;
  CHAR8  *EndofHeader;

  if ((String == NULL) || (FieldName == NULL) || (FieldValue == NULL)) {
    return NULL;
  }

  *FieldName    = NULL;
  *FieldValue   = NULL;
  FieldNameStr  = NULL;
  FieldValueStr = NULL;
  StrPtr        = NULL;
  EndofHeader   = NULL;

  //
  // Check whether the raw HTTP header string is valid or not.
  //
  EndofHeader = AsciiStrStr (String, "\r\n\r\n");
  if (EndofHeader == NULL) {
    return NULL;
  }

  //
  // Each header field consists of a name followed by a colon (":") and the field value.
  // The field value MAY be preceded by any amount of LWS, though a single SP is preferred.
  //
  // message-header = field-name ":" [ field-value ]
  // field-name = token
  // field-value = *( field-content | LWS )
  //
  // Note: "*(element)" allows any number element, including zero; "1*(element)" requires at least one element.
  //       [element] means element is optional.
  //       LWS  = [CRLF] 1*(SP|HT), it can be ' ' or '\t' or '\r\n ' or '\r\n\t'.
  //       CRLF = '\r\n'.
  //       SP   = ' '.
  //       HT   = '\t' (Tab).
  //
  FieldNameStr  = String;
  FieldValueStr = AsciiStrGetNextToken (FieldNameStr, ':');
  if (FieldValueStr == NULL) {
    return NULL;
  }

  //
  // Replace ':' with 0, then FieldName has been retrived from String.
  //
  *(FieldValueStr - 1) = 0;

  //
  // Handle FieldValueStr, skip all the preceded LWS.
  //
  while (TRUE) {
    if ((*FieldValueStr == ' ') || (*FieldValueStr == '\t')) {
      //
      // Boundary condition check.
      //
      if ((UINTN)EndofHeader - (UINTN)FieldValueStr < 1) {
        //
        // Wrong String format!
        //
        return NULL;
      }

      FieldValueStr++;
    } else if (*FieldValueStr == '\r') {
      //
      // Boundary condition check.
      //
      if ((UINTN)EndofHeader - (UINTN)FieldValueStr < 3) {
        //
        // No more preceded LWS, so break here.
        //
        break;
      }

      if (*(FieldValueStr + 1) == '\n' ) {
        if ((*(FieldValueStr + 2) == ' ') || (*(FieldValueStr + 2) == '\t')) {
          FieldValueStr = FieldValueStr + 3;
        } else {
          //
          // No more preceded LWS, so break here.
          //
          break;
        }
      } else {
        //
        // Wrong String format!
        //
        return NULL;
      }
    } else {
      //
      // No more preceded LWS, so break here.
      //
      break;
    }
  }

  StrPtr = FieldValueStr;
  do {
    //
    // Handle the LWS within the field value.
    //
    StrPtr = AsciiStrGetNextToken (StrPtr, '\r');
    if ((StrPtr == NULL) || (*StrPtr != '\n')) {
      //
      // Wrong String format!
      //
      return NULL;
    }

    StrPtr++;
  } while (*StrPtr == ' ' || *StrPtr == '\t');

  //
  // Replace '\r' with 0
  //
  *(StrPtr - 2) = 0;

  //
  // Get FieldName and FieldValue.
  //
  *FieldName  = FieldNameStr;
  *FieldValue = FieldValueStr;

  return StrPtr;
}

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
  )
{
  UINTN  Index;

  if (HeaderFields != NULL) {
    for (Index = 0; Index < FieldCount; Index++) {
      if (HeaderFields[Index].FieldName != NULL) {
        FreePool (HeaderFields[Index].FieldName);
      }

      if (HeaderFields[Index].FieldValue != NULL) {
        FreePool (HeaderFields[Index].FieldValue);
      }
    }

    FreePool (HeaderFields);
  }
}

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
  )
{
  EFI_STATUS                   Status;
  UINTN                        StrLength;
  CHAR8                        *RequestPtr;
  UINTN                        HttpHdrSize;
  UINTN                        MsgSize;
  BOOLEAN                      Success;
  VOID                         *HttpHdr;
  EFI_HTTP_HEADER              **AppendList;
  UINTN                        Index;
  EFI_HTTP_UTILITIES_PROTOCOL  *HttpUtilitiesProtocol;

  Status                = EFI_SUCCESS;
  HttpHdrSize           = 0;
  MsgSize               = 0;
  Success               = FALSE;
  HttpHdr               = NULL;
  AppendList            = NULL;
  HttpUtilitiesProtocol = NULL;

  //
  // 1. If we have a Request, we cannot have a NULL Url
  // 2. If we have a Request, HeaderCount can not be non-zero
  // 3. If we do not have a Request, HeaderCount should be zero
  // 4. If we do not have Request and Headers, we need at least a message-body
  //
  if (((Message == NULL) || (RequestMsg == NULL) || (RequestMsgSize == NULL)) ||
      ((Message->Data.Request != NULL) && (Url == NULL)) ||
      ((Message->Data.Request != NULL) && (Message->HeaderCount == 0)) ||
      ((Message->Data.Request == NULL) && (Message->HeaderCount != 0)) ||
      ((Message->Data.Request == NULL) && (Message->HeaderCount == 0) && (Message->BodyLength == 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Message->HeaderCount != 0) {
    //
    // Locate the HTTP_UTILITIES protocol.
    //
    Status = gBS->LocateProtocol (
                    &gEfiHttpUtilitiesProtocolGuid,
                    NULL,
                    (VOID **)&HttpUtilitiesProtocol
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to locate Http Utilities protocol. Status = %r.\n", Status));
      return Status;
    }

    //
    // Build AppendList to send into HttpUtilitiesBuild
    //
    AppendList = AllocateZeroPool (sizeof (EFI_HTTP_HEADER *) * (Message->HeaderCount));
    if (AppendList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    for (Index = 0; Index < Message->HeaderCount; Index++) {
      AppendList[Index] = &Message->Headers[Index];
    }

    //
    // Build raw HTTP Headers
    //
    Status = HttpUtilitiesProtocol->Build (
                                      HttpUtilitiesProtocol,
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

    if (EFI_ERROR (Status) || (HttpHdr == NULL)) {
      return Status;
    }
  }

  //
  // If we have headers to be sent, account for it.
  //
  if (Message->HeaderCount != 0) {
    MsgSize = HttpHdrSize;
  }

  //
  // If we have a request line, account for the fields.
  //
  if (Message->Data.Request != NULL) {
    MsgSize += HTTP_METHOD_MAXIMUM_LEN + AsciiStrLen (HTTP_VERSION_CRLF_STR) + AsciiStrLen (Url);
  }

  //
  // If we have a message body to be sent, account for it.
  //
  MsgSize += Message->BodyLength;

  //
  // memory for the string that needs to be sent to TCP
  //
  *RequestMsg = NULL;
  *RequestMsg = AllocateZeroPool (MsgSize);
  if (*RequestMsg == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  RequestPtr = *RequestMsg;
  //
  // Construct header request
  //
  if (Message->Data.Request != NULL) {
    switch (Message->Data.Request->Method) {
      case HttpMethodGet:
        StrLength = sizeof (HTTP_METHOD_GET) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_GET, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodPut:
        StrLength = sizeof (HTTP_METHOD_PUT) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_PUT, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodPatch:
        StrLength = sizeof (HTTP_METHOD_PATCH) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_PATCH, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodPost:
        StrLength = sizeof (HTTP_METHOD_POST) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_POST, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodHead:
        StrLength = sizeof (HTTP_METHOD_HEAD) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_HEAD, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodDelete:
        StrLength = sizeof (HTTP_METHOD_DELETE) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_DELETE, StrLength);
        RequestPtr += StrLength;
        break;
      case HttpMethodConnect:
        StrLength = sizeof (HTTP_METHOD_CONNECT) - 1;
        CopyMem (RequestPtr, HTTP_METHOD_CONNECT, StrLength);
        RequestPtr += StrLength;
        break;
      default:
        ASSERT (FALSE);
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    StrLength = AsciiStrLen (EMPTY_SPACE);
    CopyMem (RequestPtr, EMPTY_SPACE, StrLength);
    RequestPtr += StrLength;

    StrLength = AsciiStrLen (Url);
    CopyMem (RequestPtr, Url, StrLength);
    RequestPtr += StrLength;

    StrLength = sizeof (HTTP_VERSION_CRLF_STR) - 1;
    CopyMem (RequestPtr, HTTP_VERSION_CRLF_STR, StrLength);
    RequestPtr += StrLength;

    if (HttpHdr != NULL) {
      //
      // Construct header
      //
      CopyMem (RequestPtr, HttpHdr, HttpHdrSize);
      RequestPtr += HttpHdrSize;
    }
  }

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
  (*RequestMsgSize) = (UINTN)(RequestPtr) - (UINTN)(*RequestMsg);
  Success           = TRUE;

Exit:

  if (!Success) {
    if (*RequestMsg != NULL) {
      FreePool (*RequestMsg);
    }

    *RequestMsg = NULL;
    return Status;
  }

  if (HttpHdr != NULL) {
    FreePool (HttpHdr);
  }

  return EFI_SUCCESS;
}

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
      return HTTP_STATUS_300_MULTIPLE_CHOICES;
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
    case 308:
      return HTTP_STATUS_308_PERMANENT_REDIRECT;
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
    case 429:
      return HTTP_STATUS_429_TOO_MANY_REQUESTS;
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
  )
{
  UINTN  Index;

  if (FieldName == NULL) {
    return FALSE;
  }

  for (Index = 0; Index < DeleteCount; Index++) {
    if (DeleteList[Index] == NULL) {
      continue;
    }

    if (AsciiStrCmp (FieldName, DeleteList[Index]) == 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Create a HTTP_IO_HEADER to hold the HTTP header items.

  @param[in]  MaxHeaderCount         The maximun number of HTTP header in this holder.

  @return    A pointer of the HTTP header holder or NULL if failed.

**/
HTTP_IO_HEADER *
HttpIoCreateHeader (
  UINTN  MaxHeaderCount
  )
{
  HTTP_IO_HEADER  *HttpIoHeader;

  if (MaxHeaderCount == 0) {
    return NULL;
  }

  HttpIoHeader = AllocateZeroPool (sizeof (HTTP_IO_HEADER) + MaxHeaderCount * sizeof (EFI_HTTP_HEADER));
  if (HttpIoHeader == NULL) {
    return NULL;
  }

  HttpIoHeader->MaxHeaderCount = MaxHeaderCount;
  HttpIoHeader->Headers        = (EFI_HTTP_HEADER *)(HttpIoHeader + 1);

  return HttpIoHeader;
}

/**
  Destroy the HTTP_IO_HEADER and release the resources.

  @param[in]  HttpIoHeader       Point to the HTTP header holder to be destroyed.

**/
VOID
HttpIoFreeHeader (
  IN  HTTP_IO_HEADER  *HttpIoHeader
  )
{
  UINTN  Index;

  if (HttpIoHeader != NULL) {
    if (HttpIoHeader->HeaderCount != 0) {
      for (Index = 0; Index < HttpIoHeader->HeaderCount; Index++) {
        FreePool (HttpIoHeader->Headers[Index].FieldName);
        ZeroMem (HttpIoHeader->Headers[Index].FieldValue, AsciiStrSize (HttpIoHeader->Headers[Index].FieldValue));
        FreePool (HttpIoHeader->Headers[Index].FieldValue);
      }
    }

    FreePool (HttpIoHeader);
  }
}

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
  )
{
  EFI_HTTP_HEADER  *Header;
  UINTN            StrSize;
  CHAR8            *NewFieldValue;

  if ((HttpIoHeader == NULL) || (FieldName == NULL) || (FieldValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Header = HttpFindHeader (HttpIoHeader->HeaderCount, HttpIoHeader->Headers, FieldName);
  if (Header == NULL) {
    //
    // Add a new header.
    //
    if (HttpIoHeader->HeaderCount >= HttpIoHeader->MaxHeaderCount) {
      return EFI_OUT_OF_RESOURCES;
    }

    Header = &HttpIoHeader->Headers[HttpIoHeader->HeaderCount];

    StrSize           = AsciiStrSize (FieldName);
    Header->FieldName = AllocatePool (StrSize);
    if (Header->FieldName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Header->FieldName, FieldName, StrSize);
    Header->FieldName[StrSize -1] = '\0';

    StrSize            = AsciiStrSize (FieldValue);
    Header->FieldValue = AllocatePool (StrSize);
    if (Header->FieldValue == NULL) {
      FreePool (Header->FieldName);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Header->FieldValue, FieldValue, StrSize);
    Header->FieldValue[StrSize -1] = '\0';

    HttpIoHeader->HeaderCount++;
  } else {
    //
    // Update an existing one.
    //
    StrSize       = AsciiStrSize (FieldValue);
    NewFieldValue = AllocatePool (StrSize);
    if (NewFieldValue == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewFieldValue, FieldValue, StrSize);
    NewFieldValue[StrSize -1] = '\0';

    if (Header->FieldValue != NULL) {
      FreePool (Header->FieldValue);
    }

    Header->FieldValue = NewFieldValue;
  }

  return EFI_SUCCESS;
}
