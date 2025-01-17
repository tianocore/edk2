/** @file
  RedfishCrentialDxe produces the EdkIIRedfishCredentialProtocol for the consumer
  to get the Redfish credential Info and to restrict Redfish access from UEFI side.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2024 American Megatrends International LLC<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <RedfishCredentialDxe.h>

#define REDFISH_VERSION_DEFAULT_STRING  L"v1"

REDFISH_CREDENTIAL_PRIVATE  *mCredentialPrivate = NULL;

/**
  Callback function executed when the ExitBootServices event group is signaled.

  @param[in]  Event    Event whose notification function is being invoked.
  @param[out] Context  Pointer to the buffer pass in.
**/
VOID
EFIAPI
RedfishCredentialExitBootServicesEventNotify (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  LibCredentialExitBootServicesNotify ((EDKII_REDFISH_CREDENTIAL_PROTOCOL *)Context);
}

/**
  Callback function executed when the EndOfDxe event group is signaled.

  @param[in]  Event    Event whose notification function is being invoked.
  @param[out] Context  Pointer to the buffer pass in.
**/
VOID
EFIAPI
RedfishCredentialEndOfDxeEventNotify (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  LibCredentialEndOfDxeNotify ((EDKII_REDFISH_CREDENTIAL_PROTOCOL *)Context);

  //
  // Close event, so it will not be invoked again.
  //
  gBS->CloseEvent (Event);
}

EFI_STATUS
ReleaseCredentialPrivate (
  );

EFI_STATUS
IterateThroughBootstrapAccounts (
  IN  REDFISH_SERVICE  RedfishService
  );

/**
  Retrieve platform's Redfish authentication information.

  This functions returns the Redfish authentication method together with the user Id and
  password.
  - For AuthMethodNone, the UserId and Password could be used for HTTP header authentication
    as defined by RFC7235.
  - For AuthMethodRedfishSession, the UserId and Password could be used for Redfish
    session login as defined by  Redfish API specification (DSP0266).

  Callers are responsible for and freeing the returned string storage.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[out]  AuthMethod          Type of Redfish authentication method.
  @param[out]  UserId              The pointer to store the returned UserId string.
  @param[out]  Password            The pointer to store the returned Password string.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_ACCESS_DENIED        SecureBoot is disabled after EndOfDxe.
  @retval EFI_INVALID_PARAMETER    This or AuthMethod or UserId or Password is NULL.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources.
  @retval EFI_UNSUPPORTED          Unsupported authentication method is found.

**/
EFI_STATUS
EFIAPI
RedfishCredentialGetAuthInfo (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This,
  OUT EDKII_REDFISH_AUTH_METHOD          *AuthMethod,
  OUT CHAR8                              **UserId,
  OUT CHAR8                              **Password
  )
{
  if ((This == NULL) || (AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return LibCredentialGetAuthInfo (This, AuthMethod, UserId, Password);
}

/**
  Notify the Redfish service provider to stop provide configuration service to this platform.

  This function should be called when the platfrom is about to leave the safe environment.
  It will notify the Redfish service provider to abort all logined session, and prohibit
  further login with original auth info. GetAuthInfo() will return EFI_UNSUPPORTED once this
  function is returned.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[in]   ServiceStopType     Reason of stopping Redfish service.

  @retval EFI_SUCCESS              Service has been stoped successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL or given the worng ServiceStopType.
  @retval EFI_UNSUPPORTED          Not support to stop Redfish service.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishCredentialStopService (
  IN     EDKII_REDFISH_CREDENTIAL_PROTOCOL           *This,
  IN     EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE  ServiceStopType
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return LibStopRedfishService (This, ServiceStopType);
}

/**
  Retrieve platform's Redfish authentication information.

  This functions returns the Redfish authentication method together with the user Id and
  password.
  - For AuthMethodNone, the UserId and Password could be used for HTTP header authentication
    as defined by RFC7235.
  - For AuthMethodRedfishSession, the UserId and Password could be used for Redfish
    session login as defined by  Redfish API specification (DSP0266).

  Callers are responsible for and freeing the returned string storage.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL2_PROTOCOL instance.
  @param[out]  AuthMethod          Type of Redfish authentication method.
  @param[out]  UserId              The pointer to store the returned UserId string.
  @param[out]  Password            The pointer to store the returned Password string.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_ACCESS_DENIED        SecureBoot is disabled after EndOfDxe.
  @retval EFI_INVALID_PARAMETER    This or AuthMethod or UserId or Password is NULL.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources.
  @retval EFI_UNSUPPORTED          Unsupported authentication method is found.

**/
EFI_STATUS
EFIAPI
RedfishCredential2GetAuthInfo (
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL  *This,
  OUT EDKII_REDFISH_AUTH_METHOD           *AuthMethod,
  OUT CHAR8                               **UserId,
  OUT CHAR8                               **Password
  )
{
  EFI_STATUS  Status;

  if ((AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  Status = mCredentialPrivate->RedfishCredentialProtocol.GetAuthInfo (
                                                           &mCredentialPrivate->RedfishCredentialProtocol,
                                                           AuthMethod,
                                                           UserId,
                                                           Password
                                                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to retrieve Redfish credential - %r\n", __func__, Status));
  }

  return Status;
}

/**
  Notifies the Redfish service provider to stop providing configuration service to this platform.
  Deletes the bootstrap account on BMC side, so it will not be used by any other driver.

  This function should be called when the platfrom is about to leave the safe environment.
  It will delete the bootstrap account sending DELETE request to BMC.
  It will notify the Redfish service provider to abort all logined session, and prohibit
  further login with original auth info. GetAuthInfo() will return EFI_UNSUPPORTED once this
  function is returned.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL2_PROTOCOL instance.
  @param[in]   ServiceStopType     Reason of stopping Redfish service.

  @retval EFI_SUCCESS              Service has been stoped successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL or given the worng ServiceStopType.
  @retval EFI_UNSUPPORTED          Not support to stop Redfish service.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishCredential2StopService (
  IN     EDKII_REDFISH_CREDENTIAL2_PROTOCOL          *This,
  IN     EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE  ServiceStopType
  )
{
  EFI_STATUS            Status;
  REDFISH_SERVICE_LIST  *Instance;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if ((ServiceStopType == ServiceStopTypeExitBootService) ||
      (ServiceStopType == ServiceStopTypeNone))
  {
    // Check PCD and skip the action if platform library is responsible for deleting account
    // on exit boot service event
    if (FixedPcdGetBool (PcdRedfishCredentialDeleteAccount)) {
      if (!IsListEmpty (&mCredentialPrivate->RedfishServiceList)) {
        Instance = (REDFISH_SERVICE_LIST *)GetFirstNode (&mCredentialPrivate->RedfishServiceList);
        IterateThroughBootstrapAccounts (Instance->RedfishService);
      }

      ReleaseCredentialPrivate ();
    }
  }

  Status = mCredentialPrivate->RedfishCredentialProtocol.StopService (
                                                           &mCredentialPrivate->RedfishCredentialProtocol,
                                                           ServiceStopType
                                                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to stop service - %r\n", __func__, Status));
  }

  return Status;
}

/**
  Function sends DELETE request to BMC for the account defined by the target URI.

  @param[in]  RedfishService       Pointer to Redfish Service to be used
                                   for sending DELETE request to BMC.
  @param[in]  TargetUri            URI of bootstrap account to send DELETE request to.

**/
EFI_STATUS
EFIAPI
DeleteRedfishBootstrapAccount (
  IN REDFISH_SERVICE  RedfishService,
  IN CHAR16           *TargetUri
  )
{
  EFI_STATUS        Status;
  REDFISH_RESPONSE  RedfishResponse;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if ((RedfishService == NULL) || (mCredentialPrivate->AuthMethod != AuthMethodHttpBasic)) {
    DEBUG ((DEBUG_ERROR, "%a: Redfish service is not available\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Remove bootstrap account at /redfish/v1/AccountService/AccountId
  //
  ZeroMem (&RedfishResponse, sizeof (REDFISH_RESPONSE));
  Status = RedfishHttpDeleteResourceEx (
             RedfishService,
             TargetUri,
             "{}",
             2,
             NULL,
             &RedfishResponse
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: can not remove bootstrap account at BMC: %r", __func__, Status));
    DumpRedfishResponse (__func__, DEBUG_ERROR, &RedfishResponse);
  } else {
    DEBUG (
      (REDFISH_CREDENTIAL_DEBUG, "%a: bootstrap account: %a is removed from: %s\nURI - %s",
       __func__, mCredentialPrivate->AccountName, REDFISH_MANAGER_ACCOUNT_COLLECTION_URI, TargetUri)
      );
  }

  RedfishHttpFreeResponse (&RedfishResponse);

  return Status;
}

/**
  Get the information about specific Account.
  Checks the User Name and if name matches delete that account


  @param[in]  RedfishService       Pointer to Redfish Service to be used
                                   for sending DELETE request to BMC.
  @param[in]  AccountUri           URI of bootstrap account to verify.

**/
BOOLEAN
ProcessRedfishBootstarpAccount (
  IN REDFISH_SERVICE  RedfishService,
  IN EFI_STRING       AccountUri
  )
{
  EDKII_JSON_VALUE  JsonUserName;
  EDKII_JSON_VALUE  JsonValue;
  EFI_STATUS        Status;
  REDFISH_RESPONSE  RedfishResponse;
  REDFISH_REQUEST   RedfishRequest;
  BOOLEAN           Ret;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return FALSE;
  }

  if ((RedfishService == NULL) || IS_EMPTY_STRING (AccountUri) ||
      (mCredentialPrivate->AuthMethod != AuthMethodHttpBasic))
  {
    return FALSE;
  }

  ZeroMem (&RedfishResponse, sizeof (REDFISH_RESPONSE));
  ZeroMem (&RedfishRequest, sizeof (REDFISH_REQUEST));
  Status = RedfishHttpGetResource (RedfishService, AccountUri, &RedfishRequest, &RedfishResponse, FALSE);
  if (EFI_ERROR (Status) || (RedfishResponse.Payload == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: can not get account from BMC: %r", __func__, Status));
    DumpRedfishResponse (__func__, DEBUG_ERROR, &RedfishResponse);
    return FALSE;
  }

  Ret       = FALSE;
  JsonValue = RedfishJsonInPayload (RedfishResponse.Payload);
  if (JsonValueIsObject (JsonValue)) {
    JsonUserName = JsonObjectGetValue (JsonValueGetObject (JsonValue), "UserName");
    if (JsonValueIsString (JsonUserName) && (JsonValueGetAsciiString (JsonUserName) != NULL)) {
      if (AsciiStrCmp (mCredentialPrivate->AccountName, JsonValueGetAsciiString (JsonUserName)) == 0) {
        DeleteRedfishBootstrapAccount (RedfishService, AccountUri);
        Ret = TRUE;
      }
    }
  }

  RedfishHttpFreeResponse (&RedfishResponse);
  RedfishHttpFreeRequest (&RedfishRequest);

  return Ret;
}

/**
  This function returns the string of Redfish service version.

  @param[out]  ServiceVersionStr    Redfish service string.

  @return     EFI_STATUS

**/
EFI_STATUS
RedfishGetServiceVersion (
  OUT CHAR16  **ServiceVersionStr
  )
{
  *ServiceVersionStr = (CHAR16 *)PcdGetPtr (PcdDefaultRedfishVersion);
  if (*ServiceVersionStr == NULL) {
    *ServiceVersionStr = REDFISH_VERSION_DEFAULT_STRING;
  }

  return EFI_SUCCESS;
}

/**
  Iterates through all account in the account collection
  Get the information about specific Account.
  Checks the User Name and if name matches delete that account


  @param[in]  RedfishService       Pointer to Redfish Service to be used
                                   for sending DELETE request to BMC.

**/
EFI_STATUS
IterateThroughBootstrapAccounts (
  IN  REDFISH_SERVICE  RedfishService
  )
{
  EFI_STATUS        Status;
  EDKII_JSON_VALUE  JsonMembers;
  EDKII_JSON_VALUE  JsonValue;
  EDKII_JSON_VALUE  OdataId;
  CHAR16            TargetUri[REDFISH_URI_LENGTH];
  CHAR16            *RedfishVersion;
  REDFISH_RESPONSE  RedfishResponse;
  REDFISH_REQUEST   RedfishRequest;
  UINTN             MembersCount, Index;

  RedfishVersion = NULL;
  Status         = EFI_NOT_FOUND;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if ((RedfishService == NULL) || (mCredentialPrivate->AuthMethod != AuthMethodHttpBasic) ||
      IS_EMPTY_STRING (mCredentialPrivate->AccountName))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Carving the URI
  //

  Status = RedfishGetServiceVersion (&RedfishVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: can not get Redfish version\n", __func__));
    return Status;
  }

  UnicodeSPrint (
    TargetUri,
    (sizeof (CHAR16) * REDFISH_URI_LENGTH),
    L"/redfish/%s/%s",
    RedfishVersion,
    REDFISH_MANAGER_ACCOUNT_COLLECTION_URI
    );

  DEBUG ((REDFISH_CREDENTIAL_DEBUG, "%a: account collection URI: %s\n", __func__, TargetUri));

  ZeroMem (&RedfishResponse, sizeof (REDFISH_RESPONSE));
  ZeroMem (&RedfishRequest, sizeof (REDFISH_REQUEST));
  Status = RedfishHttpGetResource (RedfishService, TargetUri, &RedfishRequest, &RedfishResponse, FALSE);
  if (EFI_ERROR (Status) || (RedfishResponse.Payload == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: can not get accounts from BMC: %r\n", __func__, Status));
    DumpRedfishResponse (__func__, DEBUG_ERROR, &RedfishResponse);
    return Status;
  }

  JsonValue = RedfishJsonInPayload (RedfishResponse.Payload);
  if (!JsonValueIsObject (JsonValue)) {
    Status = EFI_LOAD_ERROR;
    goto ON_EXIT;
  }

  JsonMembers = JsonObjectGetValue (JsonValueGetObject (JsonValue), "Members");
  if (!JsonValueIsArray (JsonMembers)) {
    Status = EFI_LOAD_ERROR;
    goto ON_EXIT;
  }

  Status = EFI_NOT_FOUND;

  MembersCount = JsonArrayCount (JsonValueGetArray (JsonMembers));
  for (Index = 0; Index < MembersCount; Index++) {
    JsonValue = JsonArrayGetValue (JsonValueGetArray (JsonMembers), Index);
    if (!JsonValueIsObject (JsonValue)) {
      Status = EFI_LOAD_ERROR;
      goto ON_EXIT;
    }

    OdataId = JsonObjectGetValue (JsonValueGetObject (JsonValue), "@odata.id");
    if (!JsonValueIsString (OdataId) || (JsonValueGetAsciiString (OdataId) == NULL)) {
      Status = EFI_LOAD_ERROR;
      goto ON_EXIT;
    }

    UnicodeSPrint (
      TargetUri,
      (sizeof (CHAR16) * REDFISH_URI_LENGTH),
      L"%a",
      JsonValueGetAsciiString (OdataId)
      );
    DEBUG ((REDFISH_CREDENTIAL_DEBUG, "%a: account URI:        %s\n", __func__, TargetUri));
    // Verify bootstrap account User Name and delete the account if User Name matches
    if (ProcessRedfishBootstarpAccount (RedfishService, TargetUri)) {
      Status = EFI_SUCCESS;
      break;
    }
  }

ON_EXIT:

  RedfishHttpFreeResponse (&RedfishResponse);
  RedfishHttpFreeRequest (&RedfishRequest);

  return Status;
}

/**
  Retrieve platform's Redfish authentication information.

  This functions returns the Redfish authentication method together with the user Id.
  For AuthMethodNone, UserId will point to NULL which means authentication
  is not required to access the Redfish service.
  Callers are responsible for freeing the returned string storage pointed by UserId.

  @param[out]  AuthMethod          Type of Redfish authentication method.
  @param[out]  UserId              The pointer to store the returned UserId string.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_INVALID_PARAMETER    AuthMethod or UserId or Password is NULL.
  @retval EFI_UNSUPPORTED          Unsupported authentication method is found.
**/
EFI_STATUS
RedfishGetAuthConfig (
  OUT  EDKII_REDFISH_AUTH_METHOD  *AuthMethod,
  OUT  CHAR8                      **UserId
  )
{
  EFI_STATUS  Status;
  CHAR8       *Password;

  Password = NULL;

  if ((AuthMethod == NULL) || (UserId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  Status = mCredentialPrivate->RedfishCredentialProtocol.GetAuthInfo (
                                                           &mCredentialPrivate->RedfishCredentialProtocol,
                                                           AuthMethod,
                                                           UserId,
                                                           &Password
                                                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to retrieve Redfish credential - %r\n", __func__, Status));
    return Status;
  }

  if (Password != NULL) {
    ZeroMem (Password, AsciiStrSize (Password));
    FreePool (Password);
  }

  return Status;
}

/**
  This function clears Redfish service internal list.

  @retval EFI_SUCCESS              Redfish service is deleted from list successfully.
  @retval Others                   Fail to remove the entry

**/
EFI_STATUS
ClearRedfishServiceList (
  VOID
  )
{
  REDFISH_SERVICE_LIST  *Instance;
  REDFISH_SERVICE_LIST  *NextInstance;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if (!IsListEmpty (&mCredentialPrivate->RedfishServiceList)) {
    //
    // Free memory of REDFISH_SERVICE_LIST instance.
    //
    Instance = (REDFISH_SERVICE_LIST *)GetFirstNode (&mCredentialPrivate->RedfishServiceList);
    do {
      NextInstance = NULL;
      if (!IsNodeAtEnd (&mCredentialPrivate->RedfishServiceList, &Instance->NextInstance)) {
        NextInstance = (REDFISH_SERVICE_LIST *)GetNextNode (
                                                 &mCredentialPrivate->RedfishServiceList,
                                                 &Instance->NextInstance
                                                 );
      }

      RemoveEntryList (&Instance->NextInstance);
      FreePool ((VOID *)Instance);
      Instance = NextInstance;
    } while (Instance != NULL);
  }

  return EFI_SUCCESS;
}

/**
  The function adds a new Redfish service to internal list

  @param[in]  RedfishService       Pointer to REDFISH_SERVICE to be added to the list.

  @retval EFI_SUCCESS              Redfish service is added to list successfully.
  @retval EFI_OUT_OF_RESOURCES     Out of resources error.
**/
EFI_STATUS
AddRedfishServiceToList (
  IN REDFISH_SERVICE  RedfishService
  )
{
  BOOLEAN               ServiceFound;
  REDFISH_SERVICE_LIST  *RedfishServiceInstance;

  RedfishServiceInstance = NULL;
  ServiceFound           = FALSE;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if (!IsListEmpty (&mCredentialPrivate->RedfishServiceList)) {
    RedfishServiceInstance = (REDFISH_SERVICE_LIST *)GetFirstNode (&mCredentialPrivate->RedfishServiceList);
    do {
      if (RedfishServiceInstance->RedfishService == RedfishService) {
        ServiceFound = TRUE;
        break;
      }

      if (IsNodeAtEnd (&mCredentialPrivate->RedfishServiceList, &RedfishServiceInstance->NextInstance)) {
        break;
      }

      RedfishServiceInstance = (REDFISH_SERVICE_LIST *)GetNextNode (
                                                         &mCredentialPrivate->RedfishServiceList,
                                                         &RedfishServiceInstance->NextInstance
                                                         );
    } while (TRUE);
  }

  if (!ServiceFound) {
    RedfishServiceInstance = (REDFISH_SERVICE_LIST *)AllocateZeroPool (sizeof (REDFISH_SERVICE_LIST));
    if (RedfishServiceInstance == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    RedfishServiceInstance->RedfishService = RedfishService;
    InsertTailList (&mCredentialPrivate->RedfishServiceList, &RedfishServiceInstance->NextInstance);
  }

  return EFI_SUCCESS;
}

/**
  This function deletes Redfish service from internal list.

  @param[in]  RedfishService       Pointer to REDFISH_SERVICE to be delete from the list.

  @retval EFI_SUCCESS              Redfish service is deleted from list successfully.
  @retval Others                   Fail to remove the entry

**/
EFI_STATUS
DeleteRedfishServiceFromList (
  IN REDFISH_SERVICE  RedfishService
  )
{
  REDFISH_SERVICE_LIST  *RedfishServiceInstance;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  if (!IsListEmpty (&mCredentialPrivate->RedfishServiceList)) {
    RedfishServiceInstance = (REDFISH_SERVICE_LIST *)GetFirstNode (&mCredentialPrivate->RedfishServiceList);
    do {
      if (RedfishServiceInstance->RedfishService == RedfishService) {
        RemoveEntryList (&RedfishServiceInstance->NextInstance);
        FreePool (RedfishServiceInstance);
        return EFI_SUCCESS;
      }

      if (IsNodeAtEnd (&mCredentialPrivate->RedfishServiceList, &RedfishServiceInstance->NextInstance)) {
        break;
      }

      RedfishServiceInstance = (REDFISH_SERVICE_LIST *)GetNextNode (&mCredentialPrivate->RedfishServiceList, &RedfishServiceInstance->NextInstance);
    } while (TRUE);
  }

  return EFI_NOT_FOUND;
}

/**
  Register Redfish service instance so protocol knows that some module uses bootstrap account.

  @param[in]  This              Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[in]  RedfishService    Redfish service instance to register.

  @retval EFI_SUCCESS           This Redfish service instance has been registered successfully.
  @retval Others                Fail to register Redfish Service

**/
EFI_STATUS
EFIAPI
RedfishCredential2RegisterService (
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL  *This,
  IN  REDFISH_SERVICE                     RedfishService
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if (mCredentialPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed with error - %r\n", __func__, EFI_NOT_STARTED));
    return EFI_NOT_STARTED;
  }

  // Check if AuthMethod has been initialized yet
  if (mCredentialPrivate->AuthMethod == AuthMethodMax) {
    Status = RedfishGetAuthConfig (
               &mCredentialPrivate->AuthMethod,
               &mCredentialPrivate->AccountName
               );
  }

  // Bootstrap account should be deleted only if Basic Authentication is used.
  if (!EFI_ERROR (Status) && (mCredentialPrivate->AuthMethod == AuthMethodHttpBasic)) {
    Status = AddRedfishServiceToList (RedfishService);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to register Redfish service - %r\n", __func__, Status));
    }
  }

  return Status;
}

/**
  Unregister Redfish service instance and delete the bootstrap account
  when all registered services unregistered.

  @param[in]  This              Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[in]  RedfishService    Redfish service instance to unregister.

  @retval EFI_SUCCESS           This Redfish service instance has been unregistered successfully.
  @retval Others                Fail to unregister Redfish Service

**/
EFI_STATUS
EFIAPI
RedfishCredential2UnregisterService (
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL  *This,
  IN  REDFISH_SERVICE                     RedfishService
  )
{
  EFI_STATUS  Status;

  // Bootstrap account should be deleted only if Basic Authentication is used.
  if (mCredentialPrivate->AuthMethod != AuthMethodHttpBasic) {
    return EFI_SUCCESS;
  }

  // Delete Redfish Service from the registered list
  Status = DeleteRedfishServiceFromList (RedfishService);
  // Check if registered list is empty
  if (IsListEmpty (&mCredentialPrivate->RedfishServiceList)) {
    // Iterate through all accounts in the account collection and delete the bootstrap account
    Status = IterateThroughBootstrapAccounts (RedfishService);
    if (!EFI_ERROR (Status)) {
      if (mCredentialPrivate->AccountName != NULL) {
        ZeroMem (mCredentialPrivate->AccountName, AsciiStrSize (mCredentialPrivate->AccountName));
        FreePool (mCredentialPrivate->AccountName);
        mCredentialPrivate->AccountName = NULL;
      }

      mCredentialPrivate->AuthMethod = AuthMethodMax;
      Status                         = mCredentialPrivate->RedfishCredentialProtocol.StopService (
                                                                                       &mCredentialPrivate->RedfishCredentialProtocol,
                                                                                       ServiceStopTypeNone
                                                                                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to stop service - %r\n", __func__, Status));
      }
    }
  }

  return Status;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS    This function always complete successfully.

**/
EFI_STATUS
EFIAPI
RedfishCredentialDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mCredentialPrivate = (REDFISH_CREDENTIAL_PRIVATE *)AllocateZeroPool (sizeof (REDFISH_CREDENTIAL_PRIVATE));
  if (mCredentialPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mCredentialPrivate->AuthMethod = AuthMethodMax;
  InitializeListHead (&mCredentialPrivate->RedfishServiceList);

  mCredentialPrivate->RedfishCredentialProtocol.GetAuthInfo = RedfishCredentialGetAuthInfo;
  mCredentialPrivate->RedfishCredentialProtocol.StopService = RedfishCredentialStopService;

  mCredentialPrivate->RedfishCredential2Protocol.Revision                 = REDFISH_CREDENTIAL_PROTOCOL_REVISION;
  mCredentialPrivate->RedfishCredential2Protocol.GetAuthInfo              = RedfishCredential2GetAuthInfo;
  mCredentialPrivate->RedfishCredential2Protocol.StopService              = RedfishCredential2StopService;
  mCredentialPrivate->RedfishCredential2Protocol.RegisterRedfishService   = RedfishCredential2RegisterService;
  mCredentialPrivate->RedfishCredential2Protocol.UnregisterRedfishService = RedfishCredential2UnregisterService;

  //
  // Install the RedfishCredentialProtocol onto Handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCredentialPrivate->Handle,
                  &gEdkIIRedfishCredentialProtocolGuid,
                  &mCredentialPrivate->RedfishCredentialProtocol,
                  &gEdkIIRedfishCredential2ProtocolGuid,
                  &mCredentialPrivate->RedfishCredential2Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // After EndOfDxe, if SecureBoot is disabled, Redfish Credential Protocol should return
  // error code to caller to avoid the 3rd code to bypass Redfish Credential Protocol and
  // retrieve userid/pwd directly. So, here, we create EndOfDxe Event to check SecureBoot
  // status.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RedfishCredentialEndOfDxeEventNotify,
                  (VOID *)&mCredentialPrivate->RedfishCredentialProtocol,
                  &gEfiEndOfDxeEventGroupGuid,
                  &mCredentialPrivate->EndOfDxeEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // After ExitBootServices, Redfish Credential Protocol should stop the service.
  // So, here, we create ExitBootService Event to stop service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RedfishCredentialExitBootServicesEventNotify,
                  (VOID *)&mCredentialPrivate->RedfishCredentialProtocol,
                  &gEfiEventExitBootServicesGuid,
                  &mCredentialPrivate->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (mCredentialPrivate->EndOfDxeEvent);
    mCredentialPrivate->EndOfDxeEvent = NULL;
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  gBS->UninstallMultipleProtocolInterfaces (
         mCredentialPrivate->Handle,
         &gEdkIIRedfishCredentialProtocolGuid,
         &mCredentialPrivate->RedfishCredentialProtocol,
         &gEdkIIRedfishCredential2ProtocolGuid,
         &mCredentialPrivate->RedfishCredential2Protocol,
         NULL
         );

  FreePool (mCredentialPrivate);

  return Status;
}

/**
  Releases all resources allocated by the module.
  Uninstall all the protocols installed in the driver entry point.

  @retval    EFI_SUCCESS           The resources are released.
  @retval    Others                Failed to release the resources.

**/
EFI_STATUS
ReleaseCredentialPrivate (
  )
{
  if (mCredentialPrivate != NULL) {
    if (mCredentialPrivate->AccountName != NULL) {
      ZeroMem (mCredentialPrivate->AccountName, AsciiStrSize (mCredentialPrivate->AccountName));
      FreePool (mCredentialPrivate->AccountName);
      mCredentialPrivate->AccountName = NULL;
    }

    ClearRedfishServiceList ();
  }

  return EFI_SUCCESS;
}

/**
  This is the unload handle for Redfish Credentials module.

  Uninstall all the protocols installed in the driver entry point.
  Clear all allocated resources.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
RedfishCredentialDxeDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mCredentialPrivate != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mCredentialPrivate->Handle,
           &gEdkIIRedfishCredentialProtocolGuid,
           &mCredentialPrivate->RedfishCredentialProtocol,
           &gEdkIIRedfishCredential2ProtocolGuid,
           &mCredentialPrivate->RedfishCredential2Protocol,
           NULL
           );

    if (mCredentialPrivate->EndOfDxeEvent != NULL) {
      gBS->CloseEvent (mCredentialPrivate->EndOfDxeEvent);
      mCredentialPrivate->EndOfDxeEvent = NULL;
    }

    if (mCredentialPrivate->ExitBootServiceEvent != NULL) {
      gBS->CloseEvent (mCredentialPrivate->ExitBootServiceEvent);
      mCredentialPrivate->ExitBootServiceEvent = NULL;
    }

    ReleaseCredentialPrivate ();

    FreePool (mCredentialPrivate);
    mCredentialPrivate = NULL;
  }

  return EFI_SUCCESS;
}
