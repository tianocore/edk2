/** @file
  HII Config Access protocol implementation of TCG configuration module.

Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcgConfigImpl.h"

CHAR16  mTcgStorageName[] = L"TCG_CONFIGURATION";

TCG_CONFIG_PRIVATE_DATA  mTcgConfigPrivateDateTemplate = {
  TCG_CONFIG_PRIVATE_DATA_SIGNATURE,
  {
    TcgExtractConfig,
    TcgRouteConfig,
    TcgCallback
  }
};

HII_VENDOR_DEVICE_PATH  mTcgHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TCG_CONFIG_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Get current state of TPM device.

  @param[in]   TcgProtocol          Point to EFI_TCG_PROTOCOL instance.
  @param[out]  TpmEnable            Flag to indicate TPM is enabled or not.
  @param[out]  TpmActivate          Flag to indicate TPM is activated or not.

  @retval EFI_SUCCESS               State is successfully returned.
  @retval EFI_DEVICE_ERROR          Failed to get TPM response.
  @retval Others                    Other errors as indicated.

**/
EFI_STATUS
GetTpmState (
  IN  EFI_TCG_PROTOCOL  *TcgProtocol,
  OUT BOOLEAN           *TpmEnable   OPTIONAL,
  OUT BOOLEAN           *TpmActivate OPTIONAL
  )
{
  EFI_STATUS           Status;
  TPM_RSP_COMMAND_HDR  *TpmRsp;
  UINT32               TpmSendSize;
  TPM_PERMANENT_FLAGS  *TpmPermanentFlags;
  UINT8                CmdBuf[64];

  ASSERT (TcgProtocol != NULL);

  //
  // Get TPM Permanent flags (TpmEnable, TpmActivate)
  //
  if ((TpmEnable != NULL) || (TpmActivate != NULL)) {
    TpmSendSize           = sizeof (TPM_RQU_COMMAND_HDR) + sizeof (UINT32) * 3;
    *(UINT16 *)&CmdBuf[0] = SwapBytes16 (TPM_TAG_RQU_COMMAND);
    *(UINT32 *)&CmdBuf[2] = SwapBytes32 (TpmSendSize);
    *(UINT32 *)&CmdBuf[6] = SwapBytes32 (TPM_ORD_GetCapability);

    *(UINT32 *)&CmdBuf[10] = SwapBytes32 (TPM_CAP_FLAG);
    *(UINT32 *)&CmdBuf[14] = SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT));
    *(UINT32 *)&CmdBuf[18] = SwapBytes32 (TPM_CAP_FLAG_PERMANENT);

    Status = TcgProtocol->PassThroughToTpm (
                            TcgProtocol,
                            TpmSendSize,
                            CmdBuf,
                            sizeof (CmdBuf),
                            CmdBuf
                            );
    TpmRsp = (TPM_RSP_COMMAND_HDR *)&CmdBuf[0];
    if (EFI_ERROR (Status) || (TpmRsp->tag != SwapBytes16 (TPM_TAG_RSP_COMMAND)) || (TpmRsp->returnCode != 0)) {
      return EFI_DEVICE_ERROR;
    }

    TpmPermanentFlags = (TPM_PERMANENT_FLAGS *)&CmdBuf[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];

    if (TpmEnable != NULL) {
      *TpmEnable = (BOOLEAN) !TpmPermanentFlags->disable;
    }

    if (TpmActivate != NULL) {
      *TpmActivate = (BOOLEAN) !TpmPermanentFlags->deactivated;
    }
  }

  return EFI_SUCCESS;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]   Request           A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[out]  Progress          On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param[out]  Results           A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
TcgExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT EFI_STRING                           *Progress,
  OUT EFI_STRING                           *Results
  )
{
  EFI_STATUS               Status;
  TCG_CONFIG_PRIVATE_DATA  *PrivateData;
  EFI_STRING               ConfigRequestHdr;
  EFI_STRING               ConfigRequest;
  BOOLEAN                  AllocatedRequest;
  UINTN                    Size;
  BOOLEAN                  TpmEnable;
  BOOLEAN                  TpmActivate;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gTcgConfigFormSetGuid, mTcgStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  PrivateData = TCG_CONFIG_PRIVATE_DATA_FROM_THIS (This);

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  PrivateData->Configuration->TpmOperation = PHYSICAL_PRESENCE_NO_ACTION;

  //
  // Get current TPM state.
  //
  if (PrivateData->TcgProtocol != NULL) {
    Status = GetTpmState (PrivateData->TcgProtocol, &TpmEnable, &TpmActivate);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PrivateData->Configuration->TpmEnable   = TpmEnable;
    PrivateData->Configuration->TpmActivate = TpmActivate;
  }

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gTcgConfigFormSetGuid, mTcgStorageName, PrivateData->DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, sizeof (TCG_CONFIGURATION));
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)PrivateData->Configuration,
                                sizeof (TCG_CONFIGURATION),
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[out] Progress           A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
TcgRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  )
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  TCG_CONFIGURATION  TcgConfiguration;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gTcgConfigFormSetGuid, mTcgStorageName)) {
    return EFI_NOT_FOUND;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (TCG_CONFIGURATION);
  Status     = gHiiConfigRouting->ConfigToBlock (
                                    gHiiConfigRouting,
                                    Configuration,
                                    (UINT8 *)&TcgConfiguration,
                                    &BufferSize,
                                    Progress
                                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Save TPM request to variable space.

  @param[in] PpRequest             Physical Presence request command.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

**/
EFI_STATUS
SavePpRequest (
  IN UINT8  PpRequest
  )
{
  EFI_STATUS             Status;
  UINTN                  DataSize;
  EFI_PHYSICAL_PRESENCE  PpData;

  //
  // Save TPM command to variable.
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE);
  Status   = gRT->GetVariable (
                    PHYSICAL_PRESENCE_VARIABLE,
                    &gEfiPhysicalPresenceGuid,
                    NULL,
                    &DataSize,
                    &PpData
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PpData.PPRequest = PpRequest;
  Status           = gRT->SetVariable (
                            PHYSICAL_PRESENCE_VARIABLE,
                            &gEfiPhysicalPresenceGuid,
                            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                            DataSize,
                            &PpData
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
TcgCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                Action,
  IN     EFI_QUESTION_ID                   QuestionId,
  IN     UINT8                             Type,
  IN     EFI_IFR_TYPE_VALUE                *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  )
{
  TCG_CONFIG_PRIVATE_DATA  *PrivateData;
  CHAR16                   State[32];

  if ((This == NULL) || (Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    if (QuestionId == KEY_TPM_ACTION) {
      PrivateData = TCG_CONFIG_PRIVATE_DATA_FROM_THIS (This);
      UnicodeSPrint (
        State,
        sizeof (State),
        L"%s, and %s",
        PrivateData->Configuration->TpmEnable   ? L"Enabled"   : L"Disabled",
        PrivateData->Configuration->TpmActivate ? L"Activated" : L"Deactivated"
        );
      HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TPM_STATE_CONTENT), State, NULL);
    }

    return EFI_SUCCESS;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGED) || (QuestionId != KEY_TPM_ACTION)) {
    return EFI_UNSUPPORTED;
  }

  SavePpRequest (Value->u8);
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;

  return EFI_SUCCESS;
}

/**
  This function publish the TCG configuration Form for TPM device.

  @param[in, out]  PrivateData   Points to TCG configuration private data.

  @retval EFI_SUCCESS            HII Form is installed for this network device.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallTcgConfigForm (
  IN OUT TCG_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  DriverHandle = NULL;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mTcgHiiVendorDevicePath,
                        &gEfiHiiConfigAccessProtocolGuid,
                        ConfigAccess,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = DriverHandle;

  //
  // Publish the HII package list
  //
  HiiHandle = HiiAddPackages (
                &gTcgConfigFormSetGuid,
                DriverHandle,
                TcgConfigDxeStrings,
                TcgConfigBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTcgHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );

    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle = HiiHandle;

  return EFI_SUCCESS;
}

/**
  This function removes TCG configuration Form.

  @param[in, out]  PrivateData   Points to TCG configuration private data.

**/
VOID
UninstallTcgConfigForm (
  IN OUT TCG_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  //
  // Uninstall HII package list
  //
  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }

  //
  // Uninstall HII Config Access Protocol
  //
  if (PrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           PrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTcgHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &PrivateData->ConfigAccess,
           NULL
           );
    PrivateData->DriverHandle = NULL;
  }

  if (PrivateData->Configuration != NULL) {
    FreePool (PrivateData->Configuration);
  }

  FreePool (PrivateData);
}
