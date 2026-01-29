/** @file
  Provides a set of utility APIs that allow to create/read/update/delete
  (CRUD) Redfish resources and provide basic query.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishMisc.h"

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
  )
{
  REDFISH_SERVICE            RedfishService;
  EDKII_REDFISH_AUTH_METHOD  AuthMethod;
  CHAR8                      *UserId;
  CHAR8                      *Password;
  EFI_STATUS                 Status;

  RedfishService = NULL;
  UserId         = NULL;
  Password       = NULL;

  //
  // Check Input Parameters.
  //
  if (RedfishConfigServiceInfo == NULL) {
    return NULL;
  }

  //
  // Get Authentication Configuration.
  //
  Status = RedfishGetAuthInfo (&AuthMethod, &UserId, &Password);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Create a redfish service node based on Redfish network host interface.
  //
  RedfishService = RedfishCreateLibredfishService (
                     RedfishConfigServiceInfo,
                     AuthMethod,
                     UserId,
                     Password
                     );

ON_EXIT:
  if (UserId != NULL) {
    FreePool (UserId);
  }

  if (Password != NULL) {
    FreePool (Password);
  }

  return RedfishService;
}

/**
  Free the Service and all its related resources.

  @param[in]    RedfishService     The Service to access the Redfish resources.

**/
VOID
EFIAPI
RedfishCleanupService (
  IN REDFISH_SERVICE  RedfishService
  )
{
  if (RedfishService == NULL) {
    return;
  }

  cleanupServiceEnumerator (RedfishService);
}

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
  )
{
  EDKII_JSON_VALUE  CopyValue;

  CopyValue = JsonValueClone (Value);
  return createRedfishPayload (CopyValue, RedfishService);
}

/**
  Free the RedfishPayload and all its related resources.

  @param[in]    Payload        Payload to be freed.

**/
VOID
EFIAPI
RedfishCleanupPayload (
  IN REDFISH_PAYLOAD  Payload
  )
{
  if (Payload == NULL) {
    return;
  }

  cleanupPayload ((redfishPayload *)Payload);
}

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
  )
{
  if (Payload == NULL) {
    return NULL;
  }

  return ((redfishPayload *)Payload)->json;
}

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
  )
{
  if (Payload == NULL) {
    return NULL;
  }

  return ((redfishPayload *)Payload)->service;
}

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
  )
{
  UINTN       BufSize;
  CHAR8       *RetRedPath;
  EFI_GUID    SystemUuid;
  EFI_STATUS  Status;

  if (RedPath == NULL) {
    return NULL;
  }

  //
  // Find system UUID from SMBIOS table.
  //
  if (FromSmbios) {
    Status = NetLibGetSystemGuid (&SystemUuid);
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    // AsciiStrLen ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx") = 36
    BufSize = AsciiStrSize (RedPath) + AsciiStrLen ("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX");
  } else {
    BufSize = AsciiStrSize (RedPath) + AsciiStrLen (IdString);
  }

  RetRedPath = AllocateZeroPool (BufSize);
  if (RetRedPath == NULL) {
    return NULL;
  }

  if (FromSmbios) {
    AsciiSPrint (RetRedPath, BufSize, RedPath, &SystemUuid);
  } else {
    AsciiSPrint (RetRedPath, BufSize, RedPath, IdString);
  }

  return RetRedPath;
}

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
  )
{
  if ((RedfishService == NULL) || (RedPath == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  RedResponse->Payload = (REDFISH_PAYLOAD)getPayloadByPath (RedfishService, RedPath, &(RedResponse->StatusCode));

  //
  // 1. If the returned Payload is NULL, indicates any error happen.
  // 2. If the returned StatusCode is NULL, indicates any error happen.
  //
  if ((RedResponse->Payload == NULL) || (RedResponse->StatusCode == NULL)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // 3. If the returned StatusCode is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Get a redfish response addressed by URI, including HTTP StatusCode, Headers
  and Payload which record any HTTP response messages.

  Callers are responsible for freeing the HTTP StatusCode, Headers and Payload returned in
  redfish response data.

  @param[in]    RedfishService    The Service to access the URI resources.
  @param[in]    Uri               String to address a resource.
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
  )
{
  EDKII_JSON_VALUE  JsonValue;

  if ((RedfishService == NULL) || (Uri == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue            = getUriFromServiceEx (RedfishService, Uri, &RedResponse->Headers, &RedResponse->HeaderCount, &RedResponse->StatusCode);
  RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);

  //
  // 1. If the returned Payload is NULL, indicates any error happen.
  // 2. If the returned StatusCode is NULL, indicates any error happen.
  //
  if ((RedResponse->Payload == NULL) || (RedResponse->StatusCode == NULL)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // 3. If the returned StatusCode is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  if ((Payload == NULL) || (RedPath == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  RedResponse->Payload = (REDFISH_PAYLOAD)getPayloadForPathString (Payload, RedPath, &(RedResponse->StatusCode));

  //
  // 1. If the returned Payload is NULL, indicates any error happen.
  //
  if (RedResponse->Payload == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // 2. If StatusCode is not NULL and the returned value of StatusCode is not 2XX, indicates any
  //    error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((RedResponse->StatusCode != NULL) && \
      ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
       (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT)
      ))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonValue;

  Status    = EFI_SUCCESS;
  JsonValue = NULL;

  if ((RedfishService == NULL) || (Uri == NULL) || (Content == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue = (EDKII_JSON_VALUE)patchUriFromServiceEx (
                                  RedfishService,
                                  Uri,
                                  Content,
                                  &(RedResponse->Headers),
                                  &(RedResponse->HeaderCount),
                                  &(RedResponse->StatusCode)
                                  );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    Status = EFI_DEVICE_ERROR;
  }

ON_EXIT:
  if (JsonValue != NULL) {
    RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);
    if (RedResponse->Payload == NULL) {
      //
      // Ignore the error when create RedfishPayload, just free the JsonValue since it's not what
      // we care about if the returned StatusCode is 2XX.
      //
      JsonValueFree (JsonValue);
    }
  }

  return Status;
}

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
  )
{
  if ((Target == NULL) || (Payload == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  RedResponse->Payload = (REDFISH_PAYLOAD)patchPayload (
                                            Target,
                                            Payload,
                                            &(RedResponse->StatusCode)
                                            );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonValue;

  Status    = EFI_SUCCESS;
  JsonValue = NULL;

  if ((RedfishService == NULL) || (Uri == NULL) || (Content == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue = (EDKII_JSON_VALUE)postUriFromServiceEx (
                                  RedfishService,
                                  Uri,
                                  Content,
                                  ContentSize,
                                  ContentType,
                                  &(RedResponse->Headers),
                                  &(RedResponse->HeaderCount),
                                  &(RedResponse->StatusCode)
                                  );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    Status = EFI_DEVICE_ERROR;
  }

ON_EXIT:
  if (JsonValue != NULL) {
    RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);
    if (RedResponse->Payload == NULL) {
      //
      // Ignore the error when create RedfishPayload, just free the JsonValue since it's not what
      // we care about if the returned StatusCode is 2XX.
      //
      JsonValueFree (JsonValue);
    }
  }

  return Status;
}

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
  )
{
  if ((Target == NULL) || (Payload == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  RedResponse->Payload = (REDFISH_PAYLOAD)postPayload (
                                            Target,
                                            Payload,
                                            &(RedResponse->StatusCode)
                                            );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonValue;

  Status    = EFI_SUCCESS;
  JsonValue = NULL;

  if ((RedfishService == NULL) || (Uri == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue = (EDKII_JSON_VALUE)deleteUriFromService (
                                  RedfishService,
                                  Uri,
                                  &(RedResponse->StatusCode)
                                  );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    Status = EFI_DEVICE_ERROR;
  }

ON_EXIT:
  if (JsonValue != NULL) {
    RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);
    if (RedResponse->Payload == NULL) {
      //
      // Ignore the error when create RedfishPayload, just free the JsonValue since it's not what
      // we care about if the returned StatusCode is 2XX.
      //
      JsonValueFree (JsonValue);
    }
  }

  return Status;
}

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
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonValue;

  Status    = EFI_SUCCESS;
  JsonValue = NULL;

  if ((RedfishService == NULL) || (Content == NULL) || (Uri == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue = (EDKII_JSON_VALUE)deleteUriFromServiceEx (
                                  RedfishService,
                                  Uri,
                                  Content,
                                  &(RedResponse->StatusCode)
                                  );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    Status = EFI_DEVICE_ERROR;
  }

ON_EXIT:
  if (JsonValue != NULL) {
    RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);
    if (RedResponse->Payload == NULL) {
      //
      // Ignore the error when create RedfishPayload, just free the JsonValue since it's not what
      // we care about if the returned StatusCode is 2XX.
      //
      JsonValueFree (JsonValue);
    }
  }

  return Status;
}

/**
  Dump text in fractions.

  @param[in]  String   ASCII string to dump.

**/
VOID
RedfishDumpJsonStringFractions (
  IN CHAR8  *String
  )
{
  CHAR8  *NextFraction;
  UINTN  StringFractionSize;
  UINTN  StrLen;
  UINTN  Count;
  CHAR8  BackupChar;

  StringFractionSize = 200;
  if (String == NULL) {
    return;
  }

  DEBUG ((DEBUG_MANAGEABILITY, "JSON text:\n"));
  NextFraction = String;
  StrLen       = AsciiStrLen (String);
  if (StrLen == 0) {
    return;
  }

  for (Count = 0; Count < (StrLen / StringFractionSize); Count++) {
    BackupChar                           = *(NextFraction + StringFractionSize);
    *(NextFraction + StringFractionSize) = 0;
    DEBUG ((DEBUG_MANAGEABILITY, "%a", NextFraction));
    *(NextFraction + StringFractionSize) = BackupChar;
    NextFraction                        += StringFractionSize;
  }

  if ((StrLen % StringFractionSize) != 0) {
    DEBUG ((DEBUG_MANAGEABILITY, "%a\n\n", NextFraction));
  }
}

/**
  Dump text in JSON value.

  @param[in]  JsonValue       The Redfish JSON value to dump.

**/
VOID
RedfishDumpJson (
  IN EDKII_JSON_VALUE  JsonValue
  )
{
  CHAR8  *String;

  String = JsonDumpString (JsonValue, 0);
  if (String == NULL) {
    return;
  }

  RedfishDumpJsonStringFractions (String);
  FreePool (String);
}

/**
  Extract the JSON text content from REDFISH_PAYLOAD and dump to debug console.

  @param[in]  Payload       The Redfish payload to dump.

**/
VOID
RedfishDumpPayload (
  IN REDFISH_PAYLOAD  Payload
  )
{
  EDKII_JSON_VALUE  JsonValue;
  CHAR8             *String;

  JsonValue = NULL;
  String    = NULL;

  if (Payload == NULL) {
    return;
  }

  JsonValue = RedfishJsonInPayload (Payload);
  if (JsonValue == NULL) {
    return;
  }

  String = JsonDumpString (JsonValue, 0);
  if (String == NULL) {
    return;
  }

  RedfishDumpJsonStringFractions (String);
  FreePool (String);
}

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
  )
{
  if (StatusCode != NULL) {
    FreePool (StatusCode);
    StatusCode = NULL;
  }

  if ((HeaderCount != 0) && (Headers != NULL)) {
    HttpFreeHeaderFields (Headers, HeaderCount);
    Headers = NULL;
  }

  if (Payload != NULL) {
    RedfishCleanupPayload (Payload);
    Payload = NULL;
  }
}

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
  )
{
  UINTN             Index;
  EDKII_JSON_VALUE  OdataType;
  EDKII_JSON_VALUE  JsonValue;

  if ((Payload == NULL) || (OdataTypeName == NULL)) {
    return FALSE;
  }

  JsonValue = RedfishJsonInPayload (Payload);
  if (!JsonValueIsObject (JsonValue)) {
    return FALSE;
  }

  OdataType = JsonObjectGetValue (JsonValueGetObject (JsonValue), "@odata.type");
  if (!JsonValueIsString (OdataType) || (JsonValueGetAsciiString (OdataType) == NULL)) {
    return FALSE;
  }

  for (Index = 0; Index < OdataTypeMappingListSize; Index++) {
    if ((AsciiStrCmp (OdataTypeMappingList[Index].OdataTypeName, OdataTypeName) == 0) &&
        (AsciiStrCmp (OdataTypeMappingList[Index].OdataType, JsonValueGetAsciiString (OdataType)) == 0))
    {
      return TRUE;
    }
  }

  DEBUG ((DEBUG_MANAGEABILITY, "%a: This Odata type is not in the list.\n", __func__));
  return FALSE;
}

/**
  Check if the payload is collection

  @param[in]  Payload                  The Redfish payload to be checked.

  @return TRUE if the payload is  collection.

**/
BOOLEAN
RedfishIsPayloadCollection (
  IN REDFISH_PAYLOAD  Payload
  )
{
  return isPayloadCollection (Payload);
}

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
  )
{
  if ((Payload == NULL) || (CollectionSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!RedfishIsPayloadCollection (Payload)) {
    return EFI_INVALID_PARAMETER;
  }

  *CollectionSize = (UINTN)getCollectionSize (Payload);
  return EFI_SUCCESS;
}

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
  )
{
  REDFISH_RESPONSE  RedfishResponse;
  REDFISH_PAYLOAD   PayloadReturn;

  PayloadReturn = (VOID *)getPayloadByIndex (Payload, Index, &RedfishResponse.StatusCode);
  if ((PayloadReturn == NULL) ||
      ((*(RedfishResponse.StatusCode) < HTTP_STATUS_200_OK) && (*(RedfishResponse.StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT)))
  {
    return NULL;
  }

  return PayloadReturn;
}

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
  )
{
  EFI_STATUS        Status;
  REDFISH_RESPONSE  TempResponse;

  if (Redpath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = RedfishGetByService (RedfishService, Redpath, &TempResponse);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Response == NULL) {
    RedfishFreeResponse (
      TempResponse.StatusCode,
      TempResponse.HeaderCount,
      TempResponse.Headers,
      TempResponse.Payload
      );
  } else {
    CopyMem ((VOID *)Response, (VOID *)&TempResponse, sizeof (REDFISH_RESPONSE));
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonValue;

  Status    = EFI_SUCCESS;
  JsonValue = NULL;

  if ((RedfishService == NULL) || (Uri == NULL) || (Content == NULL) || (RedResponse == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (RedResponse, sizeof (REDFISH_RESPONSE));

  JsonValue = (EDKII_JSON_VALUE)putUriFromServiceEx (
                                  RedfishService,
                                  Uri,
                                  Content,
                                  ContentSize,
                                  ContentType,
                                  &(RedResponse->Headers),
                                  &(RedResponse->HeaderCount),
                                  &(RedResponse->StatusCode)
                                  );

  //
  // 1. If the returned StatusCode is NULL, indicates any error happen.
  //
  if (RedResponse->StatusCode == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // 2. If the returned StatusCode is not NULL and the value is not 2XX, indicates any error happen.
  //    NOTE: If there is any error message returned from server, it will be returned in
  //          Payload within RedResponse.
  //
  if ((*(RedResponse->StatusCode) < HTTP_STATUS_200_OK) || \
      (*(RedResponse->StatusCode) > HTTP_STATUS_206_PARTIAL_CONTENT))
  {
    Status = EFI_DEVICE_ERROR;
  }

ON_EXIT:
  if (JsonValue != NULL) {
    RedResponse->Payload = createRedfishPayload (JsonValue, RedfishService);
    if (RedResponse->Payload == NULL) {
      //
      // Ignore the error when create RedfishPayload, just free the JsonValue since it's not what
      // we care about if the returned StatusCode is 2XX.
      //
      JsonValueFree (JsonValue);
    }
  }

  return Status;
}
