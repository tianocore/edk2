/** @file
  RedfishHttpData handles internal data to support Redfish HTTP protocol.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishHttpData.h"
#include "RedfishHttpOperation.h"

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
  )
{
  if ((Service == NULL) || IS_EMPTY_STRING (Token)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Service->SessionToken != NULL) {
    FreePool (Service->SessionToken);
  }

  Service->SessionToken = ASCII_STR_DUPLICATE (Token);
  if (Service->SessionToken == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  This function release Redfish Service.

  @param[in]  Service         Pointer to service instance.

  @retval     EFI_SUCCESS     Service is released.
  @retval     Others          Error occurs.

**/
EFI_STATUS
ReleaseRedfishService (
  IN REDFISH_SERVICE_PRIVATE  *Service
  )
{
  if (Service == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Service->Host != NULL) {
    FreePool (Service->Host);
  }

  if (Service->HostName != NULL) {
    FreePool (Service->HostName);
  }

  if (Service->BasicAuth != NULL) {
    ZeroMem (Service->BasicAuth, AsciiStrSize (Service->BasicAuth));
    FreePool (Service->BasicAuth);
  }

  if (Service->SessionToken != NULL) {
    ZeroMem (Service->SessionToken, AsciiStrSize (Service->SessionToken));
    FreePool (Service->SessionToken);
  }

  FreePool (Service);

  return EFI_SUCCESS;
}

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
  )
{
  REDFISH_SERVICE_PRIVATE  *NewService;
  UINTN                    AuthStrSize;

  if (IS_EMPTY_STRING (Host) || IS_EMPTY_STRING (HostName) || (RestEx == NULL)) {
    return NULL;
  }

  NewService = AllocateZeroPool (sizeof (REDFISH_SERVICE_PRIVATE));
  if (NewService == NULL) {
    return NULL;
  }

  NewService->Signature = REDFISH_HTTP_SERVICE_SIGNATURE;
  NewService->Host      = ASCII_STR_DUPLICATE (Host);
  if (NewService->Host == NULL) {
    goto ON_ERROR;
  }

  NewService->HostName = ASCII_STR_DUPLICATE (HostName);
  if (NewService->HostName == NULL) {
    goto ON_ERROR;
  }

  if (!IS_EMPTY_STRING (BasicAuth)) {
    AuthStrSize           = AsciiStrSize (BasicAuth) + AsciiStrLen (REDFISH_HTTP_BASIC_AUTH_STR);
    NewService->BasicAuth = AllocateZeroPool (AuthStrSize);
    if (NewService->BasicAuth == NULL) {
      goto ON_ERROR;
    }

    AsciiSPrint (NewService->BasicAuth, AuthStrSize, "%a%a", REDFISH_HTTP_BASIC_AUTH_STR, BasicAuth);
  }

  if (!IS_EMPTY_STRING (SessionToken)) {
    NewService->SessionToken = ASCII_STR_DUPLICATE (SessionToken);
    if (NewService->SessionToken == NULL) {
      goto ON_ERROR;
    }
  }

  NewService->RestEx = RestEx;

  return NewService;

ON_ERROR:

  ReleaseRedfishService (NewService);

  return NULL;
}

/**
  This function release Redfish Payload.

  @param[in]  Payload         Pointer to payload instance.

  @retval     EFI_SUCCESS     Payload is released.
  @retval     Others          Error occurs.

**/
EFI_STATUS
ReleaseRedfishPayload (
  IN REDFISH_PAYLOAD_PRIVATE  *Payload
  )
{
  if (Payload == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Payload->Service != NULL) {
    ReleaseRedfishService (Payload->Service);
  }

  if (Payload->JsonValue != NULL) {
    JsonValueFree (Payload->JsonValue);
  }

  FreePool (Payload);

  return EFI_SUCCESS;
}

/**
  This function creat new payload. Server and JsonObj are
  copied to newly created payload.

  @param[in]  Service          Pointer to Service instance.
  @param[in]  JsonValue        Pointer to JSON value.

  @retval     REDFISH_PAYLOAD_PRIVATE  Newly created payload.
  @retval     NULL                     Error occurs.

**/
REDFISH_PAYLOAD_PRIVATE *
CreateRedfishPayload (
  IN REDFISH_SERVICE_PRIVATE  *Service,
  IN EDKII_JSON_VALUE         JsonValue
  )
{
  REDFISH_PAYLOAD_PRIVATE  *NewPayload;

  if ((Service == NULL) || (JsonValue == NULL)) {
    return NULL;
  }

  NewPayload = AllocateZeroPool (sizeof (REDFISH_PAYLOAD_PRIVATE));
  if (NewPayload == NULL) {
    return NULL;
  }

  NewPayload->Signature = REDFISH_HTTP_PAYLOAD_SIGNATURE;
  NewPayload->Service   = CreateRedfishService (Service->Host, Service->HostName, Service->BasicAuth, Service->SessionToken, Service->RestEx);
  if (NewPayload->Service == NULL) {
    goto ON_ERROR;
  }

  NewPayload->JsonValue = JsonValueClone (JsonValue);
  if (NewPayload->JsonValue == NULL) {
    goto ON_ERROR;
  }

  return NewPayload;

ON_ERROR:

  ReleaseRedfishPayload (NewPayload);

  return NULL;
}

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
  )
{
  REDFISH_PAYLOAD_PRIVATE  *Payload;
  UINTN                    Index;

  if ((SrcResponse == NULL) || (DstResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (SrcResponse == DstResponse) {
    return EFI_SUCCESS;
  }

  //
  // Status code
  //
  if (SrcResponse->StatusCode != NULL) {
    DstResponse->StatusCode = AllocateCopyPool (sizeof (EFI_HTTP_STATUS_CODE), SrcResponse->StatusCode);
    if (DstResponse->StatusCode == NULL) {
      goto ON_ERROR;
    }
  }

  //
  // Header
  //
  if ((SrcResponse->HeaderCount > 0) && (SrcResponse->Headers != NULL)) {
    DstResponse->HeaderCount = 0;
    DstResponse->Headers     = AllocateZeroPool (sizeof (EFI_HTTP_HEADER) * SrcResponse->HeaderCount);
    if (DstResponse->Headers == NULL) {
      goto ON_ERROR;
    }

    DstResponse->HeaderCount = SrcResponse->HeaderCount;

    for (Index = 0; Index < SrcResponse->HeaderCount; Index++) {
      DstResponse->Headers[Index].FieldName = ASCII_STR_DUPLICATE (SrcResponse->Headers[Index].FieldName);
      if (DstResponse->Headers[Index].FieldName == NULL) {
        goto ON_ERROR;
      }

      DstResponse->Headers[Index].FieldValue = ASCII_STR_DUPLICATE (SrcResponse->Headers[Index].FieldValue);
      if (DstResponse->Headers[Index].FieldValue == NULL) {
        goto ON_ERROR;
      }
    }
  }

  //
  // Payload
  //
  if (SrcResponse->Payload != NULL) {
    Payload = (REDFISH_PAYLOAD_PRIVATE *)SrcResponse->Payload;
    if (Payload->Signature != REDFISH_HTTP_PAYLOAD_SIGNATURE) {
      DEBUG ((DEBUG_ERROR, "%a: signature check failure\n", __func__));
      goto ON_ERROR;
    }

    DstResponse->Payload = CreateRedfishPayload (Payload->Service, Payload->JsonValue);
    if (DstResponse->Payload  == NULL) {
      goto ON_ERROR;
    }
  }

  return EFI_SUCCESS;

ON_ERROR:

  ReleaseRedfishResponse (DstResponse);

  return EFI_OUT_OF_RESOURCES;
}

/**
  This function clone input response and return to caller

  @param[in]  Response      Response to clone.

  @retval     REDFISH_RESPONSE *  Response is cloned.
  @retval     NULL                Errors occur.

**/
REDFISH_RESPONSE *
CloneRedfishResponse (
  IN REDFISH_RESPONSE  *Response
  )
{
  EFI_STATUS        Status;
  REDFISH_RESPONSE  *NewResponse;

  if (Response == NULL) {
    return NULL;
  }

  NewResponse = AllocateZeroPool (sizeof (REDFISH_RESPONSE));
  if (NewResponse == NULL) {
    return NULL;
  }

  Status = CopyRedfishResponse (Response, NewResponse);
  if (EFI_ERROR (Status)) {
    FreePool (NewResponse);
    return NULL;
  }

  return NewResponse;
}

/**
  Release REDFISH_HTTP_CACHE_DATA resource

  @param[in]    Data    Pointer to REDFISH_HTTP_CACHE_DATA instance

  @retval EFI_SUCCESS             REDFISH_HTTP_CACHE_DATA is released successfully.
  @retval EFI_INVALID_PARAMETER   Data is NULL

**/
EFI_STATUS
ReleaseHttpCacheData (
  IN REDFISH_HTTP_CACHE_DATA  *Data
  )
{
  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Uri != NULL) {
    FreePool (Data->Uri);
  }

  if (Data->Response != NULL) {
    ReleaseRedfishResponse (Data->Response);
    FreePool (Data->Response);
  }

  FreePool (Data);

  return EFI_SUCCESS;
}

/**
  Create new cache data.

  @param[in]    Uri       The URI string matching to this cache data.
  @param[in]    Response  HTTP response.

  @retval REDFISH_HTTP_CACHE_DATA *   Pointer to newly created cache data.
  @retval NULL                        No memory available.

**/
REDFISH_HTTP_CACHE_DATA *
NewHttpCacheData (
  IN  EFI_STRING        Uri,
  IN  REDFISH_RESPONSE  *Response
  )
{
  REDFISH_HTTP_CACHE_DATA  *NewData;
  UINTN                    Size;

  if (IS_EMPTY_STRING (Uri) || (Response == NULL)) {
    return NULL;
  }

  NewData = AllocateZeroPool (sizeof (REDFISH_HTTP_CACHE_DATA));
  if (NewData == NULL) {
    return NULL;
  }

  NewData->Signature = REDFISH_HTTP_CACHE_SIGNATURE;
  Size               = StrSize (Uri);
  NewData->Uri       = AllocateCopyPool (Size, Uri);
  if (NewData->Uri == NULL) {
    goto ON_ERROR;
  }

  NewData->Response = Response;
  NewData->HitCount = 1;

  return NewData;

ON_ERROR:

  if (NewData != NULL) {
    ReleaseHttpCacheData (NewData);
  }

  return NULL;
}

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
  )
{
  LIST_ENTRY               *List;
  REDFISH_HTTP_CACHE_DATA  *Data;

  if (IS_EMPTY_STRING (Uri)) {
    return NULL;
  }

  if (IsListEmpty (ListHeader)) {
    return NULL;
  }

  Data = NULL;
  List = GetFirstNode (ListHeader);
  while (!IsNull (ListHeader, List)) {
    Data = REDFISH_HTTP_CACHE_FROM_LIST (List);

    if (StrCmp (Data->Uri, Uri) == 0) {
      return Data;
    }

    List = GetNextNode (ListHeader, List);
  }

  return NULL;
}

/**
  Search on given ListHeader and return cache data with minimum hit count.

  @param[in]    ListHeader  Target list to search.

  @retval REDFISH_HTTP_CACHE_DATA   Target cache data is returned.
  @retval NULL                      No cache data is found.

**/
REDFISH_HTTP_CACHE_DATA *
FindUnusedHttpCacheData (
  IN  LIST_ENTRY  *ListHeader
  )
{
  LIST_ENTRY               *List;
  REDFISH_HTTP_CACHE_DATA  *Data;
  REDFISH_HTTP_CACHE_DATA  *UnusedData;
  UINTN                    HitCount;

  if (IsListEmpty (ListHeader)) {
    return NULL;
  }

  Data       = NULL;
  UnusedData = NULL;
  HitCount   = 0;

  List       = GetFirstNode (ListHeader);
  Data       = REDFISH_HTTP_CACHE_FROM_LIST (List);
  UnusedData = Data;
  HitCount   = Data->HitCount;
  List       = GetNextNode (ListHeader, List);

  while (!IsNull (ListHeader, List)) {
    Data = REDFISH_HTTP_CACHE_FROM_LIST (List);

    if (Data->HitCount < HitCount) {
      HitCount   = Data->HitCount;
      UnusedData = Data;
    }

    List = GetNextNode (ListHeader, List);
  }

  return UnusedData;
}

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
  )
{
  if ((List == NULL) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: delete: %s\n", __func__, Data->Uri));

  RemoveEntryList (&Data->List);
  --List->Count;

  return ReleaseHttpCacheData (Data);
}

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
  )
{
  REDFISH_HTTP_CACHE_DATA  *NewData;
  REDFISH_HTTP_CACHE_DATA  *OldData;
  REDFISH_HTTP_CACHE_DATA  *UnusedData;
  REDFISH_RESPONSE         *NewResponse;

  if ((List == NULL) || IS_EMPTY_STRING (Uri) || (Response == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If same cache data exist, replace it with latest one.
  //
  OldData = FindHttpCacheData (&List->Head, Uri);
  if (OldData != NULL) {
    DeleteHttpCacheData (List, OldData);
  }

  //
  // Check capacity
  //
  if (List->Count >= List->Capacity) {
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: list is full and retire unused cache\n", __func__));
    UnusedData = FindUnusedHttpCacheData (&List->Head);
    if (UnusedData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DeleteHttpCacheData (List, UnusedData);
  }

  //
  // Clone a local copy
  //
  NewResponse = CloneRedfishResponse (Response);
  if (NewResponse == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewData = NewHttpCacheData (Uri, NewResponse);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InsertTailList (&List->Head, &NewData->List);
  ++List->Count;

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: cache(%d/%d) %s\n", __func__, List->Count, List->Capacity, NewData->Uri));

  return EFI_SUCCESS;
}

/**
  Release all cache from list.

  @param[in]    CacheList    The list to be released.

  @retval EFI_SUCCESS             All cache data are released.
  @retval EFI_INVALID_PARAMETER   CacheList is NULL.

**/
EFI_STATUS
ReleaseCacheList (
  IN  REDFISH_HTTP_CACHE_LIST  *CacheList
  )
{
  LIST_ENTRY               *List;
  LIST_ENTRY               *Next;
  REDFISH_HTTP_CACHE_DATA  *Data;

  if (CacheList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&CacheList->Head)) {
    return EFI_SUCCESS;
  }

  Data = NULL;
  Next = NULL;
  List = GetFirstNode (&CacheList->Head);
  while (!IsNull (&CacheList->Head, List)) {
    Data = REDFISH_HTTP_CACHE_FROM_LIST (List);
    Next = GetNextNode (&CacheList->Head, List);

    DeleteHttpCacheData (CacheList, Data);

    List = Next;
  }

  return EFI_SUCCESS;
}
