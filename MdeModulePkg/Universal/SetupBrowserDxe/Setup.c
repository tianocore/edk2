/** @file
Entry and initialization module for the browser.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"
#include "Ui.h"


SETUP_DRIVER_PRIVATE_DATA  mPrivateData = {
  SETUP_DRIVER_SIGNATURE,
  NULL,
  {
    SendForm,
    BrowserCallback
  },
  {
    UnicodeVSPrint,
    UnicodeVSPrintAsciiFormat,
    UnicodeValueToString,                         
    AsciiVSPrint,          
    AsciiVSPrintUnicodeFormat,
    AsciiValueToString
  }
};

EFI_HII_DATABASE_PROTOCOL         *mHiiDatabase;
EFI_HII_STRING_PROTOCOL           *mHiiString;
EFI_HII_CONFIG_ROUTING_PROTOCOL   *mHiiConfigRouting;

BANNER_DATA           *BannerData;
EFI_HII_HANDLE        FrontPageHandle;
UINTN                 gClassOfVfr;
UINTN                 gFunctionKeySetting;
BOOLEAN               gResetRequired;
BOOLEAN               gNvUpdateRequired;
EFI_HII_HANDLE        gHiiHandle;
UINT16                gDirection;
EFI_SCREEN_DESCRIPTOR gScreenDimensions;
BOOLEAN               gUpArrow;
BOOLEAN               gDownArrow;

//
// Browser Global Strings
//
CHAR16            *gFunctionOneString;
CHAR16            *gFunctionTwoString;
CHAR16            *gFunctionNineString;
CHAR16            *gFunctionTenString;
CHAR16            *gEnterString;
CHAR16            *gEnterCommitString;
CHAR16            *gEnterEscapeString;
CHAR16            *gEscapeString;
CHAR16            *gSaveFailed;
CHAR16            *gMoveHighlight;
CHAR16            *gMakeSelection;
CHAR16            *gDecNumericInput;
CHAR16            *gHexNumericInput;
CHAR16            *gToggleCheckBox;
CHAR16            *gPromptForData;
CHAR16            *gPromptForPassword;
CHAR16            *gPromptForNewPassword;
CHAR16            *gConfirmPassword;
CHAR16            *gConfirmError;
CHAR16            *gPassowordInvalid;
CHAR16            *gPressEnter;
CHAR16            *gEmptyString;
CHAR16            *gAreYouSure;
CHAR16            *gYesResponse;
CHAR16            *gNoResponse;
CHAR16            *gMiniString;
CHAR16            *gPlusString;
CHAR16            *gMinusString;
CHAR16            *gAdjustNumber;
CHAR16            *gSaveChanges;
CHAR16            *gOptionMismatch;

CHAR16            gPromptBlockWidth;
CHAR16            gOptionBlockWidth;
CHAR16            gHelpBlockWidth;

EFI_GUID  gZeroGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
EFI_GUID  gSetupBrowserGuid = {
  0xab368524, 0xb60c, 0x495b, {0xa0, 0x9, 0x12, 0xe8, 0x5b, 0x1a, 0xea, 0x32}
};

FUNCTIION_KEY_SETTING gFunctionKeySettingTable[] = {
  //
  // Boot Manager
  //
  {
    {
      0x847bc3fe,
      0xb974,
      0x446d,
      {
        0x94,
        0x49,
        0x5a,
        0xd5,
        0x41,
        0x2e,
        0x99,
        0x3b
      }
    },
    NONE_FUNCTION_KEY_SETTING
  },
  //
  // Device Manager
  //
  {
    {
      0x3ebfa8e6,
      0x511d,
      0x4b5b,
      {
        0xa9,
        0x5f,
        0xfb,
        0x38,
        0x26,
        0xf,
        0x1c,
        0x27
      }
    },
    NONE_FUNCTION_KEY_SETTING
  },
  //
  // BMM FormSet.
  //
  {
    {
      0x642237c7,
      0x35d4,
      0x472d,
      {
        0x83,
        0x65,
        0x12,
        0xe0,
        0xcc,
        0xf2,
        0x7a,
        0x22
      }
    },
    NONE_FUNCTION_KEY_SETTING
  },
  //
  // BMM File Explorer FormSet.
  //
  {
    {
      0x1f2d63e1,
      0xfebd,
      0x4dc7,
      {
        0x9c,
        0xc5,
        0xba,
        0x2b,
        0x1c,
        0xef,
        0x9c,
        0x5b
      }
    },
    NONE_FUNCTION_KEY_SETTING
  },
};

/**
  This is the routine which an external caller uses to direct the browser
  where to obtain it's information.


  @param This            The Form Browser protocol instanse.
  @param Handles         A pointer to an array of Handles.  If HandleCount > 1 we
                         display a list of the formsets for the handles specified.
  @param HandleCount     The number of Handles specified in Handle.
  @param FormSetGuid     This field points to the EFI_GUID which must match the Guid
                         field in the EFI_IFR_FORM_SET op-code for the specified
                         forms-based package. If FormSetGuid is NULL, then this
                         function will display the first found forms package.
  @param FormId          This field specifies which EFI_IFR_FORM to render as the first
                         displayable page. If this field has a value of 0x0000, then
                         the forms browser will render the specified forms in their encoded order.
  @param ScreenDimensions Points to recommended form dimensions, including any non-content area, in 
                          characters.
  @param ActionRequest   Points to the action recommended by the form.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval  EFI_NOT_FOUND          No valid forms could be found to display.

**/
EFI_STATUS
EFIAPI
SendForm (
  IN  CONST EFI_FORM_BROWSER2_PROTOCOL *This,
  IN  EFI_HII_HANDLE                   *Handles,
  IN  UINTN                            HandleCount,
  IN  EFI_GUID                         *FormSetGuid, OPTIONAL
  IN  UINT16                           FormId, OPTIONAL
  IN  CONST EFI_SCREEN_DESCRIPTOR      *ScreenDimensions, OPTIONAL
  OUT EFI_BROWSER_ACTION_REQUEST       *ActionRequest  OPTIONAL
  )
{
  EFI_STATUS                    Status;
  UI_MENU_SELECTION             *Selection;
  UINTN                         Index;
  FORM_BROWSER_FORMSET          *FormSet;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  Status = EFI_SUCCESS;
  ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // Seed the dimensions in the global
  //
  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &gScreenDimensions.RightColumn,
                 &gScreenDimensions.BottomRow
                 );

  if (ScreenDimensions != NULL) {
    //
    // Check local dimension vs. global dimension.
    //
    if ((gScreenDimensions.RightColumn < ScreenDimensions->RightColumn) ||
        (gScreenDimensions.BottomRow < ScreenDimensions->BottomRow)
        ) {
      return EFI_INVALID_PARAMETER;
    } else {
      //
      // Local dimension validation.
      //
      if ((ScreenDimensions->RightColumn > ScreenDimensions->LeftColumn) &&
          (ScreenDimensions->BottomRow > ScreenDimensions->TopRow) &&
          ((ScreenDimensions->RightColumn - ScreenDimensions->LeftColumn) > 2) &&
          (
            (ScreenDimensions->BottomRow - ScreenDimensions->TopRow) > STATUS_BAR_HEIGHT +
            SCROLL_ARROW_HEIGHT *
            2 +
            FRONT_PAGE_HEADER_HEIGHT +
            FOOTER_HEIGHT +
            1
          )
        ) {
        CopyMem (&gScreenDimensions, (VOID *) ScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
      } else {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  gOptionBlockWidth = (CHAR16) ((gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) / 3);
  gHelpBlockWidth   = gOptionBlockWidth;
  gPromptBlockWidth = gOptionBlockWidth;

  //
  // Initialize the strings for the browser, upon exit of the browser, the strings will be freed
  //
  InitializeBrowserStrings ();

  gFunctionKeySetting = DEFAULT_FUNCTION_KEY_SETTING;
  gClassOfVfr         = EFI_SETUP_APPLICATION_SUBCLASS;

  //
  // Ensure we are in Text mode
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));

  Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Set console control to text mode.
  //
  ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenText);

  for (Index = 0; Index < HandleCount; Index++) {
    Selection = AllocateZeroPool (sizeof (UI_MENU_SELECTION));
    ASSERT (Selection != NULL);

    Selection->Handle = Handles[Index];
    if (FormSetGuid != NULL) {
      CopyMem (&Selection->FormSetGuid, FormSetGuid, sizeof (EFI_GUID));
      Selection->FormId = FormId;
    }

    do {
      FormSet = AllocateZeroPool (sizeof (FORM_BROWSER_FORMSET));
      ASSERT (FormSet != NULL);

      //
      // Initialize internal data structures of FormSet
      //
      Status = InitializeFormSet (Selection->Handle, &Selection->FormSetGuid, FormSet);
      if (EFI_ERROR (Status) || IsListEmpty (&FormSet->FormListHead)) {
        DestroyFormSet (FormSet);
        break;
      }
      Selection->FormSet = FormSet;

      //
      // Initialize current settings of Questions in this FormSet
      //
      Status = InitializeCurrentSetting (FormSet);
      if (EFI_ERROR (Status)) {
        DestroyFormSet (FormSet);
        break;
      }

      //
      // Display this formset
      //
      gCurrentSelection = Selection;

      Status = SetupBrowser (Selection);

      gCurrentSelection = NULL;
      DestroyFormSet (FormSet);

      if (EFI_ERROR (Status)) {
        break;
      }

    } while (Selection->Action == UI_ACTION_REFRESH_FORMSET);

    FreePool (Selection);
  }

  if (ActionRequest != NULL) {
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    if (gResetRequired) {
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
    }
  }

  FreeBrowserStrings ();

  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  gST->ConOut->ClearScreen (gST->ConOut);

  return Status;
}


/**
  This function is called by a callback handler to retrieve uncommitted state
  data from the browser.

  @param  This                   A pointer to the EFI_FORM_BROWSER2_PROTOCOL
                                 instance.
  @param  ResultsDataSize        A pointer to the size of the buffer associated
                                 with ResultsData.
  @param  ResultsData            A string returned from an IFR browser or
                                 equivalent. The results string will have no
                                 routing information in them.
  @param  RetrieveData           A BOOLEAN field which allows an agent to retrieve
                                 (if RetrieveData = TRUE) data from the uncommitted
                                 browser state information or set (if RetrieveData
                                 = FALSE) data in the uncommitted browser state
                                 information.
  @param  VariableGuid           An optional field to indicate the target variable
                                 GUID name to use.
  @param  VariableName           An optional field to indicate the target
                                 human-readable variable name.

  @retval EFI_SUCCESS            The results have been distributed or are awaiting
                                 distribution.
  @retval EFI_BUFFER_TOO_SMALL   The ResultsDataSize specified was too small to
                                 contain the results data.

**/
EFI_STATUS
EFIAPI
BrowserCallback (
  IN CONST EFI_FORM_BROWSER2_PROTOCOL  *This,
  IN OUT UINTN                         *ResultsDataSize,
  IN OUT EFI_STRING                    ResultsData,
  IN BOOLEAN                           RetrieveData,
  IN CONST EFI_GUID                    *VariableGuid, OPTIONAL
  IN CONST CHAR16                      *VariableName  OPTIONAL
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Link;
  FORMSET_STORAGE       *Storage;
  FORM_BROWSER_FORMSET  *FormSet;
  BOOLEAN               Found;
  CHAR16                *ConfigResp;
  CHAR16                *StrPtr;
  UINTN                 BufferSize;
  UINTN                 TmpSize;

  if (ResultsDataSize == NULL || ResultsData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (gCurrentSelection == NULL) {
    return EFI_NOT_READY;
  }

  Storage = NULL;
  ConfigResp = NULL;
  FormSet = gCurrentSelection->FormSet;

  //
  // Find target storage
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  if (IsNull (&FormSet->StorageListHead, Link)) {
    return EFI_UNSUPPORTED;
  }

  if (VariableGuid != NULL) {
    //
    // Try to find target storage
    //
    Found = FALSE;
    while (!IsNull (&FormSet->StorageListHead, Link)) {
      Storage = FORMSET_STORAGE_FROM_LINK (Link);
      Link = GetNextNode (&FormSet->StorageListHead, Link);

      if (CompareGuid (&Storage->Guid, (EFI_GUID *) VariableGuid)) {
        if (Storage->Type == EFI_HII_VARSTORE_BUFFER) {
          //
          // Buffer storage require both GUID and Name
          //
          if (VariableName == NULL) {
            return EFI_NOT_FOUND;
          }

          if (StrCmp (Storage->Name, (CHAR16 *) VariableName) != 0) {
            continue;
          }
        }
        Found = TRUE;
        break;
      }
    }

    if (!Found) {
      return EFI_NOT_FOUND;
    }
  } else {
    //
    // GUID/Name is not specified, take the first storage in FormSet
    //
    Storage = FORMSET_STORAGE_FROM_LINK (Link);
  }

  if (RetrieveData) {
    //
    // Skip if there is no RequestElement
    //
    if (Storage->ElementCount == 0) {
      return EFI_SUCCESS;
    }

    //
    // Generate <ConfigResp>
    //
    Status = StorageToConfigResp (Storage, &ConfigResp);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Skip <ConfigHdr> and '&' to point to <ConfigBody>
    //
    StrPtr = ConfigResp + StrLen (Storage->ConfigHdr) + 1;

    BufferSize = StrSize (StrPtr);
    if (*ResultsDataSize < BufferSize) {
      *ResultsDataSize = BufferSize;

      FreePool (ConfigResp);
      return EFI_BUFFER_TOO_SMALL;
    }

    *ResultsDataSize = BufferSize;
    CopyMem (ResultsData, StrPtr, BufferSize);

    FreePool (ConfigResp);
  } else {
    //
    // Prepare <ConfigResp>
    //
    TmpSize = StrLen (ResultsData);
    BufferSize = (TmpSize + StrLen (Storage->ConfigHdr) + 2) * sizeof (CHAR16);
    ConfigResp = AllocateZeroPool (BufferSize);
    ASSERT (ConfigResp != NULL);

    StrCpy (ConfigResp, Storage->ConfigHdr);
    StrCat (ConfigResp, L"&");
    StrCat (ConfigResp, ResultsData);

    //
    // Update Browser uncommited data
    //
    Status = ConfigRespToStorage (Storage, ConfigResp);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
  Initialize Setup Browser driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval EFI_SUCCESS    The Setup Browser module is initialized correctly..
  @return Other value if failed to initialize the Setup Browser module.

**/
EFI_STATUS
EFIAPI
InitializeSetup (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  HiiDriverHandle;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;

  //
  // Locate required Hii relative protocols
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &mHiiDatabase
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **) &mHiiString
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &mHiiConfigRouting
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data
  //
  Status = HiiLibCreateHiiDriverHandle (&HiiDriverHandle);
  ASSERT_EFI_ERROR (Status);

  PackageList = HiiLibPreparePackageList (1, &gSetupBrowserGuid, SetupBrowserStrings);
  ASSERT (PackageList != NULL);
  Status = mHiiDatabase->NewPackageList (
                           mHiiDatabase,
                           PackageList,
                           HiiDriverHandle,
                           &gHiiHandle
                           );
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize Driver private data
  //
  BannerData = AllocateZeroPool (sizeof (BANNER_DATA));
  ASSERT (BannerData != NULL);

  //
  // Install FormBrowser2 protocol
  //
  mPrivateData.Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiFormBrowser2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.FormBrowser2
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install Print protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiPrint2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.Print
                  );

  //
  // Install Ecp Print protocol, which is defined in
  // Edk\Foundation\Protocol\Print\Print.h with protocol
  // GUID of { 0xdf2d868e, 0x32fc, 0x4cf0, {0x8e, 0x6b, 0xff, 0xd9, 0x5d, 0x13, 0x43, 0xd0 }}
  // This is support previous module that written to consume this protocol.
  // 
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiPrintProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.Print
                  );

  return Status;
}


/**
  Create a new string in HII Package List.

  @param  String                 The String to be added
  @param  HiiHandle              The package list in the HII database to insert the
                                 specified string.

  @return The output string.

**/
EFI_STRING_ID
NewString (
  IN  CHAR16                   *String,
  IN  EFI_HII_HANDLE           HiiHandle
  )
{
  EFI_STRING_ID  StringId;
  EFI_STATUS     Status;

  StringId = 0;
  Status = HiiLibNewString (HiiHandle, &StringId, String);
  ASSERT_EFI_ERROR (Status);

  return StringId;
}


/**
  Delete a string from HII Package List.

  @param  StringId               Id of the string in HII database.
  @param  HiiHandle              The HII package list handle.

  @retval EFI_SUCCESS            The string was deleted successfully.

**/
EFI_STATUS
DeleteString (
  IN  EFI_STRING_ID            StringId,
  IN  EFI_HII_HANDLE           HiiHandle
  )
{
  CHAR16  NullChar;

  NullChar = CHAR_NULL;
  return HiiLibSetString (HiiHandle, StringId, &NullChar);
}


/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STATUS  Status;
  CHAR16      *String;
  UINTN       BufferLength;

  //
  // Set default string size assumption at no more than 256 bytes
  //
  BufferLength = 0x100;
  String = AllocateZeroPool (BufferLength);
  ASSERT (String != NULL);

  Status = HiiLibGetString (HiiHandle, Token, String, &BufferLength);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (String);
    String = AllocateZeroPool (BufferLength);
    ASSERT (String != NULL);

    Status = HiiLibGetString (HiiHandle, Token, String, &BufferLength);
  }
  ASSERT_EFI_ERROR (Status);

  return String;
}


/**
  Allocate new memory and then copy the Unicode string Source to Destination.

  @param  Dest                   Location to copy string
  @param  Src                    String to copy

**/
VOID
NewStringCpy (
  IN OUT CHAR16       **Dest,
  IN CHAR16           *Src
  )
{
  if (*Dest != NULL) {
    FreePool (*Dest);
  }
  *Dest = AllocateCopyPool (StrSize (Src), Src);
  ASSERT (*Dest != NULL);
}


/**
  Allocate new memory and concatinate Source on the end of Destination.

  @param  Dest                   String to added to the end of.
  @param  Src                    String to concatinate.

**/
VOID
NewStringCat (
  IN OUT CHAR16       **Dest,
  IN CHAR16           *Src
  )
{
  CHAR16  *NewString;
  UINTN   TmpSize;

  if (*Dest == NULL) {
    NewStringCpy (Dest, Src);
    return;
  }

  TmpSize = StrSize (*Dest);
  NewString = AllocateZeroPool (TmpSize + StrSize (Src) - 1);
  ASSERT (NewString != NULL);

  StrCpy (NewString, *Dest);
  StrCat (NewString, Src);

  FreePool (*Dest);
  *Dest = NewString;
}


/**
  Synchronize Storage's Edit copy to Shadow copy.

  @param  Storage                The Storage to be synchronized.

**/
VOID
SynchronizeStorage (
  IN FORMSET_STORAGE         *Storage
  )
{
  LIST_ENTRY              *Link;
  NAME_VALUE_NODE         *Node;

  switch (Storage->Type) {
  case EFI_HII_VARSTORE_BUFFER:
    CopyMem (Storage->Buffer, Storage->EditBuffer, Storage->Size);
    break;

  case EFI_HII_VARSTORE_NAME_VALUE:
    Link = GetFirstNode (&Storage->NameValueListHead);
    while (!IsNull (&Storage->NameValueListHead, Link)) {
      Node = NAME_VALUE_NODE_FROM_LINK (Link);

      NewStringCpy (&Node->Value, Node->EditValue);

      Link = GetNextNode (&Storage->NameValueListHead, Link);
    }
    break;

  case EFI_HII_VARSTORE_EFI_VARIABLE:
  default:
    break;
  }
}


/**
  Get Value for given Name from a NameValue Storage.

  @param  Storage                The NameValue Storage.
  @param  Name                   The Name.
  @param  Value                  The retured Value.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
GetValueByName (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  *Name,
  IN OUT CHAR16              **Value
  )
{
  LIST_ENTRY              *Link;
  NAME_VALUE_NODE         *Node;

  *Value = NULL;

  Link = GetFirstNode (&Storage->NameValueListHead);
  while (!IsNull (&Storage->NameValueListHead, Link)) {
    Node = NAME_VALUE_NODE_FROM_LINK (Link);

    if (StrCmp (Name, Node->Name) == 0) {
      NewStringCpy (Value, Node->EditValue);
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Storage->NameValueListHead, Link);
  }

  return EFI_NOT_FOUND;
}


/**
  Set Value of given Name in a NameValue Storage.

  @param  Storage                The NameValue Storage.
  @param  Name                   The Name.
  @param  Value                  The Value to set.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
SetValueByName (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  *Name,
  IN CHAR16                  *Value
  )
{
  LIST_ENTRY              *Link;
  NAME_VALUE_NODE         *Node;

  Link = GetFirstNode (&Storage->NameValueListHead);
  while (!IsNull (&Storage->NameValueListHead, Link)) {
    Node = NAME_VALUE_NODE_FROM_LINK (Link);

    if (StrCmp (Name, Node->Name) == 0) {
      if (Node->EditValue != NULL) {
        FreePool (Node->EditValue);
      }
      Node->EditValue = AllocateCopyPool (StrSize (Value), Value);
      ASSERT (Node->EditValue != NULL);
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Storage->NameValueListHead, Link);
  }

  return EFI_NOT_FOUND;
}


/**
  Convert setting of Buffer Storage or NameValue Storage to <ConfigResp>.

  @param  Storage                The Storage to be conveted.
  @param  ConfigResp             The returned <ConfigResp>.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
StorageToConfigResp (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  **ConfigResp
  )
{
  EFI_STATUS  Status;
  EFI_STRING  Progress;
  LIST_ENTRY              *Link;
  NAME_VALUE_NODE         *Node;

  Status = EFI_SUCCESS;

  switch (Storage->Type) {
  case EFI_HII_VARSTORE_BUFFER:
    Status = mHiiConfigRouting->BlockToConfig (
                                  mHiiConfigRouting,
                                  Storage->ConfigRequest,
                                  Storage->EditBuffer,
                                  Storage->Size,
                                  ConfigResp,
                                  &Progress
                                  );
    break;

  case EFI_HII_VARSTORE_NAME_VALUE:
    *ConfigResp = NULL;
    NewStringCat (ConfigResp, Storage->ConfigHdr);

    Link = GetFirstNode (&Storage->NameValueListHead);
    while (!IsNull (&Storage->NameValueListHead, Link)) {
      Node = NAME_VALUE_NODE_FROM_LINK (Link);

      NewStringCat (ConfigResp, L"&");
      NewStringCat (ConfigResp, Node->Name);
      NewStringCat (ConfigResp, L"=");
      NewStringCat (ConfigResp, Node->EditValue);

      Link = GetNextNode (&Storage->NameValueListHead, Link);
    }
    break;

  case EFI_HII_VARSTORE_EFI_VARIABLE:
  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}


/**
  Convert <ConfigResp> to settings in Buffer Storage or NameValue Storage.

  @param  Storage                The Storage to receive the settings.
  @param  ConfigResp             The <ConfigResp> to be converted.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
ConfigRespToStorage (
  IN FORMSET_STORAGE         *Storage,
  IN CHAR16                  *ConfigResp
  )
{
  EFI_STATUS  Status;
  EFI_STRING  Progress;
  UINTN       BufferSize;
  CHAR16      *StrPtr;
  CHAR16      *Name;
  CHAR16      *Value;

  Status = EFI_SUCCESS;

  switch (Storage->Type) {
  case EFI_HII_VARSTORE_BUFFER:
    BufferSize = Storage->Size;
    Status = mHiiConfigRouting->ConfigToBlock (
                                  mHiiConfigRouting,
                                  ConfigResp,
                                  Storage->EditBuffer,
                                  &BufferSize,
                                  &Progress
                                  );
    break;

  case EFI_HII_VARSTORE_NAME_VALUE:
    StrPtr = StrStr (ConfigResp, L"&");
    while (StrPtr != NULL) {
      //
      // Skip '&'
      //
      StrPtr = StrPtr + 1;
      Name = StrPtr;
      StrPtr = StrStr (StrPtr, L"=");
      if (StrPtr == NULL) {
        break;
      }
      *StrPtr = 0;

      //
      // Skip '='
      //
      StrPtr = StrPtr + 1;
      Value = StrPtr;
      StrPtr = StrStr (StrPtr, L"&");
      if (StrPtr != NULL) {
        *StrPtr = 0;
      }
      SetValueByName (Storage, Name, Value);
    }
    break;

  case EFI_HII_VARSTORE_EFI_VARIABLE:
  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}


/**
  Get Question's current Value.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               Question to be initialized.
  @param  Cached                 TRUE:  get from Edit copy FALSE: get from original
                                 Storage

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
GetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN BOOLEAN                          Cached
  )
{
  EFI_STATUS          Status;
  BOOLEAN             Enabled;
  BOOLEAN             Pending;
  UINT8               *Dst;
  UINTN               StorageWidth;
  EFI_TIME            EfiTime;
  FORMSET_STORAGE     *Storage;
  EFI_IFR_TYPE_VALUE  *QuestionValue;
  CHAR16              *ConfigRequest;
  CHAR16              *Progress;
  CHAR16              *Result;
  CHAR16              *Value;
  CHAR16              *StringPtr;
  UINTN               Length;
  BOOLEAN             IsBufferStorage;
  BOOLEAN             IsString;

  Status = EFI_SUCCESS;

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    return Status;
  }

  //
  // Question value is provided by an Expression, evaluate it
  //
  if (Question->ValueExpression != NULL) {
    Status = EvaluateExpression (FormSet, Form, Question->ValueExpression);
    if (!EFI_ERROR (Status)) {
      CopyMem (&Question->HiiValue, &Question->ValueExpression->Result, sizeof (EFI_HII_VALUE));
    }
    return Status;
  }

  //
  // Question value is provided by RTC
  //
  Storage = Question->Storage;
  QuestionValue = &Question->HiiValue.Value;
  if (Storage == NULL) {
    //
    // It's a Question without storage, or RTC date/time
    //
    if (Question->Operand == EFI_IFR_DATE_OP || Question->Operand == EFI_IFR_TIME_OP) {
      //
      // Date and time define the same Flags bit
      //
      switch (Question->Flags & EFI_QF_DATE_STORAGE) {
      case QF_DATE_STORAGE_TIME:
        Status = gRT->GetTime (&EfiTime, NULL);
        break;

      case QF_DATE_STORAGE_WAKEUP:
        Status = gRT->GetWakeupTime (&Enabled, &Pending, &EfiTime);
        break;

      case QF_DATE_STORAGE_NORMAL:
      default:
        //
        // For date/time without storage
        //
        return EFI_SUCCESS;
      }

      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (Question->Operand == EFI_IFR_DATE_OP) {
        QuestionValue->date.Year  = EfiTime.Year;
        QuestionValue->date.Month = EfiTime.Month;
        QuestionValue->date.Day   = EfiTime.Day;
      } else {
        QuestionValue->time.Hour   = EfiTime.Hour;
        QuestionValue->time.Minute = EfiTime.Minute;
        QuestionValue->time.Second = EfiTime.Second;
      }
    }

    return EFI_SUCCESS;
  }

  //
  // Question value is provided by EFI variable
  //
  StorageWidth = Question->StorageWidth;
  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    if (Question->BufferValue != NULL) {
      Dst = Question->BufferValue;
    } else {
      Dst = (UINT8 *) QuestionValue;
    }

    Status = gRT->GetVariable (
                     Question->VariableName,
                     &Storage->Guid,
                     NULL,
                     &StorageWidth,
                     Dst
                     );
    //
    // Always return success, even this EFI variable doesn't exist
    //
    return EFI_SUCCESS;
  }

  //
  // Question Value is provided by Buffer Storage or NameValue Storage
  //
  if (Question->BufferValue != NULL) {
    //
    // This Question is password or orderedlist
    //
    Dst = Question->BufferValue;
  } else {
    //
    // Other type of Questions
    //
    Dst = (UINT8 *) &Question->HiiValue.Value;
  }

  IsBufferStorage = (BOOLEAN) ((Storage->Type == EFI_HII_VARSTORE_BUFFER) ? TRUE : FALSE);
  IsString = (BOOLEAN) ((Question->HiiValue.Type == EFI_IFR_TYPE_STRING) ?  TRUE : FALSE);
  if (Cached) {
    if (IsBufferStorage) {
      //
      // Copy from storage Edit buffer
      //
      CopyMem (Dst, Storage->EditBuffer + Question->VarStoreInfo.VarOffset, StorageWidth);
    } else {
      Status = GetValueByName (Storage, Question->VariableName, &Value);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (IsString) {
        //
        // Convert Config String to Unicode String, e.g "0041004200430044" => "ABCD"
        //
        Length = StorageWidth + sizeof (CHAR16);
        Status = ConfigStringToUnicode ((CHAR16 *) Dst, &Length, Value);
      } else {
        Status = HexStringToBuf (Dst, &StorageWidth, Value, NULL);
      }

      FreePool (Value);
    }
  } else {
    //
    // Request current settings from Configuration Driver
    //
    if (FormSet->ConfigAccess == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // <ConfigRequest> ::= <ConfigHdr> + <BlockName> ||
    //                   <ConfigHdr> + "&" + <VariableName>
    //
    if (IsBufferStorage) {
      Length = StrLen (Storage->ConfigHdr);
      Length += StrLen (Question->BlockName);
    } else {
      Length = StrLen (Storage->ConfigHdr);
      Length += StrLen (Question->VariableName) + 1;
    }
    ConfigRequest = AllocateZeroPool ((Length + 1) * sizeof (CHAR16));
    ASSERT (ConfigRequest != NULL);

    StrCpy (ConfigRequest, Storage->ConfigHdr);
    if (IsBufferStorage) {
      StrCat (ConfigRequest, Question->BlockName);
    } else {
      StrCat (ConfigRequest, L"&");
      StrCat (ConfigRequest, Question->VariableName);
    }

    Status = FormSet->ConfigAccess->ExtractConfig (
                                      FormSet->ConfigAccess,
                                      ConfigRequest,
                                      &Progress,
                                      &Result
                                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Skip <ConfigRequest>
    //
    Value = Result + Length;
    if (IsBufferStorage) {
      //
      // Skip "&VALUE"
      //
      Value = Value + 6;
    }
    if (*Value != '=') {
      FreePool (Result);
      return EFI_NOT_FOUND;
    }
    //
    // Skip '=', point to value
    //
    Value = Value + 1;

    //
    // Suppress <AltResp> if any
    //
    StringPtr = Value;
    while (*StringPtr != L'\0' && *StringPtr != L'&') {
      StringPtr++;
    }
    *StringPtr = L'\0';

    if (!IsBufferStorage && IsString) {
      //
      // Convert Config String to Unicode String, e.g "0041004200430044" => "ABCD"
      //
      Length = StorageWidth + sizeof (CHAR16);
      Status = ConfigStringToUnicode ((CHAR16 *) Dst, &Length, Value);
    } else {
      Status = HexStringToBuf (Dst, &StorageWidth, Value, NULL);
      if (EFI_ERROR (Status)) {
        FreePool (Result);
        return Status;
      }
    }

    //
    // Synchronize Edit Buffer
    //
    if (IsBufferStorage) {
      CopyMem (Storage->EditBuffer + Question->VarStoreInfo.VarOffset, Dst, StorageWidth);
    } else {
      SetValueByName (Storage, Question->VariableName, Value);
    }
    FreePool (Result);
  }

  return Status;
}


/**
  Save Question Value to edit copy(cached) or Storage(uncached).

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               Pointer to the Question.
  @param  Cached                 TRUE:  set to Edit copy FALSE: set to original
                                 Storage

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
SetQuestionValue (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN OUT FORM_BROWSER_STATEMENT       *Question,
  IN BOOLEAN                          Cached
  )
{
  EFI_STATUS          Status;
  BOOLEAN             Enabled;
  BOOLEAN             Pending;
  UINT8               *Src;
  EFI_TIME            EfiTime;
  UINTN               BufferLen;
  UINTN               StorageWidth;
  FORMSET_STORAGE     *Storage;
  EFI_IFR_TYPE_VALUE  *QuestionValue;
  CHAR16              *ConfigResp;
  CHAR16              *Progress;
  CHAR16              *Value;
  UINTN               Length;
  BOOLEAN             IsBufferStorage;
  BOOLEAN             IsString;

  Status = EFI_SUCCESS;

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    return Status;
  }

  //
  // If Question value is provided by an Expression, then it is read only
  //
  if (Question->ValueExpression != NULL) {
    return Status;
  }

  //
  // Question value is provided by RTC
  //
  Storage = Question->Storage;
  QuestionValue = &Question->HiiValue.Value;
  if (Storage == NULL) {
    //
    // It's a Question without storage, or RTC date/time
    //
    if (Question->Operand == EFI_IFR_DATE_OP || Question->Operand == EFI_IFR_TIME_OP) {
      //
      // Date and time define the same Flags bit
      //
      switch (Question->Flags & EFI_QF_DATE_STORAGE) {
      case QF_DATE_STORAGE_TIME:
        Status = gRT->GetTime (&EfiTime, NULL);
        break;

      case QF_DATE_STORAGE_WAKEUP:
        Status = gRT->GetWakeupTime (&Enabled, &Pending, &EfiTime);
        break;

      case QF_DATE_STORAGE_NORMAL:
      default:
        //
        // For date/time without storage
        //
        return EFI_SUCCESS;
      }

      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (Question->Operand == EFI_IFR_DATE_OP) {
        EfiTime.Year  = QuestionValue->date.Year;
        EfiTime.Month = QuestionValue->date.Month;
        EfiTime.Day   = QuestionValue->date.Day;
      } else {
        EfiTime.Hour   = QuestionValue->time.Hour;
        EfiTime.Minute = QuestionValue->time.Minute;
        EfiTime.Second = QuestionValue->time.Second;
      }

      if ((Question->Flags & EFI_QF_DATE_STORAGE) == QF_DATE_STORAGE_TIME) {
        Status = gRT->SetTime (&EfiTime);
      } else {
        Status = gRT->SetWakeupTime (TRUE, &EfiTime);
      }
    }

    return Status;
  }

  //
  // Question value is provided by EFI variable
  //
  StorageWidth = Question->StorageWidth;
  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    if (Question->BufferValue != NULL) {
      Src = Question->BufferValue;
    } else {
      Src = (UINT8 *) QuestionValue;
    }

    Status = gRT->SetVariable (
                     Question->VariableName,
                     &Storage->Guid,
                     Storage->Attributes,
                     StorageWidth,
                     Src
                     );
    return Status;
  }

  //
  // Question Value is provided by Buffer Storage or NameValue Storage
  //
  if (Question->BufferValue != NULL) {
    Src = Question->BufferValue;
  } else {
    Src = (UINT8 *) &Question->HiiValue.Value;
  }

  IsBufferStorage = (BOOLEAN) ((Storage->Type == EFI_HII_VARSTORE_BUFFER) ? TRUE : FALSE);
  IsString = (BOOLEAN) ((Question->HiiValue.Type == EFI_IFR_TYPE_STRING) ?  TRUE : FALSE);
  if (IsBufferStorage) {
    //
    // Copy to storage edit buffer
    //
    CopyMem (Storage->EditBuffer + Question->VarStoreInfo.VarOffset, Src, StorageWidth);
  } else {
    if (IsString) {
      //
      // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
      //
      Value = NULL;
      BufferLen = ((StrLen ((CHAR16 *) Src) * 4) + 1) * sizeof (CHAR16);
      Value = AllocateZeroPool (BufferLen);
      ASSERT (Value != NULL);
      Status = UnicodeToConfigString (Value, &BufferLen, (CHAR16 *) Src);
      ASSERT_EFI_ERROR (Status);
    } else {
      BufferLen = StorageWidth * 2 + 1;
      Value = AllocateZeroPool (BufferLen * sizeof (CHAR16));
      ASSERT (Value != NULL);
      BufToHexString (Value, &BufferLen, Src, StorageWidth);
      ToLower (Value);
    }

    Status = SetValueByName (Storage, Question->VariableName, Value);
    FreePool (Value);
  }

  if (!Cached) {
    //
    // <ConfigResp> ::= <ConfigHdr> + <BlockName> + "&VALUE=" + "<HexCh>StorageWidth * 2" ||
    //                <ConfigHdr> + "&" + <VariableName> + "=" + "<string>"
    //
    if (IsBufferStorage) {
      Length = StrLen (Question->BlockName) + 7;
    } else {
      Length = StrLen (Question->VariableName) + 2;
    }
    if (!IsBufferStorage && IsString) {
      Length += (StrLen ((CHAR16 *) Src) * 4);
    } else {
      Length += (StorageWidth * 2);
    }
    ConfigResp = AllocateZeroPool ((StrLen (Storage->ConfigHdr) + Length + 1) * sizeof (CHAR16));
    ASSERT (ConfigResp != NULL);

    StrCpy (ConfigResp, Storage->ConfigHdr);
    if (IsBufferStorage) {
      StrCat (ConfigResp, Question->BlockName);
      StrCat (ConfigResp, L"&VALUE=");
    } else {
      StrCat (ConfigResp, L"&");
      StrCat (ConfigResp, Question->VariableName);
      StrCat (ConfigResp, L"=");
    }

    Value = ConfigResp + StrLen (ConfigResp);
    if (!IsBufferStorage && IsString) {
      //
      // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
      //
      BufferLen = ((StrLen ((CHAR16 *) Src) * 4) + 1) * sizeof (CHAR16);
      Status = UnicodeToConfigString (Value, &BufferLen, (CHAR16 *) Src);
      ASSERT_EFI_ERROR (Status);
    } else {
      BufferLen = StorageWidth * 2 + 1;
      BufToHexString (Value, &BufferLen, Src, StorageWidth);
      ToLower (Value);
    }

    //
    // Submit Question Value to Configuration Driver
    //
    if (FormSet->ConfigAccess != NULL) {
      Status = FormSet->ConfigAccess->RouteConfig (
                                        FormSet->ConfigAccess,
                                        ConfigResp,
                                        &Progress
                                        );
      if (EFI_ERROR (Status)) {
        FreePool (ConfigResp);
        return Status;
      }
    }
    FreePool (ConfigResp);

    //
    // Synchronize shadow Buffer
    //
    SynchronizeStorage (Storage);
  }

  return Status;
}


/**
  Perform inconsistent check for a Form.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  Question               The Question to be validated.
  @param  Type                   Validation type: InConsistent or NoSubmit

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
ValidateQuestion (
  IN  FORM_BROWSER_FORMSET            *FormSet,
  IN  FORM_BROWSER_FORM               *Form,
  IN  FORM_BROWSER_STATEMENT          *Question,
  IN  UINTN                           Type
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  LIST_ENTRY              *ListHead;
  EFI_STRING              PopUp;
  EFI_INPUT_KEY           Key;
  FORM_EXPRESSION         *Expression;

  if (Type == EFI_HII_EXPRESSION_INCONSISTENT_IF) {
    ListHead = &Question->InconsistentListHead;
  } else if (Type == EFI_HII_EXPRESSION_NO_SUBMIT_IF) {
    ListHead = &Question->NoSubmitListHead;
  } else {
    return EFI_UNSUPPORTED;
  }

  Link = GetFirstNode (ListHead);
  while (!IsNull (ListHead, Link)) {
    Expression = FORM_EXPRESSION_FROM_LINK (Link);

    //
    // Evaluate the expression
    //
    Status = EvaluateExpression (FormSet, Form, Expression);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Expression->Result.Value.b) {
      //
      // Condition meet, show up error message
      //
      if (Expression->Error != 0) {
        PopUp = GetToken (Expression->Error, FormSet->HiiHandle);
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, PopUp, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
        FreePool (PopUp);
      }

      return EFI_NOT_READY;
    }

    Link = GetNextNode (ListHead, Link);
  }

  return EFI_SUCCESS;
}


/**
  Perform NoSubmit check for a Form.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
NoSubmitCheck (
  IN  FORM_BROWSER_FORMSET            *FormSet,
  IN  FORM_BROWSER_FORM               *Form
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Question;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    Status = ValidateQuestion (FormSet, Form, Question, EFI_HII_EXPRESSION_NO_SUBMIT_IF);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (&Form->StatementListHead, Link);
  }

  return EFI_SUCCESS;
}


/**
  Submit a Form.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
SubmitForm (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  EFI_STRING              ConfigResp;
  EFI_STRING              Progress;
  FORMSET_STORAGE         *Storage;

  //
  // Validate the Form by NoSubmit check
  //
  Status = NoSubmitCheck (FormSet, Form);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Submit Buffer storage or Name/Value storage
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = FORMSET_STORAGE_FROM_LINK (Link);
    Link = GetNextNode (&FormSet->StorageListHead, Link);

    if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
      continue;
    }

    //
    // Skip if there is no RequestElement
    //
    if (Storage->ElementCount == 0) {
      continue;
    }

    //
    // Prepare <ConfigResp>
    //
    Status = StorageToConfigResp (Storage, &ConfigResp);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Send <ConfigResp> to Configuration Driver
    //
    if (FormSet->ConfigAccess != NULL) {
      Status = FormSet->ConfigAccess->RouteConfig (
                                        FormSet->ConfigAccess,
                                        ConfigResp,
                                        &Progress
                                        );
      if (EFI_ERROR (Status)) {
        FreePool (ConfigResp);
        return Status;
      }
    }
    FreePool (ConfigResp);

    //
    // Config success, update storage shadow Buffer
    //
    SynchronizeStorage (Storage);
  }

  gNvUpdateRequired = FALSE;

  return EFI_SUCCESS;
}


/**
  Reset Question to its default value.

  @param  FormSet                The form set.
  @param  Form                   The form.
  @param  Question               The question.
  @param  DefaultId              The Class of the default.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  QUESTION_DEFAULT        *Default;
  QUESTION_OPTION         *Option;
  EFI_HII_VALUE           *HiiValue;
  UINT8                   Index;

  Status = EFI_SUCCESS;

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    return Status;
  }

  //
  // There are three ways to specify default value for a Question:
  //  1, use nested EFI_IFR_DEFAULT (highest priority)
  //  2, set flags of EFI_ONE_OF_OPTION (provide Standard and Manufacturing default)
  //  3, set flags of EFI_IFR_CHECKBOX (provide Standard and Manufacturing default) (lowest priority)
  //
  HiiValue = &Question->HiiValue;

  //
  // EFI_IFR_DEFAULT has highest priority
  //
  if (!IsListEmpty (&Question->DefaultListHead)) {
    Link = GetFirstNode (&Question->DefaultListHead);
    while (!IsNull (&Question->DefaultListHead, Link)) {
      Default = QUESTION_DEFAULT_FROM_LINK (Link);

      if (Default->DefaultId == DefaultId) {
        if (Default->ValueExpression != NULL) {
          //
          // Default is provided by an Expression, evaluate it
          //
          Status = EvaluateExpression (FormSet, Form, Default->ValueExpression);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          CopyMem (HiiValue, &Default->ValueExpression->Result, sizeof (EFI_HII_VALUE));
        } else {
          //
          // Default value is embedded in EFI_IFR_DEFAULT
          //
          CopyMem (HiiValue, &Default->Value, sizeof (EFI_HII_VALUE));
        }

        return EFI_SUCCESS;
      }

      Link = GetNextNode (&Question->DefaultListHead, Link);
    }
  }

  //
  // EFI_ONE_OF_OPTION
  //
  if ((Question->Operand == EFI_IFR_ONE_OF_OP) && !IsListEmpty (&Question->OptionListHead)) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // OneOfOption could only provide Standard and Manufacturing default
      //
      Link = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Option = QUESTION_OPTION_FROM_LINK (Link);

        if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT) != 0)) ||
            ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT_MFG) != 0))
           ) {
          CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));

          return EFI_SUCCESS;
        }

        Link = GetNextNode (&Question->OptionListHead, Link);
      }
    }
  }

  //
  // EFI_IFR_CHECKBOX - lowest priority
  //
  if (Question->Operand == EFI_IFR_CHECKBOX_OP) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING)  {
      //
      // Checkbox could only provide Standard and Manufacturing default
      //
      if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT) != 0)) ||
          ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Question->Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) != 0))
         ) {
        HiiValue->Value.b = TRUE;
      } else {
        HiiValue->Value.b = FALSE;
      }

      return EFI_SUCCESS;
    }
  }

  //
  // For Questions without default
  //
  switch (Question->Operand) {
  case EFI_IFR_NUMERIC_OP:
    //
    // Take minimal value as numeric's default value
    //
    HiiValue->Value.u64 = Question->Minimum;
    break;

  case EFI_IFR_ONE_OF_OP:
    //
    // Take first oneof option as oneof's default value
    //
    Link = GetFirstNode (&Question->OptionListHead);
    if (!IsNull (&Question->OptionListHead, Link)) {
      Option = QUESTION_OPTION_FROM_LINK (Link);
      CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));
    }
    break;

  case EFI_IFR_ORDERED_LIST_OP:
    //
    // Take option sequence in IFR as ordered list's default value
    //
    Index = 0;
    Link = GetFirstNode (&Question->OptionListHead);
    while (!IsNull (&Question->OptionListHead, Link)) {
      Option = QUESTION_OPTION_FROM_LINK (Link);

      Question->BufferValue[Index] = Option->Value.Value.u8;

      Index++;
      if (Index >= Question->MaxContainers) {
        break;
      }

      Link = GetNextNode (&Question->OptionListHead, Link);
    }
    break;

  default:
    Status = EFI_NOT_FOUND;
    break;
  }

  return Status;
}


/**
  Reset Questions in a Form to their default value.

  @param  FormSet                FormSet data structure.
  @param  Form                   The Form which to be reset.
  @param  DefaultId              The Class of the default.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
ExtractFormDefault (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN UINT16                           DefaultId
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Question;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&Form->StatementListHead, Link);

    //
    // If Question is suppressed, don't reset it to default
    //
    if (Question->SuppressExpression != NULL) {
      Status = EvaluateExpression (FormSet, Form, Question->SuppressExpression);
      if (!EFI_ERROR (Status) && Question->SuppressExpression->Result.Value.b) {
        continue;
      }
    }

    //
    // Reset Question to its default value
    //
    Status = GetQuestionDefault (FormSet, Form, Question, DefaultId);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Synchronize Buffer storage's Edit buffer
    //
    if ((Question->Storage != NULL) &&
        (Question->Storage->Type != EFI_HII_VARSTORE_EFI_VARIABLE)) {
      SetQuestionValue (FormSet, Form, Question, TRUE);
    }
  }

  return EFI_SUCCESS;
}


/**
  Initialize Question's Edit copy from Storage.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
LoadFormConfig (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form
  )
{
  EFI_STATUS              Status;
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Question;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    //
    // Initialize local copy of Value for each Question
    //
    Status = GetQuestionValue (FormSet, Form, Question, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (&Form->StatementListHead, Link);
  }

  return EFI_SUCCESS;
}


/**
  Fill storage's edit copy with settings requested from Configuration Driver.

  @param  FormSet                FormSet data structure.
  @param  Storage                Buffer Storage.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
LoadStorage (
  IN FORM_BROWSER_FORMSET    *FormSet,
  IN FORMSET_STORAGE         *Storage
  )
{
  EFI_STATUS  Status;
  EFI_STRING  Progress;
  EFI_STRING  Result;
  CHAR16      *StrPtr;

  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    return EFI_SUCCESS;
  }

  if (FormSet->ConfigAccess == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Storage->ElementCount == 0) {
    //
    // Skip if there is no RequestElement
    //
    return EFI_SUCCESS;
  }

  //
  // Request current settings from Configuration Driver
  //
  Status = FormSet->ConfigAccess->ExtractConfig (
                                    FormSet->ConfigAccess,
                                    Storage->ConfigRequest,
                                    &Progress,
                                    &Result
                                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert Result from <ConfigAltResp> to <ConfigResp>
  //
  StrPtr = StrStr (Result, L"ALTCFG");
  if (StrPtr != NULL) {
    *StrPtr = L'\0';
  }

  Status = ConfigRespToStorage (Storage, Result);
  FreePool (Result);
  return Status;
}


/**
  Get current setting of Questions.

  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
InitializeCurrentSetting (
  IN OUT FORM_BROWSER_FORMSET             *FormSet
  )
{
  LIST_ENTRY              *Link;
  FORMSET_STORAGE         *Storage;
  FORM_BROWSER_FORM       *Form;
  EFI_STATUS              Status;

  //
  // Extract default from IFR binary
  //
  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);

    Status = ExtractFormDefault (FormSet, Form, EFI_HII_DEFAULT_CLASS_STANDARD);

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }

  //
  // Request current settings from Configuration Driver
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = FORMSET_STORAGE_FROM_LINK (Link);

    Status = LoadStorage (FormSet, Storage);

    //
    // Now Edit Buffer is filled with default values(lower priority) and current
    // settings(higher priority), sychronize it to shadow Buffer
    //
    if (!EFI_ERROR (Status)) {
      SynchronizeStorage (Storage);
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }

  return EFI_SUCCESS;
}


/**
  Fetch the Ifr binary data of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  BinaryLength           The length of the FormSet IFR binary.
  @param  BinaryData             The buffer designed to receive the FormSet.

  @retval EFI_SUCCESS            Buffer filled with the requested FormSet.
                                 BufferLength was updated.
  @retval EFI_INVALID_PARAMETER  The handle is unknown.
  @retval EFI_NOT_FOUND          A form or FormSet on the requested handle cannot
                                 be found with the requested FormId.

**/
EFI_STATUS
GetIfrBinaryData (
  IN  EFI_HII_HANDLE   Handle,
  IN OUT EFI_GUID      *FormSetGuid,
  OUT UINTN            *BinaryLength,
  OUT UINT8            **BinaryData
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  BOOLEAN                      ReturnDefault;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;

  OpCodeData = NULL;
  Package = NULL;
  ZeroMem (&PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));;

  //
  // if FormSetGuid is NULL or zero GUID, return first FormSet in the package list
  //
  if (FormSetGuid == NULL || CompareGuid (FormSetGuid, &gZeroGuid)) {
    ReturnDefault = TRUE;
  } else {
    ReturnDefault = FALSE;
  }

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (HiiPackageList != NULL);
  
  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Check whether return default FormSet
          //
          if (ReturnDefault) {
            break;
          }

          //
          // FormSet GUID is specified, check it
          //
          if (CompareGuid (FormSetGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
            break;
          }
        }

        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }

      if (Offset2 < PackageHeader.Length) {
        //
        // Target formset found
        //
        break;
      }
    }

    Offset += PackageHeader.Length;
  }

  if (Offset >= PackageListLength) {
    //
    // Form package not found in this Package List
    //
    FreePool (HiiPackageList);
    return EFI_NOT_FOUND;
  }

  if (ReturnDefault && FormSetGuid != NULL) {
    //
    // Return the default FormSet GUID
    //
    CopyMem (FormSetGuid, &((EFI_IFR_FORM_SET *) OpCodeData)->Guid, sizeof (EFI_GUID));
  }

  //
  // To determine the length of a whole FormSet IFR binary, one have to parse all the Opcodes
  // in this FormSet; So, here just simply copy the data from start of a FormSet to the end
  // of the Form Package.
  //
  *BinaryLength = PackageHeader.Length - Offset2;
  *BinaryData = AllocateCopyPool (*BinaryLength, OpCodeData);

  FreePool (HiiPackageList);

  if (*BinaryData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}


/**
  Initialize the internal data structure of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
InitializeFormSet (
  IN  EFI_HII_HANDLE                   Handle,
  IN OUT EFI_GUID                      *FormSetGuid,
  OUT FORM_BROWSER_FORMSET             *FormSet
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DriverHandle;
  UINT16                    Index;

  Status = GetIfrBinaryData (Handle, FormSetGuid, &FormSet->IfrBinaryLength, &FormSet->IfrBinaryData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FormSet->HiiHandle = Handle;
  CopyMem (&FormSet->Guid, FormSetGuid, sizeof (EFI_GUID));

  //
  // Retrieve ConfigAccess Protocol associated with this HiiPackageList
  //
  Status = mHiiDatabase->GetPackageListHandle (mHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  FormSet->DriverHandle = DriverHandle;
  Status = gBS->HandleProtocol (
                  DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  (VOID **) &FormSet->ConfigAccess
                  );
  if (EFI_ERROR (Status)) {
    //
    // Configuration Driver don't attach ConfigAccess protocol to its HII package
    // list, then there will be no configuration action required
    //
    FormSet->ConfigAccess = NULL;
  }

  //
  // Parse the IFR binary OpCodes
  //
  Status = ParseOpCodes (FormSet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gClassOfVfr = FormSet->SubClass;
  if (gClassOfVfr == EFI_FRONT_PAGE_SUBCLASS) {
    FrontPageHandle = FormSet->HiiHandle;
  }

  //
  // Match GUID to find out the function key setting. If match fail, use the default setting.
  //
  for (Index = 0; Index < sizeof (gFunctionKeySettingTable) / sizeof (FUNCTIION_KEY_SETTING); Index++) {
    if (CompareGuid (&FormSet->Guid, &(gFunctionKeySettingTable[Index].FormSetGuid))) {
      //
      // Update the function key setting.
      //
      gFunctionKeySetting = gFunctionKeySettingTable[Index].KeySetting;
      //
      // Function key prompt can not be displayed if the function key has been disabled.
      //
      if ((gFunctionKeySetting & FUNCTION_ONE) != FUNCTION_ONE) {
        gFunctionOneString = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
      }

      if ((gFunctionKeySetting & FUNCTION_TWO) != FUNCTION_TWO) {
        gFunctionTwoString = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
      }

      if ((gFunctionKeySetting & FUNCTION_NINE) != FUNCTION_NINE) {
        gFunctionNineString = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
      }

      if ((gFunctionKeySetting & FUNCTION_TEN) != FUNCTION_TEN) {
        gFunctionTenString = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
      }
    }
  }

  return Status;
}
