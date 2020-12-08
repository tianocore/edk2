/** @file
  This file defines the EDKII_REDFISH_CREDENTIAL_PROTOCOL interface.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_CREDENTIAL_H_
#define EDKII_REDFISH_CREDENTIAL_H_

typedef struct _EDKII_REDFISH_CREDENTIAL_PROTOCOL EDKII_REDFISH_CREDENTIAL_PROTOCOL;

#define EDKII_REDFISH_CREDENTIAL_PROTOCOL_GUID \
    {  \
      0x8804377, 0xaf7a, 0x4496, { 0x8a, 0x7b, 0x17, 0x59, 0x0, 0xe9, 0xab, 0x46 }  \
    }

typedef enum {
  AuthMethodNone,            ///< No authentication is required.
  AuthMethodHttpBasic,       ///< Basic authentication is required.
  AuthMethodRedfishSession,  ///< Session authentication is required.
  AuthMethodMax
} EDKII_REDFISH_AUTH_METHOD;

typedef enum {
  ServiceStopTypeNone = 0,            ///< Stop Redfsih service without reason.
  ServiceStopTypeSecureBootDisabled,  ///< Stop Redfsih service becasue EFI
                                      ///< Secure Boot is disabled.
  ServiceStopTypeExitBootService,     ///< Stop Redfsih service becasue existing
                                      ///< Boot Service.
  ServiceStopTypeMax
} EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE;


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
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL_PROTOCOL_GET_AUTH_INFO) (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL    *This,
  OUT EDKII_REDFISH_AUTH_METHOD            *AuthMethod,
  OUT CHAR8                                **UserId,
  OUT CHAR8                                **Password
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
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL_PROTOCOL_STOP_SERVICE) (
  IN     EDKII_REDFISH_CREDENTIAL_PROTOCOL            *This,
  IN     EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE   ServiceStopType
  );

struct _EDKII_REDFISH_CREDENTIAL_PROTOCOL {
  EDKII_REDFISH_CREDENTIAL_PROTOCOL_GET_AUTH_INFO      GetAuthInfo;
  EDKII_REDFISH_CREDENTIAL_PROTOCOL_STOP_SERVICE       StopService;
};

extern EFI_GUID gEdkIIRedfishCredentialProtocolGuid;

#endif
