/** @file
  HII Config Access protocol implementation of VLAN configuration module.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VlanConfigImpl.h"

CHAR16                           mVlanStorageName[] = L"VlanNvData";
EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting = NULL;

VLAN_CONFIG_PRIVATE_DATA  mVlanConfigPrivateDateTemplate = {
  VLAN_CONFIG_PRIVATE_DATA_SIGNATURE,
  {
    VlanExtractConfig,
    VlanRouteConfig,
    VlanCallback
  }
};

VENDOR_DEVICE_PATH  mHiiVendorDevicePathNode = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    {
      (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
      (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
    }
  },
  VLAN_CONFIG_FORM_SET_GUID
};

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
VlanExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT EFI_STRING                           *Progress,
  OUT EFI_STRING                           *Results
  )
{
  EFI_STATUS                Status;
  UINTN                     BufferSize;
  VLAN_CONFIGURATION        Configuration;
  VLAN_CONFIG_PRIVATE_DATA  *PrivateData;
  EFI_STRING                ConfigRequestHdr;
  EFI_STRING                ConfigRequest;
  BOOLEAN                   AllocatedRequest;
  UINTN                     Size;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gVlanConfigFormSetGuid, mVlanStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  //
  // Retrieve the pointer to the UEFI HII Config Routing Protocol
  //
  if (mHiiConfigRouting == NULL) {
    gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&mHiiConfigRouting);
  }

  ASSERT (mHiiConfigRouting != NULL);

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  PrivateData = VLAN_CONFIG_PRIVATE_DATA_FROM_THIS (This);
  ZeroMem (&Configuration, sizeof (VLAN_CONFIGURATION));
  BufferSize    = sizeof (Configuration);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gVlanConfigFormSetGuid, mVlanStorageName, PrivateData->DriverHandle);
    // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
    if (ConfigRequestHdr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    // MU_CHANGE End - CodeQL Change - unguardednullreturndereference
    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
    if (ConfigRequest == NULL) {
      ASSERT (ConfigRequest != NULL);
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    // MU_CHANGE End - CodeQL Change - unguardednullreturndereference

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = mHiiConfigRouting->BlockToConfig (
                                mHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
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
  @param[out]  Progress          A pointer to a string filled in with the offset of
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
VlanRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gVlanConfigFormSetGuid, mVlanStorageName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);
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
VlanCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                Action,
  IN     EFI_QUESTION_ID                   QuestionId,
  IN     UINT8                             Type,
  IN     EFI_IFR_TYPE_VALUE                *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  )
{
  VLAN_CONFIG_PRIVATE_DATA  *PrivateData;
  VLAN_CONFIGURATION        *Configuration;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  UINTN                     Index;
  EFI_HANDLE                VlanHandle;

  PrivateData = VLAN_CONFIG_PRIVATE_DATA_FROM_THIS (This);

  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    return EFI_SUCCESS;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGED) && (Action != EFI_BROWSER_ACTION_CHANGING)) {
    //
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Get Browser data
  //
  Configuration = AllocateZeroPool (sizeof (VLAN_CONFIGURATION));
  // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
  if (Configuration == NULL) {
    ASSERT (Configuration != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  // MU_CHANGE End - CodeQL Change - unguardednullreturndereference
  HiiGetBrowserData (&gVlanConfigFormSetGuid, mVlanStorageName, sizeof (VLAN_CONFIGURATION), (UINT8 *)Configuration);

  VlanConfig = PrivateData->VlanConfig;

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) {
      case VLAN_ADD_QUESTION_ID:
        //
        // Add a VLAN
        //
        VlanConfig->Set (VlanConfig, Configuration->VlanId, Configuration->Priority);
        VlanUpdateForm (PrivateData);

        //
        // Connect the newly created VLAN device
        //
        VlanHandle = NetLibGetVlanHandle (PrivateData->ControllerHandle, Configuration->VlanId);
        if (VlanHandle == NULL) {
          //
          // There may be no child handle created for VLAN ID 0, connect the parent handle
          //
          VlanHandle = PrivateData->ControllerHandle;
        }

        gBS->ConnectController (VlanHandle, NULL, NULL, TRUE);

        //
        // Clear UI data
        //
        *ActionRequest          = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        Configuration->VlanId   = 0;
        Configuration->Priority = 0;
        break;

      case VLAN_REMOVE_QUESTION_ID:
        //
        // Remove VLAN
        //
        ASSERT (PrivateData->NumberOfVlan <= MAX_VLAN_NUMBER);
        for (Index = 0; Index < PrivateData->NumberOfVlan; Index++) {
          if (Configuration->VlanList[Index] != 0) {
            //
            // Checkbox is selected, need remove this VLAN
            //
            VlanConfig->Remove (VlanConfig, PrivateData->VlanId[Index]);
          }
        }

        VlanUpdateForm (PrivateData);
        if (PrivateData->NumberOfVlan == 0) {
          //
          // No VLAN device now, connect the physical NIC handle.
          // Note: PrivateData->NumberOfVlan has been updated by VlanUpdateForm()
          //
          gBS->ConnectController (PrivateData->ControllerHandle, NULL, NULL, TRUE);
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        ZeroMem (Configuration->VlanList, MAX_VLAN_NUMBER);
        break;

      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
      case VLAN_UPDATE_QUESTION_ID:
        //
        // Update current VLAN list into Form.
        //
        VlanUpdateForm (PrivateData);
        break;

      default:
        break;
    }
  }

  HiiSetBrowserData (&gVlanConfigFormSetGuid, mVlanStorageName, sizeof (VLAN_CONFIGURATION), (UINT8 *)Configuration, NULL);
  FreePool (Configuration);
  return EFI_SUCCESS;
}

/**
  This function update VLAN list in the VLAN configuration Form.

  @param[in, out]  PrivateData   Points to VLAN configuration private data.

**/
VOID
VlanUpdateForm (
  IN OUT VLAN_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  UINT16                    NumberOfVlan;
  UINTN                     Index;
  EFI_VLAN_FIND_DATA        *VlanData;
  VOID                      *StartOpCodeHandle;
  EFI_IFR_GUID_LABEL        *StartLabel;
  VOID                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL        *EndLabel;
  CHAR16                    *String;
  CHAR16                    VlanStr[30];
  CHAR16                    VlanIdStr[6];
  UINTN                     DigitalCount;
  EFI_STRING_ID             StringId;

  //
  // Find current VLAN configuration
  //
  VlanData     = NULL;
  NumberOfVlan = 0;
  VlanConfig   = PrivateData->VlanConfig;
  VlanConfig->Find (VlanConfig, NULL, &NumberOfVlan, &VlanData);

  //
  // Update VLAN configuration in PrivateData
  //
  if (NumberOfVlan > MAX_VLAN_NUMBER) {
    NumberOfVlan = MAX_VLAN_NUMBER;
  }

  PrivateData->NumberOfVlan = NumberOfVlan;

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
  if (StartOpCodeHandle == NULL) {
    ASSERT (StartOpCodeHandle != NULL);
    return;
  }

  // MU_CHANGE End - CodeQL Change - unguardednullreturndereference

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
  if (EndOpCodeHandle == NULL) {
    ASSERT (EndOpCodeHandle != NULL);
    return;
  }

  // MU_CHANGE End - CodeQL Change - unguardednullreturndereference
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_VLAN_LIST;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  ZeroMem (PrivateData->VlanId, MAX_VLAN_NUMBER);
  for (Index = 0; Index < NumberOfVlan; Index++) {
    String = VlanStr;

    StrCpyS (String, (sizeof (VlanStr) /sizeof (CHAR16)), L"  VLAN ID:");
    String += 10;
    //
    // Pad VlanId string up to 4 characters with space
    //
    UnicodeValueToStringS (VlanIdStr, sizeof (VlanIdStr), 0, VlanData[Index].VlanId, 5);
    DigitalCount = StrnLenS (VlanIdStr, ARRAY_SIZE (VlanIdStr));
    SetMem16 (String, (4 - DigitalCount) * sizeof (CHAR16), L' ');
    StrCpyS (String + 4 - DigitalCount, (sizeof (VlanStr) /sizeof (CHAR16)) - 10 - (4 - DigitalCount), VlanIdStr);
    String += 4;

    StrCpyS (String, (sizeof (VlanStr) /sizeof (CHAR16)) - 10 - (4 - DigitalCount) - 4, L", Priority:");
    String += 11;
    UnicodeValueToStringS (
      String,
      sizeof (VlanStr) - ((UINTN)String - (UINTN)VlanStr),
      0,
      VlanData[Index].Priority,
      4
      );
    String += StrnLenS (String, ARRAY_SIZE (VlanStr) - ((UINTN)String - (UINTN)VlanStr) / sizeof (CHAR16));
    *String = 0;

    StringId = HiiSetString (PrivateData->HiiHandle, 0, VlanStr, NULL);
    // MU_CHANGE Start - CodeQL Change - unguardednullreturndereference
    if (StringId == 0) {
      ASSERT (StringId != 0);
      continue;
    }

    // MU_CHANGE End - CodeQL Change - unguardednullreturndereference

    HiiCreateCheckBoxOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID)(VLAN_LIST_VAR_OFFSET + Index),
      VLAN_CONFIGURATION_VARSTORE_ID,
      (UINT16)(VLAN_LIST_VAR_OFFSET + Index),
      StringId,
      STRING_TOKEN (STR_VLAN_VLAN_LIST_HELP),
      0,
      0,
      NULL
      );

    //
    // Save VLAN id to private data
    //
    PrivateData->VlanId[Index] = VlanData[Index].VlanId;
  }

  HiiUpdateForm (
    PrivateData->HiiHandle,     // HII handle
    &gVlanConfigFormSetGuid,    // Formset GUID
    VLAN_CONFIGURATION_FORM_ID, // Form ID
    StartOpCodeHandle,          // Label for where to insert opcodes
    EndOpCodeHandle             // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  if (VlanData != NULL) {
    FreePool (VlanData);
  }
}

/**
  This function publish the VLAN configuration Form for a network device. The
  HII Config Access protocol will be installed on a child handle of the network
  device.

  @param[in, out]  PrivateData   Points to VLAN configuration private data.

  @retval EFI_SUCCESS            HII Form is installed for this network device.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallVlanConfigForm (
  IN OUT VLAN_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  CHAR16                          Str[26 + sizeof (EFI_MAC_ADDRESS) * 2 + 1];
  CHAR16                          *MacString;
  EFI_DEVICE_PATH_PROTOCOL        *ChildDevicePath;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_VLAN_CONFIG_PROTOCOL        *VlanConfig;

  //
  // Create child handle and install HII Config Access Protocol
  //
  ChildDevicePath = AppendDevicePathNode (
                      PrivateData->ParentDevicePath,
                      (CONST EFI_DEVICE_PATH_PROTOCOL *)&mHiiVendorDevicePathNode
                      );
  if (ChildDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->ChildDevicePath = ChildDevicePath;

  DriverHandle = NULL;
  ConfigAccess = &PrivateData->ConfigAccess;
  Status       = gBS->InstallMultipleProtocolInterfaces (
                        &DriverHandle,
                        &gEfiDevicePathProtocolGuid,
                        ChildDevicePath,
                        &gEfiHiiConfigAccessProtocolGuid,
                        ConfigAccess,
                        NULL
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->DriverHandle = DriverHandle;

  //
  // Establish the parent-child relationship between the new created
  // child handle and the ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  PrivateData->ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  (VOID **)&VlanConfig,
                  PrivateData->ImageHandle,
                  PrivateData->DriverHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish the HII package list
  //
  HiiHandle = HiiAddPackages (
                &gVlanConfigFormSetGuid,
                DriverHandle,
                VlanConfigDxeStrings,
                VlanConfigBin,
                NULL
                );
  if (HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->HiiHandle = HiiHandle;

  //
  // Update formset title help string.
  //
  MacString = NULL;
  Status    = NetLibGetMacString (PrivateData->ControllerHandle, PrivateData->ImageHandle, &MacString);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->MacString = MacString;

  StrCpyS (Str, sizeof (Str) / sizeof (CHAR16), L"VLAN Configuration (MAC:");
  StrCatS (Str, sizeof (Str) / sizeof (CHAR16), MacString);
  StrCatS (Str, sizeof (Str) / sizeof (CHAR16), L")");
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_VLAN_FORM_SET_TITLE_HELP),
    Str,
    NULL
    );

  //
  // Update form title help string.
  //
  HiiSetString (
    HiiHandle,
    STRING_TOKEN (STR_VLAN_FORM_HELP),
    Str,
    NULL
    );

  return EFI_SUCCESS;
}

/**
  This function remove the VLAN configuration Form for a network device. The
  child handle for HII Config Access protocol will be destroyed.

  @param[in, out]  PrivateData   Points to VLAN configuration private data.

  @retval EFI_SUCCESS            HII Form has been uninstalled successfully.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
UninstallVlanConfigForm (
  IN OUT VLAN_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  EFI_STATUS                Status;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;

  //
  // End the parent-child relationship.
  //
  Status = gBS->CloseProtocol (
                  PrivateData->ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  PrivateData->ImageHandle,
                  PrivateData->DriverHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Uninstall HII Config Access Protocol
  //
  if (PrivateData->DriverHandle != NULL) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    PrivateData->DriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    PrivateData->ChildDevicePath,
                    &gEfiHiiConfigAccessProtocolGuid,
                    &PrivateData->ConfigAccess,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
             PrivateData->ControllerHandle,
             &gEfiVlanConfigProtocolGuid,
             (VOID **)&VlanConfig,
             PrivateData->ImageHandle,
             PrivateData->DriverHandle,
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
      return Status;
    }

    PrivateData->DriverHandle = NULL;

    if (PrivateData->ChildDevicePath != NULL) {
      FreePool (PrivateData->ChildDevicePath);
      PrivateData->ChildDevicePath = NULL;
    }
  }

  //
  // Free MAC string
  //
  if (PrivateData->MacString != NULL) {
    FreePool (PrivateData->MacString);
    PrivateData->MacString = NULL;
  }

  //
  // Uninstall HII package list
  //
  if (PrivateData->HiiHandle != NULL) {
    HiiRemovePackages (PrivateData->HiiHandle);
    PrivateData->HiiHandle = NULL;
  }

  return EFI_SUCCESS;
}
