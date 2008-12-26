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

#define MENU_ITEM_NUM  \
  (sizeof (mDeviceManagerMenuItemTable) / sizeof (DEVICE_MANAGER_MENU_ITEM))

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
  DEVICE_MANAGER_CALLBACK_DATA *PrivateData;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DEVICE_MANAGER_CALLBACK_DATA_FROM_THIS (This);

  switch (QuestionId) {
  case DEVICE_MANAGER_KEY_VBIOS:
    PrivateData->VideoBios = Value->u8;
    gRT->SetVariable (
           L"VBIOS",
           &gEfiGenericPlatformVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
           sizeof (UINT8),
           &PrivateData->VideoBios
           );

    //
    // Tell browser not to ask for confirmation of changes,
    // since we have already applied.
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    break;

  default:
    //
    // The key corresponds the Handle Index which was requested to be displayed
    //
    gCallbackKey = QuestionId;

    //
    // Request to exit SendForm(), so as to switch to selected form
    //
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    break;
  }

  return EFI_SUCCESS;
}

/**

  This function registers HII packages to HII database.

  @retval EFI_SUCCESS This function complete successfully.
  @return Other value if failed to register HII packages.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;

  //
  // Create driver handle used by HII database
  //
  Status = HiiLibCreateHiiDriverHandle (&gDeviceManagerPrivate.DriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Config Access protocol to driver handle
  //
  Status = gBS->InstallProtocolInterface (
                  &gDeviceManagerPrivate.DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gDeviceManagerPrivate.ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PackageList = HiiLibPreparePackageList (2, &mDeviceManagerGuid, DeviceManagerVfrBin, BdsDxeStrings);
  ASSERT (PackageList != NULL);

  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           gDeviceManagerPrivate.DriverHandle,
                           &gDeviceManagerPrivate.HiiHandle
                           );
  FreePool (PackageList);

  return Status;
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
  UINTN                       Count;
  UINTN                       Index;
  CHAR16                      *String;
  UINTN                       StringLength;
  EFI_HII_UPDATE_DATA         UpdateData[MENU_ITEM_NUM];
  EFI_STRING_ID               Token;
  EFI_STRING_ID               TokenHelp;
  IFR_OPTION                  *IfrOptionList;
  UINT8                       *VideoOption;
  UINTN                       VideoOptionSize;
  EFI_HII_HANDLE              *HiiHandles;
  UINTN                       HandleBufferLength;
  UINTN                       NumberOfHiiHandles;
  EFI_HII_HANDLE              HiiHandle;
  UINT16                      FormSetClass;
  EFI_STRING_ID               FormSetTitle;
  EFI_STRING_ID               FormSetHelp;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;

  IfrOptionList       = NULL;
  VideoOption         = NULL;
  HiiHandles          = NULL;
  HandleBufferLength  = 0;

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
  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    //
    // Allocate space for creation of UpdateData Buffer
    //
    UpdateData[Index].BufferSize = 0x1000;
    UpdateData[Index].Offset = 0;
    UpdateData[Index].Data = AllocatePool (0x1000);
    ASSERT (UpdateData[Index].Data != NULL);

    CreateSubTitleOpCode (mDeviceManagerMenuItemTable[Index].StringId, 0, 0, 1,  &UpdateData[Index]);
  }

  //
  // Get all the Hii handles
  //
  Status = HiiLibGetHiiHandles (&HandleBufferLength, &HiiHandles);
  ASSERT_EFI_ERROR (Status);

  HiiHandle = gDeviceManagerPrivate.HiiHandle;

  StringLength  = 0x1000;
  String        = AllocateZeroPool (StringLength);
  ASSERT (String != NULL);

  //
  // Search for formset of each class type
  //
  NumberOfHiiHandles = HandleBufferLength / sizeof (EFI_HII_HANDLE);
  for (Index = 0; Index < NumberOfHiiHandles; Index++) {
    IfrLibExtractClassFromHiiHandle (HiiHandles[Index], &FormSetClass, &FormSetTitle, &FormSetHelp);

    if (FormSetClass == EFI_NON_DEVICE_CLASS) {
      continue;
    }

    Token = 0;
    *String = 0;
    StringLength = 0x1000;
    HiiLibGetString (HiiHandles[Index], FormSetTitle, String, &StringLength);
    HiiLibNewString (HiiHandle, &Token, String);

    TokenHelp = 0;
    *String = 0;
    StringLength = 0x1000;
    HiiLibGetString (HiiHandles[Index], FormSetHelp, String, &StringLength);
    HiiLibNewString (HiiHandle, &TokenHelp, String);

    for (Count = 0; Count < MENU_ITEM_NUM; Count++) {
      if (FormSetClass & mDeviceManagerMenuItemTable[Count].Class) {
        CreateActionOpCode (
          (EFI_QUESTION_ID) (Index + DEVICE_KEY_OFFSET),
          Token,
          TokenHelp,
          EFI_IFR_FLAG_CALLBACK,
          0,
          &UpdateData[Count]
          );
      }
    }
  }
  FreePool (String);

  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    //
    // Add End Opcode for Subtitle
    //
    CreateEndOpCode (&UpdateData[Index]);

    IfrLibUpdateForm (
      HiiHandle,
      &mDeviceManagerGuid,
      DEVICE_MANAGER_FORM_ID,
      mDeviceManagerMenuItemTable[Index].Class,
      FALSE,
      &UpdateData[Index]
      );
  }

  //
  // Add oneof for video BIOS selection
  //
  VideoOption = BdsLibGetVariableAndSize (
                  L"VBIOS",
                  &gEfiGenericPlatformVariableGuid,
                  &VideoOptionSize
                  );
  if (VideoOption == NULL) {
    gDeviceManagerPrivate.VideoBios = 0;
  } else {
    gDeviceManagerPrivate.VideoBios = VideoOption[0];
    FreePool (VideoOption);
  }

  ASSERT (gDeviceManagerPrivate.VideoBios <= 1);

  IfrOptionList = AllocatePool (2 * sizeof (IFR_OPTION));
  ASSERT (IfrOptionList != NULL);
  IfrOptionList[0].Flags        = 0;
  IfrOptionList[0].StringToken  = STRING_TOKEN (STR_ONE_OF_PCI);
  IfrOptionList[0].Value.u8     = 0;
  IfrOptionList[1].Flags        = 0;
  IfrOptionList[1].StringToken  = STRING_TOKEN (STR_ONE_OF_AGP);
  IfrOptionList[1].Value.u8     = 1;
  IfrOptionList[gDeviceManagerPrivate.VideoBios].Flags |= EFI_IFR_OPTION_DEFAULT;

  UpdateData[0].Offset = 0;
  CreateOneOfOpCode (
    DEVICE_MANAGER_KEY_VBIOS,
    0,
    0,
    STRING_TOKEN (STR_ONE_OF_VBIOS),
    STRING_TOKEN (STR_ONE_OF_VBIOS_HELP),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    2,
    &UpdateData[0]
    );

  IfrLibUpdateForm (
    HiiHandle,
    &mDeviceManagerGuid,
    DEVICE_MANAGER_FORM_ID,
    LABEL_VBIOS,
    FALSE,
    &UpdateData[0]
    );

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
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
  gHiiDatabase->RemovePackageList (gHiiDatabase, HiiHandle);
  PackageList = HiiLibPreparePackageList (2, &mDeviceManagerGuid, DeviceManagerVfrBin, BdsDxeStrings);
  ASSERT (PackageList != NULL);
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           gDeviceManagerPrivate.DriverHandle,
                           &gDeviceManagerPrivate.HiiHandle
                           );
  FreePool (PackageList);

  for (Index = 0; Index < MENU_ITEM_NUM; Index++) {
    FreePool (UpdateData[Index].Data);
  }
  FreePool (HiiHandles);

  return Status;
}
