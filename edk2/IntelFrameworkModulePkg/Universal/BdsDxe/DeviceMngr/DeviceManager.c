/** @file
  The platform device manager reference implementation

Copyright (c) 2004 - 2009, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DeviceManager.h"

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DeviceManagerCallback
  },
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DriverHealthCallback
  }
};

EFI_GUID mDeviceManagerGuid = DEVICE_MANAGER_FORMSET_GUID;
EFI_GUID mDriverHealthGuid = DRIVER_HEALTH_FORMSET_GUID;

DEVICE_MANAGER_MENU_ITEM  mDeviceManagerMenuItemTable[] = {
  { STRING_TOKEN (STR_DISK_DEVICE),     EFI_DISK_DEVICE_CLASS },
  { STRING_TOKEN (STR_VIDEO_DEVICE),    EFI_VIDEO_DEVICE_CLASS },
  { STRING_TOKEN (STR_NETWORK_DEVICE),  EFI_NETWORK_DEVICE_CLASS },
  { STRING_TOKEN (STR_INPUT_DEVICE),    EFI_INPUT_DEVICE_CLASS },
  { STRING_TOKEN (STR_ON_BOARD_DEVICE), EFI_ON_BOARD_DEVICE_CLASS },
  { STRING_TOKEN (STR_OTHER_DEVICE),    EFI_OTHER_DEVICE_CLASS }
};

HII_VENDOR_DEVICE_PATH  mDeviceManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {102579A0-3686-466e-ACD8-80C087044F4A}
    //
    { 0x102579a0, 0x3686, 0x466e, { 0xac, 0xd8, 0x80, 0xc0, 0x87, 0x4, 0x4f, 0x4a } }
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

HII_VENDOR_DEVICE_PATH  mDriverHealthHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
          (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {D8F76651-1675-4986-BED4-3824B2F1F4C8}
    //
    { 0xd8f76651, 0x1675, 0x4986, { 0xbe, 0xd4, 0x38, 0x24, 0xb2, 0xf1, 0xf4, 0xc8 } }
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
  This function is invoked if user selected a interactive opcode from Device Manager's
  Formset. The decision by user is saved to gCallbackKey for later processing. If
  user set VBIOS, the new value is saved to EFI variable.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DeviceManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  gCallbackKey = QuestionId;

  //
  // Request to exit SendForm(), so as to switch to selected form
  //
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;

  return EFI_SUCCESS;
}

/**

  This function registers HII packages to HII database.

  @retval  EFI_SUCCESS           HII packages for the Device Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Device Manager failed to be registered.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gDeviceManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDeviceManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gDeviceManagerPrivate.DriverHealthHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mDriverHealthHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gDeviceManagerPrivate.DriverHealthConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gDeviceManagerPrivate.HiiHandle = HiiAddPackages (
                                      &mDeviceManagerGuid,
                                      gDeviceManagerPrivate.DriverHandle,
                                      DeviceManagerVfrBin,
                                      BdsDxeStrings,
                                      NULL
                                      );
  if (gDeviceManagerPrivate.HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }
  
  //
  // Publish Driver Health HII data
  //
  gDeviceManagerPrivate.DriverHealthHiiHandle = HiiAddPackages (
                                                  &mDeviceManagerGuid,
                                                  gDeviceManagerPrivate.DriverHealthHandle,
                                                  DriverHealthVfrBin,
                                                  BdsDxeStrings,
                                                  NULL
                                                  );
  if (gDeviceManagerPrivate.DriverHealthHiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Extract the displayed formset for given HII handle and class guid.

  @param Handle          The HII handle.
  @param SetupClassGuid  The class guid specifies which form set will be displayed.
  @param FormSetTitle    Formset title string.
  @param FormSetHelp     Formset help string.

  @retval  TRUE          The formset for given HII handle will be displayed.
  @return  FALSE         The formset for given HII handle will not be displayed.

**/
BOOLEAN
ExtractDisplayedHiiFormFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  IN      EFI_GUID            *SetupClassGuid,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  EFI_GUID                     *ClassGuid;
  UINT8                        ClassGuidNum;

  ASSERT (Handle != NULL);
  ASSERT (SetupClassGuid != NULL);  
  ASSERT (FormSetTitle != NULL);
  ASSERT (FormSetHelp != NULL);

  *FormSetTitle = 0;
  *FormSetHelp  = 0;
  ClassGuidNum  = 0;
  ClassGuid     = NULL;

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  //
  // Handle is a invalid handle. Check if Handle is corrupted.
  //
  ASSERT (Status != EFI_NOT_FOUND);
  //
  // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
  //
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  
  HiiPackageList = AllocatePool (BufferSize);
  ASSERT (HiiPackageList != NULL);

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet Opcode in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Find FormSet OpCode
          //
          ClassGuidNum = ((EFI_IFR_FORM_SET *) OpCodeData)->Flags;
          ClassGuid = (EFI_GUID *) (VOID *)(OpCodeData + sizeof (EFI_IFR_FORM_SET));
          while (ClassGuidNum-- > 0) {
            if (CompareGuid (SetupClassGuid, ClassGuid)) {
              CopyMem (FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
              CopyMem (FormSetHelp, &((EFI_IFR_FORM_SET *) OpCodeData)->Help, sizeof (EFI_STRING_ID));
              FreePool (HiiPackageList);
              return TRUE;
            }
          }
        }
        
        //
        // Go to next opcode
        //
        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }
    }
    
    //
    // Go to next package
    //
    Offset += PackageHeader.Length;
  }

  FreePool (HiiPackageList);

  return FALSE;
}

/**
  Call the browser and display the device manager to allow user
  to configure the platform.

  This function create the dynamic content for device manager. It includes
  section header for all class of devices, one-of opcode to set VBIOS.
  
  @retval  EFI_SUCCESS             Operation is successful.
  @return  Other values if failed to clean up the dynamic content from HII
           database.

**/
EFI_STATUS
CallDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;
  EFI_STRING_ID               FormSetTitle;
  EFI_STRING_ID               FormSetHelp;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;
  UINTN                       NumHandles;
  EFI_HANDLE                  *DriverHealthHandles;

  HiiHandles    = NULL;
  Status        = EFI_SUCCESS;
  gCallbackKey  = 0;
  NumHandles    = 0;
  DriverHealthHandles = NULL;

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // Create Subtitle OpCodes
  //
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
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_DEVICES_LIST;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_DEVICES_LIST), 0, 0, 1);

  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  HiiHandle = gDeviceManagerPrivate.HiiHandle;

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    if (!ExtractDisplayedHiiFormFromHiiHandle (HiiHandles[Index], &gEfiHiiPlatformSetupFormsetGuid, &FormSetTitle, &FormSetHelp)) {
      continue;
    }

    String = HiiGetString (HiiHandles[Index], FormSetTitle, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STR_MISSING_STRING, NULL);
      ASSERT (String != NULL);
    }
    Token = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], FormSetHelp, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STR_MISSING_STRING, NULL);
      ASSERT (String != NULL);
    }
    TokenHelp = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET),
      Token,
      TokenHelp,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  //
  // Add End Opcode for Subtitle
  //
  HiiCreateEndOpCode (StartOpCodeHandle);

  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiDriverHealthProtocolGuid,
                NULL,
                &NumHandles,
                &DriverHealthHandles
                );
  //
  // If there are no drivers installed driver health protocol
  //
  if (NumHandles == 0) {
    HiiSetString (HiiHandle, STRING_TOKEN (STR_DM_DRIVER_HEALTH_TITLE), GetStringById (STRING_TOKEN (STR_EMPTY_STRING)), NULL);
    HiiSetString (HiiHandle, STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY), GetStringById (STRING_TOKEN (STR_EMPTY_STRING)), NULL);
  } else {
    //
    // Check All Driver health status
    //
    if (!PlaformHealthStatusCheck ()) {
      //
      // At least one driver in the platform are not in healthy status
      //
      HiiSetString (HiiHandle, STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY), GetStringById (STRING_TOKEN (STR_DRIVER_NOT_HEALTH)), NULL);
    } else {
      //
      // For the string of STR_DRIVER_HEALTH_ALL_HEALTHY previously has been updated and we need to update it while re-entry.
      //
      HiiSetString (HiiHandle, STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY), GetStringById (STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY)), NULL);
    }
  }

  HiiUpdateForm (
    HiiHandle,
    &mDeviceManagerGuid,
    DEVICE_MANAGER_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &mDeviceManagerGuid,
                           0,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display
  //
  if ((gCallbackKey != 0) && (gCallbackKey != DEVICE_MANAGER_KEY_DRIVER_HEALTH)) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = gFormBrowser2->SendForm (
                             gFormBrowser2,
                             &HiiHandles[gCallbackKey - DEVICE_KEY_OFFSET],
                             1,
                             NULL,
                             0,
                             NULL,
                             &ActionRequest
                             );

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    //
    // Force return to Device Manager
    //
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
  }

  //
  // Driver Health item chose. 
  //
  if (gCallbackKey == DEVICE_MANAGER_KEY_DRIVER_HEALTH) {
    CallDriverHealth ();
    //
    // Force return to Device Manager
    //
    gCallbackKey = FRONT_PAGE_KEY_DEVICE_MANAGER;
  }

  //
  // Cleanup dynamic created strings in HII database by reinstall the packagelist
  //
  HiiRemovePackages (HiiHandle);

  gDeviceManagerPrivate.HiiHandle = HiiAddPackages (
                                      &mDeviceManagerGuid,
                                      gDeviceManagerPrivate.DriverHandle,
                                      DeviceManagerVfrBin,
                                      BdsDxeStrings,
                                      NULL
                                      );
  if (gDeviceManagerPrivate.HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
  FreePool (HiiHandles);

  return Status;
}

/**
  This function is invoked if user selected a interactive opcode from Driver Health's
  Formset. The decision by user is saved to gCallbackKey for later processing.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DriverHealthCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  gCallbackKey = QuestionId;

  //
  // Request to exit SendForm(), so as to switch to selected form
  //
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;

  return EFI_SUCCESS;
}

/**
  Collect and display the platform's driver health relative information, allow user to do interactive 
  operation while the platform is unhealthy.

  This function display a form which divided into two parts. The one list all modules which has installed 
  driver health protocol. The list usually contain driver name, controller name, and it's health info.
  While the driver name can't be retrieved, will use device path as backup. The other part of the form provide
  a choice to the user to repair all platform.

**/
VOID
CallDriverHealth (
  VOID
  )
{
  EFI_STATUS                  Status; 
  EFI_HII_HANDLE              HiiHandle;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *StartLabelRepair;
  EFI_IFR_GUID_LABEL          *EndLabel;
  EFI_IFR_GUID_LABEL          *EndLabelRepair;
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  VOID                        *StartOpCodeHandleRepair;
  VOID                        *EndOpCodeHandleRepair;
  UINTN                       Index;
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  EFI_STRING                  String;
  EFI_STRING                  TmpString;
  EFI_STRING                  DriverName;
  EFI_STRING                  ControllerName;
  LIST_ENTRY                  DriverHealthList;
  DRIVER_HEALTH_INFO          *DriverHealthInfo;
  LIST_ENTRY                  *Link;
  EFI_DEVICE_PATH_PROTOCOL    *DriverDevicePath;
  UINTN                       Length;

  HiiHandle           = gDeviceManagerPrivate.DriverHealthHiiHandle;
  Index               = 0;
  Length              = 0;
  DriverHealthInfo    = NULL;  
  DriverDevicePath    = NULL;
  InitializeListHead (&DriverHealthList);

  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  StartOpCodeHandleRepair = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandleRepair != NULL);

  EndOpCodeHandleRepair = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandleRepair != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_DRIVER_HEALTH;

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabelRepair = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandleRepair, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabelRepair->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabelRepair->Number       = LABEL_DRIVER_HEALTH_REAPIR_ALL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_DRIVER_HEALTH_END;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabelRepair = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandleRepair, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabelRepair->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabelRepair->Number       = LABEL_DRIVER_HEALTH_REAPIR_ALL_END;

  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_DH_STATUS_LIST), 0, 0, 1);

  Status = GetAllControllersHealthStatus (&DriverHealthList);
  ASSERT (Status != EFI_OUT_OF_RESOURCES);

  Link = GetFirstNode (&DriverHealthList);

  while (!IsNull (&DriverHealthList, Link)) {   
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
    
    //
    // Assume no line strings is longer than 512 bytes.
    //
    String = (EFI_STRING) AllocateZeroPool (0x200);
    ASSERT (String != NULL);

    Status = DriverHealthGetDriverName (DriverHealthInfo->DriverHandle, &DriverName);
    if (EFI_ERROR (Status)) {
      //
      // Can not get the Driver name, so use the Device path
      //
      DriverDevicePath = DevicePathFromHandle (DriverHealthInfo->DriverHandle);
      DriverName       = DevicePathToStr (DriverDevicePath);
    }
    //
    // Add the Driver name & Controller name into FormSetTitle string
    // 
    StrnCat (String, DriverName, StrLen (DriverName));


    Status = DriverHealthGetControllerName (
               DriverHealthInfo->DriverHandle, 
               DriverHealthInfo->ControllerHandle, 
               DriverHealthInfo->ChildHandle, 
               &ControllerName
               );

    if (!EFI_ERROR (Status)) {
      //
      // Can not get the Controller name, just let it empty.
      //
      StrnCat (String, L"    ", StrLen (L"    "));
      StrnCat (String, ControllerName, StrLen (ControllerName));   
    }
   
    //
    // Add the message of the Module itself provided after the string item.
    //
    if ((DriverHealthInfo->MessageList != NULL) && (DriverHealthInfo->MessageList->StringId != 0)) {
       StrnCat (String, L"    ", StrLen (L"    "));
       TmpString = HiiGetString (
                     DriverHealthInfo->MessageList->HiiHandle, 
                     DriverHealthInfo->MessageList->StringId, 
                     NULL
                     );
       //
       // Assert if can not retrieve the message string
       //
       ASSERT (TmpString != NULL);
       StrnCat (String, TmpString, StrLen (TmpString));
       FreePool (TmpString);
    } else {
      //
      // Update the string will be displayed base on the driver's health status
      //
      switch(DriverHealthInfo->HealthStatus) {
      case EfiDriverHealthStatusRepairRequired:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_REPAIR_REQUIRED)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_REPAIR_REQUIRED)), Length);
        break;
      case EfiDriverHealthStatusConfigurationRequired:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_CONFIGURATION_REQUIRED)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_CONFIGURATION_REQUIRED)), Length);
        break;
      case EfiDriverHealthStatusFailed:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_OPERATION_FAILED)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_OPERATION_FAILED)), Length);
        break;
      case EfiDriverHealthStatusReconnectRequired:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_RECONNECT_REQUIRED)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_RECONNECT_REQUIRED)), Length);
        break;
      case EfiDriverHealthStatusRebootRequired:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_REBOOT_REQUIRED)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_REBOOT_REQUIRED)), Length);
        break;
      default:
        Length = StrLen (GetStringById (STRING_TOKEN (STR_DRIVER_HEALTH_HEALTHY)));
        StrnCat (String, GetStringById (STRING_TOKEN (STR_DRIVER_HEALTH_HEALTHY)), Length);
        break;
      }
    }

    Token = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    TokenHelp = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_SINGLE_HELP)), NULL);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (Index + DRIVER_HEALTH_KEY_OFFSET),
      Token,
      TokenHelp,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
    Index++;
    Link = GetNextNode (&DriverHealthList, Link);
  }
   
  //
  // Add End Opcode for Subtitle
  // 
  HiiCreateEndOpCode (StartOpCodeHandle);

  HiiCreateSubTitleOpCode (StartOpCodeHandleRepair, STRING_TOKEN (STR_DRIVER_HEALTH_REPAIR_ALL), 0, 0, 1);
  TokenHelp = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_ALL_HELP)), NULL);  

  if (PlaformHealthStatusCheck ()) {
    //
    // No action need to do for the platform
    //
    Token = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DRIVER_HEALTH_ALL_HEALTHY)), NULL);
    HiiCreateActionOpCode (
      StartOpCodeHandleRepair,
      0,
      Token,
      TokenHelp,
      EFI_IFR_FLAG_READ_ONLY,
      0
      );
  } else {
    //
    // Create ActionOpCode only while the platform need to do health related operation.
    //
    Token = HiiSetString (HiiHandle, 0, GetStringById( STRING_TOKEN (STR_DH_REPAIR_ALL_TITLE)), NULL);
    HiiCreateActionOpCode (
      StartOpCodeHandleRepair,
      (EFI_QUESTION_ID) DRIVER_HEALTH_REPAIR_ALL_KEY,
      Token,
      TokenHelp,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  HiiCreateEndOpCode (StartOpCodeHandleRepair);

  Status = HiiUpdateForm (
             HiiHandle,
             &mDriverHealthGuid,
             DRIVER_HEALTH_FORM_ID,
             StartOpCodeHandle,
             EndOpCodeHandle
             );
  ASSERT (Status != EFI_NOT_FOUND);
  ASSERT (Status != EFI_BUFFER_TOO_SMALL);

  Status = HiiUpdateForm (
            HiiHandle,
            &mDriverHealthGuid,
            DRIVER_HEALTH_FORM_ID,
            StartOpCodeHandleRepair,
            EndOpCodeHandleRepair
    );
  ASSERT (Status != EFI_NOT_FOUND);
  ASSERT (Status != EFI_BUFFER_TOO_SMALL);

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &mDriverHealthGuid,
                           DRIVER_HEALTH_FORM_ID,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display.
  // Process the diver health status states here.
  // 
  if (gCallbackKey >= DRIVER_HEALTH_KEY_OFFSET && gCallbackKey != DRIVER_HEALTH_REPAIR_ALL_KEY) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

    Link = GetFirstNode (&DriverHealthList);
    Index = 0;

    while (!IsNull (&DriverHealthList, Link)) {
      //
      // Got the item relative node in the List
      //
      if (Index == (gCallbackKey - DRIVER_HEALTH_KEY_OFFSET)) { 
        DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
        //
        // Process the driver's healthy status for the specify module
        //
        ProcessSingleControllerHealth (
          DriverHealthInfo->DriverHealth,
          DriverHealthInfo->ControllerHandle,      
          DriverHealthInfo->ChildHandle,
          DriverHealthInfo->HealthStatus,
          &(DriverHealthInfo->MessageList),
          DriverHealthInfo->HiiHandle
       );  
       break;
      }
      Index++;
      Link = GetNextNode (&DriverHealthList, Link);
    }

    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }
    
    //
    // Force return to the form of Driver Health in Device Manager 
    //
    gCallbackKey = DRIVER_HEALTH_RETURN_KEY;
  }

  //
  // Repair the whole platform
  //
  if (gCallbackKey == DRIVER_HEALTH_REPAIR_ALL_KEY) {
    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    
    PlatformRepairAll (&DriverHealthList);

    gCallbackKey = DRIVER_HEALTH_RETURN_KEY;
  }
   
  //
  // Cleanup dynamic created strings in HII database by reinstall the packagelist
  //
  
  HiiRemovePackages (HiiHandle);

  gDeviceManagerPrivate.DriverHealthHiiHandle = HiiAddPackages (
                                                  &mDriverHealthGuid,
                                                  gDeviceManagerPrivate.DriverHealthHandle,
                                                  DriverHealthVfrBin,
                                                  BdsDxeStrings,
                                                  NULL
                                                  );
  if (gDeviceManagerPrivate.DriverHealthHiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }
  //
  // Free driver health info list
  //
  while (!IsListEmpty (&DriverHealthList)) {

    Link = GetFirstNode(&DriverHealthList);
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
    RemoveEntryList (Link);

    if (DriverHealthInfo->MessageList != NULL) {
      FreePool(DriverHealthInfo->MessageList);
      FreePool (DriverHealthInfo);
    }   
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle); 
  HiiFreeOpCodeHandle (StartOpCodeHandleRepair);
  HiiFreeOpCodeHandle (EndOpCodeHandleRepair); 

  if (gCallbackKey == DRIVER_HEALTH_RETURN_KEY) {
    //
    // Force return to Driver Health Form
    //
    gCallbackKey = DEVICE_MANAGER_KEY_DRIVER_HEALTH;
    CallDriverHealth ();
  }
}


/*
  Check the Driver Health status of a single controller and try to process it if not healthy.

  This function called by CheckAllControllersHealthStatus () function in order to process a specify
  contoller's health state.

  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health 
                           status on.  This is an optional parameter that may be NULL. 
  @param HealthStatus       The health status of the controller.
  @param MessageList        An array of warning or error messages associated 
                           with the controller specified by ControllerHandle and 
                           ChildHandle.  This is an optional parameter that may be NULL.
  @param FormHiiHandle      The HII handle for an HII form associated with the 
                           controller specified by ControllerHandle and ChildHandle.

  @retval EFI_INVALID_PARAMETER   HealthStatus or DriverHealth is NULL.
  @retval HealthStatus            The Health status of specify controller.
  @retval EFI_OUT_OF_RESOURCES    The list of Driver Health Protocol handles can not be retrieved.
  @retval EFI_NOT_FOUND           No controller in the platform install Driver Health Protocol.
  @retval EFI_SUCCESS             The Health related operation has been taken successfully.

*/
EFI_STATUS
EFIAPI
GetSingleControllerHealthStatus (
  IN OUT LIST_ENTRY                   *DriverHealthList,
  IN EFI_HANDLE                       DriverHandle,
  IN EFI_HANDLE                       ControllerHandle,  OPTIONAL
  IN EFI_HANDLE                       ChildHandle,       OPTIONAL
  IN EFI_DRIVER_HEALTH_PROTOCOL       *DriverHealth,
  IN EFI_DRIVER_HEALTH_STATUS         *HealthStatus
  )
{
  EFI_STATUS                     Status;
  EFI_DRIVER_HEALTH_HII_MESSAGE  *MessageList;
  EFI_HII_HANDLE                 FormHiiHandle;
  DRIVER_HEALTH_INFO             *DriverHealthInfo;

  if (HealthStatus == NULL) {
    //
    // If HealthStatus is NULL, then return EFI_INVALID_PARAMETER
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume the HealthStatus is healthy
  //
  *HealthStatus = EfiDriverHealthStatusHealthy;

  if (DriverHealth == NULL) {
    //
    // If DriverHealth is NULL, then return EFI_INVALID_PARAMETER
    //
    return EFI_INVALID_PARAMETER;
  }

  if (ControllerHandle == NULL) {
    //
    // If ControllerHandle is NULL, the return the cumulative health status of the driver
    //
    Status = DriverHealth->GetHealthStatus (DriverHealth, NULL, NULL, HealthStatus, NULL, NULL);
    if (*HealthStatus == EfiDriverHealthStatusHealthy) {
      //
      // Add the driver health related information into the list
      //
      DriverHealthInfo = AllocateZeroPool (sizeof (DRIVER_HEALTH_INFO));
      if (DriverHealthInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      DriverHealthInfo->Signature          = DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE;
      DriverHealthInfo->DriverHandle       = DriverHandle;
      DriverHealthInfo->ControllerHandle   = NULL;
      DriverHealthInfo->ChildHandle        = NULL;
      DriverHealthInfo->HiiHandle          = NULL;
      DriverHealthInfo->DriverHealth       = DriverHealth;
      DriverHealthInfo->MessageList        = NULL;
      DriverHealthInfo->HealthStatus       = *HealthStatus;

      InsertTailList (DriverHealthList, &DriverHealthInfo->Link);
    }
    return Status;
  }

  MessageList   = NULL;
  FormHiiHandle = NULL;

  //
  // Collect the health status with the optional HII message list
  //
  Status = DriverHealth->GetHealthStatus (DriverHealth, ControllerHandle, ChildHandle, HealthStatus, &MessageList, &FormHiiHandle);

  if (EFI_ERROR (Status)) {
    //
    // If the health status could not be retrieved, then return immediately
    //
    return Status;
  }

  //
  // Add the driver health related information into the list
  //
  DriverHealthInfo = AllocateZeroPool (sizeof (DRIVER_HEALTH_INFO));
  if (DriverHealthInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DriverHealthInfo->Signature          = DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE; 
  DriverHealthInfo->DriverHandle       = DriverHandle;
  DriverHealthInfo->ControllerHandle   = ControllerHandle;
  DriverHealthInfo->ChildHandle        = ChildHandle;
  DriverHealthInfo->HiiHandle          = FormHiiHandle;
  DriverHealthInfo->DriverHealth       = DriverHealth;
  DriverHealthInfo->MessageList        = MessageList;
  DriverHealthInfo->HealthStatus       = *HealthStatus;

  InsertTailList (DriverHealthList, &DriverHealthInfo->Link);

  return EFI_SUCCESS;
}

/**
  Collects all the EFI Driver Health Protocols currently present in the EFI Handle Database, 
  and queries each EFI Driver Health Protocol to determine if one or more of the controllers 
  managed by each EFI Driver Health Protocol instance are not healthy.  

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information. 

  @retval    EFI_NOT_FOUND         No controller in the platform install Driver Health Protocol.
  @retval    EFI_SUCCESS           All the controllers in the platform are healthy.
  @retval    EFI_OUT_OF_RESOURCES  The list of Driver Health Protocol handles can not be retrieved.

**/
EFI_STATUS
GetAllControllersHealthStatus (
  IN OUT LIST_ENTRY  *DriverHealthList
  )
{
  EFI_STATUS                 Status; 
  UINTN                      NumHandles;
  EFI_HANDLE                 *DriverHealthHandles;
  EFI_DRIVER_HEALTH_PROTOCOL *DriverHealth;
  EFI_DRIVER_HEALTH_STATUS   HealthStatus;
  UINTN                      DriverHealthIndex;
  EFI_HANDLE                 *Handles;
  UINTN                      HandleCount;
  UINTN                      ControllerIndex;
  UINTN                      ChildIndex;
 
  //
  // Initialize local variables
  //
  Handles                 = NULL;
  DriverHealthHandles     = NULL;
  NumHandles              = 0;
  HandleCount             = 0;

  HealthStatus = EfiDriverHealthStatusHealthy;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverHealthProtocolGuid,
                  NULL,
                  &NumHandles,
                  &DriverHealthHandles
                  );

  if (Status == EFI_NOT_FOUND || NumHandles == 0) {
    //
    // If there are no Driver Health Protocols handles, then return EFI_NOT_FOUND
    //
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status) || DriverHealthHandles == NULL) {
    //
    // If the list of Driver Health Protocol handles can not be retrieved, then 
    // return EFI_OUT_OF_RESOURCES
    //
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Check the health status of all controllers in the platform
  // Start by looping through all the Driver Health Protocol handles in the handle database
  //
  for (DriverHealthIndex = 0; DriverHealthIndex < NumHandles; DriverHealthIndex++) {
    //
    // Skip NULL Driver Health Protocol handles
    //
    if (DriverHealthHandles[DriverHealthIndex] == NULL) {
      continue;
    }

    //
    // Retrieve the Driver Health Protocol from DriverHandle
    //
    Status = gBS->HandleProtocol ( 
                    DriverHealthHandles[DriverHealthIndex],
                    &gEfiDriverHealthProtocolGuid,
                    (VOID **)&DriverHealth
                    );
    if (EFI_ERROR (Status)) {
      //
      // If the Driver Health Protocol can not be retrieved, then skip to the next
      // Driver Health Protocol handle
      //
      continue;
    }

    //
    // Check the health of all the controllers managed by a Driver Health Protocol handle
    //
    Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], NULL, NULL, DriverHealth, &HealthStatus);

    //
    // If Status is an error code, then the health information could not be retrieved, so assume healthy
    // and skip to the next Driver Health Protocol handle
    //
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // If all the controllers managed by this Driver Health Protocol are healthy, then skip to the next 
    // Driver Health Protocol handle
    //
    if (HealthStatus == EfiDriverHealthStatusHealthy) {
      continue;
    }

    //
    // See if the list of all handles in the handle database has been retrieved
    //
    //
    if (Handles == NULL) {
      //
      // Retrieve the list of all handles from the handle database
      //
      Status = gBS->LocateHandleBuffer (
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &Handles
        );
      if (EFI_ERROR (Status) || Handles == NULL) {
        //
        // If all the handles in the handle database can not be retrieved, then 
        // return EFI_OUT_OF_RESOURCES
        //
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
    }
    //
    // Loop through all the controller handles in the handle database
    //
    for (ControllerIndex = 0; ControllerIndex < HandleCount; ControllerIndex++) {
      //
      // Skip NULL controller handles
      //
      if (Handles[ControllerIndex] == NULL) {
        continue;
      }

      Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], NULL, DriverHealth, &HealthStatus);
      if (EFI_ERROR (Status)) {
        //
        // If Status is an error code, then the health information could not be retrieved, so assume healthy
        //
        HealthStatus = EfiDriverHealthStatusHealthy;
      }

      //
      // If CheckHealthSingleController() returned an error on a terminal state, then do not check the health of child controllers
      //
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Loop through all the child handles in the handle database
      //
      for (ChildIndex = 0; ChildIndex < HandleCount; ChildIndex++) {
        //
        // Skip NULL child handles
        //
        if (Handles[ChildIndex] == NULL) {
          continue;
        }

        Status = GetSingleControllerHealthStatus (DriverHealthList, DriverHealthHandles[DriverHealthIndex], Handles[ControllerIndex], Handles[ChildIndex], DriverHealth, &HealthStatus);
        if (EFI_ERROR (Status)) {
          //
          // If Status is an error code, then the health information could not be retrieved, so assume healthy
          //
          HealthStatus = EfiDriverHealthStatusHealthy;
        }

        //
        // If CheckHealthSingleController() returned an error on a terminal state, then skip to the next child
        //
        if (EFI_ERROR (Status)) {
          continue;
        }
      }
    }
  }

  Status = EFI_SUCCESS;

Done:
  if (Handles != NULL) {
    gBS->FreePool (Handles);
  }
  if (DriverHealthHandles != NULL) {
    gBS->FreePool (DriverHealthHandles);
  }

  return Status;
}


/*
  Check the healthy status of the platform, this function will return immediately while found one driver 
  in the platform are not healthy.

  @retval FALSE      at least one driver in the platform are not healthy.
  @retval TRUE       No controller install Driver Health Protocol,
                     or all controllers in the platform are in healthy status.
*/
BOOLEAN
PlaformHealthStatusCheck (
  VOID
  )
{
  EFI_DRIVER_HEALTH_STATUS          HealthStatus;
  EFI_STATUS                        Status;
  UINTN                             Index;
  UINTN                             NoHandles;
  EFI_HANDLE                        *DriverHealthHandles;
  EFI_DRIVER_HEALTH_PROTOCOL        *DriverHealth;
  BOOLEAN                           AllHealthy;

  //
  // Initialize local variables
  //
  DriverHealthHandles = NULL;
  DriverHealth        = NULL;

  HealthStatus = EfiDriverHealthStatusHealthy;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverHealthProtocolGuid,
                  NULL,
                  &NoHandles,
                  &DriverHealthHandles
                  );
  //
  // There are no handles match the search for Driver Health Protocol has been installed.
  //
  if (Status == EFI_NOT_FOUND) {
    return TRUE;
  }
  //
  // Assume all modules are healthy.
  // 
  AllHealthy = TRUE;

  //
  // Found one or more Handles.
  //
  if (!EFI_ERROR (Status)) {    
    for (Index = 0; Index < NoHandles; Index++) {
      Status = gBS->HandleProtocol (
                      DriverHealthHandles[Index],
                      &gEfiDriverHealthProtocolGuid,
                      (VOID **) &DriverHealth
                      );
      if (!EFI_ERROR (Status)) {
        Status = DriverHealth->GetHealthStatus (
                                 DriverHealth,
                                 NULL,
                                 NULL,
                                 &HealthStatus,
                                 NULL,
                                 NULL
                                 );
      }
      //
      // Get the healthy status of the module
      //
      if (!EFI_ERROR (Status)) {
         if (HealthStatus != EfiDriverHealthStatusHealthy) {
           //
           // Return immediately one driver's status not in healthy.
           //
           return FALSE;         
         }
      }
    }
  }
  return AllHealthy;
}

/**
  Processes a single controller using the EFI Driver Health Protocol associated with 
  that controller. This algorithm continues to query the GetHealthStatus() service until
  one of the legal terminal states of the EFI Driver Health Protocol is reached. This may 
  require the processing of HII Messages, HII Form, and invocation of repair operations.

  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health 
                            status on.  This is an optional parameter that may be NULL. 
  @param HealthStatus       The health status of the controller.
  @param MessageList        An array of warning or error messages associated 
                            with the controller specified by ControllerHandle and 
                            ChildHandle.  This is an optional parameter that may be NULL.
  @param FormHiiHandle      The HII handle for an HII form associated with the 
                            controller specified by ControllerHandle and ChildHandle.
**/
VOID
ProcessSingleControllerHealth (
    IN  EFI_DRIVER_HEALTH_PROTOCOL         *DriverHealth,
    IN  EFI_HANDLE                         ControllerHandle, OPTIONAL
    IN  EFI_HANDLE                         ChildHandle,      OPTIONAL
    IN  EFI_DRIVER_HEALTH_STATUS           HealthStatus,
    IN  EFI_DRIVER_HEALTH_HII_MESSAGE      **MessageList,    OPTIONAL
    IN  EFI_HII_HANDLE                     FormHiiHandle
  )
{
  EFI_STATUS                         Status;
  EFI_DRIVER_HEALTH_STATUS           LocalHealthStatus;
  
  LocalHealthStatus = HealthStatus;
  //
  // If the module need to be repaired or reconfiguration,  will process it until
  // reach a terminal status. The status from EfiDriverHealthStatusRepairRequired after repair 
  // will be in (Health, Failed, Configuration Required).
  //
  while( LocalHealthStatus == EfiDriverHealthStatusConfigurationRequired ||
         LocalHealthStatus == EfiDriverHealthStatusRepairRequired) {

    if (LocalHealthStatus == EfiDriverHealthStatusRepairRequired) {
      Status = DriverHealth->Repair (
                               DriverHealth,
                               ControllerHandle,
                               ChildHandle,
                               (EFI_DRIVER_HEALTH_REPAIR_PROGRESS_NOTIFY) RepairNotify
                               );
    }
    //
    // Via a form of the driver need to do configuration provided to process of status in 
    // EfiDriverHealthStatusConfigurationRequired. The status after configuration should be in
    // (Healthy, Reboot Required, Failed, Reconnect Required, Repair Required).   
    //
    if (LocalHealthStatus == EfiDriverHealthStatusConfigurationRequired) {
      Status = gFormBrowser2->SendForm (
                                gFormBrowser2,
                                &FormHiiHandle,
                                1,
                                &gEfiHiiDriverHealthFormsetGuid,
                                0,
                                NULL,
                                NULL
                                );
      ASSERT( !EFI_ERROR (Status));
    }

    Status = DriverHealth->GetHealthStatus (
                              DriverHealth,
                              ControllerHandle,
                              ChildHandle,
                              &LocalHealthStatus,
                              NULL,
                              &FormHiiHandle
                              );
   ASSERT_EFI_ERROR (Status);

   if (*MessageList != NULL) {
      ProcessMessages (*MessageList);
   }  
  }
  
  //
  // Health status in {Healthy, Failed} may also have Messages need to process
  //
  if (LocalHealthStatus == EfiDriverHealthStatusHealthy || LocalHealthStatus == EfiDriverHealthStatusFailed) {
    if (*MessageList != NULL) {
      ProcessMessages (*MessageList);
    }
  }
  //
  // Check for RebootRequired or ReconnectRequired
  //
  if (LocalHealthStatus == EfiDriverHealthStatusRebootRequired) {
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }
  
  //
  // Do reconnect if need.
  //
  if (LocalHealthStatus == EfiDriverHealthStatusReconnectRequired) {
    Status = gBS->DisconnectController (ControllerHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
        //
        // Disconnect failed.  Need to promote reconnect to a reboot.
        //
        gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }
    gBS->ConnectController (ControllerHandle, NULL, NULL, TRUE);
  }
}


/**
  Platform specific notification function for controller repair operations.

  If the driver for a controller support the Driver Health Protocol and the
  current state of the controller is EfiDriverHealthStatusRepairRequired then
  when the Repair() service of the Driver Health Protocol is called, this 
  platform specific notification function can display the progress of the repair
  operation.  Some platforms may choose to not display anything, other may choose
  to show the percentage complete on text consoles, and other may choose to render
  a progress bar on text and graphical consoles.

  This function displays the percentage of the repair operation that has been
  completed on text consoles.  The percentage is Value / Limit * 100%.
  
  @param  Value               Value in the range 0..Limit the the repair has completed..
  @param  Limit               The maximum value of Value

**/
VOID
RepairNotify (
  IN  UINTN Value,
  IN  UINTN Limit
  )
{
  UINTN Percent;

  if (Limit  == 0) {
    Print(L"Repair Progress Undefined\n\r");
  } else {
    Percent = Value * 100 / Limit;
    Print(L"Repair Progress = %3d%%\n\r", Percent);
  }
}

/**
  Processes a set of messages returned by the GetHealthStatus ()
  service of the EFI Driver Health Protocol

  @param    MessageList  The MessageList point to messages need to processed.  

**/
VOID
ProcessMessages (
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      *MessageList
  )
{
  UINTN                           MessageIndex;
  EFI_STRING                      MessageString;

  for (MessageIndex = 0;
       MessageList[MessageIndex].HiiHandle != NULL;
       MessageIndex++) {

    MessageString = HiiGetString (
                        MessageList[MessageIndex].HiiHandle,
                        MessageList[MessageIndex].StringId,
                        NULL
                        );
    if (MessageString != NULL) {
      //
      // User can customize the output. Just simply print out the MessageString like below. 
      // Also can use the HiiHandle to display message on the front page.
      // 
      // Print(L"%s\n",MessageString);
      // gBS->Stall (100000);
    }
  }

}

/*
  Repair the whole platform.

  This function is the main entry for user choose "Repair All" in the front page.
  It will try to do recovery job till all the driver health protocol installed modules 
  reach a terminal state.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information.

*/
VOID
PlatformRepairAll (
  IN LIST_ENTRY  *DriverHealthList
  )
{ 
  DRIVER_HEALTH_INFO          *DriverHealthInfo;
  LIST_ENTRY                  *Link;

  ASSERT (DriverHealthList != NULL);

  Link = GetFirstNode (DriverHealthList);

  while (!IsNull (DriverHealthList, Link)) {   
    DriverHealthInfo = DEVICE_MANAGER_HEALTH_INFO_FROM_LINK (Link);
    //
    // Do driver health status operation by each link node
    //
    ASSERT (DriverHealthInfo != NULL);

    ProcessSingleControllerHealth ( 
        DriverHealthInfo->DriverHealth,
        DriverHealthInfo->ControllerHandle,
        DriverHealthInfo->ChildHandle,
        DriverHealthInfo->HealthStatus,
        &(DriverHealthInfo->MessageList),
        DriverHealthInfo->HiiHandle
        );

    Link = GetNextNode (DriverHealthList, Link);
  }
}

/**

  Select the best matching language according to front page policy for best user experience. 
  
  This function supports both ISO 639-2 and RFC 4646 language codes, but language 
  code types may not be mixed in a single call to this function. 

  @param  SupportedLanguages   A pointer to a Null-terminated ASCII string that
                               contains a set of language codes in the format 
                               specified by Iso639Language.
  @param  Iso639Language       If TRUE, then all language codes are assumed to be
                               in ISO 639-2 format.  If FALSE, then all language
                               codes are assumed to be in RFC 4646 language format.

  @retval NULL                 The best matching language could not be found in SupportedLanguages.
  @retval NULL                 There are not enough resources available to return the best matching 
                               language.
  @retval Other                A pointer to a Null-terminated ASCII string that is the best matching 
                               language in SupportedLanguages.
**/
CHAR8 *
DriverHealthSelectBestLanguage (
  IN CHAR8        *SupportedLanguages,
  IN BOOLEAN      Iso639Language
  )
{
  CHAR8           *LanguageVariable;
  CHAR8           *BestLanguage;

  LanguageVariable =  GetEfiGlobalVariable (Iso639Language ? L"Lang" : L"PlatformLang");

  BestLanguage = GetBestLanguage(
                   SupportedLanguages,
                   Iso639Language,
                   (LanguageVariable != NULL) ? LanguageVariable : "",
                   Iso639Language ? "eng" : "en-US",
                   NULL
                   );
  if (LanguageVariable != NULL) {
    FreePool (LanguageVariable);
  }

  return BestLanguage;
}



/**

  This is an internal worker function to get the Component Name (2) protocol interface
  and the language it supports.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ComponentName        A pointer to the Component Name (2) protocol interface.
  @param  SupportedLanguage    The best suitable language that matches the SupportedLangues interface for the 
                               located Component Name (2) instance.

  @param  EFI_SUCCESS          The Component Name (2) protocol instance is successfully located and we find
                               the best matching language it support.
  @param  EFI_UNSUPPORTED      The input Language is not supported by the Component Name (2) protocol.
  @param  Other                Some error occurs when locating Component Name (2) protocol instance or finding
                               the supported language.

**/
EFI_STATUS
GetComponentNameWorker (
  IN  EFI_GUID                    *ProtocolGuid,
  IN  EFI_HANDLE                  DriverBindingHandle,
  OUT EFI_COMPONENT_NAME_PROTOCOL **ComponentName,
  OUT CHAR8                       **SupportedLanguage
  )
{
  EFI_STATUS                      Status;

  //
  // Locate Component Name (2) protocol on the driver binging handle.
  //
  Status = gBS->OpenProtocol (
                 DriverBindingHandle,
                 ProtocolGuid,
                 (VOID **) ComponentName,
                 NULL,
                 NULL,
                 EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Apply shell policy to select the best language.
  //
  *SupportedLanguage = DriverHealthSelectBestLanguage (
                         (*ComponentName)->SupportedLanguages,
                         (BOOLEAN) (ProtocolGuid == &gEfiComponentNameProtocolGuid)
                         );
  if (*SupportedLanguage == NULL) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**

  This is an internal worker function to get driver name from Component Name (2) protocol interface.


  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
GetDriverNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and 
  // find the best language this instance supports. 
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  //
  // Get the driver name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetDriverName (
                            ComponentName,
                            BestLanguage,
                            DriverName
                            );
  FreePool (BestLanguage);
 
  return Status;
}

/**

  This function gets driver name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the driver name.
  If the attempt fails, it then gets the driver name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
DriverHealthGetDriverName (
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  )
{
  EFI_STATUS      Status;

  //
  // Get driver name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetDriverNameWorker (&gEfiComponentName2ProtocolGuid, DriverBindingHandle, DriverName);
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the driver name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetDriverNameWorker (&gEfiComponentNameProtocolGuid, DriverBindingHandle, DriverName);
  }

  return Status;
}



/**
  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval  EFI_SUCCESS         The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval  Other               The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
GetControllerNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  )
{
  EFI_STATUS                     Status;
  CHAR8                          *BestLanguage;
  EFI_COMPONENT_NAME_PROTOCOL    *ComponentName;

  //
  // Retrieve Component Name (2) protocol instance on the driver binding handle and 
  // find the best language this instance supports. 
  //
  Status = GetComponentNameWorker (
             ProtocolGuid,
             DriverBindingHandle,
             &ComponentName,
             &BestLanguage
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the controller name from Component Name (2) protocol instance on the driver binging handle.
  //
  Status = ComponentName->GetControllerName (
                            ComponentName,
                            ControllerHandle,
                            ChildHandle,
                            BestLanguage,
                            ControllerName
                            );
  FreePool (BestLanguage);

  return Status;
}

/**

  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name. 
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support. 

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  Language             An ASCII string that represents the language command line option.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval EFI_SUCCESS          The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
DriverHealthGetControllerName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  )
{
  EFI_STATUS      Status;

  //
  // Get controller name from UEFI 2.0 Component Name 2 protocol interface.
  //
  Status = GetControllerNameWorker (
             &gEfiComponentName2ProtocolGuid,
             DriverBindingHandle,
             ControllerHandle,
             ChildHandle,
             ControllerName
             );
  if (EFI_ERROR (Status)) {
    //
    // If it fails to get the controller name from Component Name protocol interface, we should fall back on
    // EFI 1.1 Component Name protocol interface.
    //
    Status = GetControllerNameWorker (
               &gEfiComponentNameProtocolGuid,
               DriverBindingHandle,
               ControllerHandle,
               ChildHandle,
               ControllerName
               );
  }

  return Status;
}
