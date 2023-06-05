/** @file
  HII-to-Redfish Bios example driver.

  (C) Copyright 2022 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Hii2RedfishBiosDxe.h"

EFI_GUID        mHii2RedfishBiosGuid = HII_2_REDFISH_BIOS_FORMSET_GUID;
EFI_HII_HANDLE  mHiiHandle;
EFI_HANDLE      DriverHandle;
CHAR16          Hii2RedfishEfiVar[] = L"Hii2RedfishBiosEfiVar";

///
/// HII specific Vendor Device Path definition.
///
HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    HII_2_REDFISH_BIOS_FORMSET_GUID
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
  Initial HII variable if it does not exist.

  @retval EFI_SUCESS     HII variable is initialized.

**/
EFI_STATUS
InitialHiiVairable (
  VOID
  )
{
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  HII_2_REDFISH_BIOS_EFI_VARSTORE_DATA  Hii2RedfishBiosVar;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (HII_2_REDFISH_BIOS_EFI_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      Hii2RedfishEfiVar,
                      &gHii2RedfishBiosFormsetGuid,
                      NULL,
                      &BufferSize,
                      &Hii2RedfishBiosVar
                      );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Initialization
  //
  Hii2RedfishBiosVar.RefishBiosOption1Data = STR_BIOS_OPTION_1_ITEM_1;
  StrCpyS (Hii2RedfishBiosVar.RefishBiosOption2Data, ID_STRING_MAX_WITH_TERMINATOR, L"Default");
  Hii2RedfishBiosVar.RefishBiosOption3Data = 5;
  Hii2RedfishBiosVar.RefishBiosOption4Data = TRUE;

  Status = gRT->SetVariable (
                  Hii2RedfishEfiVar,
                  &gHii2RedfishBiosFormsetGuid,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  BufferSize,
                  &Hii2RedfishBiosVar
                  );

  return Status;
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request            A null-terminated Unicode string in
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
Hii2RedfishBiosExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Request == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether request for EFI Varstore. EFI varstore get data
  // through hii database, not support in this path.
  //
  if (HiiIsConfigHdrMatch (Request, &gHii2RedfishBiosFormsetGuid, L"Hii2RedfishBiosEfiVar")) {
    return EFI_UNSUPPORTED;
  }

  return EFI_NOT_FOUND;
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
Hii2RedfishBiosRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  DEBUG ((DEBUG_INFO, "%a, unsupported\n", __FUNCTION__));

  return EFI_UNSUPPORTED;
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
Hii2RedfishBiosDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  DEBUG ((DEBUG_INFO, "%a, action: 0x%x QID: 0x%x\n", __FUNCTION__, Action, QuestionId));

  return EFI_UNSUPPORTED;
}

EFI_HII_CONFIG_ACCESS_PROTOCOL  mHii2RedfishConfigAccess = {
  Hii2RedfishBiosExtractConfig,
  Hii2RedfishBiosRouteConfig,
  Hii2RedfishBiosDriverCallback
};

/**
  Main entry for this driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
Hii2RedfishBiosDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DriverHandle = NULL;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        &mHiiVendorDevicePath,
                        &gEfiHiiConfigAccessProtocolGuid,
                        &mHii2RedfishConfigAccess,
                        NULL
                        );

  //
  // Publish our HII data
  //
  mHiiHandle = HiiAddPackages (
                 &mHii2RedfishBiosGuid,
                 DriverHandle,
                 Hii2RedfishBiosDxeStrings,
                 Hii2RedfishBiosVfrBin,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitialHiiVairable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to initial variable: %r\n", __FUNCTION__, Status));
  }

  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
Hii2RedfishBiosDxeDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mHiiHandle != NULL) {
    HiiRemovePackages (mHiiHandle);
  }

  return EFI_SUCCESS;
}
