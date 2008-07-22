/** @file
  The platform boot manager reference implement

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootManager.h"

UINT16             mKeyInput;
EFI_GUID           mBootManagerGuid = BOOT_MANAGER_FORMSET_GUID;
LIST_ENTRY         *mBootOptionsList;
BDS_COMMON_OPTION  *gOption;

BOOT_MANAGER_CALLBACK_DATA  gBootManagerPrivate = {
  BOOT_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    FakeExtractConfig,
    FakeRouteConfig,
    BootManagerCallback
  }
};

/**
  This call back funtion is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


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
BootManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BDS_COMMON_OPTION       *Option;
  LIST_ENTRY              *Link;
  UINT16                  KeyCount;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the key count
  //
  KeyCount = 0;

  for (Link = mBootOptionsList->ForwardLink; Link != mBootOptionsList; Link = Link->ForwardLink) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    KeyCount++;

    gOption = Option;

    //
    // Is this device the one chosen?
    //
    if (KeyCount == QuestionId) {
      //
      // Assigning the returned Key to a global allows the original routine to know what was chosen
      //
      mKeyInput = QuestionId;

      //
      // Request to exit SendForm(), so that we could boot the selected option
      //
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
      break;
    }
  }

  return EFI_SUCCESS;
}

/**

  Registers HII packages for the Boot Manger to HII Database.
  It also registers the browser call back function.

  @return Status of HiiLibCreateHiiDriverHandle() and gHiiDatabase->NewPackageList()

**/
EFI_STATUS
InitializeBootManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;

  //
  // Create driver handle used by HII database
  //
  Status = HiiLibCreateHiiDriverHandle (&gBootManagerPrivate.DriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Config Access protocol to driver handle
  //
  Status = gBS->InstallProtocolInterface (
                  &gBootManagerPrivate.DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gBootManagerPrivate.ConfigAccess
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  PackageList = HiiLibPreparePackageList (2, &mBootManagerGuid, BootManagerVfrBin, BdsDxeStrings);
  ASSERT (PackageList != NULL);

  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           gBootManagerPrivate.DriverHandle,
                           &gBootManagerPrivate.HiiHandle
                           );
  FreePool (PackageList);

  return Status;
}

/**
  This funtion invokees Boot Manager. If all devices have not a chance to be connected,
  the connect all will be triggered. It then enumerate all boot options. If 
  a boot option from the Boot Manager page is selected, Boot Manager will boot
  from this boot option.
  
**/
VOID
CallBootManager (
  VOID
  )
{
  EFI_STATUS                  Status;
  BDS_COMMON_OPTION           *Option;
  LIST_ENTRY                  *Link;
  EFI_HII_UPDATE_DATA         UpdateData;
  CHAR16                      *ExitData;
  UINTN                       ExitDataSize;
  EFI_STRING_ID               Token;
  EFI_INPUT_KEY               Key;
  LIST_ENTRY                  BdsBootOptionList;
  CHAR16                      *HelpString;
  EFI_STRING_ID               HelpToken;
  UINT16                      *TempStr;
  EFI_HII_HANDLE              HiiHandle;
  EFI_BROWSER_ACTION_REQUEST  ActionRequest;
  UINTN                       TempSize;

  gOption = NULL;
  InitializeListHead (&BdsBootOptionList);

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // BugBug: Here we can not remove the legacy refresh macro, so we need
  // get the boot order every time from "BootOrder" variable.
  // Recreate the boot option list base on the BootOrder variable
  //
  BdsLibEnumerateAllBootOption (&BdsBootOptionList);

  mBootOptionsList  = &BdsBootOptionList;

  HiiHandle = gBootManagerPrivate.HiiHandle;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData.BufferSize = 0x1000;
  UpdateData.Offset = 0;
  UpdateData.Data = AllocateZeroPool (0x1000);
  ASSERT (UpdateData.Data != NULL);

  mKeyInput = 0;

  for (Link = BdsBootOptionList.ForwardLink; Link != &BdsBootOptionList; Link = Link->ForwardLink) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    //
    // At this stage we are creating a menu entry, thus the Keys are reproduceable
    //
    mKeyInput++;

    //
    // Don't display the boot option marked as LOAD_OPTION_HIDDEN
    //
    if (Option->Attribute & LOAD_OPTION_HIDDEN) {
      continue;
    }

    HiiLibNewString (HiiHandle, &Token, Option->Description);

    TempStr = DevicePathToStr (Option->DevicePath);
    TempSize = StrSize (TempStr);
    HelpString = AllocateZeroPool (TempSize + StrSize (L"Device Path : "));
    StrCat (HelpString, L"Device Path : ");
    StrCat (HelpString, TempStr);

    HiiLibNewString (HiiHandle, &HelpToken, HelpString);

    CreateActionOpCode (
      mKeyInput,
      Token,
      HelpToken,
      EFI_IFR_FLAG_CALLBACK,
      0,
      &UpdateData
      );
  }

  IfrLibUpdateForm (
    HiiHandle,
    &mBootManagerGuid,
    BOOT_MANAGER_FORM_ID,
    LABEL_BOOT_OPTION,
    FALSE,
    &UpdateData
    );
  FreePool (UpdateData.Data);

  //
  // Drop the TPL level from TPL_APPLICATION to TPL_APPLICATION
  //
  gBS->RestoreTPL (TPL_APPLICATION);

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

  if (gOption == NULL) {
    gBS->RaiseTPL (TPL_APPLICATION);
    return ;
  }

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

  //
  // Raise the TPL level back to TPL_APPLICATION
  //
  gBS->RaiseTPL (TPL_APPLICATION);

  //
  // parse the selected option
  //
  Status = BdsLibBootViaBootOption (gOption, gOption->DevicePath, &ExitDataSize, &ExitData);

  if (!EFI_ERROR (Status)) {
    gOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
    PlatformBdsBootSuccess (gOption);
  } else {
    gOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
    PlatformBdsBootFail (gOption, Status, ExitData, ExitDataSize);
    gST->ConOut->OutputString (
                  gST->ConOut,
                  GetStringById (STRING_TOKEN (STR_ANY_KEY_CONTINUE))
                  );
    gBS->RestoreTPL (TPL_APPLICATION);
    //
    // BdsLibUiWaitForSingleEvent (gST->ConIn->WaitForKey, 0);
    //
    gBS->RaiseTPL (TPL_APPLICATION);
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  }
}
