/** @file
  HII-to-Redfish memory driver.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Hii2RedfishMemoryDxe.h"

EFI_GUID        mHii2RedfishMemoryGuid = HII_2_REDFISH_MEMORY_FORMSET_GUID;
EFI_HII_HANDLE  mHiiHandle;
EFI_HANDLE      DriverHandle;
CHAR16          Hii2RedfishEfiVar[] = L"Hii2RedfishMemoryEfiVar";

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
    HII_2_REDFISH_MEMORY_FORMSET_GUID
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
  EFI_STATUS                              Status;
  UINTN                                   BufferSize;
  HII_2_REDFISH_MEMORY_EFI_VARSTORE_DATA  Hii2RedfishMemoryVar;
  UINTN                                   Index;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (HII_2_REDFISH_MEMORY_EFI_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      Hii2RedfishEfiVar,
                      &gHii2RedfishMemoryFormsetGuid,
                      NULL,
                      &BufferSize,
                      &Hii2RedfishMemoryVar
                      );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Initialization
  //
  for (Index = 0; Index < MEMORY_MAX_NO; Index++) {
    Hii2RedfishMemoryVar.Memory[Index].BaseModuleType      = STR_MEMORY_RDIMM_PROMPT;
    Hii2RedfishMemoryVar.Memory[Index].BusWidthBits        = 20;
    Hii2RedfishMemoryVar.Memory[Index].ConfigurationLocked = TRUE;
    StrCpyS (Hii2RedfishMemoryVar.Memory[Index].ModuleProductId, ID_STRING_MAX_WITH_TERMINATOR, L"1234");
  }

  Status = gRT->SetVariable (
                  Hii2RedfishEfiVar,
                  &gHii2RedfishMemoryFormsetGuid,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  BufferSize,
                  &Hii2RedfishMemoryVar
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
Hii2RedfishMemoryExtractConfig (
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
  if (HiiIsConfigHdrMatch (Request, &gHii2RedfishMemoryFormsetGuid, L"Hii2RedfishMemoryEfiVar")) {
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
Hii2RedfishMemoryRouteConfig (
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
Hii2RedfishMemoryDriverCallback (
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
  Hii2RedfishMemoryExtractConfig,
  Hii2RedfishMemoryRouteConfig,
  Hii2RedfishMemoryDriverCallback
};

/**
  Main entry for this driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
Hii2RedfishMemoryDxeDriverEntryPoint (
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
                 &mHii2RedfishMemoryGuid,
                 DriverHandle,
                 Hii2RedfishMemoryDxeStrings,
                 Hii2RedfishMemoryVfrBin,
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
Hii2RedfishMemoryDxeDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mHiiHandle != NULL) {
    HiiRemovePackages (mHiiHandle);
  }

  return EFI_SUCCESS;
}
