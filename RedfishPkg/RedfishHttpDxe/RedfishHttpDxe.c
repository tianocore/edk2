/** @file
  RedfishHttpDxe produces EdkIIRedfishHttpProtocol
  for EDK2 Redfish Feature driver to do HTTP operations.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishHttpDxe.h"
#include "RedfishHttpData.h"
#include "RedfishHttpOperation.h"

REDFISH_HTTP_CACHE_PRIVATE  *mRedfishHttpCachePrivate = NULL;

/**
  Debug output the cache list.

  @param[in]    Msg            Debug message string.
  @param[in]    ErrorLevel     Output error level.
  @param[in]    CacheList      Target list to dump.

  @retval EFI_SUCCESS             Debug dump finished.
  @retval EFI_INVALID_PARAMETER   HttpCacheList is NULL.

**/
EFI_STATUS
DebugPrintHttpCacheList (
  IN  CONST CHAR8              *Msg,
  IN  UINTN                    ErrorLevel,
  IN  REDFISH_HTTP_CACHE_LIST  *CacheList
  )
{
  LIST_ENTRY               *List;
  REDFISH_HTTP_CACHE_DATA  *Data;
  UINTN                    Index;

  if (CacheList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_EMPTY_STRING (Msg)) {
    DEBUG ((ErrorLevel, "%a\n", Msg));
  }

  if (IsListEmpty (&CacheList->Head)) {
    DEBUG ((ErrorLevel, "list is empty\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((ErrorLevel, "list count: %d capacity: %d\n", CacheList->Count, CacheList->Capacity));
  Data  = NULL;
  Index = 0;
  List  = GetFirstNode (&CacheList->Head);
  while (!IsNull (&CacheList->Head, List)) {
    Data = REDFISH_HTTP_CACHE_FROM_LIST (List);

    DEBUG ((ErrorLevel, "%d) Uri: %s Hit: %d\n", ++Index, Data->Uri, Data->HitCount));

    List = GetNextNode (&CacheList->Head, List);
  }

  return EFI_SUCCESS;
}

/**

  Check HTTP status code to see if we like to retry HTTP request or not.

  @param[in]  StatusCode      HTTP status code.

  @retval     BOOLEAN         Return true when we like to retry request.
                              Return false when we don't want to retry request.

**/
BOOLEAN
RedfishRetryRequired (
  IN EFI_HTTP_STATUS_CODE  *StatusCode
  )
{
  if (StatusCode == NULL) {
    return TRUE;
  }

  if ((*StatusCode == HTTP_STATUS_500_INTERNAL_SERVER_ERROR) ||
      (*StatusCode == HTTP_STATUS_UNSUPPORTED_STATUS))
  {
    return TRUE;
  }

  return FALSE;
}

/**

  This function follows below sections in Redfish specification to
  check HTTP status code and see if this is success response or not.

  7.5.2 Modification success responses
  7.11 POST (action)

  @param[in]  Method          HTTP method of this status code.
  @param[in]  StatusCode      HTTP status code.

  @retval     BOOLEAN         Return true when this is success response.
                              Return false when this is not success response.

**/
BOOLEAN
RedfishSuccessResponse (
  IN EFI_HTTP_METHOD       Method,
  IN EFI_HTTP_STATUS_CODE  *StatusCode
  )
{
  BOOLEAN  SuccessResponse;

  if (StatusCode == NULL) {
    return TRUE;
  }

  SuccessResponse = FALSE;
  switch (Method) {
    case HttpMethodPost:
      if ((*StatusCode ==   HTTP_STATUS_200_OK) ||
          (*StatusCode ==   HTTP_STATUS_201_CREATED) ||
          (*StatusCode == HTTP_STATUS_202_ACCEPTED) ||
          (*StatusCode == HTTP_STATUS_204_NO_CONTENT))
      {
        SuccessResponse = TRUE;
      }

      break;
    case HttpMethodPatch:
    case HttpMethodPut:
    case HttpMethodDelete:
      if ((*StatusCode ==   HTTP_STATUS_200_OK) ||
          (*StatusCode == HTTP_STATUS_202_ACCEPTED) ||
          (*StatusCode == HTTP_STATUS_204_NO_CONTENT))
      {
        SuccessResponse = TRUE;
      }

      break;
    default:
      //
      // Return true for unsupported method to prevent false alarm.
      //
      SuccessResponse = TRUE;
      break;
  }

  return SuccessResponse;
}

/**

  Convert Unicode string to ASCII string. It's call responsibility to release returned buffer.

  @param[in]  UnicodeStr      Unicode string to convert.

  @retval     CHAR8 *         ASCII string returned.
  @retval     NULL            Errors occur.

**/
CHAR8 *
StringUnicodeToAscii (
  IN EFI_STRING  UnicodeStr
  )
{
  CHAR8       *AsciiStr;
  UINTN       AsciiStrSize;
  EFI_STATUS  Status;

  if (IS_EMPTY_STRING (UnicodeStr)) {
    return NULL;
  }

  AsciiStrSize = StrLen (UnicodeStr) + 1;
  AsciiStr     = AllocateZeroPool (AsciiStrSize);
  if (AsciiStr == NULL) {
    return NULL;
  }

  Status = UnicodeStrToAsciiStrS (UnicodeStr, AsciiStr, AsciiStrSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UnicodeStrToAsciiStrS failed: %r\n", Status));
    FreePool (AsciiStr);
    return NULL;
  }

  return AsciiStr;
}

/**
  Return HTTP method in ASCII string. Caller does not need
  to free returned string buffer.

  @param[in]  Method         HTTP method.

  @retval CHAR8 *   Method in string.
**/
CHAR8 *
HttpMethodToString (
  IN  EFI_HTTP_METHOD  Method
  )
{
  switch (Method) {
    case HttpMethodGet:
      return HTTP_METHOD_GET;
      break;
    case HttpMethodPost:
      return HTTP_METHOD_POST;
      break;
    case HttpMethodPatch:
      return HTTP_METHOD_PATCH;
      break;
    case HttpMethodPut:
      return HTTP_METHOD_PUT;
      break;
    case HttpMethodDelete:
      return HTTP_METHOD_DELETE;
      break;
    default:
      break;
  }

  return "Unknown";
}

/**
  Report HTTP communication error via report status code.

  @param[in]  Method         HTTP method.
  @param[in]  Uri            The URI which has failure.
  @param[in]  HttpStatusCode HTTP status code.

**/
VOID
ReportHttpError (
  IN  EFI_HTTP_METHOD       Method,
  IN  EFI_STRING            Uri,
  IN  EFI_HTTP_STATUS_CODE  *HttpStatusCode  OPTIONAL
  )
{
  CHAR8  ErrorMsg[REDFISH_ERROR_MSG_MAX];

  if (IS_EMPTY_STRING (Uri)) {
    DEBUG ((DEBUG_ERROR, "%a: no URI to report error status\n", __func__));
    return;
  }

  //
  // Report failure of URI and HTTP status code.
  //
  AsciiSPrint (ErrorMsg, sizeof (ErrorMsg), REDFISH_HTTP_ERROR_REPORT, HttpMethodToString (Method), (HttpStatusCode == NULL ? HTTP_STATUS_UNSUPPORTED_STATUS : *HttpStatusCode), Uri);
  DEBUG ((DEBUG_ERROR, "%a\n", ErrorMsg));
  //
  // TODO:
  // Below PI status code is approved by PIWG and wait for specification published.
  // We will uncomment below report status code after PI status code get published.
  // REF: https://bugzilla.tianocore.org/show_bug.cgi?id=4483
  //
  // REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
  //  EFI_ERROR_CODE | EFI_ERROR_MAJOR,
  //  EFI_COMPUTING_UNIT_MANAGEABILITY | EFI_MANAGEABILITY_EC_REDFISH_COMMUNICATION_ERROR,
  //  ErrorMsg,
  //  AsciiStrSize (ErrorMsg)
  //  );
}

/**
  This function create Redfish service. It's caller's responsibility to free returned
  Redfish service by calling FreeService ().

  @param[in]  This                       Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishConfigServiceInfo   Redfish config service information.

  @retval     REDFISH_SERVICE  Redfish service is created.
  @retval     NULL             Errors occur.

**/
REDFISH_SERVICE
EFIAPI
RedfishCreateRedfishService (
  IN  EDKII_REDFISH_HTTP_PROTOCOL         *This,
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo
  )
{
  EFI_STATUS                  Status;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;
  REDFISH_SERVICE_PRIVATE     *NewService;
  CHAR8                       *AsciiLocation;
  CHAR8                       *Host;
  CHAR8                       *BasicAuthString;
  UINTN                       BasicAuthStrSize;
  CHAR8                       *EncodedAuthString;
  UINTN                       EncodedAuthStrSize;
  EDKII_REDFISH_AUTH_METHOD   AuthMethod;
  CHAR8                       *Username;
  CHAR8                       *Password;
  UINTN                       UsernameSize;
  UINTN                       PasswordSize;
  EFI_REST_EX_PROTOCOL        *RestEx;

  if ((This == NULL) || (RedfishConfigServiceInfo == NULL)) {
    return NULL;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: service location: %s\n", __func__, RedfishConfigServiceInfo->RedfishServiceLocation));

  Private            = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  BasicAuthString    = NULL;
  EncodedAuthString  = NULL;
  Username           = NULL;
  Password           = NULL;
  NewService         = NULL;
  AsciiLocation      = NULL;
  Host               = NULL;
  BasicAuthStrSize   = 0;
  EncodedAuthStrSize = 0;
  UsernameSize       = 0;
  PasswordSize       = 0;

  //
  // Build host and host name from service location
  //
  if (!IS_EMPTY_STRING (RedfishConfigServiceInfo->RedfishServiceLocation)) {
    AsciiLocation = StringUnicodeToAscii (RedfishConfigServiceInfo->RedfishServiceLocation);
    if (AsciiLocation == NULL) {
      goto ON_RELEASE;
    }

    Host = AllocateZeroPool (REDFISH_HOST_NAME_MAX);
    if (AsciiLocation == NULL) {
      goto ON_RELEASE;
    }

    if (RedfishConfigServiceInfo->RedfishServiceUseHttps) {
      AsciiSPrint (Host, REDFISH_HOST_NAME_MAX, "https://%a", AsciiLocation);
    } else {
      AsciiSPrint (Host, REDFISH_HOST_NAME_MAX, "http://%a", AsciiLocation);
    }

    DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Host: %a\n", __func__, Host));
  }

  //
  // Find Rest Ex protocol
  //
  if (RedfishConfigServiceInfo->RedfishServiceRestExHandle != NULL) {
    Status = gBS->HandleProtocol (
                    RedfishConfigServiceInfo->RedfishServiceRestExHandle,
                    &gEfiRestExProtocolGuid,
                    (VOID **)&RestEx
                    );
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Rest Ex protocol is not available\n", __func__));
    goto ON_RELEASE;
  }

  //
  // Get credential
  //
  if (Private->CredentialProtocol == NULL) {
    //
    // No credential available on this system.
    //
    DEBUG ((DEBUG_WARN, "%a: no credential protocol available\n", __func__));
  } else {
    Status = Private->CredentialProtocol->GetAuthInfo (
                                            Private->CredentialProtocol,
                                            &AuthMethod,
                                            &Username,
                                            &Password
                                            );
    if (EFI_ERROR (Status) || ((AuthMethod != AuthMethodNone) && (IS_EMPTY_STRING (Username) || IS_EMPTY_STRING (Password)))) {
      DEBUG ((DEBUG_ERROR, "%a: cannot get authentication information: %r\n", __func__, Status));
      goto ON_RELEASE;
    } else if (AuthMethod != AuthMethodNone) {
      DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Auth method: 0x%x username: %a password: %a\n", __func__, AuthMethod, Username, Password));

      //
      // Perform base64 encoding (RFC 7617)
      //
      UsernameSize     = AsciiStrSize (Username);
      PasswordSize     = AsciiStrSize (Password);
      BasicAuthStrSize =  UsernameSize + PasswordSize;  // one byte taken from null-terminator for ':'
      BasicAuthString  = AllocateZeroPool (BasicAuthStrSize);
      if (BasicAuthString == NULL) {
        goto ON_RELEASE;
      }

      AsciiSPrint (
        BasicAuthString,
        BasicAuthStrSize,
        "%a:%a",
        Username,
        Password
        );

      Status = Base64Encode (
                 (CONST UINT8 *)BasicAuthString,
                 BasicAuthStrSize,
                 EncodedAuthString,
                 &EncodedAuthStrSize
                 );
      if ((Status == EFI_BUFFER_TOO_SMALL) && (EncodedAuthStrSize > 0)) {
        EncodedAuthString = AllocateZeroPool (EncodedAuthStrSize);
        if (EncodedAuthString == NULL) {
          goto ON_RELEASE;
        }

        Status = Base64Encode (
                   (CONST UINT8 *)BasicAuthString,
                   BasicAuthStrSize,
                   EncodedAuthString,
                   &EncodedAuthStrSize
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Base64Encode failure: %r\n", __func__, Status));
        }

        DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Basic authorization: %a\n", __func__, EncodedAuthString));
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Base64Encode failure: %r\n", __func__, Status));
        goto ON_RELEASE;
      }
    }
  }

  NewService = CreateRedfishService (Host, AsciiLocation, EncodedAuthString, NULL, RestEx);
  if (NewService == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: CreateRedfishService\n", __func__));
    goto ON_RELEASE;
  }

  if (Private->CredentialProtocol != NULL) {
    Status = Private->CredentialProtocol->RegisterRedfishService (Private->CredentialProtocol, NewService);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to register Redfish service - %r\n", __func__, Status));
    }
  }

ON_RELEASE:

  if (BasicAuthString != NULL) {
    ZeroMem (BasicAuthString, BasicAuthStrSize);
    FreePool (BasicAuthString);
  }

  if (EncodedAuthString != NULL) {
    ZeroMem (BasicAuthString, EncodedAuthStrSize);
    FreePool (EncodedAuthString);
  }

  if (Username != NULL) {
    ZeroMem (Username, UsernameSize);
    FreePool (Username);
  }

  if (Password != NULL) {
    ZeroMem (Password, PasswordSize);
    FreePool (Password);
  }

  if (AsciiLocation != NULL) {
    FreePool (AsciiLocation);
  }

  if (Host != NULL) {
    FreePool (Host);
  }

  return NewService;
}

/**
  This function free resources in Redfish service. RedfishService is no longer available
  after this function returns successfully.

  @param[in]  This            Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishService  Pointer to Redfish service to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishFreeRedfishService (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              RedfishService
  )
{
  EFI_STATUS                  Status;
  REDFISH_SERVICE_PRIVATE     *Service;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (RedfishService == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);

  Service = (REDFISH_SERVICE_PRIVATE *)RedfishService;
  if (Service->Signature != REDFISH_HTTP_SERVICE_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: signature check failure\n", __func__));
  }

  if (Private->CredentialProtocol != NULL) {
    Status = Private->CredentialProtocol->UnregisterRedfishService (Private->CredentialProtocol, RedfishService);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to unregister Redfish service - %r\n", __func__, Status));
    } else {
      if (Service->RestEx != NULL) {
        Status = Service->RestEx->Configure (Service->RestEx, NULL);
        DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: release RestEx instance: %r\n", __func__, Status));
      }
    }
  }

  return ReleaseRedfishService (Service);
}

/**
  This function returns JSON value in given RedfishPayload. Returned JSON value
  is a reference to the JSON value in RedfishPayload. Any modification to returned
  JSON value will change JSON value in RedfishPayload.

  @param[in]  This            Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishPayload  Pointer to Redfish payload.

  @retval     EDKII_JSON_VALUE   JSON value is returned.
  @retval     NULL               Errors occur.

**/
EDKII_JSON_VALUE
EFIAPI
RedfishJsonInRedfishPayload (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_PAYLOAD              RedfishPayload
  )
{
  REDFISH_PAYLOAD_PRIVATE  *Payload;

  if ((This == NULL) || (RedfishPayload == NULL)) {
    return NULL;
  }

  Payload = (REDFISH_PAYLOAD_PRIVATE *)RedfishPayload;
  if (Payload->Signature != REDFISH_HTTP_PAYLOAD_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: signature check failure\n", __func__));
  }

  return Payload->JsonValue;
}

/**
  Perform HTTP GET to Get redfish resource from given resource URI with
  cache mechanism supported. It's caller's responsibility to free Response
  by calling FreeResponse ().

  @param[in]  This          Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Service       Redfish service instance to perform HTTP GET.
  @param[in]  Uri           Target resource URI.
  @param[in]  Request       Additional request context. This is optional.
  @param[out] Response      HTTP response from redfish service.
  @param[in]  UseCache      If it is TRUE, this function will search for
                            cache first. If it is FALSE, this function
                            will query Redfish URI directly.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishGetResource (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  REDFISH_REQUEST              *Request OPTIONAL,
  OUT REDFISH_RESPONSE             *Response,
  IN  BOOLEAN                      UseCache
  )
{
  EFI_STATUS                  Status;
  REDFISH_HTTP_CACHE_DATA     *CacheData;
  UINTN                       RetryCount;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (Service == NULL) || (Response == NULL) || IS_EMPTY_STRING (Uri)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Get URI: %s cache: %a\n", __func__, Uri, (UseCache ? "true" : "false")));

  Private    = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  CacheData  = NULL;
  RetryCount = 0;
  ZeroMem (Response, sizeof (REDFISH_RESPONSE));

  if (Private->CacheDisabled) {
    UseCache = FALSE;
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: cache is disabled by PCD!\n", __func__));
  }

  //
  // Search for cache list.
  //
  if (UseCache) {
    CacheData = FindHttpCacheData (&Private->CacheList.Head, Uri);
    if (CacheData != NULL) {
      DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: cache hit! %s\n", __func__, Uri));

      //
      // Copy cached response to caller's buffer.
      //
      Status               = CopyRedfishResponse (CacheData->Response, Response);
      CacheData->HitCount += 1;
      return Status;
    }
  }

  //
  // Get resource from redfish service.
  //
  do {
    RetryCount += 1;
    Status      = HttpSendReceive (
                    Service,
                    Uri,
                    HttpMethodGet,
                    Request,
                    Response
                    );
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: HTTP request: %s :%r\n", __func__, Uri, Status));
    if (!EFI_ERROR (Status) || (RetryCount >= Private->RetrySetting.MaximumRetryGet)) {
      break;
    }

    //
    // Retry when BMC is not ready.
    //
    if ((Response->StatusCode != NULL)) {
      DEBUG_CODE (
        DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
        );

      if (!RedfishRetryRequired (Response->StatusCode)) {
        break;
      }

      //
      // Release response for next round of request.
      //
      This->FreeResponse (This, Response);
    }

    DEBUG ((DEBUG_WARN, "%a: RedfishGetByUriEx failed, retry (%d/%d)\n", __func__, RetryCount, Private->RetrySetting.MaximumRetryGet));
    if (Private->RetrySetting.RetryWait > 0) {
      gBS->Stall (Private->RetrySetting.RetryWait);
    }
  } while (TRUE);

  if (EFI_ERROR (Status)) {
    DEBUG_CODE (
      DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
      );
    //
    // Report status code for Redfish failure
    //
    ReportHttpError (HttpMethodGet, Uri, Response->StatusCode);
    DEBUG ((DEBUG_ERROR, "%a: get %s failed (%d/%d): %r\n", __func__, Uri, RetryCount, Private->RetrySetting.MaximumRetryGet, Status));
    goto ON_RELEASE;
  }

  if (!Private->CacheDisabled) {
    //
    // Keep response in cache list
    //
    Status = AddHttpCacheData (&Private->CacheList, Uri, Response);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to cache %s: %r\n", __func__, Uri, Status));
      goto ON_RELEASE;
    }

    DEBUG_CODE (
      DebugPrintHttpCacheList (__func__, REDFISH_HTTP_CACHE_DEBUG_DUMP, &Private->CacheList);
      );
  }

ON_RELEASE:

  return Status;
}

/**
  This function free resources in Request. Request is no longer available
  after this function returns successfully.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Request      HTTP request to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishFreeRequest (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_REQUEST              *Request
  )
{
  if ((This == NULL) || (Request == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: entry\n", __func__));

  return ReleaseRedfishRequest (Request);
}

/**
  This function free resources in given Response.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Response     HTTP response to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishFreeResponse (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_RESPONSE             *Response
  )
{
  if ((This == NULL) || (Response == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: entry\n", __func__));

  return ReleaseRedfishResponse (Response);
}

/**
  This function expire the cached response of given URI.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Uri          Target response of URI.

  @retval     EFI_SUCCESS     Target response is expired successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishExpireResponse (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  EFI_STRING                   Uri
  )
{
  REDFISH_HTTP_CACHE_PRIVATE  *Private;
  REDFISH_HTTP_CACHE_DATA     *CacheData;

  if ((This == NULL) || IS_EMPTY_STRING (Uri)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: expire URI: %s\n", __func__, Uri));

  Private = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);

  CacheData = FindHttpCacheData (&Private->CacheList.Head, Uri);
  if (CacheData == NULL) {
    return EFI_NOT_FOUND;
  }

  return DeleteHttpCacheData (&Private->CacheList, CacheData);
}

/**
  Perform HTTP PATCH to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling FreeResponse ().

  @param[in]  This          Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Service       Redfish service instance to perform HTTP PATCH.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to patch.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional. When ContentType is NULL, content
                            type HTTP_CONTENT_TYPE_APP_JSON will be used.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishPatchResource (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  )
{
  EFI_STATUS                  Status;
  UINTN                       RetryCount;
  REDFISH_REQUEST             Request;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (Service == NULL) || (Response == NULL) || IS_EMPTY_STRING (Uri) || IS_EMPTY_STRING (Content)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Patch URI: %s\n", __func__, Uri));

  Private    = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  RetryCount = 0;
  ZeroMem (Response, sizeof (REDFISH_RESPONSE));
  ZeroMem (&Request, sizeof (REDFISH_REQUEST));

  Request.Content       = Content;
  Request.ContentLength = ContentSize;
  Request.ContentType   = ContentType;

  //
  // Patch resource to redfish service.
  //
  do {
    RetryCount += 1;
    Status      = HttpSendReceive (
                    Service,
                    Uri,
                    HttpMethodPatch,
                    &Request,
                    Response
                    );
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: HTTP request: %s :%r\n", __func__, Uri, Status));
    if (!EFI_ERROR (Status) || (RetryCount >= Private->RetrySetting.MaximumRetryPatch)) {
      break;
    }

    //
    // Retry when BMC is not ready.
    //
    if ((Response->StatusCode != NULL)) {
      DEBUG_CODE (
        DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
        );

      if (!RedfishRetryRequired (Response->StatusCode)) {
        break;
      }

      //
      // Release response for next round of request.
      //
      This->FreeResponse (This, Response);
    }

    DEBUG ((DEBUG_WARN, "%a: RedfishPatchToUriEx failed, retry (%d/%d)\n", __func__, RetryCount, Private->RetrySetting.MaximumRetryPatch));
    if (Private->RetrySetting.RetryWait > 0) {
      gBS->Stall (Private->RetrySetting.RetryWait);
    }
  } while (TRUE);

  //
  // Redfish resource is updated. Automatically expire the cached response
  // so application can directly get resource from Redfish service again.
  //
  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Resource is updated, expire URI: %s\n", __func__, Uri));
  RedfishExpireResponse (This, Uri);

  if (EFI_ERROR (Status) || !RedfishSuccessResponse (HttpMethodPatch, Response->StatusCode)) {
    DEBUG_CODE (
      DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
      );
    //
    // Report status code for Redfish failure
    //
    ReportHttpError (HttpMethodPatch, Uri, Response->StatusCode);
    DEBUG ((DEBUG_ERROR, "%a: patch %s failed (%d/%d): %r\n", __func__, Uri, RetryCount, Private->RetrySetting.MaximumRetryPatch, Status));
    goto ON_RELEASE;
  }

ON_RELEASE:

  return Status;
}

/**
  Perform HTTP PUT to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling FreeResponse ().

  @param[in]  This          Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Service       Redfish service instance to perform HTTP PUT.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to put.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional. When ContentType is NULL, content
                            type HTTP_CONTENT_TYPE_APP_JSON will be used.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishPutResource (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  )
{
  EFI_STATUS                  Status;
  UINTN                       RetryCount;
  REDFISH_REQUEST             Request;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (Service == NULL) || (Response == NULL) || IS_EMPTY_STRING (Uri) || IS_EMPTY_STRING (Content)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Put URI: %s\n", __func__, Uri));

  Private    = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  RetryCount = 0;
  ZeroMem (Response, sizeof (REDFISH_RESPONSE));
  ZeroMem (&Request, sizeof (REDFISH_REQUEST));

  Request.Content       = Content;
  Request.ContentLength = ContentSize;
  Request.ContentType   = ContentType;

  //
  // Patch resource to redfish service.
  //
  do {
    RetryCount += 1;
    Status      = HttpSendReceive (
                    Service,
                    Uri,
                    HttpMethodPut,
                    &Request,
                    Response
                    );
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: HTTP request: %s :%r\n", __func__, Uri, Status));
    if (!EFI_ERROR (Status) || (RetryCount >= Private->RetrySetting.MaximumRetryPut)) {
      break;
    }

    //
    // Retry when BMC is not ready.
    //
    if ((Response->StatusCode != NULL)) {
      DEBUG_CODE (
        DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
        );

      if (!RedfishRetryRequired (Response->StatusCode)) {
        break;
      }

      //
      // Release response for next round of request.
      //
      This->FreeResponse (This, Response);
    }

    DEBUG ((DEBUG_WARN, "%a: RedfishPutToUri failed, retry (%d/%d)\n", __func__, RetryCount, Private->RetrySetting.MaximumRetryPut));
    if (Private->RetrySetting.RetryWait > 0) {
      gBS->Stall (Private->RetrySetting.RetryWait);
    }
  } while (TRUE);

  //
  // Redfish resource is updated. Automatically expire the cached response
  // so application can directly get resource from Redfish service again.
  //
  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Resource is updated, expire URI: %s\n", __func__, Uri));
  RedfishExpireResponse (This, Uri);

  if (EFI_ERROR (Status) || !RedfishSuccessResponse (HttpMethodPut, Response->StatusCode)) {
    DEBUG_CODE (
      DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
      );
    //
    // Report status code for Redfish failure
    //
    ReportHttpError (HttpMethodPut, Uri, Response->StatusCode);
    DEBUG ((DEBUG_ERROR, "%a: put %s failed (%d/%d): %r\n", __func__, Uri, RetryCount, Private->RetrySetting.MaximumRetryPut, Status));
    goto ON_RELEASE;
  }

ON_RELEASE:

  return Status;
}

/**
  Perform HTTP POST to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling FreeResponse ().

  @param[in]  This          Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Service       Redfish service instance to perform HTTP POST.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to post.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional. When ContentType is NULL, content
                            type HTTP_CONTENT_TYPE_APP_JSON will be used.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishPostResource (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  )
{
  EFI_STATUS                  Status;
  UINTN                       RetryCount;
  REDFISH_REQUEST             Request;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (Service == NULL) || (Response == NULL) || IS_EMPTY_STRING (Uri) || IS_EMPTY_STRING (Content)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Post URI: %s\n", __func__, Uri));

  Private    = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  RetryCount = 0;
  ZeroMem (Response, sizeof (REDFISH_RESPONSE));
  ZeroMem (&Request, sizeof (REDFISH_REQUEST));

  Request.Content       = Content;
  Request.ContentLength = ContentSize;
  Request.ContentType   = ContentType;

  //
  // Patch resource to redfish service.
  //
  do {
    RetryCount += 1;
    Status      = HttpSendReceive (
                    Service,
                    Uri,
                    HttpMethodPost,
                    &Request,
                    Response
                    );
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: HTTP request: %s :%r\n", __func__, Uri, Status));
    if (!EFI_ERROR (Status) || (RetryCount >= Private->RetrySetting.MaximumRetryPost)) {
      break;
    }

    //
    // Retry when BMC is not ready.
    //
    if ((Response->StatusCode != NULL)) {
      DEBUG_CODE (
        DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
        );

      if (!RedfishRetryRequired (Response->StatusCode)) {
        break;
      }

      //
      // Release response for next round of request.
      //
      This->FreeResponse (This, Response);
    }

    DEBUG ((DEBUG_WARN, "%a: RedfishPostToUri failed, retry (%d/%d)\n", __func__, RetryCount, Private->RetrySetting.MaximumRetryPost));
    if (Private->RetrySetting.RetryWait > 0) {
      gBS->Stall (Private->RetrySetting.RetryWait);
    }
  } while (TRUE);

  //
  // Redfish resource is updated. Automatically expire the cached response
  // so application can directly get resource from Redfish service again.
  //
  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Resource is updated, expire URI: %s\n", __func__, Uri));
  RedfishExpireResponse (This, Uri);

  if (EFI_ERROR (Status) || !RedfishSuccessResponse (HttpMethodPost, Response->StatusCode)) {
    DEBUG_CODE (
      DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
      );
    //
    // Report status code for Redfish failure
    //
    ReportHttpError (HttpMethodPost, Uri, Response->StatusCode);
    DEBUG ((DEBUG_ERROR, "%a: post %s failed (%d/%d): %r\n", __func__, Uri, RetryCount, Private->RetrySetting.MaximumRetryPost, Status));
    goto ON_RELEASE;
  }

ON_RELEASE:

  return Status;
}

/**
  Perform HTTP DELETE to delete redfish resource on given resource URI.
  It's caller's responsibility to free Response by calling FreeResponse ().

  @param[in]  This          Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Service       Redfish service instance to perform HTTP DELETE.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       JSON represented properties to be deleted. This is
                            optional.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content if Content is not NULL.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional. When Content is not NULL and
                            ContentType is NULL, content type HTTP_CONTENT_TYPE_APP_JSON
                            will be used.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
EFIAPI
RedfishDeleteResource (
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content OPTIONAL,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  )
{
  EFI_STATUS                  Status;
  UINTN                       RetryCount;
  REDFISH_REQUEST             Request;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  if ((This == NULL) || (Service == NULL) || (Response == NULL) || IS_EMPTY_STRING (Uri)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Delete URI: %s\n", __func__, Uri));

  Private    = REDFISH_HTTP_CACHE_PRIVATE_FROM_THIS (This);
  RetryCount = 0;
  ZeroMem (Response, sizeof (REDFISH_RESPONSE));
  ZeroMem (&Request, sizeof (REDFISH_REQUEST));

  Request.Content       = Content;
  Request.ContentLength = ContentSize;
  Request.ContentType   = ContentType;

  //
  // Patch resource to redfish service.
  //
  do {
    RetryCount += 1;
    Status      = HttpSendReceive (
                    Service,
                    Uri,
                    HttpMethodDelete,
                    &Request,
                    Response
                    );
    DEBUG ((REDFISH_HTTP_CACHE_DEBUG_REQUEST, "%a: HTTP request: %s :%r\n", __func__, Uri, Status));
    if (!EFI_ERROR (Status) || (RetryCount >= Private->RetrySetting.MaximumRetryDelete)) {
      break;
    }

    //
    // Retry when BMC is not ready.
    //
    if ((Response->StatusCode != NULL)) {
      DEBUG_CODE (
        DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
        );

      if (!RedfishRetryRequired (Response->StatusCode)) {
        break;
      }

      //
      // Release response for next round of request.
      //
      This->FreeResponse (This, Response);
    }

    DEBUG ((DEBUG_WARN, "%a: RedfishDeleteByUri failed, retry (%d/%d)\n", __func__, RetryCount, Private->RetrySetting.MaximumRetryDelete));
    if (Private->RetrySetting.RetryWait > 0) {
      gBS->Stall (Private->RetrySetting.RetryWait);
    }
  } while (TRUE);

  //
  // Redfish resource is updated. Automatically expire the cached response
  // so application can directly get resource from Redfish service again.
  //
  DEBUG ((REDFISH_HTTP_CACHE_DEBUG, "%a: Resource is updated, expire URI: %s\n", __func__, Uri));
  RedfishExpireResponse (This, Uri);

  if (EFI_ERROR (Status) || !RedfishSuccessResponse (HttpMethodDelete, Response->StatusCode)) {
    DEBUG_CODE (
      DumpRedfishResponse (NULL, DEBUG_ERROR, Response);
      );
    //
    // Report status code for Redfish failure
    //
    ReportHttpError (HttpMethodDelete, Uri, Response->StatusCode);
    DEBUG ((DEBUG_ERROR, "%a: delete %s failed (%d/%d): %r\n", __func__, Uri, RetryCount, Private->RetrySetting.MaximumRetryDelete, Status));
    goto ON_RELEASE;
  }

ON_RELEASE:

  return Status;
}

EDKII_REDFISH_HTTP_PROTOCOL  mEdkIIRedfishHttpProtocol = {
  EDKII_REDFISH_HTTP_PROTOCOL_REVISION,
  RedfishCreateRedfishService,
  RedfishFreeRedfishService,
  RedfishJsonInRedfishPayload,
  RedfishGetResource,
  RedfishPatchResource,
  RedfishPutResource,
  RedfishPostResource,
  RedfishDeleteResource,
  RedfishFreeRequest,
  RedfishFreeResponse,
  RedfishExpireResponse
};

/**
  Unloads an image.

  @param[in]  ImageHandle         Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS             The image has been unloaded.
  @retval EFI_INVALID_PARAMETER   ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
RedfishHttpDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mRedfishHttpCachePrivate == NULL) {
    return EFI_SUCCESS;
  }

  if (!IsListEmpty (&mRedfishHttpCachePrivate->CacheList.Head)) {
    ReleaseCacheList (&mRedfishHttpCachePrivate->CacheList);
  }

  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEdkIIRedfishHttpProtocolGuid,
         &mRedfishHttpCachePrivate->Protocol,
         NULL
         );

  FreePool (mRedfishHttpCachePrivate);
  mRedfishHttpCachePrivate = NULL;

  return EFI_SUCCESS;
}

/**
  This is a EDKII_REDFISH_CREDENTIAL_PROTOCOL notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
CredentialProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                  Status;
  REDFISH_HTTP_CACHE_PRIVATE  *Private;

  Private = (REDFISH_HTTP_CACHE_PRIVATE *)Context;
  if (Private->Signature != REDFISH_HTTP_DRIVER_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: signature check failure\n", __func__));
    return;
  }

  //
  // Locate HII credential protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEdkIIRedfishCredential2ProtocolGuid,
                  NULL,
                  (VOID **)&Private->CredentialProtocol
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  gBS->CloseEvent (Event);
}

/**
  Main entry for this driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
RedfishHttpEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  if (mRedfishHttpCachePrivate != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mRedfishHttpCachePrivate = AllocateZeroPool (sizeof (REDFISH_HTTP_CACHE_PRIVATE));
  if (mRedfishHttpCachePrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initial cache list and protocol instance.
  //
  mRedfishHttpCachePrivate->Signature   = REDFISH_HTTP_DRIVER_SIGNATURE;
  mRedfishHttpCachePrivate->ImageHandle = ImageHandle;
  CopyMem (&mRedfishHttpCachePrivate->Protocol, &mEdkIIRedfishHttpProtocol, sizeof (EDKII_REDFISH_HTTP_PROTOCOL));
  mRedfishHttpCachePrivate->CacheList.Capacity = REDFISH_HTTP_CACHE_LIST_SIZE;
  mRedfishHttpCachePrivate->CacheList.Count    = 0x00;
  mRedfishHttpCachePrivate->CacheDisabled      = PcdGetBool (PcdHttpCacheDisabled);
  InitializeListHead (&mRedfishHttpCachePrivate->CacheList.Head);

  //
  // Get retry settings
  //
  mRedfishHttpCachePrivate->RetrySetting.MaximumRetryGet    = PcdGet16 (PcdHttpGetRetry);
  mRedfishHttpCachePrivate->RetrySetting.MaximumRetryPut    = PcdGet16 (PcdHttpPutRetry);
  mRedfishHttpCachePrivate->RetrySetting.MaximumRetryPatch  = PcdGet16 (PcdHttpPatchRetry);
  mRedfishHttpCachePrivate->RetrySetting.MaximumRetryPost   = PcdGet16 (PcdHttpPostRetry);
  mRedfishHttpCachePrivate->RetrySetting.MaximumRetryDelete = PcdGet16 (PcdHttpDeleteRetry);
  mRedfishHttpCachePrivate->RetrySetting.RetryWait          = PcdGet16 (PcdHttpRetryWaitInSecond) * 1000000U;

  //
  // Install the gEdkIIRedfishHttpProtocolGuid onto Handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mRedfishHttpCachePrivate->ImageHandle,
                  &gEdkIIRedfishHttpProtocolGuid,
                  &mRedfishHttpCachePrivate->Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: cannot install Redfish http protocol: %r\n", __func__, Status));
    RedfishHttpDriverUnload (ImageHandle);
    return Status;
  }

  //
  // Install protocol notification if credential protocol is installed.
  //
  mRedfishHttpCachePrivate->NotifyEvent = EfiCreateProtocolNotifyEvent (
                                            &gEdkIIRedfishCredential2ProtocolGuid,
                                            TPL_CALLBACK,
                                            CredentialProtocolInstalled,
                                            mRedfishHttpCachePrivate,
                                            &Registration
                                            );
  if (mRedfishHttpCachePrivate->NotifyEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create protocol notification for gEdkIIRedfishCredential2ProtocolGuid\n", __func__));
    ASSERT (FALSE);
    RedfishHttpDriverUnload (ImageHandle);
    return Status;
  }

  return EFI_SUCCESS;
}
