/** @file
  This file defines the EDKII_REDFISH_CREDENTIAL2_PROTOCOL interface.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2024 American Megatrends International LLC<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_CREDENTIAL2_H_
#define EDKII_REDFISH_CREDENTIAL2_H_

#include <Protocol/EdkIIRedfishCredential.h>
#include <RedfishServiceData.h>

typedef struct _EDKII_REDFISH_CREDENTIAL2_PROTOCOL EDKII_REDFISH_CREDENTIAL2_PROTOCOL;

#define REDFISH_CREDENTIAL_PROTOCOL_REVISION  0x00010000

#define EDKII_REDFISH_CREDENTIAL2_PROTOCOL_GUID \
    {  \
      0x936b81dc, 0x348c, 0x42e3, { 0x9e, 0x82, 0x2, 0x91, 0x4f, 0xd3, 0x48, 0x86 }  \
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
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL2_PROTOCOL_GET_AUTH_INFO)(
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL    *This,
  OUT EDKII_REDFISH_AUTH_METHOD             *AuthMethod,
  OUT CHAR8                                 **UserId,
  OUT CHAR8                                 **Password
  );

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

  @retval EFI_SUCCESS              Service has been stopped successfully.
  @retval EFI_INVALID_PARAMETER    This is NULL.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL2_PROTOCOL_STOP_SERVICE)(
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL            *This,
  IN  EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE    ServiceStopType
  );

/**
  Register Redfish service instance so protocol knows that some module uses bootstrap account .

  @param[in]  This            Pointer to EDKII_REDFISH_CREDENTIAL2_PROTOCOL instance.
  @param[in]  RedfishService  Redfish service instance to register.

  @retval EFI_SUCCESS         This Redfish service instance has been registered successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL2_PROTOCOL_REGISTER_REDFISH_SERVICE)(
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL    *This,
  IN  REDFISH_SERVICE                       RedfishService
  );

/**
  Unregister Redfish service instance and delete the bootstrap account
  when all registered services unregistered.

  @param[in]  This            Pointer to EDKII_REDFISH_CREDENTIAL2_PROTOCOL instance.
  @param[in]  RedfishService  Redfish service instance to unregister.

  @retval EFI_SUCCESS       This Redfish service instance has been unregistered successfully.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CREDENTIAL2_PROTOCOL_UNREGISTER_REDFISH_SERVICE)(
  IN  EDKII_REDFISH_CREDENTIAL2_PROTOCOL    *This,
  IN  REDFISH_SERVICE                       RedfishService
  );

struct _EDKII_REDFISH_CREDENTIAL2_PROTOCOL {
  UINT64                                                           Revision;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL_GET_AUTH_INFO                 GetAuthInfo;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL_STOP_SERVICE                  StopService;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL_REGISTER_REDFISH_SERVICE      RegisterRedfishService;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL_UNREGISTER_REDFISH_SERVICE    UnregisterRedfishService;
};

extern EFI_GUID  gEdkIIRedfishCredential2ProtocolGuid;

#endif
