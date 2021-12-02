/** @file
  Internal Functions for RedfishLib.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishMisc.h"

EDKII_REDFISH_CREDENTIAL_PROTOCOL  *mCredentialProtocol = NULL;

/**
  This function returns the string of Redfish service version.

  @param[in]   RedfishService Redfish service instance.
  @param[out]  ServiceVersionStr   Redfish service string.

  @return     EFI_STATUS

**/
EFI_STATUS
RedfishGetServiceVersion (
  IN  REDFISH_SERVICE  RedfishService,
  OUT CHAR8            **ServiceVersionStr
  )
{
  redfishService  *Redfish;
  CHAR8           **KeysArray;
  UINTN           KeysNum;

  if ((RedfishService == NULL) || (ServiceVersionStr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Redfish = (redfishService *)RedfishService;
  if (Redfish->versions == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  KeysArray = JsonObjectGetKeys (Redfish->versions, &KeysNum);
  if ((KeysNum == 0) || (KeysArray  == NULL)) {
    return EFI_NOT_FOUND;
  }

  *ServiceVersionStr = *KeysArray;
  return EFI_SUCCESS;
}

/**
  Creates a REDFISH_SERVICE which can be later used to access the Redfish resources.

  This function will configure REST EX child according to parameters described in
  Redfish network host interface in SMBIOS type 42 record. The service enumerator will also
  handle the authentication flow automatically if HTTP basic auth or Redfish session
  login is configured to use.

  @param[in]  RedfishConfigServiceInfo Redfish service information the EFI Redfish
                                       feature driver communicates with.
  @param[in]  AuthMethod   None, HTTP basic auth, or Redfish session login.
  @param[in]  UserId       User Name used for authentication.
  @param[in]  Password     Password used for authentication.

  @return     New created Redfish service, or NULL if error happens.

**/
REDFISH_SERVICE
RedfishCreateLibredfishService (
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo,
  IN  EDKII_REDFISH_AUTH_METHOD           AuthMethod,
  IN  CHAR8                               *UserId,
  IN  CHAR8                               *Password
  )
{
  UINTN                     Flags;
  enumeratorAuthentication  Auth;
  redfishService            *Redfish;

  Redfish = NULL;

  ZeroMem (&Auth, sizeof (Auth));
  if (AuthMethod == AuthMethodHttpBasic) {
    Auth.authType = REDFISH_AUTH_BASIC;
  } else if (AuthMethod == AuthMethodRedfishSession) {
    Auth.authType = REDFISH_AUTH_SESSION;
  }

  Auth.authCodes.userPass.username = UserId;
  Auth.authCodes.userPass.password = Password;

  Flags = REDFISH_FLAG_SERVICE_NO_VERSION_DOC;

  if (AuthMethod != AuthMethodNone) {
    Redfish = createServiceEnumerator (RedfishConfigServiceInfo, NULL, &Auth, (unsigned int)Flags);
  } else {
    Redfish = createServiceEnumerator (RedfishConfigServiceInfo, NULL, NULL, (unsigned int)Flags);
  }

  //
  // Zero the Password after use.
  //
  if (Password != NULL) {
    ZeroMem (Password, AsciiStrLen (Password));
  }

  return (REDFISH_SERVICE)Redfish;
}

/**
  Retrieve platform's Redfish authentication information.

  This functions returns the Redfish authentication method together with the user
  Id and password.
  For AuthMethodNone, UserId and Password will point to NULL which means authentication
  is not required to access the Redfish service.
  For AuthMethodHttpBasic, the UserId and Password could be used for
  HTTP header authentication as defined by RFC7235. For AuthMethodRedfishSession,
  the UserId and Password could be used for Redfish session login as defined by
  Redfish API specification (DSP0266).

  Callers are responsible for freeing the returned string storage pointed by UserId
  and Password.

  @param[out]  AuthMethod          Type of Redfish authentication method.
  @param[out]  UserId              The pointer to store the returned UserId string.
  @param[out]  Password            The pointer to store the returned Password string.

  @retval EFI_SUCCESS              Get the authentication information successfully.
  @retval EFI_INVALID_PARAMETER    AuthMethod or UserId or Password is NULL.
  @retval EFI_UNSUPPORTED          Unsupported authentication method is found.
**/
EFI_STATUS
RedfishGetAuthInfo (
  OUT  EDKII_REDFISH_AUTH_METHOD  *AuthMethod,
  OUT  CHAR8                      **UserId,
  OUT  CHAR8                      **Password
  )
{
  EFI_STATUS  Status;

  if ((AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Locate Redfish Credential Protocol.
  //
  if (mCredentialProtocol == NULL) {
    Status = gBS->LocateProtocol (&gEdkIIRedfishCredentialProtocolGuid, NULL, (VOID **)&mCredentialProtocol);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  ASSERT (mCredentialProtocol != NULL);

  Status = mCredentialProtocol->GetAuthInfo (mCredentialProtocol, AuthMethod, UserId, Password);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RedfishGetAuthInfo: failed to retrieve Redfish credential - %r\n", Status));
    return Status;
  }

  return Status;
}

/**
  This function returns the string of Redfish service version.

  @param[in]   ServiceVerisonStr The string of Redfish service version.
  @param[in]   Url               The URL to build Redpath with ID.
                                 Start with "/", for example "/Registries"
  @param[in]   Id                ID string
  @param[out]  Redpath           Pointer to retrive Redpath, caller has to free
                                 the memory allocated for this string.
  @return     EFI_STATUS

**/
EFI_STATUS
RedfishBuildRedpathUseId (
  IN  CHAR8  *ServiceVerisonStr,
  IN  CHAR8  *Url,
  IN  CHAR8  *Id,
  OUT CHAR8  **Redpath
  )
{
  UINTN  RedpathSize;

  if ((Redpath == NULL) || (ServiceVerisonStr == NULL) || (Url == NULL) || (Id == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RedpathSize = AsciiStrLen ("/") +
                AsciiStrLen (ServiceVerisonStr) +
                AsciiStrLen (Url) +
                AsciiStrLen ("[Id=]") +
                AsciiStrLen (Id) + 1;
  *Redpath = AllocatePool (RedpathSize);
  if (*Redpath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiSPrint (*Redpath, RedpathSize, "/%a%a[Id=%a]", ServiceVerisonStr, Url, Id);
  return EFI_SUCCESS;
}
