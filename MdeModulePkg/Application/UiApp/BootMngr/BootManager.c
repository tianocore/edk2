/** @file
  The platform boot manager reference implementation

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootManager.h"

EFI_GUID           mBootManagerGuid = BOOT_MANAGER_FORMSET_GUID;
CHAR16             *mDeviceTypeStr[] = {
  L"Legacy BEV",
  L"Legacy Floppy",
  L"Legacy Hard Drive",
  L"Legacy CD ROM",
  L"Legacy PCMCIA",
  L"Legacy USB",
  L"Legacy Embedded Network",
  L"Legacy Unknown Device"
};


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
  This call back function is registered with Boot Manager formset.
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
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  UINT16                        KeyCount;
  EFI_INPUT_KEY                 Key;
  EFI_BOOT_MANAGER_LOAD_OPTION  Option;

  if (Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the key count
  //
  KeyCount = 0;
  Option.Attributes = 0;
  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    KeyCount++;

    EfiBootManagerInitializeLoadOption (
      &Option,
      BootOption[Index].OptionNumber,
      BootOption[Index].OptionType,
      BootOption[Index].Attributes,
      BootOption[Index].Description,
      BootOption[Index].FilePath,
      BootOption[Index].OptionalData,
      BootOption[Index].OptionalDataSize
      );

    //
    // Is this device the one chosen?
    //
    if (KeyCount == QuestionId) {
      //
      // Clear  the  screen  before.
      //
      gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
      gST->ConOut->ClearScreen (gST->ConOut);
      //
      // Check any reset required change is applied? if yes, reset system
      //
      SetupResetReminder();
      //
      // Parse the selected option.
      //
      BdsSetConsoleMode(FALSE);
      EfiBootManagerBoot (&Option);
      BdsSetConsoleMode(TRUE);

      if (EFI_ERROR (Option.Status)) {
        gST->ConOut->OutputString (
                      gST->ConOut,
                      GetStringById (STRING_TOKEN (STR_ANY_KEY_CONTINUE))
                      );
        gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      }
      break;
    }
  }
  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  return EFI_SUCCESS;
}

/**
  Register HII packages to HII database.

**/
VOID
InitializeBootManager  (
  VOID
  )
{
  EFI_STATUS      Status;

  if (!gConnectAllHappened){
    EfiBootManagerConnectAll();
    gConnectAllHappened = TRUE;
  }

  //
  // Install Device Path Protocol and Config Access protocol to driver handle
  //
  gBootManagerPrivate.DriverHandle = NULL;
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
                                    UiAppStrings,
                                    NULL
                                    );
  ASSERT(gBootManagerPrivate.HiiHandle != NULL);
}


/**
  Enumerate possible boot options.

**/
VOID
EnumerateBootOptions  (
  VOID
  )
{
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;
  EFI_STRING_ID                 Token;
  CHAR16                        *HelpString;
  EFI_STRING_ID                 HelpToken;
  UINT16                        *TempStr;
  EFI_HII_HANDLE                HiiHandle;
  UINTN                         TempSize;
  VOID                          *StartOpCodeHandle;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  EFI_IFR_GUID_LABEL            *EndLabel;
  UINT16                        DeviceType;
  BOOLEAN                       IsLegacyOption;
  BOOLEAN                       NeedEndOp;
  UINT16                        KeyInput;
  UINTN                         DestMax;

  DeviceType = (UINT16) -1;

  //
  // for better user experience
  // 1. User changes HD configuration (e.g.: unplug HDD), here we have a chance to remove the HDD boot option
  // 2. User enables/disables UEFI PXE, here we have a chance to add/remove EFI Network boot option
  //
  EfiBootManagerRefreshAllBootOption ();

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

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

  KeyInput = 0;
  NeedEndOp = FALSE;
  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // At this stage we are creating a menu entry, thus the Keys are reproduceable
    //
    KeyInput++;

    //
    // Don't display the hidden/inactive boot option
    //
    if (((BootOption[Index].Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0)) {
      continue;
    }

    //
    // Group the legacy boot option in the sub title created dynamically
    //
    IsLegacyOption = (BOOLEAN) (
                       (DevicePathType (BootOption[Index].FilePath) == BBS_DEVICE_PATH) &&
                       (DevicePathSubType (BootOption[Index].FilePath) == BBS_BBS_DP)
                       );

    if (!IsLegacyOption && NeedEndOp) {
      NeedEndOp = FALSE;
      HiiCreateEndOpCode (StartOpCodeHandle);
    }
    
    if (IsLegacyOption && DeviceType != ((BBS_BBS_DEVICE_PATH *) BootOption[Index].FilePath)->DeviceType) {
      if (NeedEndOp) {
        HiiCreateEndOpCode (StartOpCodeHandle);
      }

      DeviceType = ((BBS_BBS_DEVICE_PATH *) BootOption[Index].FilePath)->DeviceType;
      Token      = HiiSetString (
                     HiiHandle,
                     0,
                     mDeviceTypeStr[
                       MIN (DeviceType & 0xF, sizeof (mDeviceTypeStr) / sizeof (mDeviceTypeStr[0]) - 1)
                       ],
                     NULL
                     );
      HiiCreateSubTitleOpCode (StartOpCodeHandle, Token, 0, 0, 1);
      NeedEndOp = TRUE;
    }

    ASSERT (BootOption[Index].Description != NULL);
    
    Token = HiiSetString (HiiHandle, 0, BootOption[Index].Description, NULL);

    TempStr = UiDevicePathToStr (BootOption[Index].FilePath);
    TempSize = StrSize (TempStr);
    DestMax = (TempSize + StrSize (L"Device Path : ")) / sizeof(CHAR16);
    HelpString = AllocateZeroPool (TempSize + StrSize (L"Device Path : "));
    ASSERT (HelpString != NULL);
    StrCatS (HelpString, DestMax, L"Device Path : ");
    StrCatS (HelpString, DestMax, TempStr);

    HelpToken = HiiSetString (HiiHandle, 0, HelpString, NULL);

    HiiCreateActionOpCode (
      StartOpCodeHandle,
      KeyInput,
      Token,
      HelpToken,
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  if (NeedEndOp) {
    HiiCreateEndOpCode (StartOpCodeHandle);
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
  
  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);
}


/**
  Remove the installed  packages from the HII Database. 

**/
VOID
FreeBootManager (
  VOID
  )
{
  EFI_STATUS      Status;
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gBootManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBootManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gBootManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  
  HiiRemovePackages (gBootManagerPrivate.HiiHandle);
}
