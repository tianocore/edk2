/** @file
  The platform device manager reference implementation

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DeviceManager.h"
#include <Guid/HiiPlatformSetupFormset.h>

DEVICE_MANAGER_CALLBACK_DATA  gDeviceManagerPrivate = {
  DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    DeviceManagerCallback
  }
};

EFI_GUID mDeviceManagerGuid = DEVICE_MANAGER_FORMSET_GUID;

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

/**
  This function is invoked if user selected a iteractive opcode from Device Manager's
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

  HiiHandles    = NULL;
  Status        = EFI_SUCCESS;
  gCallbackKey  = 0;

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
  if (gCallbackKey != 0) {
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
