/** @file
Header file for HttpLib.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DXE_HTTP_LIB_H_
#define _DXE_HTTP_LIB_H_

#include <Uefi.h>
#include <Library/NetLib.h>
#include <Library/HttpLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Http11.h>
#include <Protocol/HttpUtilities.h>

#define BIT(x)  (1 << x)

#define HTTP_VERSION_CRLF_STR  " HTTP/1.1\r\n"
#define EMPTY_SPACE            " "

#define NET_IS_HEX_CHAR(Ch)   \
  ((('0' <= (Ch)) && ((Ch) <= '9')) ||  \
   (('A' <= (Ch)) && ((Ch) <= 'F')) ||  \
   (('a' <= (Ch)) && ((Ch) <= 'f')))

//
// Field index of the HTTP URL parse result.
//
#define   HTTP_URI_FIELD_SCHEME     0
#define   HTTP_URI_FIELD_AUTHORITY  1
#define   HTTP_URI_FIELD_PATH       2
#define   HTTP_URI_FIELD_QUERY      3
#define   HTTP_URI_FIELD_FRAGMENT   4
#define   HTTP_URI_FIELD_USERINFO   5
#define   HTTP_URI_FIELD_HOST       6
#define   HTTP_URI_FIELD_PORT       7
#define   HTTP_URI_FIELD_MAX        8

#define   HTTP_URI_PORT_MAX_NUM  65535

//
// Structure to store the parse result of a HTTP URL.
//
typedef struct {
  UINT32    Offset;
  UINT32    Length;
} HTTP_URL_FILED_DATA;

typedef struct {
  UINT16                 FieldBitMap;
  HTTP_URL_FILED_DATA    FieldData[HTTP_URI_FIELD_MAX];
} HTTP_URL_PARSER;

typedef enum {
  UrlParserUrlStart,
  UrlParserScheme,
  UrlParserSchemeColon,            // ":"
  UrlParserSchemeColonSlash,       // ":/"
  UrlParserSchemeColonSlashSlash,  // "://"
  UrlParserAuthority,
  UrlParserAtInAuthority,
  UrlParserPath,
  UrlParserQueryStart,    // "?"
  UrlParserQuery,
  UrlParserFragmentStart, // "#"
  UrlParserFragment,
  UrlParserUserInfo,
  UrlParserHostStart,     // "@"
  UrlParserHost,
  UrlParserHostIpv6,      // "["(Ipv6 address) "]"
  UrlParserPortStart,     // ":"
  UrlParserPort,
  UrlParserStateMax
} HTTP_URL_PARSE_STATE;

#endif
