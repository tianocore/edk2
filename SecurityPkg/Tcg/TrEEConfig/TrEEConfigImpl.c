/** @file
  HII Config Access protocol implementation of TREE configuration module.
  NOTE: This module is only for reference only, each platform should have its own setup page.

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TrEEConfigImpl.h"
#include <Library/PcdLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Guid/TpmInstance.h>

TPM_INSTANCE_ID  mTpmInstanceId[TPM_DEVICE_MAX + 1] = TPM_INSTANCE_ID_LIST;

TREE_CONFIG_PRIVATE_DATA         mTrEEConfigPrivateDateTemplate = {
  TREE_CONFIG_PRIVATE_DATA_SIGNATURE,
  {
    TrEEExtractConfig,
    TrEERouteConfig,
    TrEECallback
  }
};

HII_VENDOR_DEVICE_PATH          mTrEEHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TREE_CONFIG_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { 
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

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
TrEEExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL        *This,
  IN CONST EFI_STRING                            Request,
       OUT EFI_STRING                            *Progress,
       OUT EFI_STRING                            *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  Save TPM request to variable space.

  @param[in] PpRequest             Physical Presence request command.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    Others                Other errors as indicated.

**/
EFI_STATUS
SaveTrEEPpRequest (
  IN UINT8                         PpRequest
  )
{
  EFI_STATUS                       Status;
  UINTN                            DataSize;
  EFI_TREE_PHYSICAL_PRESENCE       PpData;

  //
  // Save TPM command to variable.
  //
  DataSize = sizeof (EFI_TREE_PHYSICAL_PRESENCE);
  Status = gRT->GetVariable (
                  TREE_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpData
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }                
                  
  PpData.PPRequest = PpRequest;
  Status = gRT->SetVariable (
                  TREE_PHYSICAL_PRESENCE_VARIABLE,
                  &gEfiTrEEPhysicalPresenceGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  &PpData
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return EFI_SUCCESS;
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
TrEERouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN CONST EFI_STRING                          Configuration,
       OUT EFI_STRING                          *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
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
TrEECallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN     EFI_BROWSER_ACTION                    Action,
  IN     EFI_QUESTION_ID                       QuestionId,
  IN     UINT8                                 Type,
  IN     EFI_IFR_TYPE_VALUE                    *Value,
     OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  if ((This == NULL) || (Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if (QuestionId == KEY_TPM_DEVICE) {
      return EFI_SUCCESS;
    }
    if (QuestionId == KEY_TPM2_OPERATION) {
      return SaveTrEEPpRequest (Value->u8);
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  This function publish the TREE configuration Form for TPM device.

  @param[in, out]  PrivateData   Points to TREE configuration private data.

  @retval EFI_SUCCESS            HII Form is installed for this network device.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallTrEEConfigForm (
  IN OUT TREE_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  DriverHandle = NULL;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mTrEEHiiVendorDevicePath,
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
                &gTrEEConfigFormSetGuid,
                DriverHandle,
                TrEEConfigDxeStrings,
                TrEEConfigBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTrEEHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );  

    return EFI_OUT_OF_RESOURCES;
  }
  
  PrivateData->HiiHandle = HiiHandle;

  //
  // Update static data
  //
  switch (PrivateData->TpmDeviceDetected) {
  case TPM_DEVICE_NULL:
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TREE_DEVICE_STATE_CONTENT), L"Not Found", NULL);
    break;
  case TPM_DEVICE_1_2:
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TREE_DEVICE_STATE_CONTENT), L"TPM 1.2", NULL);
    break;
  case TPM_DEVICE_2_0_DTPM:
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TREE_DEVICE_STATE_CONTENT), L"TPM 2.0 (DTPM)", NULL);
    break;
  default:
    HiiSetString (PrivateData->HiiHandle, STRING_TOKEN (STR_TREE_DEVICE_STATE_CONTENT), L"Unknown", NULL);
    break;
  }

  return EFI_SUCCESS;  
}

/**
  This function removes TREE configuration Form.

  @param[in, out]  PrivateData   Points to TREE configuration private data.

**/
VOID
UninstallTrEEConfigForm (
  IN OUT TREE_CONFIG_PRIVATE_DATA    *PrivateData
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
           &mTrEEHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &PrivateData->ConfigAccess,
           NULL
           );
    PrivateData->DriverHandle = NULL;
  }
  
  FreePool (PrivateData);
}
