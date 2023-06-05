/** @file
  HII-to-Redfish boot driver.

  (C) Copyright 2022 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Hii2RedfishBootDxe.h"

EFI_GUID                                 mHii2RedfishBootGuid = HII_2_REDFISH_BOOT_FORMSET_GUID;
EFI_HII_HANDLE                           mHiiHandle;
EFI_HANDLE                               mDriverHandle;
EFI_EVENT                                mEvent               = NULL;
CHAR16                                   mHii2RedfishEfiVar[] = L"Hii2RedfishBootEfiVar";
HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA  mBootOptionsVarData;

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
    HII_2_REDFISH_BOOT_FORMSET_GUID
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
  This function add 'x-uefi-' configuration language to given string ID.

  @param[in] HiiHandle                HII handle
  @param[in] StringId                 String token ID
  @param[in] ConfigLang               Configure language of question

  @retval EFI_STATUS

**/
EFI_STATUS
UpdateConfigLanguageToQuestion (
  IN  EFI_HII_HANDLE  HiiHandle,
  IN  EFI_STRING_ID   StringId,
  IN  EFI_STRING      ConfigLang
  )
{
  CHAR16  ConfigLanguage[32];

  if ((HiiHandle == NULL) || (StringId == 0) || (ConfigLang == NULL)) {
    return EFI_INVALID_LANGUAGE;
  }

  UnicodeSPrint (ConfigLanguage, sizeof (ConfigLanguage), ConfigLang);

  DEBUG ((DEBUG_INFO, "%a, add config-language for string(%d): %s\n", __FUNCTION__, StringId, ConfigLanguage));

  HiiSetString (
    HiiHandle,
    StringId,
    ConfigLanguage,
    COMPUTER_SYSTEM_SECHEMA_VERSION
    );

  return EFI_SUCCESS;
}

/**
  This function add 'x-uefi-' configuration language to given string ID.

  @param[in] HiiHandle                HII handle
  @param[in] StringId                 String token ID
  @param[in] Index                    The index of boot option
  @param[in] BootOption               Boot option context

  @retval EFI_STATUS

**/
EFI_STATUS
UpdateConfigLanguageToValues (
  IN  EFI_HII_HANDLE                HiiHandle,
  IN  EFI_STRING_ID                 StringId,
  IN  UINTN                         Index,
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  CHAR16  ConfigLanguage[10];

  if ((HiiHandle == NULL) || (StringId == 0) || (BootOption == NULL)) {
    return EFI_INVALID_LANGUAGE;
  }

  UnicodeSPrint (ConfigLanguage, sizeof (ConfigLanguage), L"Boot%04x", BootOption->OptionNumber);

  DEBUG ((DEBUG_INFO, "%a, add config-language for string(%d): %s\n", __FUNCTION__, StringId, ConfigLanguage));

  HiiSetString (
    HiiHandle,
    StringId,
    ConfigLanguage,
    COMPUTER_SYSTEM_SECHEMA_VERSION
    );

  return EFI_SUCCESS;
}

/**
  This function creates boot order with ordered-list op-codes in runtime.

  @retval EFI_STATUS

**/
EFI_STATUS
RefreshBootOrderList (
  VOID
  )
{
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  EFI_STRING_ID                 Token;
  VOID                          *StartOpCodeHandle;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  EFI_IFR_GUID_LABEL            *EndLabel;
  BOOLEAN                       IsLegacyOption;
  VOID                          *OptionsOpCodeHandle;
  UINTN                         OptionIndex;

  //
  // for better user experience
  // 1. User changes HD configuration (e.g.: unplug HDD), here we have a chance to remove the HDD boot option
  // 2. User enables/disables UEFI PXE, here we have a chance to add/remove EFI Network boot option
  //
  EfiBootManagerRefreshAllBootOption ();

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  if (BootOptionCount == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Initial var store
  //
  ZeroMem (&mBootOptionsVarData, sizeof (HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA));

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_BOOT_OPTION;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_BOOT_OPTION_END;

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0, OptionIndex = 0; Index < BootOptionCount; Index++) {
    //
    // Don't display hidden boot options, but retain inactive ones.
    //
    if ((BootOption[Index].Attributes & LOAD_OPTION_HIDDEN) != 0) {
      continue;
    }

    //
    // Group the legacy boot option in the sub title created dynamically
    //
    IsLegacyOption = (BOOLEAN)(
                               (DevicePathType (BootOption[Index].FilePath) == BBS_DEVICE_PATH) &&
                               (DevicePathSubType (BootOption[Index].FilePath) == BBS_BBS_DP)
                               );

    //
    // Don't display legacy boot options
    //
    if (IsLegacyOption) {
      continue;
    }

    mBootOptionsVarData.BootOptionOrder[OptionIndex++] = (UINT32)BootOption[Index].OptionNumber;

    ASSERT (BootOption[Index].Description != NULL);

    Token = HiiSetString (mHiiHandle, 0, BootOption[Index].Description, NULL);

    //
    // Add boot option
    //
    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      Token,
      0,
      EFI_IFR_TYPE_NUM_SIZE_32,
      BootOption[Index].OptionNumber
      );

    //
    // Add x-uefi configure language for boot options.
    //
    UpdateConfigLanguageToValues (mHiiHandle, Token, OptionIndex, &BootOption[Index]);
  }

  //
  // Create ordered list op-code
  //
  HiiCreateOrderedListOpCode (
    StartOpCodeHandle,                        // Container for dynamic created opcodes
    BOOT_ORDER_LIST,                          // Question ID
    BOOT_OPTION_VAR_STORE_ID,                 // VarStore ID
    (UINT16)VAR_OFFSET (BootOptionOrder),     // Offset in Buffer Storage
    STRING_TOKEN (STR_BOOT_ORDER_LIST),       // Question prompt text
    STRING_TOKEN (STR_BOOT_ORDER_LIST_HELP),  // Question help text
    0,                                        // Question flag
    EFI_IFR_UNIQUE_SET,                       // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
    EFI_IFR_TYPE_NUM_SIZE_32,                 // Data type of Question value
    MAX_BOOT_OPTIONS,                         // Maximum container
    OptionsOpCodeHandle,                      // Option Opcode list
    NULL                                      // Default Opcode is NULL
    );

  //
  // Add x-uefi configure language for boot order.
  //
  UpdateConfigLanguageToQuestion (mHiiHandle, STRING_TOKEN (STR_BOOT_ORDER_LIST), COMPUTER_SYSTEM_BOOT_BOOTORDER);

  //
  // Update HII form
  //
  HiiUpdateForm (
    mHiiHandle,
    &mHii2RedfishBootGuid,
    FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  return EFI_SUCCESS;
}

/**
  This function update the "BootOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh BootOptionMenu
  with the new "BootOrder" list.

  @param[in] BootOptionVar    Boot option NV data

  @retval EFI_SUCCESS             The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
UpdateBootorderList (
  IN HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA  *BootOptionVar
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      OrderIndex;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINT16      OptionNumber;

  if (BootOptionVar == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First check whether BootOrder is present in current configuration
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **)&BootOrder, &BootOrderSize);
  if (BootOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // OptionOrder is subset of BootOrder
  //
  for (OrderIndex = 0; (OrderIndex < MAX_BOOT_OPTIONS) && (BootOptionVar->BootOptionOrder[OrderIndex] != 0); OrderIndex++) {
    for (Index = OrderIndex; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if ((BootOrder[Index] == (UINT16)BootOptionVar->BootOptionOrder[OrderIndex]) && (OrderIndex != Index)) {
        OptionNumber = BootOrder[Index];
        CopyMem (&BootOrder[OrderIndex + 1], &BootOrder[OrderIndex], (Index - OrderIndex) * sizeof (UINT16));
        BootOrder[OrderIndex] = OptionNumber;
      }
    }
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  BootOrderSize,
                  BootOrder
                  );
  FreePool (BootOrder);

  return Status;
}

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
  HII_2_REDFISH_BOOT_EFI_VARSTORE_DATA  Hii2RedfishBootVar;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (HII_2_REDFISH_BOOT_EFI_VARSTORE_DATA);
  Status     = gRT->GetVariable (
                      mHii2RedfishEfiVar,
                      &gHii2RedfishBootFormsetGuid,
                      NULL,
                      &BufferSize,
                      &Hii2RedfishBootVar
                      );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // Initialization
  //
  Hii2RedfishBootVar.BootSourceOverrideEnabled = STR_DISABLED;
  Hii2RedfishBootVar.BootSourceOverrideMode    = STR_UEFI;
  Hii2RedfishBootVar.BootSourceOverrideTarget  = STR_TARGET_NONE;
  Hii2RedfishBootVar.Reversed                  = 0x00;

  Status = gRT->SetVariable (
                  mHii2RedfishEfiVar,
                  &gHii2RedfishBootFormsetGuid,
                  VARIABLE_ATTRIBUTE_NV_BS,
                  BufferSize,
                  &Hii2RedfishBootVar
                  );

  //
  // Initial var store
  //
  ZeroMem (&mBootOptionsVarData, sizeof (HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA));

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
Hii2RedfishBootExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  EFI_STATUS  Status;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;

  if (Request == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check whether request for EFI Varstore. EFI varstore get data
  // through hii database, not support in this path.
  //
  if (HiiIsConfigHdrMatch (Request, &gHii2RedfishBootFormsetGuid, L"Hii2RedfishBootEfiVar")) {
    return EFI_UNSUPPORTED;
  }

  //
  // Handle boot order list
  //
  if (HiiIsConfigHdrMatch (Request, &gHii2RedfishBootFormsetGuid, L"Hii2RedfishBootOptionVar")) {
    Status = gHiiConfigRouting->BlockToConfig (
                                  gHiiConfigRouting,
                                  Request,
                                  (UINT8 *)&mBootOptionsVarData,
                                  sizeof (HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA),
                                  Results,
                                  Progress
                                  );

    //
    // Set Progress string to the original request string.
    //
    if (Request == NULL) {
      *Progress = NULL;
    } else if (StrStr (Request, L"OFFSET") == NULL) {
      *Progress = Request + StrLen (Request);
    }

    return EFI_SUCCESS;
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
Hii2RedfishBootRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_STATUS                               Status;
  UINTN                                    BufferSize;
  HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA  BootOptionsVar;

  *Progress = Configuration;

  //
  // Check whether request for EFI Varstore. EFI varstore get data
  // through hii database, not support in this path.
  //
  if (HiiIsConfigHdrMatch (Configuration, &gHii2RedfishBootFormsetGuid, L"Hii2RedfishBootEfiVar")) {
    return EFI_UNSUPPORTED;
  }

  //
  // Handle boot order list
  //
  if (HiiIsConfigHdrMatch (Configuration, &gHii2RedfishBootFormsetGuid, L"Hii2RedfishBootOptionVar")) {
    BufferSize = sizeof (HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA);
    ZeroMem (&BootOptionsVar, sizeof (HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA));
    Status = gHiiConfigRouting->ConfigToBlock (
                                  gHiiConfigRouting,
                                  Configuration,
                                  (UINT8 *)&BootOptionsVar,
                                  &BufferSize,
                                  Progress
                                  );

    if (CompareMem (BootOptionsVar.BootOptionOrder, mBootOptionsVarData.BootOptionOrder, (sizeof (UINT32) * MAX_BOOT_OPTIONS))) {
      Status = UpdateBootorderList (&BootOptionsVar);
      if (!EFI_ERROR (Status)) {
        //
        // Boot order update successed. Copy it to local copy.
        //
        CopyMem (mBootOptionsVarData.BootOptionOrder, BootOptionsVar.BootOptionOrder, (sizeof (UINT32) * MAX_BOOT_OPTIONS));
      }
    }

    return EFI_SUCCESS;
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
Hii2RedfishBootDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  DEBUG ((DEBUG_INFO, "%a, action: 0x%x QID: 0x%x\n", __FUNCTION__, Action, QuestionId));

  if ((QuestionId == QUESTION_ID_BOOT_SOURCE_OVERRIDE_ENABLED) && (Action == EFI_BROWSER_ACTION_FORM_OPEN)) {
    RefreshBootOrderList ();

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

EFI_HII_CONFIG_ACCESS_PROTOCOL  mHii2RedfishConfigAccess = {
  Hii2RedfishBootExtractConfig,
  Hii2RedfishBootRouteConfig,
  Hii2RedfishBootDriverCallback
};

/**
  Callback function executed when the ready-to-provisioning event group is signaled.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[out]  Context  Pointer to the Context buffer

**/
VOID
EFIAPI
Hii2RedfishBootReadyToProvisioning (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  //
  // Refresh boot order and create configure language
  //
  RefreshBootOrderList ();
}

/**
  Main entry for this driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
Hii2RedfishBootDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mDriverHandle = NULL;
  Status        = gBS->InstallMultipleProtocolInterfaces (
                         &mDriverHandle,
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
                 &mHii2RedfishBootGuid,
                 mDriverHandle,
                 Hii2RedfishBootDxeStrings,
                 Hii2RedfishBootVfrBin,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitialHiiVairable ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to initial variable: %r\n", __FUNCTION__, Status));
  }

  //
  // Register read-to-provisioning event
  //
  Status = CreateReadyToProvisioningEvent (
             Hii2RedfishBootReadyToProvisioning,
             NULL,
             &mEvent
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a, failed to register ready-to-provisioning event: %r\n", __FUNCTION__, Status));
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
Hii2RedfishBootDxeDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  if (mHiiHandle != NULL) {
    HiiRemovePackages (mHiiHandle);
  }

  if (mEvent != NULL) {
    gBS->CloseEvent (mEvent);
  }

  return EFI_SUCCESS;
}
