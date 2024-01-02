/** @file
  Redfish HTTP cache library helps Redfish application to get Redfish resource
  from BMC with cache mechanism enabled.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RedfishHttpLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

EDKII_REDFISH_HTTP_PROTOCOL  *mRedfishHttpProtocol = NULL;

/**
  This function create Redfish service. It's caller's responsibility to free returned
  Redfish service by calling FreeService ().

  @param[in]  RedfishConfigServiceInfo   Redfish config service information.

  @retval     REDFISH_SERVICE  Redfish service is created.
  @retval     NULL             Errors occur.

**/
REDFISH_SERVICE
RedfishCreateService (
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return NULL;
  }

  return mRedfishHttpProtocol->CreateService (
                                 mRedfishHttpProtocol,
                                 RedfishConfigServiceInfo
                                 );
}

/**
  This function free resources in Redfish service. RedfishService is no longer available
  after this function returns successfully.

  @param[in]  RedfishService  Pointer to Redfish service to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishCleanupService (
  IN  REDFISH_SERVICE  RedfishService
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->FreeService (
                                 mRedfishHttpProtocol,
                                 RedfishService
                                 );
}

/**
  This function returns JSON value in given RedfishPayload. Returned JSON value
  is a reference to the JSON value in RedfishPayload. Any modification to returned
  JSON value will change JSON value in RedfishPayload.

  @param[in]  RedfishPayload  Pointer to Redfish payload.

  @retval     EDKII_JSON_VALUE   JSON value is returned.
  @retval     NULL               Errors occur.

**/
EDKII_JSON_VALUE
RedfishJsonInPayload (
  IN  REDFISH_PAYLOAD  RedfishPayload
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return NULL;
  }

  return mRedfishHttpProtocol->JsonInPayload (
                                 mRedfishHttpProtocol,
                                 RedfishPayload
                                 );
}

/**
  This function free resources in Request. Request is no longer available
  after this function returns successfully.

  @param[in]  Request      HTTP request to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpFreeRequest (
  IN  REDFISH_REQUEST  *Request
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->FreeRequest (
                                 mRedfishHttpProtocol,
                                 Request
                                 );
}

/**
  This function free resources in Response. Response is no longer available
  after this function returns successfully.

  @param[in]  Response     HTTP response to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpFreeResponse (
  IN  REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->FreeResponse (
                                 mRedfishHttpProtocol,
                                 Response
                                 );
}

/**
  This function expire the cached response of given URI.

  @param[in]  Uri          Target response of URI.

  @retval     EFI_SUCCESS     Target response is expired successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpExpireResponse (
  IN  EFI_STRING  Uri
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->ExpireResponse (
                                 mRedfishHttpProtocol,
                                 Uri
                                 );
}

/**
  Get redfish resource from given resource URI with cache mechanism
  supported. It's caller's responsibility to Response by calling
  RedfishHttpFreeResponse ().

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
RedfishHttpGetResource (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  REDFISH_REQUEST   *Request OPTIONAL,
  OUT REDFISH_RESPONSE  *Response,
  IN  BOOLEAN           UseCache
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->GetResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Request,
                                 Response,
                                 UseCache
                                 );
}

/**
  Perform HTTP PATCH to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP PATCH.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to patch.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPatchResource (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PatchResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 0,
                                 NULL,
                                 Response
                                 );
}

/**
  Perform HTTP PATCH to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP PATCH.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to patch.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPatchResourceEx (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  IN  UINTN             ContentSize OPTIONAL,
  IN  CHAR8             *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PatchResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 ContentSize,
                                 ContentType,
                                 Response
                                 );
}

/**
  Perform HTTP PUT to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP PUT.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to put.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPutResource (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PutResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 0,
                                 NULL,
                                 Response
                                 );
}

/**
  Perform HTTP PUT to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP PUT.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to put.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPutResourceEx (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  IN  UINTN             ContentSize OPTIONAL,
  IN  CHAR8             *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PutResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 ContentSize,
                                 ContentType,
                                 Response
                                 );
}

/**
  Perform HTTP POST to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP POST.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to post.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPostResource (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PostResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 0,
                                 NULL,
                                 Response
                                 );
}

/**
  Perform HTTP POST to send redfish resource to given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP POST.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       Data to post.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpPostResourceEx (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content,
  IN  UINTN             ContentSize OPTIONAL,
  IN  CHAR8             *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->PostResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 ContentSize,
                                 ContentType,
                                 Response
                                 );
}

/**
  Perform HTTP DELETE to delete redfish resource on given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP DELETE.
  @param[in]  Uri           Target resource URI.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpDeleteResource (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->DeleteResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 NULL,
                                 0,
                                 NULL,
                                 Response
                                 );
}

/**
  Perform HTTP DELETE to delete redfish resource on given resource URI.
  It's caller's responsibility to free Response by calling RedfishHttpFreeResponse ().

  @param[in]  Service       Redfish service instance to perform HTTP DELETE.
  @param[in]  Uri           Target resource URI.
  @param[in]  Content       JSON represented properties to be deleted. This is
                            optional.
  @param[in]  ContentSize   Size of the Content to be send to Redfish service.
                            This is optional. When ContentSize is 0, ContentSize
                            is the size of Content if Content is not NULL.
  @param[in]  ContentType   Type of the Content to be send to Redfish service.
                            This is optional.
  @param[out] Response      HTTP response from redfish service.

  @retval     EFI_SUCCESS     Resource is returned successfully.
  @retval     Others          Errors occur.

**/
EFI_STATUS
RedfishHttpDeleteResourceEx (
  IN  REDFISH_SERVICE   Service,
  IN  EFI_STRING        Uri,
  IN  CHAR8             *Content OPTIONAL,
  IN  UINTN             ContentSize OPTIONAL,
  IN  CHAR8             *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE  *Response
  )
{
  if (mRedfishHttpProtocol == NULL) {
    return EFI_NOT_READY;
  }

  return mRedfishHttpProtocol->DeleteResource (
                                 mRedfishHttpProtocol,
                                 Service,
                                 Uri,
                                 Content,
                                 ContentSize,
                                 ContentType,
                                 Response
                                 );
}

/**
  Callback function when gEdkIIRedfishHttpProtocolGuid is installed.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
RedfishHttpProtocolIsReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mRedfishHttpProtocol != NULL) {
    gBS->CloseEvent (Event);
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEdkIIRedfishHttpProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishHttpProtocol
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  gBS->CloseEvent (Event);
}

/**

  Initial HTTP library instance.

  @param[in] ImageHandle     The image handle.
  @param[in] SystemTable     The system table.

  @retval  EFI_SUCCESS  Initial library successfully.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
RedfishHttpConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *Registration;

  EfiCreateProtocolNotifyEvent (
    &gEdkIIRedfishHttpProtocolGuid,
    TPL_CALLBACK,
    RedfishHttpProtocolIsReady,
    NULL,
    &Registration
    );

  return EFI_SUCCESS;
}
