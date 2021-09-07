/** @file
  RedfishCrentialDxe produces the EdkIIRedfishCredentialProtocol for the consumer
  to get the Redfish credential Info and to restrict Redfish access from UEFI side.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <RedfishCredentialDxe.h>

EDKII_REDFISH_CREDENTIAL_PROTOCOL mRedfishCredentialProtocol = {
  RedfishCredentialGetAuthInfo,
  RedfishCredentialStopService
};

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
  IN  EDKII_REDFISH_CREDENTIAL_PROTOCOL    *This,
  OUT EDKII_REDFISH_AUTH_METHOD            *AuthMethod,
  OUT CHAR8                                **UserId,
  OUT CHAR8                                **Password
  )
{
  if (This == NULL || AuthMethod == NULL || UserId == NULL || Password == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return LibCredentialGetAuthInfo (This, AuthMethod, UserId,Password);
}

/**
  Notify the Redfish service provide to stop provide configuration service to this platform.

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
  IN     EDKII_REDFISH_CREDENTIAL_PROTOCOL    *This,
  IN     EDKII_REDFISH_CREDENTIAL_STOP_SERVICE_TYPE ServiceStopType
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return LibStopRedfishService (This, ServiceStopType);
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
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  EFI_EVENT   EndOfDxeEvent;
  EFI_EVENT   ExitBootServiceEvent;

  Handle = NULL;

  //
  // Install the RedfishCredentialProtocol onto Handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkIIRedfishCredentialProtocolGuid,
                  &mRedfishCredentialProtocol,
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
                  (VOID *)&mRedfishCredentialProtocol,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
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
                  (VOID *)&mRedfishCredentialProtocol,
                  &gEfiEventExitBootServicesGuid,
                  &ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (EndOfDxeEvent);
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  gBS->UninstallMultipleProtocolInterfaces (
         Handle,
         &gEdkIIRedfishCredentialProtocolGuid,
         &mRedfishCredentialProtocol,
         NULL
         );

  return Status;
}
