/** @file
  Implementation of getting bootstrap credential via IPMI.

  Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  - Redfish Host Interface Specification
  (https://www.dmtf.org/sites/default/files/standards/documents/DSP0270_1.3.0.pdf)
**/

#include "RedfishPlatformCredentialIpmiLib.h"

//
// Global flag of controlling credential service
//
BOOLEAN  mRedfishServiceStopped = FALSE;

/**
  Notify the Redfish service provide to stop provide configuration service to this platform.

  This function should be called when the platform is about to leave the safe environment.
  It will notify the Redfish service provider to abort all login session, and prohibit
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
  // Only stop credential service after leaving BIOS
  //
  if (ServiceStopType != ServiceStopTypeExitBootService) {
    return EFI_UNSUPPORTED;
  }

  //
  // Raise flag first
  //
  mRedfishServiceStopped = TRUE;

  //
  // Delete cached variable
  //
  Status = SetBootstrapAccountCredentialsToVariable (NULL, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: fail to remove bootstrap credential: %r\n", __func__, Status));
  }

  DEBUG ((DEBUG_MANAGEABILITY, "%a: bootstrap credential service stopped\n", __func__));

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
  Function to retrieve temporary user credentials for the UEFI redfish client. This function can
  also disable bootstrap credential service in BMC.

  @param[in]     DisableBootstrapControl TRUE - Tell the BMC to disable the bootstrap credential
                                                service to ensure no one else gains credentials
                                         FALSE  Allow the bootstrap credential service to continue
  @param[in,out] BootstrapUsername       A pointer to a Ascii encoded string for the credential username
                                         When DisableBootstrapControl is TRUE, this pointer can be NULL
  @param[in]     BootstrapUsernameSize   The size of BootstrapUsername including NULL terminator in bytes.
                                         Per specification, the size is USERNAME_MAX_SIZE.
  @param[in,out] BootstrapPassword       A pointer to a Ascii encoded string for the credential password
                                         When DisableBootstrapControl is TRUE, this pointer can be NULL
  @param[in]     BootstrapPasswordSize   The size of BootstrapPassword including NULL terminator in bytes.
                                         Per specification, the size is PASSWORD_MAX_SIZE.

  @retval  EFI_SUCCESS                Credentials were successfully fetched and returned. When DisableBootstrapControl
                                      is set to TRUE, the bootstrap credential service is disabled successfully.
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL when DisableBootstrapControl
                                      is set to FALSE. BootstrapUsernameSize or BootstrapPasswordSize is incorrect when
                                      DisableBootstrapControl is set to FALSE.
  @retval  EFI_DEVICE_ERROR           An IPMI failure occurred
**/
EFI_STATUS
GetBootstrapAccountCredentials (
  IN     BOOLEAN DisableBootstrapControl,
  IN OUT CHAR8 *BootstrapUsername, OPTIONAL
  IN     UINTN   BootstrapUsernameSize,
  IN OUT CHAR8   *BootstrapPassword, OPTIONAL
  IN     UINTN   BootstrapPasswordSize
  )
{
  EFI_STATUS                                  Status;
  IPMI_BOOTSTRAP_CREDENTIALS_COMMAND_DATA     CommandData;
  IPMI_BOOTSTRAP_CREDENTIALS_RESULT_RESPONSE  ResponseData;
  UINT32                                      ResponseSize;

  //
  // NULL buffer check
  //
  if (!DisableBootstrapControl && ((BootstrapUsername == NULL) || (BootstrapPassword == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BootstrapUsernameSize != USERNAME_MAX_SIZE) || (BootstrapPasswordSize != PASSWORD_MAX_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: Disable bootstrap control: 0x%x\n", __func__, DisableBootstrapControl));

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
    DEBUG ((DEBUG_ERROR, "%a: IPMI transaction failure. Returning\n", __func__));
    return Status;
  } else {
    if (ResponseData.CompletionCode != IPMI_COMP_CODE_NORMAL) {
      if (ResponseData.CompletionCode == REDFISH_IPMI_COMP_CODE_BOOTSTRAP_CREDENTIAL_DISABLED) {
        DEBUG ((DEBUG_ERROR, "%a: bootstrap credential support was disabled\n", __func__));
        return EFI_ACCESS_DENIED;
      }

      DEBUG ((DEBUG_ERROR, "%a: Completion code = 0x%x. Returning\n", __func__, ResponseData.CompletionCode));
      return EFI_PROTOCOL_ERROR;
    } else if (ResponseData.GroupExtensionId != REDFISH_IPMI_GROUP_EXTENSION) {
      DEBUG ((DEBUG_ERROR, "%a: Group Extension Response = 0x%x. Returning\n", __func__, ResponseData.GroupExtensionId));
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

  DEBUG ((DEBUG_MANAGEABILITY, "%a: get bootstrap credential via IPMI: %r\n", __func__, Status));

  return Status;
}

/**
  Function to retrieve temporary user credentials from cached boot time variable.

  @param[in,out] BootstrapUsername     A pointer to a Ascii encoded string for the credential username.
  @param[in]     BootstrapUsernameSize The size of BootstrapUsername including NULL terminator in bytes.
                                       Per specification, the size is USERNAME_MAX_SIZE.
  @param[in,out] BootstrapPassword     A pointer to a Ascii encoded string for the credential password.
  @param[in]     BootstrapPasswordSize The size of BootstrapPassword including NULL terminator in bytes.
                                       Per specification, the size is PASSWORD_MAX_SIZE.

  @retval  EFI_SUCCESS                Credentials were successfully fetched and returned.
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL.
                                      BootstrapUsernameSize or BootstrapPasswordSize is incorrect.
  @retval  EFI_NOT_FOUND              No variable found for account and credentials.
**/
EFI_STATUS
GetBootstrapAccountCredentialsFromVariable (
  IN OUT CHAR8  *BootstrapUsername,
  IN     UINTN  BootstrapUsernameSize,
  IN OUT CHAR8  *BootstrapPassword,
  IN     UINTN  BootstrapPasswordSize
  )
{
  EFI_STATUS                      Status;
  BOOTSTRAP_CREDENTIALS_VARIABLE  *CredentialVariable;
  VOID                            *Data;
  UINTN                           DataSize;

  if ((BootstrapUsername == NULL) || (BootstrapPassword == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BootstrapUsernameSize != USERNAME_MAX_SIZE) || (BootstrapPasswordSize != PASSWORD_MAX_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  DataSize = 0;
  Status   = GetVariable2 (
               CREDENTIAL_VARIABLE_NAME,
               &gEfiRedfishVariableGuid,
               (VOID *)&Data,
               &DataSize
               );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (DataSize != sizeof (BOOTSTRAP_CREDENTIALS_VARIABLE)) {
    DEBUG ((DEBUG_ERROR, "%a: data corruption. returned size: %d != structure size: %d\n", __func__, DataSize, sizeof (BOOTSTRAP_CREDENTIALS_VARIABLE)));
    FreePool (Data);
    return EFI_NOT_FOUND;
  }

  CredentialVariable = (BOOTSTRAP_CREDENTIALS_VARIABLE *)Data;

  AsciiStrCpyS (BootstrapUsername, USERNAME_MAX_SIZE, CredentialVariable->Username);
  AsciiStrCpyS (BootstrapPassword, PASSWORD_MAX_SIZE, CredentialVariable->Password);

  ZeroMem (CredentialVariable->Username, USERNAME_MAX_SIZE);
  ZeroMem (CredentialVariable->Password, PASSWORD_MAX_SIZE);

  FreePool (Data);

  DEBUG ((DEBUG_MANAGEABILITY, "%a: get bootstrap credential from variable\n", __func__));

  return EFI_SUCCESS;
}

/**
  Function to save temporary user credentials into boot time variable. When DeleteVariable is True,
  this function delete boot time variable.

  @param[in] BootstrapUsername       A pointer to a Ascii encoded string for the credential username.
  @param[in] BootstrapPassword       A pointer to a Ascii encoded string for the credential password.
  @param[in] DeleteVariable          True to remove boot time variable. False otherwise.

  @retval  EFI_SUCCESS                Credentials were successfully saved.
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL
  @retval  Others                     Error occurs
**/
EFI_STATUS
SetBootstrapAccountCredentialsToVariable (
  IN CHAR8 *BootstrapUsername, OPTIONAL
  IN CHAR8  *BootstrapPassword, OPTIONAL
  IN BOOLEAN DeleteVariable
  )
{
  EFI_STATUS                      Status;
  BOOTSTRAP_CREDENTIALS_VARIABLE  CredentialVariable;
  VOID                            *Data;

  if (!DeleteVariable && ((BootstrapUsername == NULL) || (BootstrapUsername[0] == '\0'))) {
    return EFI_INVALID_PARAMETER;
  }

  if (!DeleteVariable && ((BootstrapPassword == NULL) || (BootstrapPassword[0] == '\0'))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete variable
  //
  Status = GetVariable2 (
             CREDENTIAL_VARIABLE_NAME,
             &gEfiRedfishVariableGuid,
             (VOID *)&Data,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    FreePool (Data);
    gRT->SetVariable (
           CREDENTIAL_VARIABLE_NAME,
           &gEfiRedfishVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS,
           0,
           NULL
           );
  }

  //
  // This is request to delete credentials. We are done.
  //
  if (DeleteVariable) {
    return EFI_SUCCESS;
  }

  ZeroMem (CredentialVariable.Username, USERNAME_MAX_SIZE);
  ZeroMem (CredentialVariable.Password, PASSWORD_MAX_SIZE);

  AsciiStrCpyS (CredentialVariable.Username, USERNAME_MAX_SIZE, BootstrapUsername);
  AsciiStrCpyS (CredentialVariable.Password, PASSWORD_MAX_SIZE, BootstrapPassword);

  Status = gRT->SetVariable (
                  CREDENTIAL_VARIABLE_NAME,
                  &gEfiRedfishVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (BOOTSTRAP_CREDENTIALS_VARIABLE),
                  (VOID *)&CredentialVariable
                  );

  ZeroMem (CredentialVariable.Username, USERNAME_MAX_SIZE);
  ZeroMem (CredentialVariable.Password, PASSWORD_MAX_SIZE);

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
  BOOLEAN     DisableCredentialService;

  if ((AuthMethod == NULL) || (UserId == NULL) || (Password == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *UserId                  = NULL;
  *Password                = NULL;
  DisableCredentialService = PcdGetBool (PcdRedfishDisableBootstrapCredentialService);

  if (mRedfishServiceStopped) {
    DEBUG ((DEBUG_ERROR, "%a: credential service is stopped due to security reason\n", __func__));
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

  //
  // Get bootstrap credential from variable first
  //
  Status = GetBootstrapAccountCredentialsFromVariable (*UserId, USERNAME_MAX_SIZE, *Password, PASSWORD_MAX_SIZE);
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Make a IPMI query
  //
  Status = GetBootstrapAccountCredentials (DisableCredentialService, *UserId, USERNAME_MAX_SIZE, *Password, PASSWORD_MAX_SIZE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: fail to get bootstrap credential: %r\n", __func__, Status));
    return Status;
  }

  if (DisableCredentialService) {
    DEBUG ((DEBUG_MANAGEABILITY, "%a: credential bootstrapping control disabled\n", __func__));
  }

  Status = SetBootstrapAccountCredentialsToVariable (*UserId, *Password, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: fail to cache bootstrap credential: %r\n", __func__, Status));
  }

  return EFI_SUCCESS;
}
