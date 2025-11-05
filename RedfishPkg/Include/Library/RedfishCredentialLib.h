/** @file
  Definitinos of RedfishHostInterfaceDxe driver.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_CREDENTIAL_LIB_H_
#define REDFISH_CREDENTIAL_LIB_H_

#include <Uefi.h>

/**
  Notification of Exit Boot Service.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialExitBootServicesNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
  );

/**
  Notification of End of DXe.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialEndOfDxeNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
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
LibCredentialGetAuthInfo (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This,
  OUT EDKII_REDFISH_AUTH_METHOD          *AuthMethod,
  OUT CHAR8                              **UserId,
  OUT CHAR8                              **Password
  );

/**
  Notify the Redfish service provide to stop provide configuration service to this platform.

  This function should be called when the platfrom is about to leave the safe environment.
  It will notify the Redfish service provider to abort all logined session, and prohibit
  further login with original auth info. GetAuthInfo() will return EFI_UNSUPPORTED once this
  function is returned.

  @param[in]   This                Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL instance.
  @param[in]   ServiceStopType     Reason of stopping Redfish service.

  @retval EFI_SUCCESS              Service has been stoped successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
LibStopRedfishService (
  IN     EDKII_REDFISH_CREDENTIAL_PROTOCOL           *This,
  IN     EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE  ServiceStopType
  );

#endif
