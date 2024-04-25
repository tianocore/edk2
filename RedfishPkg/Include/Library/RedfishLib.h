/** @file
  This library provides a set of utility APIs that allow to create/read/update/delete
  (CRUD) Redfish resources and provide basic query abilities by using [URI/RedPath]
  (https://github.com/DMTF/libredfish).

  The query language is based on XPath (https://www.w3.org/TR/1999/REC-xpath-19991116/).
  This library and query language essentially treat the entire Redfish Service like it
  was a single JSON document. In other words whenever it encounters an odata.id in JSON
  document, it will retrieve the new JSON document (if needed). We name the path as
  RedPath:

  Expression       Description

  nodename         Selects the JSON entity with the name "nodename".
                   If the value of nodename is an object with "@odata.id",
                   it will continue get the value from "@odata.id".

  /                Selects from the root node

  [index]           Selects the index number JSON entity from an array or
                   object. If the JSON entity is one collection (has
                   Members & Members@odata.count), means to get the index
                   member in "Members". Index number >=1, 1 means to return
                   the first instance.

  [XXX]            Operation on JSON entity.
                   If the JSON entity is one collection (has Members &
                   Members@odata.count), means to get all elements in
                   "Members". If the JSON entity is one array, means to
                   get all elements in array. Others will match the nodename
                   directly (e.g. JSON_OBJECT, JSON_STRING, JSON_TRUE,
                   JSON_FALSE, JSON_INTEGER).

  [nodename]       Selects all the elements from an JSON entity that
                   contain a property named "nodename"

  [name=value]     Selects all the elements from an JSON entity where
                   the property "name" is equal to "value"

  [name~value]     Selects all the elements from an JSON entity where
                   the string property "name" is equal to "value" using
                   case insensitive comparison.

  [name<value]     Selects all the elements from an JSON entity where
                   the property "name" is less than "value"

  [name<=value]    Selects all the elements from an JSON entity where
                   the property "name" is less than or equal to "value"

  [name>value]     Selects all the elements from an JSON entity where
                   the property "name" is greater than "value"

  [name>=value]    Selects all the elements from an JSON entity where
                   the property "name" is greater than or equal to "value"

  Some examples:

    /v1/Chassis[1]        - Will return the first Chassis instance.
    /v1/Chassis[SKU=1234] - Will return all Chassis instances with a SKU field equal to 1234.
    /v1/Systems[Storage]  - Will return all the System instances that have Storage field populated.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_LIB_H_
#define REDFISH_LIB_H_

#include <RedfishServiceData.h>
#include <Library/JsonLib.h>

#include <Protocol/Http.h>
#include <Protocol/EdkIIRedfishConfigHandler.h>

#define ODATA_TYPE_NAME_MAX_SIZE  128
#define ODATA_TYPE_MAX_SIZE       128

///
/// Odata type-name mapping structure.
///
typedef struct {
  CONST CHAR8    OdataTypeName[ODATA_TYPE_NAME_MAX_SIZE];
  CONST CHAR8    OdataType[ODATA_TYPE_MAX_SIZE];
} REDFISH_ODATA_TYPE_MAPPING;

/**
  This function uses REST EX protocol provided in RedfishConfigServiceInfo.
  The service enumerator will also handle the authentication flow automatically
  if HTTP basic auth or Redfish session login is configured to use.

  Callers are responsible for freeing the returned service by RedfishCleanupService().

  @param[in]  RedfishConfigServiceInfo Redfish service information the EFI Redfish
                                       feature driver communicates with.

  @return     New created Redfish Service, or NULL if error happens.

**/
REDFISH_SERVICE
EFIAPI
RedfishCreateService (
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo
  );

/**
  Free the Service and all its related resources.

  @param[in]    RedfishService     The Service to access the Redfish resources.

**/
VOID
EFIAPI
RedfishCleanupService (
  IN REDFISH_SERVICE  RedfishService
  );

/**
  Create REDFISH_PAYLOAD instance in local with JSON represented resource value and
  the Redfish Service.

  The returned REDFISH_PAYLOAD can be used to create or update Redfish resource in
  server side.

  Callers are responsible for freeing the returned payload by RedfishCleanupPayload().

  @param[in]    Value                 JSON Value of the redfish resource.
  @param[in]    RedfishService        The Service to access the Redfish resources.

  @return     REDFISH_PAYLOAD instance of the resource, or NULL if error happens.

**/
REDFISH_PAYLOAD
EFIAPI
RedfishCreatePayload (
  IN EDKII_JSON_VALUE  Value,
  IN REDFISH_SERVICE   RedfishService
  );

/**
  Free the RedfishPayload and all its related resources.

  @param[in]    Payload        Payload to be freed.

**/
VOID
EFIAPI
RedfishCleanupPayload (
  IN REDFISH_PAYLOAD  Payload
  );

/**
  This function returns the decoded JSON value of a REDFISH_PAYLOAD.

  Caller doesn't need to free the returned JSON value because it will be released
  in corresponding RedfishCleanupPayload() function.

  @param[in]    Payload     A REDFISH_PAYLOAD instance.

  @return     Decoded JSON value of the payload.

**/
EDKII_JSON_VALUE
EFIAPI
RedfishJsonInPayload (
  IN REDFISH_PAYLOAD  Payload
  );

/**
  This function returns the Redfish service of a REDFISH_PAYLOAD.

  Caller doesn't need to free the returned JSON value because it will be released
  in corresponding RedfishCleanupService() function.

  @param[in]    Payload     A REDFISH_PAYLOAD instance.

  @return     Redfish service of the payload.

**/
REDFISH_SERVICE
EFIAPI
RedfishServiceInPayload (
  IN REDFISH_PAYLOAD  Payload
  );

/**
  Fill the input RedPath string with system UUID from SMBIOS table or use the customized
  ID if  FromSmbios == FALSE.

  This is a helper function to build a RedPath string which can be used to address
  a Redfish resource for this computer system. The input PathString must have a Systems
  note in format of "Systems[UUID=%g]" or "Systems[UUID~%g]" to fill the UUID value.

  Example:
    Use "/v1/Systems[UUID=%g]/Bios" to build a RedPath to address the "Bios" resource
    for this computer system.

  @param[in]    RedPath        RedPath format to be build.
  @param[in]    FromSmbios     Get system UUID from SMBIOS as computer system instance ID.
  @param[in]    IdString       The computer system instance ID.

  @return     Full RedPath with system UUID inside, or NULL if error happens.

**/
CHAR8 *
EFIAPI
RedfishBuildPathWithSystemUuid (
  IN CONST CHAR8  *RedPath,
  IN BOOLEAN      FromSmbios,
  IN CHAR8        *IdString OPTIONAL
  );

/**
  Get a redfish response addressed by a RedPath string, including HTTP StatusCode, Headers
  and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    RedPath               RedPath string to address a resource, must start
                                      from the root node.
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The corresponding redfish resource has
                                  been returned in Payload within RedResponse.
  @retval EFI_INVALID_PARAMETER   RedfishService, RedPath, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned Payload is NULL, indicates any error happen.
                                  2. If the returned StatusCode is NULL, indicates any error happen.
                                  3. If the returned StatusCode is not 2XX, indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishGetByService (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *RedPath,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Get a redfish response addressed by URI, including HTTP StatusCode, Headers
  and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService    The Service to access the URI resources.
  @param[in]    URI               String to address a resource.
  @param[out]   RedResponse       Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The corresponding redfish resource has
                                  been returned in Payload within RedResponse.
  @retval EFI_INVALID_PARAMETER   RedfishService, RedPath, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned Payload is NULL, indicates any error happen.
                                  2. If the returned StatusCode is NULL, indicates any error happen.
                                  3. If the returned StatusCode is not 2XX, indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishGetByUri (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Get a redfish response addressed by the input Payload and relative RedPath string,
  including HTTP StatusCode, Headers and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    Payload           A existing REDFISH_PAYLOAD instance.
  @param[in]    RedPath           Relative RedPath string to address a resource inside Payload.
  @param[out]   RedResponse       Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful:
                                  1. The HTTP StatusCode is NULL and the returned Payload in
                                  RedResponse is not NULL, indicates the Redfish resource has
                                  been parsed from the input payload directly.
                                  2. The HTTP StatusCode is not NULL and the value is 2XX,
                                  indicates the corresponding redfish resource has been returned
                                  in Payload within RedResponse.
  @retval EFI_INVALID_PARAMETER   Payload, RedPath, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned Payload is NULL, indicates any error happen.
                                  2. If StatusCode is not NULL and the returned value of StatusCode
                                     is not 2XX, indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishGetByPayload (
  IN     REDFISH_PAYLOAD   Payload,
  IN     CONST CHAR8       *RedPath,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP PATCH to perform updates on pre-existing Redfish resource.

  This function uses the RedfishService to patch a Redfish resource addressed by
  Uri (only the relative path is required). Changes to one or more properties within
  the target resource are represented in the input Content, properties not specified
  in Content won't be changed by this request. The corresponding redfish response will
  returned, including HTTP StatusCode, Headers and Payload which record any HTTP response
  messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    Uri                   Relative path to address the resource.
  @param[in]    Content               JSON represented properties to be update.
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The Redfish resource will be returned
                                  in Payload within RedResponse if server send it back in the HTTP
                                  response message body.
  @retval EFI_INVALID_PARAMETER   RedfishService, Uri, Content, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishPatchToUri (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  IN     CONST CHAR8       *Content,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP PATCH to perform updates on target payload. Patch to odata.id in Payload directly.

  This function uses the Payload to patch the Target. Changes to one or more properties
  within the target resource are represented in the input Payload, properties not specified
  in Payload won't be changed by this request. The corresponding redfish response will
  returned, including HTTP StatusCode, Headers and Payload which record any HTTP response
  messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    Target           The target payload to be updated.
  @param[in]    Payload          Payload with properties to be changed.
  @param[out]   RedResponse      Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The Redfish resource will be returned
                                  in Payload within RedResponse if server send it back in the HTTP
                                  response message body.
  @retval EFI_INVALID_PARAMETER   Target, Payload, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishPatchToPayload (
  IN     REDFISH_PAYLOAD   Target,
  IN     REDFISH_PAYLOAD   Payload,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP POST to create new Redfish resource in the Resource Collection.

  The POST request should be submitted to the Resource Collection in which the new resource
  is to belong. The Resource Collection is addressed by URI. The Redfish may
  ignore any service controlled properties. The corresponding redfish response will returned,
  including HTTP StatusCode, Headers and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    Uri                   Relative path to address the resource.
  @param[in]    Content               JSON represented properties to be update.
  @param[in]    ContentSize           Size of the Content to be send to Redfish service
  @param[in]    ContentType           Type of the Content to be send to Redfish service
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The Redfish resource will be returned
                                  in Payload within RedResponse if server send it back in the HTTP
                                  response message body.
  @retval EFI_INVALID_PARAMETER   RedfishService, Uri, Content, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishPostToUri (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  IN     CONST CHAR8       *Content,
  IN     UINTN             ContentSize OPTIONAL,
  IN     CONST CHAR8       *ContentType OPTIONAL,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP POST to create a new resource in target payload.

  The POST request should be submitted to the Resource Collection in which the new resource
  is to belong. The Resource Collection is addressed by Target payload. The Redfish may
  ignore any service controlled properties. The corresponding redfish response will returned,
  including HTTP StatusCode, Headers and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    Target          Target payload of the Resource Collection.
  @param[in]    Payload         The new resource to be created.
  @param[out]   RedResponse     Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The Redfish resource will be returned
                                  in Payload within RedResponse if server send it back in the HTTP
                                  response message body.
  @retval EFI_INVALID_PARAMETER   Target, Payload, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishPostToPayload (
  IN     REDFISH_PAYLOAD   Target,
  IN     REDFISH_PAYLOAD   Payload,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP DELETE to remove a resource.

  This function uses the RedfishService to remove a Redfish resource which is addressed
  by input Uri (only the relative path is required). The corresponding redfish response will
  returned, including HTTP StatusCode, Headers and Payload which record any HTTP response
  messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    Uri                   Relative path to address the resource.
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX, the Redfish resource has been removed.
                                  If there is any message returned from server, it will be returned
                                  in Payload within RedResponse.
  @retval EFI_INVALID_PARAMETER   RedfishService, Uri, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishDeleteByUri (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP DELETE to remove a resource.

  This function uses the RedfishService to remove a Redfish resource which is addressed
  by input Uri (only the relative path is required). The corresponding redfish response will
  returned, including HTTP StatusCode, Headers and Payload which record any HTTP response
  messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    Uri                   Relative path to address the resource.
  @param[in]    Content               JSON represented properties to be deleted.
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX, the Redfish resource has been removed.
                                  If there is any message returned from server, it will be returned
                                  in Payload within RedResponse.
  @retval EFI_INVALID_PARAMETER   RedfishService, Uri, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishDeleteByUriEx (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  IN     CONST CHAR8       *Content,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Use HTTP PUT to create new Redfish resource in the Resource Collection.

  This function uses the RedfishService to put a Redfish resource addressed by
  Uri (only the relative path is required). Changes to one or more properties within
  the target resource are represented in the input Content, properties not specified
  in Content won't be changed by this request. The corresponding redfish response will
  returned, including HTTP StatusCode, Headers and Payload which record any HTTP response
  messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService        The Service to access the Redfish resources.
  @param[in]    Uri                   Relative path to address the resource.
  @param[in]    Content               JSON represented properties to be update.
  @param[in]    ContentSize           Size of the Content to be send to Redfish service
  @param[in]    ContentType           Type of the Content to be send to Redfish service
  @param[out]   RedResponse           Pointer to the Redfish response data.

  @retval EFI_SUCCESS             The operation is successful, indicates the HTTP StatusCode is not
                                  NULL and the value is 2XX. The Redfish resource will be returned
                                  in Payload within RedResponse if server send it back in the HTTP
                                  response message body.
  @retval EFI_INVALID_PARAMETER   RedfishService, Uri, Content, or RedResponse is NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred. Callers can get
                                  more error info from returned HTTP StatusCode, Headers and Payload
                                  within RedResponse:
                                  1. If the returned StatusCode is NULL, indicates any error happen.
                                  2. If the returned StatusCode is not NULL and the value is not 2XX,
                                     indicates any error happen.
**/
EFI_STATUS
EFIAPI
RedfishPutToUri (
  IN     REDFISH_SERVICE   RedfishService,
  IN     CONST CHAR8       *Uri,
  IN     CONST CHAR8       *Content,
  IN     UINTN             ContentSize OPTIONAL,
  IN     CONST CHAR8       *ContentType OPTIONAL,
  OUT    REDFISH_RESPONSE  *RedResponse
  );

/**
  Dump text in fractions.

  @param[in]  String   ASCII string to dump.

**/
VOID
RedfishDumpJsonStringFractions (
  IN CHAR8  *String
  );

/**
  Extract the JSON text content from REDFISH_PAYLOAD and dump to debug console.

  @param[in]  Payload       The Redfish payload to dump.

**/
VOID
RedfishDumpPayload (
  IN REDFISH_PAYLOAD  Payload
  );

/**
  Dump text in JSON value.

  @param[in]  JsonValue       The Redfish JSON value to dump.

**/
VOID
RedfishDumpJson (
  IN EDKII_JSON_VALUE  JsonValue
  );

/**
  This function will cleanup the HTTP header and Redfish payload resources.

  @param[in]  StatusCode        The status code in HTTP response message.
  @param[in]  HeaderCount       Number of HTTP header structures in Headers list.
  @param[in]  Headers           Array containing list of HTTP headers.
  @param[in]  Payload           The Redfish payload to dump.

**/
VOID
RedfishFreeResponse (
  IN EFI_HTTP_STATUS_CODE  *StatusCode,
  IN UINTN                 HeaderCount,
  IN EFI_HTTP_HEADER       *Headers,
  IN REDFISH_PAYLOAD       Payload
  );

/**
  Check if the "@odata.type" in Payload is valid or not.

  @param[in]  Payload                  The Redfish payload to be checked.
  @param[in]  OdataTypeName            OdataType will be retrieved from mapping list.
  @param[in]  OdataTypeMappingList     The list of OdataType.
  @param[in]  OdataTypeMappingListSize The number of mapping list

  @return TRUE if the "@odata.type" in Payload is valid, otherwise FALSE.

**/
BOOLEAN
RedfishIsValidOdataType (
  IN REDFISH_PAYLOAD             Payload,
  IN CONST CHAR8                 *OdataTypeName,
  IN REDFISH_ODATA_TYPE_MAPPING  *OdataTypeMappingList,
  IN UINTN                       OdataTypeMappingListSize
  );

/**
  Check if the payload is collection

  @param[in]  Payload   The Redfish payload to be checked.

  @return TRUE if the payload is  collection.

**/
BOOLEAN
RedfishIsPayloadCollection (
  IN REDFISH_PAYLOAD  Payload
  );

/**
  Get collection size.

  @param[in]  Payload         The Redfish collection payload
  @param[in]  CollectionSize  Size of this collection

  @return EFI_SUCCESS              Collection size is returned in CollectionSize
  @return EFI_INVALID_PARAMETER    The payload is not a collection.
**/
EFI_STATUS
RedfishGetCollectionSize (
  IN REDFISH_PAYLOAD  Payload,
  IN UINTN            *CollectionSize
  );

/**
  Get Redfish payload of collection member

  @param[in]  Payload    The Redfish collection payload
  @param[in]  Index      Index of collection member

  @return NULL           Fail to get collection member.
  @return Non NULL       Payload is returned.
**/
REDFISH_PAYLOAD
RedfishGetPayloadByIndex (
  IN REDFISH_PAYLOAD  Payload,
  IN UINTN            Index
  );

/**
  Check and return Redfish resource of the given Redpath.

  @param[in]  RedfishService  Pointer to REDFISH_SERVICE
  @param[in]  Redpath         Redpath of the resource.
  @param[in]  Response        Optional return the resource.

  @return EFI_STATUS
**/
EFI_STATUS
RedfishCheckIfRedpathExist (
  IN REDFISH_SERVICE   RedfishService,
  IN CHAR8             *Redpath,
  IN REDFISH_RESPONSE  *Response OPTIONAL
  );

/**
  This function returns the string of Redfish service version.

  @param[in]  RedfishService      Redfish service instance.
  @param[out] ServiceVersionStr   Redfish service string.

  @return     EFI_STATUS

**/
EFI_STATUS
RedfishGetServiceVersion (
  IN  REDFISH_SERVICE  RedfishService,
  OUT CHAR8            **ServiceVersionStr
  );

/**
  This function returns the string of Redfish service version.

  @param[in]   ServiceVersionStr The string of Redfish service version.
  @param[in]   Url               The URL to build Redpath with ID.
                                 Start with "/", for example "/Registries"
  @param[in]   Id                ID string
  @param[out]  Redpath           Pointer to retrieved Redpath, caller has to free
                                 the memory allocated for this string.
  @return     EFI_STATUS

**/
EFI_STATUS
RedfishBuildRedpathUseId (
  IN  CHAR8  *ServiceVersionStr,
  IN  CHAR8  *Url,
  IN  CHAR8  *Id,
  OUT CHAR8  **Redpath
  );

#endif
