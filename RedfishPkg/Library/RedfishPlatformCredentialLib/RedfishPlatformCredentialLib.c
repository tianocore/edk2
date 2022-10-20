/** @file
*
*  Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "RedfishPlatformCredentialLib.h"

//
// Global flag of controlling credential service
//
BOOLEAN  mRedfishServiceStopped = FALSE;

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
  )
{
  EFI_STATUS  Status;

  if ((ServiceStopType <= ServiceStopTypeNone) || (ServiceStopType >= ServiceStopTypeMax)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Raise flag first
  //
  mRedfishServiceStopped = TRUE;

  //
  // Notify BMC to disable credential bootstrapping support.
  //
  Status = GetBootstrapAccountCredentials (TRUE, NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: fail to disable bootstrap credential: %r\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Notification of Exit Boot Service.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialExitBootServicesNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
  )
{
  //
  // Stop the credential support when system is about to enter OS.
  //
  LibStopRedfishService (This, ServiceStopTypeExitBootService);
}

/**
  Notification of End of DXe.

  @param[in]  This    Pointer to EDKII_REDFISH_CREDENTIAL_PROTOCOL.
**/
VOID
EFIAPI
LibCredentialEndOfDxeNotify (
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL  *This
  )
{
  //
  // Do nothing now.
  // We can stop credential support when system reach end-of-dxe for security reason.
  //
}

/**
  Function to retrieve temporary use credentials for the UEFI redfish client

  @param[in]  DisableBootstrapControl
                                      TRUE - Tell the BMC to disable the bootstrap credential
                                             service to ensure no one else gains credentials
                                      FALSE  Allow the bootstrap credential service to continue
  @param[out] BootstrapUsername
                                      A pointer to a UTF-8 encoded string for the credential username
                                      When DisableBootstrapControl is TRUE, this pointer can be NULL

  @param[out] BootstrapPassword
                                      A pointer to a UTF-8 encoded string for the credential password
                                      When DisableBootstrapControl is TRUE, this pointer can be NULL

  @retval  EFI_SUCCESS                Credentials were successfully fetched and returned
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL when DisableBootstrapControl
                                      is set to FALSE
  @retval  EFI_DEVICE_ERROR           An IPMI failure occurred
**/
EFI_STATUS
GetBootstrapAccountCredentials (
  IN BOOLEAN DisableBootstrapControl,
  IN OUT CHAR8 *BootstrapUsername, OPTIONAL
  IN OUT CHAR8  *BootstrapPassword    OPTIONAL
  )
{
  EFI_STATUS                                  Status;
  IPMI_BOOTSTRAP_CREDENTIALS_COMMAND_DATA     CommandData;
  IPMI_BOOTSTRAP_CREDENTIALS_RESULT_RESPONSE  ResponseData;
  UINT32                                      ResponseSize;

  if (!PcdGetBool (PcdIpmiFeatureEnable)) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI is not enabled! Unable to fetch Redfish credentials\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  //
  // NULL buffer check
  //
  if (!DisableBootstrapControl && ((BootstrapUsername == NULL) || (BootstrapPassword == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Disable bootstrap control: 0x%x\n", __FUNCTION__, DisableBootstrapControl));

  //
  // IPMI callout to NetFn 2C, command 02
  //    Request data:
  //      Byte 1: REDFISH_IPMI_GROUP_EXTENSION
  //      Byte 2: DisableBootstrapControl
  //
  CommandData.GroupExtensionId        = REDFISH_IPMI_GROUP_EXTENSION;
  CommandData.DisableBootstrapControl = (DisableBootstrapControl ? REDFISH_IPMI_BOOTSTRAP_CREDENTIAL_DISABLE : REDFISH_IPMI_BOOTSTRAP_CREDENTIAL_ENABLE);

  ResponseSize = sizeof (ResponseData);

  //
  //    Response data:
  //      Byte 1    : Completion code
  //      Byte 2    : REDFISH_IPMI_GROUP_EXTENSION
  //      Byte 3-18 : Username
  //      Byte 19-34: Password
  //
  Status = IpmiSubmitCommand (
             IPMI_NETFN_GROUP_EXT,
             REDFISH_IPMI_GET_BOOTSTRAP_CREDENTIALS_CMD,
             (UINT8 *)&CommandData,
             sizeof (CommandData),
             (UINT8 *)&ResponseData,
             &ResponseSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: IPMI transaction failure. Returning\n", __FUNCTION__));
    ASSERT_EFI_ERROR (Status);
    return Status;
  } else {
    if (ResponseData.CompletionCode != IPMI_COMP_CODE_NORMAL) {
      if (ResponseData.CompletionCode == REDFISH_IPMI_COMP_CODE_BOOTSTRAP_CREDENTIAL_DISABLED) {
        DEBUG ((DEBUG_ERROR, "%a: bootstrap credential support was disabled\n", __FUNCTION__));
        return EFI_ACCESS_DENIED;
      }

      DEBUG ((DEBUG_ERROR, "%a: Completion code = 0x%x. Returning\n", __FUNCTION__, ResponseData.CompletionCode));
      return EFI_PROTOCOL_ERROR;
    } else if (ResponseData.GroupExtensionId != REDFISH_IPMI_GROUP_EXTENSION) {
      DEBUG ((DEBUG_ERROR, "%a: Group Extension Response = 0x%x. Returning\n", __FUNCTION__, ResponseData.GroupExtensionId));
      return EFI_DEVICE_ERROR;
    } else {
      if (BootstrapUsername != NULL) {
        CopyMem (BootstrapUsername, ResponseData.Username, USERNAME_MAX_LENGTH);
        //
        // Manually append null-terminator in case 16 characters username returned.
        //
        BootstrapUsername[USERNAME_MAX_LENGTH] = '\0';
      }

      if (BootstrapPassword != NULL) {
        CopyMem (BootstrapPassword, ResponseData.Password, PASSWORD_MAX_LENGTH);
        //
        // Manually append null-terminator in case 16 characters password returned.
        //
        BootstrapPassword[PASSWORD_MAX_LENGTH] = '\0';
      }
    }
  }

  return Status;
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
  )
{
  EFI_STATUS  Status;

  if ((AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *UserId   = NULL;
  *Password = NULL;

  if (mRedfishServiceStopped) {
    DEBUG ((DEBUG_ERROR, "%a: credential service is stopped due to security reason\n", __FUNCTION__));
    return EFI_ACCESS_DENIED;
  }

  *AuthMethod = AuthMethodHttpBasic;

  *UserId = AllocateZeroPool (sizeof (CHAR8) * USERNAME_MAX_SIZE);
  if (*UserId == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Password = AllocateZeroPool (sizeof (CHAR8) * PASSWORD_MAX_SIZE);
  if (*Password == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetBootstrapAccountCredentials (FALSE, *UserId, *Password);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: fail to get bootstrap credential: %r\n", __FUNCTION__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
