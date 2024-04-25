/** @file
  This file defines the EDKII_REDFISH_HTTP_PROTOCOL interface.

  Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_HTTP_PROTOCOL_H_
#define EDKII_REDFISH_HTTP_PROTOCOL_H_

#include <RedfishServiceData.h>
#include <Library/JsonLib.h>
#include <Protocol/EdkIIRedfishConfigHandler.h>

typedef struct _EDKII_REDFISH_HTTP_PROTOCOL EDKII_REDFISH_HTTP_PROTOCOL;

/**
  This function create Redfish service. It's caller's responsibility to free returned
  Redfish service by calling FreeService ().

  @param[in]  This                       Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishConfigServiceInfo   Redfish config service information.

  @retval     REDFISH_SERVICE  Redfish service is created.
  @retval     NULL             Errors occur.

**/
typedef
REDFISH_SERVICE
(EFIAPI *REDFISH_HTTP_CREATE_SERVICE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL         *This,
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo
  );

/**
  This function free resources in Redfish service. RedfishService is no longer available
  after this function returns successfully.

  @param[in]  This            Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishService  Pointer to Redfish service to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_FREE_SERVICE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL         *This,
  IN  REDFISH_SERVICE                     RedfishService
  );

/**
  This function returns JSON value in given RedfishPayload. Returned JSON value
  is a reference to the JSON value in RedfishPayload. Any modification to returned
  JSON value will change JSON value in RedfishPayload.

  @param[in]  This            Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  RedfishPayload  Pointer to Redfish payload.

  @retval     EDKII_JSON_VALUE   JSON value is returned.
  @retval     NULL               Errors occur.

**/
typedef
EDKII_JSON_VALUE
(EFIAPI *REDFISH_HTTP_JSON_IN_PAYLOAD)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL         *This,
  IN  REDFISH_PAYLOAD                     RedfishPayload
  );

/**
  This function free resources in Request. Request is no longer available
  after this function returns successfully.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Request      HTTP request to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_FREE_REQUEST)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL *This,
  IN  REDFISH_REQUEST             *Request
  );

/**
  This function free resources in Response. Response is no longer available
  after this function returns successfully.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Response     HTTP response to be released.

  @retval     EFI_SUCCESS     Resource is released successfully.
  @retval     Others          Errors occur.

**/
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_FREE_RESPONSE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL *This,
  IN  REDFISH_RESPONSE            *Response
  );

/**
  This function expire the cached response of given URI.

  @param[in]  This         Pointer to EDKII_REDFISH_HTTP_PROTOCOL instance.
  @param[in]  Uri          Target response of URI.

  @retval     EFI_SUCCESS     Target response is expired successfully.
  @retval     Others          Errors occur.

**/
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_EXPIRE_RESPONSE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL *This,
  IN  EFI_STRING                  Uri
  );

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
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_GET_RESOURCE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL *This,
  IN  REDFISH_SERVICE             Service,
  IN  EFI_STRING                  Uri,
  IN  REDFISH_REQUEST             *Request OPTIONAL,
  OUT REDFISH_RESPONSE            *Response,
  IN  BOOLEAN                     UseCache
  );

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
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_PATCH_RESOURCE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  );

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
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_PUT_RESOURCE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  );

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
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_POST_RESOURCE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  );

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
typedef
EFI_STATUS
(EFIAPI *REDFISH_HTTP_DELETE_RESOURCE)(
  IN  EDKII_REDFISH_HTTP_PROTOCOL  *This,
  IN  REDFISH_SERVICE              Service,
  IN  EFI_STRING                   Uri,
  IN  CHAR8                        *Content OPTIONAL,
  IN  UINTN                        ContentSize OPTIONAL,
  IN  CHAR8                        *ContentType OPTIONAL,
  OUT REDFISH_RESPONSE             *Response
  );

///
/// Definition of _EDKII_REDFISH_HTTP_PROTOCOL.
///
struct _EDKII_REDFISH_HTTP_PROTOCOL {
  UINT32                          Version;
  REDFISH_HTTP_CREATE_SERVICE     CreateService;
  REDFISH_HTTP_FREE_SERVICE       FreeService;
  REDFISH_HTTP_JSON_IN_PAYLOAD    JsonInPayload;
  REDFISH_HTTP_GET_RESOURCE       GetResource;
  REDFISH_HTTP_PATCH_RESOURCE     PatchResource;
  REDFISH_HTTP_PUT_RESOURCE       PutResource;
  REDFISH_HTTP_POST_RESOURCE      PostResource;
  REDFISH_HTTP_DELETE_RESOURCE    DeleteResource;
  REDFISH_HTTP_FREE_REQUEST       FreeRequest;
  REDFISH_HTTP_FREE_RESPONSE      FreeResponse;
  REDFISH_HTTP_EXPIRE_RESPONSE    ExpireResponse;
};

#define EDKII_REDFISH_HTTP_PROTOCOL_REVISION  0x00001000

extern EFI_GUID  gEdkIIRedfishHttpProtocolGuid;

#endif
