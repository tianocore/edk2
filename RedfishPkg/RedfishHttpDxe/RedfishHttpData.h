/** @file
  Definitions of RedfishHttpData

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_HTTP_DATA_H_
#define EDKII_REDFISH_HTTP_DATA_H_

#include "RedfishHttpDxe.h"

#define REDFISH_HTTP_DRIVER_SIGNATURE   SIGNATURE_32 ('r', 'f', 'h', 'p')
#define REDFISH_HTTP_CACHE_SIGNATURE    SIGNATURE_32 ('r', 'f', 'c', 'h')
#define REDFISH_HTTP_SERVICE_SIGNATURE  SIGNATURE_32 ('r', 'f', 's', 'v')
#define REDFISH_HTTP_PAYLOAD_SIGNATURE  SIGNATURE_32 ('r', 'f', 'p', 'l')
#define REDFISH_HTTP_BASIC_AUTH_STR     "Basic "

///
/// REDFISH_SERVICE_PRIVATE definition.
///
typedef struct {
  UINT32                  Signature;
  CHAR8                   *Host;
  CHAR8                   *HostName;
  CHAR8                   *BasicAuth;
  CHAR8                   *SessionToken;
  EFI_REST_EX_PROTOCOL    *RestEx;
} REDFISH_SERVICE_PRIVATE;

///
/// REDFISH_PAYLOAD_PRIVATE definition.
///
typedef struct {
  UINT32                     Signature;
  REDFISH_SERVICE_PRIVATE    *Service;
  EDKII_JSON_VALUE           JsonValue;
} REDFISH_PAYLOAD_PRIVATE;

///
/// Definition of REDFISH_HTTP_CACHE_DATA
///
typedef struct {
  UINT32              Signature;
  LIST_ENTRY          List;
  EFI_STRING          Uri;
  UINTN               HitCount;
  REDFISH_RESPONSE    *Response;
} REDFISH_HTTP_CACHE_DATA;

#define REDFISH_HTTP_CACHE_FROM_LIST(a)  CR (a, REDFISH_HTTP_CACHE_DATA, List, REDFISH_HTTP_CACHE_SIGNATURE)

///
/// Definition of REDFISH_HTTP_CACHE_LIST
///
typedef struct {
  LIST_ENTRY    Head;
  UINTN         Count;
  UINTN         Capacity;
} REDFISH_HTTP_CACHE_LIST;

///
/// Definition of REDFISH_HTTP_RETRY_SETTING
///
typedef struct {
  UINT16    MaximumRetryGet;
  UINT16    MaximumRetryPut;
  UINT16    MaximumRetryPost;
  UINT16    MaximumRetryPatch;
  UINT16    MaximumRetryDelete;
  UINTN     RetryWait;
} REDFISH_HTTP_RETRY_SETTING;

///
/// Definition of REDFISH_HTTP_CACHE_PRIVATE
///
typedef struct {
  UINT32                                Signature;
  EFI_HANDLE                            ImageHandle;
  BOOLEAN                               CacheDisabled;
  EFI_EVENT                             NotifyEvent;
  REDFISH_HTTP_CACHE_LIST               CacheList;
  EDKII_REDFISH_HTTP_PROTOCOL           Protocol;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL    *CredentialProtocol;
  REDFISH_HTTP_RETRY_SETTING            RetrySetting;
} REDFISH_HTTP_CACHE_PRIVATE;

#define REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS(a)  CR (a, REDFISH_HTTP_CACHE_PRIVATE, Protocol, REDFISH_HTTP_DRIVER_SIGNATURE)

/**
  Search on given ListHeader for given URI string.

  @param[in]    ListHeader  Target list to search.
  @param[in]    Uri         Target URI to search.

  @retval REDFISH_HTTP_CACHE_DATA   Target cache data is found.
  @retval NULL                      No cache data with given URI is found.

**/
REDFISH_HTTP_CACHE_DATA *
FindHttpCacheData (
  IN  LIST_ENTRY  *ListHeader,
  IN  EFI_STRING  Uri
  );

/**
  This function copy the data in SrcResponse to DstResponse.

  @param[in]  SrcResponse      Source Response to copy.
  @param[out] DstResponse      Destination Response.

  @retval     EFI_SUCCESS      Response is copied successfully.
  @retval     Others           Error occurs.

**/
EFI_STATUS
CopyRedfishResponse (
  IN  REDFISH_RESPONSE  *SrcResponse,
  OUT REDFISH_RESPONSE  *DstResponse
  );

/**
  Release all cache from list.

  @param[in]    CacheList    The list to be released.

  @retval EFI_SUCCESS             All cache data are released.
  @retval EFI_INVALID_PARAMETER   CacheList is NULL.

**/
EFI_STATUS
ReleaseCacheList (
  IN  REDFISH_HTTP_CACHE_LIST  *CacheList
  );

/**
  Add new cache by given URI and HTTP response to specify List.

  @param[in]    List      Target cache list to add.
  @param[in]    Uri       The URI string matching to this cache data.
  @param[in]    Response  HTTP response.

  @retval EFI_SUCCESS   Cache data is added.
  @retval Others        Fail to add cache data.

**/
EFI_STATUS
AddHttpCacheData (
  IN  REDFISH_HTTP_CACHE_LIST  *List,
  IN  EFI_STRING               Uri,
  IN  REDFISH_RESPONSE         *Response
  );

/**
  Delete a cache data by given cache instance.

  @param[in]    List    Target cache list to be removed.
  @param[in]    Data    Pointer to the instance to be deleted.

  @retval EFI_SUCCESS   Cache data is removed.
  @retval Others        Fail to remove cache data.

**/
EFI_STATUS
DeleteHttpCacheData (
  IN  REDFISH_HTTP_CACHE_LIST  *List,
  IN  REDFISH_HTTP_CACHE_DATA  *Data
  );

/**
  This function release Redfish Payload.

  @param[in]  Payload         Pointer to payload instance.

  @retval     EFI_SUCCESS     Payload is released.
  @retval     Others          Error occurs.

**/
EFI_STATUS
ReleaseRedfishPayload (
  IN REDFISH_PAYLOAD_PRIVATE  *Payload
  );

/**
  This function creat new payload. Server and JsonObj are
  copied to newly created payload.

  @param[in]  Service          Pointer to Service instance.
  @param[in]  JsonObj          Pointer to JSON object.

  @retval     REDFISH_PAYLOAD_PRIVATE  Newly created payload.
  @retval     NULL             Error occurs.

**/
REDFISH_PAYLOAD_PRIVATE *
CreateRedfishPayload (
  IN REDFISH_SERVICE_PRIVATE  *Service,
  IN EDKII_JSON_VALUE         JsonValue
  );

/**
  This function release Redfish Service.

  @param[in]  Service         Pointer to service instance.

  @retval     EFI_SUCCESS     Service is released.
  @retval     Others          Error occurs.

**/
EFI_STATUS
ReleaseRedfishService (
  IN REDFISH_SERVICE_PRIVATE  *Service
  );

/**
  This function creat new service. Host and HostName are copied to
  newly created service instance.

  @param[in]  Host            Host string.
  @param[in]  HostName        Hostname string.
  @param[in]  BasicAuth       Basic Authorization string.
  @param[in]  SessionToken    Session token string.
  @param[in]  RestEx          Rest EX protocol instance.

  @retval     REDFISH_PAYLOAD_PRIVATE  Newly created service.
  @retval     NULL             Error occurs.

**/
REDFISH_SERVICE_PRIVATE *
CreateRedfishService (
  IN CHAR8                 *Host,
  IN CHAR8                 *HostName,
  IN CHAR8                 *BasicAuth OPTIONAL,
  IN CHAR8                 *SessionToken OPTIONAL,
  IN EFI_REST_EX_PROTOCOL  *RestEx
  );

/**
  This function update session token in Redfish Service.

  @param[in]  Service         Pointer to service instance.
  @param[in]  Token           Session token.

  @retval     EFI_SUCCESS     Session token is updated.
  @retval     Others          Error occurs.

**/
EFI_STATUS
UpdateSessionToken (
  IN REDFISH_SERVICE_PRIVATE  *Service,
  IN CHAR8                    *Token
  );

#endif
