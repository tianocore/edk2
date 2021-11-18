/** @file
  Internal Functions for RedfishLib.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_REDFISH_MISC_LIB_H_
#define DXE_REDFISH_MISC_LIB_H_

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/JsonLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/RedfishLib.h>
#include <Library/UefiLib.h>
#include <Protocol/EdkIIRedfishCredential.h>
#include <redfish.h>

#define ARRAY_SIZE(Array)  (sizeof (Array) / sizeof ((Array)[0]))

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
  IN REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishConfigServiceInfo,
  IN EDKII_REDFISH_AUTH_METHOD           AuthMethod,
  IN CHAR8                               *UserId,
  IN CHAR8                               *Password
  );

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
  );

#endif
