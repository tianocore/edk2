/** @file
  This file is cloned from DMTF libredfish library tag v1.0.0 and maintained
  by EDKII.

//----------------------------------------------------------------------------
// Copyright Notice:
// Copyright 2017 Distributed Management Task Force, Inc. All rights reserved.
// License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libredfish/LICENSE.md
//----------------------------------------------------------------------------

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBREDFISH_REDFISH_SERVICE_H_
#define LIBREDFISH_REDFISH_SERVICE_H_

#include <IndustryStandard/Http11.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HttpLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/RedfishContentCodingLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Include/Library/RedfishCrtLib.h>

#include <Protocol/EdkIIRedfishConfigHandler.h>
#include <Protocol/RestEx.h>

#include <jansson.h>

typedef struct {
  char                    *host;
  json_t                  *versions;
  unsigned int            flags;
  char                    *sessionToken;
  char                    *basicAuthStr;
  //
  // point to the <HOST> part in above "host" field, which will be put into
  // the "Host" header of HTTP request message.
  //
  char                    *HostHeaderValue;
  EFI_REST_EX_PROTOCOL    *RestEx;
} redfishService;

typedef struct {
  json_t            *json;
  redfishService    *service;
} redfishPayload;

#define REDFISH_AUTH_BASIC         0
#define REDFISH_AUTH_BEARER_TOKEN  1
#define REDFISH_AUTH_SESSION       2

#define REDFISH_HTTP_RESPONSE_TIMEOUT  5000       /// 5 seconds in uints of millisecond.

///
/// Library class public defines
///
#define HTTP_FLAG   L"http://"
#define HTTPS_FLAG  L"https://"

///
/// The redfish first URL should be "/redfish/v1/", while we use "/redfish/v1" here without "/"
/// in the end is to avoid the 301 Perment redirect response from Redfish profile simulator.
///
#define REDFISH_FIRST_URL  L"/redfish/v1"

typedef struct {
  unsigned int    authType;
  union {
    struct {
      char    *username;
      char    *password;
    } userPass;
    struct {
      char    *token;
    } authToken;
  } authCodes;
} enumeratorAuthentication;

// Values for flags
#define REDFISH_FLAG_SERVICE_NO_VERSION_DOC  0x00000001// The Redfish Service lacks the version document (in violation of the Redfish spec)
redfishService *
createServiceEnumerator (
  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo,
  const char                          *rootUri,
  enumeratorAuthentication            *auth,
  unsigned int                        flags
  );

json_t *
getUriFromService (
  redfishService        *service,
  const char            *uri,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

json_t *
patchUriFromService (
  redfishService        *service,
  const char            *uri,
  const char            *content,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

json_t *
postUriFromService (
  redfishService        *service,
  const char            *uri,
  const char            *content,
  size_t                contentLength,
  const char            *contentType,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

json_t *
deleteUriFromService (
  redfishService        *service,
  const char            *uri,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

json_t *
deleteUriFromServiceEx (
  redfishService        *service,
  const char            *uri,
  const char            *content,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
getRedfishServiceRoot (
  redfishService        *service,
  const char            *version,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

redfishPayload *
getPayloadByPath (
  redfishService        *service,
  const char            *path,
  EFI_HTTP_STATUS_CODE  **StatusCode
  );

void
cleanupServiceEnumerator (
  redfishService  *service
  );

#endif
