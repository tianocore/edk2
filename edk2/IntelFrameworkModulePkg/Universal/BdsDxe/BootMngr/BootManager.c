/** @file
  The platform boot manager reference implementation

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

HII_VENDOR_DEVICE_PATH  mBootManagerHiiVendorDevicePath = {
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
    // {1DDDBE15-481D-4d2b-8277-B191EAF66525}
    //
    { 0x1dddbe15, 0x481d, 0x4d2b, { 0x82, 0x77, 0xb1, 0x91, 0xea, 0xf6, 0x65, 0x25 } }
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

  @retval  EFI_SUCCESS           HII packages for the Boot Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Boot Manager failed to be registered.

**/
EFI_STATUS
InitializeBootManager (
  VOID
  )
{
  EFI_STATUS                  Status;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gBootManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBootManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gBootManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  gBootManagerPrivate.HiiHandle = HiiAddPackages (
                                    &mBootManagerGuid,
                                    gBootManagerPrivate.DriverHandle,
                                    BootManagerVfrBin,
                                    BdsDxeStrings,
                                    NULL
                                    );
  if (gBootManagerPrivate.HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = EFI_SUCCESS;
  }
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
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartLabel;
  EFI_IFR_GUID_LABEL          *EndLabel;

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
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LABEL_BOOT_OPTION;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_BOOT_OPTION_END;

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
    if ((Option->Attribute & LOAD_OPTION_HIDDEN) != 0) {
      continue;
    }

    Token = HiiSetString (HiiHandle, 0, Option->Description, NULL);

    TempStr = DevicePathToStr (Option->DevicePath);
    TempSize = StrSize (TempStr);
    HelpString = AllocateZeroPool (TempSize + StrSize (L"Device Path : "));
    ASSERT (HelpString != NULL);
    StrCat (HelpString, L"Device Path : ");
    StrCat (HelpString, TempStr);

    HelpToken = HiiSetString (HiiHandle, 0, HelpString, NULL);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      mKeyInput,
      Token,
      HelpToken,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  HiiUpdateForm (
    HiiHandle,
    &mBootManagerGuid,
    BOOT_MANAGER_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = gFormBrowser2->SendForm (
                           gFormBrowser2,
                           &HiiHandle,
                           1,
                           &mBootManagerGuid,
                           0,
                           NULL,
                           &ActionRequest
                           );
  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    EnableResetRequired ();
  }

  if (gOption == NULL) {
    return ;
  }

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

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
    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  }
}
