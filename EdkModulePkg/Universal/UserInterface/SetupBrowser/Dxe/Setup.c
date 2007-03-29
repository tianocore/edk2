/**@file
  Entry and initialization module for the browser.

Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"
#include "Ui.h"

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
  // BMM Formset.
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
  // BMM File Explorer Formset.
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

STATIC
EFI_STATUS
InitializeBinaryStructures (
  IN  EFI_HII_HANDLE                           *Handle,
  IN  BOOLEAN                                  UseDatabase,
  IN  EFI_IFR_PACKET                           *Packet,
  IN  UINT8                                    *NvMapOverride,
  IN  UINTN                                    NumberOfIfrImages,
  EFI_FILE_FORM_TAGS                           **FileFormTagsHead
  );

STATIC
EFI_STATUS
InitializeTagStructures (
  IN  EFI_IFR_BINARY                            *BinaryData,
  OUT EFI_FILE_FORM_TAGS                        *FileFormTags
  );

STATIC
UI_MENU_OPTION        *
DisplayHomePage (
  IN UINTN                                    NumberOfIfrImages,
  IN EFI_FILE_FORM_TAGS                       *FileFormTagsHead,
  IN UINT8                                    *CallbackData
  );

STATIC
EFI_STATUS
GetIfrBinaryData (
  IN EFI_HII_PROTOCOL *Hii,
  IN EFI_HII_HANDLE   HiiHandle,
  IN EFI_IFR_PACKET   *Packet,
  IN EFI_IFR_BINARY   *BinaryData
  );

STATIC
EFI_STATUS
InstallPrint (
  VOID
  );

STATIC
EFI_STATUS
EFIAPI
SendForm (
  IN EFI_FORM_BROWSER_PROTOCOL        * This,
  IN BOOLEAN                          UseDatabase,
  IN EFI_HII_HANDLE                   * Handle,
  IN UINTN                            HandleCount,
  IN EFI_IFR_PACKET                   * Packet,
  IN EFI_HANDLE                       CallbackHandle,
  IN UINT8                            *NvMapOverride,
  IN EFI_SCREEN_DESCRIPTOR            *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                         *ResetRequired OPTIONAL
  )
/*++

Routine Description:

  This is the routine which an external caller uses to direct the browser
  where to obtain it's information.

Arguments:

  UseDatabase -     If set to TRUE, then all information is retrieved from the HII database handle specified
                    If set to FALSE, then the passed in Packet and CallbackHandle is used and Handle is ignored

  Handle -          A pointer to an array of Handles.  If HandleCount > 1 we display a list of the formsets for the handles specified

  HandleCount -     The number of Handles specified in Handle.

  Packet -          Valid only if UseDatabase is FALSE.  Packet defines the pages being passed into
                    the browser.  This is composed of IFR data as well as String information.

  CallbackHandle -  The handle which contains the calling driver's EFI_FORM_CALLBACK_PROTOCOL interface.

  ScreenDimenions - This allows the browser to be called so that it occupies a portion of the physical screen instead of
                    dynamically determining the screen dimensions.

  NvMapOverride -   This buffer is used only when there is no NV variable to define the current settings and the caller
                    needs to provide to the browser the current settings for the "fake" NV variable.  If used, no saving
                    of an NV variable will be possible.  This parameter is also ignored if HandleCount > 1.

Returns:

--*/
{
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_FILE_FORM_TAGS          *FileFormTagsHead;
  UI_MENU_OPTION              *Selection;
  UI_MENU_OPTION              *AltSelection;
  EFI_STATUS                  Status;
  BOOLEAN                     Callback;
  VOID                        *CallbackData;
  EFI_HII_HANDLE              BackupHandle;

  ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  gPreviousValue  = AllocatePool (0x1000);
  CallbackData    = AllocatePool (0x10000);
  ASSERT (gPreviousValue != NULL);
  ASSERT (CallbackData != NULL);

  do {
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
          CopyMem (&gScreenDimensions, ScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
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
    gResetRequired      = FALSE;
    gExitRequired       = FALSE;
    gSaveRequired       = FALSE;
    gNvUpdateRequired   = FALSE;
    gActiveIfr          = 0;
    gConsistencyId      = 0;
    gPriorMenuEntry     = 0;
    BackupHandle        = *Handle;
    gMenuRefreshHead    = NULL;
    ASSERT (CallbackData);
    ZeroMem (CallbackData, 0x10000);

    //
    // We can recurse through this and might need to re-allocate this particular buffer
    //
    if (gPreviousValue == NULL) {
      gPreviousValue = AllocatePool (0x1000);
      ASSERT (gPreviousValue != NULL);
    }

    Callback      = FALSE;
    FormCallback  = NULL;

    if (CallbackHandle != NULL) {
      //
      // Retrieve the Callback protocol interface
      //
      Status = gBS->HandleProtocol (
                      CallbackHandle,
                      &gEfiFormCallbackProtocolGuid,
                      (VOID **) &FormCallback
                      );

      if (EFI_ERROR (Status)) {
        FreePool (CallbackData);
        return Status;;
      }

      Callback = TRUE;
    }
    //
    // Initializes all the internal state structures for all IFR images in system
    //
    Status = InitializeBinaryStructures (Handle, UseDatabase, Packet, NvMapOverride, HandleCount, &FileFormTagsHead);

    if (EFI_ERROR (Status)) {
      FreePool (CallbackData);
      return Status;
    }
    //
    // Beginning of the Presentation of the Data
    //
    if (UseDatabase && (HandleCount > 1)) {
      Selection = DisplayHomePage (HandleCount, FileFormTagsHead, CallbackData);
    } else {
      //
      // If passing something specific, we know there is only one Ifr
      //
      Selection = AllocateZeroPool (sizeof (UI_MENU_OPTION));
      ASSERT (Selection != NULL);
      Selection->IfrNumber  = 0;
      Selection->Handle     = Handle[0];
      UiInitMenu ();
    }

    UiInitMenuList ();

    if (UseDatabase && (HandleCount > 1)) {
      if (Selection == NULL) {
        FreePool (CallbackData);
        return EFI_SUCCESS;
      }
    }
    //
    // Launch the setup browser with the user's selection information
    //
    AltSelection = SetupBrowser (Selection, Callback, FileFormTagsHead, CallbackData);

    //
    // If the caller cares about Reset status, we can return to the caller if something happened that required a reset
    //
    if (ResetRequired != NULL) {
      *ResetRequired = gResetRequired;
    }

    if (Callback && (AltSelection != NULL)) {
      if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
        Status = FormCallback->Callback (
                                FormCallback,
                                AltSelection->ThisTag->Key,
                                CallbackData,
                                (EFI_HII_CALLBACK_PACKET **) &Packet
                                );
      }
    }

    *Handle = BackupHandle;

    if (EFI_ERROR (Status)) {
      FreePool (CallbackData);
      return Status;
    }

    if (Callback && (AltSelection == NULL)) {
      FreePool (CallbackData);
      return Status;
    }

    if (UseDatabase && (HandleCount > 1)) {
    } else {

      if (gBinaryDataHead->UnRegisterOnExit) {
        Hii->RemovePack (Hii, Handle[0]);
      }

      if (Callback &&
        ((AltSelection->ThisTag->SubClass == EFI_FRONT_PAGE_SUBCLASS) ||
        (AltSelection->ThisTag->SubClass == EFI_SINGLE_USE_SUBCLASS))) {
        //
        // If this is the FrontPage, return after every selection
        //
        FreePool (Selection);
        UiFreeMenu ();

        //
        // Clean up the allocated data buffers
        //
        FreeData (FileFormTagsHead, NULL, NULL);

        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->ClearScreen (gST->ConOut);

        FreePool (CallbackData);
        return EFI_SUCCESS;
      }

      FreePool (Selection);
      UiFreeMenu ();

      //
      // Clean up the allocated data buffers
      //
      FreeData (FileFormTagsHead, NULL, NULL);

      gST->ConOut->ClearScreen (gST->ConOut);

      if (!Callback) {
        FreePool (CallbackData);
        return EFI_SUCCESS;
      }
    }

  } while (!EFI_ERROR (Status));

  FreePool (CallbackData);
  return Status;
}

EFI_STATUS
EFIAPI
InitializeSetup (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:
  Initialize Setup

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCCESS - Setup loaded.
  other       - Setup Error

--*/
{
  EFI_STATUS                  Status;
  EFI_FORM_CONFIGURATION_DATA *FormData;
  EFI_FORM_BROWSER_PROTOCOL   *FormBrowser;
  EFI_HANDLE                  Handle;
  EFI_HII_PACKAGES            *PackageList;

  //
  // There will be only one FormConfig in the system
  // If there is another out there, someone is trying to install us
  // again.  Fail that scenario.
  //
  Status = gBS->LocateProtocol (
                  &gEfiFormBrowserProtocolGuid,
                  NULL,
                  (VOID **) &FormBrowser
                  );

  gFirstIn = TRUE;

  //
  // If there was no error, assume there is an installation and fail to load
  //
  if (!EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  FormData = AllocatePool (sizeof (EFI_FORM_CONFIGURATION_DATA));

  if (FormData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Fill in HII data
  //
  FormData->Signature               = EFI_FORM_DATA_SIGNATURE;
  FormData->FormConfig.SendForm     = SendForm;
  FormData->FormConfig.CreatePopUp  = CreateDialog;

  //
  // There should only be one HII image
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  (VOID **) &FormData->Hii
                  );

  ASSERT_EFI_ERROR (Status);

  Hii         = FormData->Hii;

  PackageList = PreparePackages (1, &gEfiFormBrowserProtocolGuid, SetupBrowserStrings);

  Status      = Hii->NewPack (Hii, PackageList, &gHiiHandle);

  FreePool (PackageList);

  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiFormBrowserProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FormData->FormConfig
                  );

  ASSERT_EFI_ERROR (Status);

  BannerData = AllocateZeroPool (sizeof (BANNER_DATA));
  ASSERT (BannerData != NULL);

  Status = InstallPrint ();
  return Status;
}

VOID
GetQuestionHeader (
  IN  EFI_TAG             *Tag,
  IN  UINT8               *RawFormSet,
  IN  UINT16              Index,
  IN  EFI_FILE_FORM_TAGS  *FileFormTags,
  IN  UINT16              CurrentVariable
  )
/*++

Routine Description:
  Initialize question tag's members.

Arguments:
  Tag              - Pointer of the current EFI_TAG structure.
  RawFormSet       - Pointer of the formset raw data.
  Index            - Offset of the current opcode in the Ifr raw data.
  FileFormTags     - Pointer of current EFI_FILE_FORM_TAGS structure.
  CurrentVariable  - Current variable number.

Returns:
  None.
--*/
{
  EFI_VARIABLE_DEFINITION *VariableDefinition;

  Tag->NumberOfLines  = 1;
  Tag->VariableNumber = CurrentVariable;
  CopyMem (&Tag->Id, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->QuestionId, sizeof (UINT16));
  CopyMem (&Tag->StorageStart, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->QuestionId, sizeof (UINT16));
  CopyMem (&Tag->StorageWidth, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Width, sizeof (UINT8));
  CopyMem (&Tag->Text, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Prompt, sizeof (UINT16));
  CopyMem (&Tag->Help, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Help, sizeof (UINT16));

  VariableDefinition = FileFormTags->VariableDefinitions;

  for (; VariableDefinition != NULL; VariableDefinition = VariableDefinition->Next) {
    //
    // Have we found the correct variable for the request?
    //
    if (CurrentVariable == VariableDefinition->VariableId) {
      if (VariableDefinition->VariableSize < (UINTN) (Tag->StorageStart + Tag->StorageWidth)) {
        VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableFakeSize + Tag->StorageWidth);
      }

      if (VariableDefinition->NvRamMap != NULL) {
        //
        // If it is an 8bit or 16bit width, then move it to Tag->Value, otherwise
        // we will never be looking for the data in Tag->Value (e.g. strings, password, etc)
        //
        if (Tag->StorageWidth == (UINT16) 1) {
          CopyMem (&Tag->Value, &VariableDefinition->NvRamMap[Tag->StorageStart], sizeof (UINT16));
        }

        if (Tag->StorageWidth == (UINT16) 2) {
          Index = (UINT16)
            (
              VariableDefinition->NvRamMap[Tag->StorageStart] +
              (VariableDefinition->NvRamMap[Tag->StorageStart + 1] * 0x100)
            );
          CopyMem (&Tag->Value, &Index, sizeof (UINT16));
        }
      } else {
        Index = 0;
        CopyMem (&Tag->Value, &Index, sizeof (UINT16));
      }
      break;
    }
  }
}

VOID
GetNumericHeader (
  IN  EFI_TAG             *Tag,
  IN  UINT8               *RawFormSet,
  IN  UINT16              Index,
  IN  UINT16              NumberOfLines,
  IN  EFI_FILE_FORM_TAGS  *FileFormTags,
  IN  UINT16              CurrentVariable
  )
/*++

Routine Description:
  Initialize numeric tag's members.

Arguments:
  Tag              - Pointer of the current EFI_TAG structure.
  RawFormSet       - Pointer of the formset raw data.
  Index            - Offset of the current opcode in the Ifr raw data.
  NumberOfLines    - Number of lines this opcode occupied.
  FileFormTags     - Pointer of current EFI_FILE_FORM_TAGS structure.
  CurrentVariable  - Current variable number.

Returns:
  None.
--*/
{
  EFI_VARIABLE_DEFINITION *VariableDefinition;

  Tag->NumberOfLines  = NumberOfLines;
  Tag->VariableNumber = CurrentVariable;
  CopyMem (&Tag->Id, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->QuestionId, sizeof (UINT16));
  CopyMem (&Tag->StorageStart, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->QuestionId, sizeof (UINT16));
  CopyMem (&Tag->StorageWidth, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Width, sizeof (UINT8));
  CopyMem (&Tag->Text, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Prompt, sizeof (UINT16));
  CopyMem (&Tag->Help, &((EFI_IFR_ONE_OF *) &RawFormSet[Index])->Help, sizeof (UINT16));
  CopyMem (&Tag->Minimum, &((EFI_IFR_NUMERIC *) &RawFormSet[Index])->Minimum, sizeof (UINT16));
  CopyMem (&Tag->Maximum, &((EFI_IFR_NUMERIC *) &RawFormSet[Index])->Maximum, sizeof (UINT16));
  CopyMem (&Tag->Step, &((EFI_IFR_NUMERIC *) &RawFormSet[Index])->Step, sizeof (UINT16));
  CopyMem (&Tag->Default, &((EFI_IFR_NUMERIC *) &RawFormSet[Index])->Default, sizeof (UINT16));
  Tag->ResetRequired  = (BOOLEAN) (((EFI_IFR_NUMERIC *) &RawFormSet[Index])->Flags & EFI_IFR_FLAG_RESET_REQUIRED);

  VariableDefinition  = FileFormTags->VariableDefinitions;

  for (; VariableDefinition != NULL; VariableDefinition = VariableDefinition->Next) {
    //
    // Have we found the correct variable for the request?
    //
    if (CurrentVariable == VariableDefinition->VariableId) {
      if (VariableDefinition->VariableSize <= (UINTN) (Tag->StorageStart + Tag->StorageWidth)) {
        if (Tag->StorageWidth == 0) {
          VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableFakeSize + 2);
        } else {
          VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableFakeSize + Tag->StorageWidth);
        }
      }

      if (VariableDefinition->NvRamMap != NULL) {
        //
        // If it is an 8bit or 16bit width, then move it to Tag->Value, otherwise
        // we will never be looking for the data in Tag->Value (e.g. strings, password, etc)
        //
        if (Tag->StorageWidth == (UINT16) 1) {
          CopyMem (&Tag->Value, &VariableDefinition->NvRamMap[Tag->StorageStart], sizeof (UINT16));
        }

        if (Tag->StorageWidth == (UINT16) 2) {
          Index = (UINT16)
            (
              VariableDefinition->NvRamMap[Tag->StorageStart] +
                (VariableDefinition->NvRamMap[Tag->StorageStart + 1] * 0x100)
            );
          CopyMem (&Tag->Value, &Index, sizeof (UINT16));
        }
      } else {
        CopyMem (&Tag->Value, &Tag->Default, sizeof (UINT16));
      }
      break;
    }
  }
}

VOID
GetTagCount (
  IN      UINT8                                 *RawFormSet,
  IN OUT  UINT16                                *NumberOfTags
  )
{
  UINT16  Index;

  //
  // Assume on entry we are pointing to an OpCode - reasonably this should
  // be a FormOp since the purpose is to count the tags in a particular Form.
  //
  for (Index = 0; RawFormSet[Index] != EFI_IFR_END_FORM_OP;) {
    //
    // If we encounter the end of a form set, bail out
    //
    if (RawFormSet[Index] == EFI_IFR_END_FORM_SET_OP) {
      break;
    }
    //
    // We treat date/time internally as three op-codes
    //
    if (RawFormSet[Index] == EFI_IFR_DATE_OP || RawFormSet[Index] == EFI_IFR_TIME_OP) {
      *NumberOfTags = (UINT16) (*NumberOfTags + 3);
    } else {
      //
      // Assume that we could have no more tags than op-codes
      //
      (*NumberOfTags)++;
    }

    Index = (UINT16) (Index + RawFormSet[Index + 1]);
  }
  //
  // Increase the tag count by one so it is inclusive of the end_form_op
  //
  (*NumberOfTags)++;
}

STATIC
VOID
AddNextInconsistentTag (
  IN OUT  EFI_INCONSISTENCY_DATA  **InconsistentTagsPtr
  )
/*++

Routine Description:
  Initialize the next inconsistent tag data and add it to the inconsistent tag list.

Arguments:
 InconsistentTagsPtr   - Pointer of the inconsistent tag's pointer.

Returns:
  None.

--*/
{
  EFI_INCONSISTENCY_DATA  *PreviousInconsistentTags;
  EFI_INCONSISTENCY_DATA  *InconsistentTags;

  InconsistentTags = *InconsistentTagsPtr;
  //
  // We just hit the end of an inconsistent expression.  Let's allocate the ->Next structure
  //
  InconsistentTags->Next = AllocatePool (sizeof (EFI_INCONSISTENCY_DATA));
  ASSERT (InconsistentTags->Next != NULL);

  //
  // Preserve current Tag entry
  //
  PreviousInconsistentTags  = InconsistentTags;

  InconsistentTags          = InconsistentTags->Next;

  //
  // This will zero on the entry including the ->Next so I don't have to do it
  //
  ZeroMem (InconsistentTags, sizeof (EFI_INCONSISTENCY_DATA));

  //
  // Point our Previous field to the previous entry
  //
  InconsistentTags->Previous  = PreviousInconsistentTags;

  *InconsistentTagsPtr        = InconsistentTags;

  return ;
}

STATIC
EFI_STATUS
InitializeTagStructures (
  IN  EFI_IFR_BINARY                            *BinaryData,
  OUT EFI_FILE_FORM_TAGS                        *FileFormTags
  )
{
  EFI_STATUS              Status;
  UINT8                   *RawFormSet;
  UINT16                  Index;
  UINT16                  QuestionIndex;
  UINT16                  NumberOfTags;
  INT16                   CurrTag;
  UINT8                   TagLength;
  EFI_FORM_TAGS           *FormTags;
  EFI_FORM_TAGS           *SavedFormTags;
  EFI_INCONSISTENCY_DATA  *InconsistentTags;
  EFI_VARIABLE_DEFINITION *VariableDefinitions;
  UINTN                   Count;
  UINT16                  Class;
  UINT16                  SubClass;
  UINT16                  TempValue;
  UINT16                  CurrentVariable;
  UINT16                  CurrentVariable2;

  //
  // Initialize some Index variable and Status
  //
  Count             = 0;
  Class             = 0;
  SubClass          = 0;
  CurrentVariable   = 0;
  CurrentVariable2  = 0;
  QuestionIndex     = 0;
  NumberOfTags      = 1;
  Status            = EFI_SUCCESS;
  FormTags          = &FileFormTags->FormTags;
  FormTags->Next    = NULL;
  if (FileFormTags->InconsistentTags == NULL) {
    InconsistentTags = NULL;
  } else {
    InconsistentTags = FileFormTags->InconsistentTags;
  }

  if (FileFormTags->VariableDefinitions == NULL) {
    VariableDefinitions = NULL;
  } else {
    VariableDefinitions = FileFormTags->VariableDefinitions;
  }
  //
  // RawFormSet now points to the beginning of the forms portion of
  // the specific IFR Binary.
  //
  RawFormSet = (UINT8 *) BinaryData->FormBinary;

  //
  // Determine the number of tags for the first form
  //
  GetTagCount (&RawFormSet[0], &NumberOfTags);

  SavedFormTags = FormTags;

  if (FormTags->Tags != NULL) {
    do {
      //
      // Advance FormTags to the last entry
      //
      for (; FormTags->Next != NULL; FormTags = FormTags->Next)
        ;

      //
      // Walk through each of the tags and free the IntList allocation
      //
      for (Index = 0; Index < NumberOfTags; Index++) {
        if (FormTags->Tags[Index].IntList != NULL) {
          FreePool (FormTags->Tags[Index].IntList);
        }
      }

      FreePool (FormTags->Tags);

      ASSERT (FormTags->Next == NULL);

      FormTags->Tags  = NULL;

      FormTags        = SavedFormTags;

    } while (FormTags->Next != NULL);
  }

  Index = 0;

  //
  // Test for an allocated buffer.  If already allocated this is due to having called this routine
  // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
  // the tag structure with current values from the NV
  //
  if (FormTags->Tags == NULL) {
    //
    // Allocate memory for our tags on the first form
    //
    FormTags->Tags = AllocateZeroPool (NumberOfTags * sizeof (EFI_TAG));
    ASSERT (FormTags->Tags);
  }
  //
  // Test for an allocated buffer.  If already allocated this is due to having called this routine
  // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
  // the tag structure with current values from the NV
  //
  if (InconsistentTags == NULL) {
    //
    // We just hit the end of an inconsistent expression.  Let's allocate the ->Next structure
    //
    InconsistentTags = AllocateZeroPool (sizeof (EFI_INCONSISTENCY_DATA));
    ASSERT (InconsistentTags != NULL);

    FileFormTags->InconsistentTags = InconsistentTags;
  }

  ZeroMem (FormTags->Tags, NumberOfTags * sizeof (EFI_TAG));

  for (CurrTag = 0; RawFormSet[Index] != EFI_IFR_END_FORM_SET_OP; CurrTag++) {
    //
    // Operand = IFR OpCode
    //
    FormTags->Tags[CurrTag].Operand = RawFormSet[Index];

    //
    // Assume for now 0 lines occupied by this OpCode
    //
    FormTags->Tags[CurrTag].NumberOfLines = 0;

    FormTags->Tags[CurrTag].Class         = Class;
    FormTags->Tags[CurrTag].SubClass      = SubClass;

    //
    // Determine the length of the Tag so we can later skip to the next tag in the form
    //
    TagLength = RawFormSet[Index + 1];
    //
    // get the length
    //
    // Operate on the Found OpCode
    //
    switch (RawFormSet[Index]) {

    case EFI_IFR_FORM_OP:
      //
      // If there was no variable op-code defined, create a dummy entry for one
      //
      if (FileFormTags->VariableDefinitions == NULL) {
        FileFormTags->VariableDefinitions = AllocateZeroPool (sizeof (EFI_VARIABLE_DEFINITION));
        ASSERT (FileFormTags->VariableDefinitions != NULL);
        IfrToFormTag (
          RawFormSet[Index],
          &FormTags->Tags[CurrTag],
          (VOID *) &RawFormSet[Index],
          FileFormTags->VariableDefinitions
          );
      } else {
        IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      }
      break;

    case EFI_IFR_SUBTITLE_OP:
    case EFI_IFR_TEXT_OP:
    case EFI_IFR_REF_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      break;

    case EFI_IFR_VARSTORE_OP:
      if (FileFormTags->VariableDefinitions == NULL) {
        VariableDefinitions = AllocateZeroPool (sizeof (EFI_VARIABLE_DEFINITION));
        ASSERT (VariableDefinitions != NULL);
        FileFormTags->VariableDefinitions = VariableDefinitions;
      }

      IfrToFormTag (
        RawFormSet[Index],
        &FormTags->Tags[CurrTag],
        (VOID *) &RawFormSet[Index],
        FileFormTags->VariableDefinitions
        );
      break;

    case EFI_IFR_VARSTORE_SELECT_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      CopyMem (&CurrentVariable, &((EFI_IFR_VARSTORE_SELECT *) &RawFormSet[Index])->VarId, sizeof (UINT16));
      CurrentVariable2 = CurrentVariable;
      break;

    case EFI_IFR_VARSTORE_SELECT_PAIR_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      CopyMem(&CurrentVariable, &((EFI_IFR_VARSTORE_SELECT_PAIR *)&RawFormSet[Index])->VarId, sizeof (UINT16));
      CopyMem (
        &CurrentVariable2,
        &((EFI_IFR_VARSTORE_SELECT_PAIR *) &RawFormSet[Index])->SecondaryVarId,
        sizeof (UINT16)
        );
      break;

    case EFI_IFR_END_FORM_OP:
      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (FormTags->Next == NULL) {
        //
        // We just hit the end of a form.  Let's allocate the ->Next structure
        //
        FormTags->Next = AllocatePool (sizeof (EFI_FORM_TAGS));
        ASSERT (FormTags->Next);
      }

      FormTags = FormTags->Next;
      ZeroMem (FormTags, sizeof (EFI_FORM_TAGS));

      //
      // Reset the tag count to one
      //
      NumberOfTags = 1;

      //
      // Reset the CurrTag value (it will be incremented, after this case statement
      // so set to a negative one so that we get the desired effect.)  Fish can beat me later.
      //
      CurrTag = -1;

      //
      // Determine the number of tags after this form.  If this is the last
      // form, then we will count the endformset and preserve that information
      // in the tag structure.
      //
      GetTagCount (&RawFormSet[Index + TagLength], &NumberOfTags);

      //
      // Allocate memory for our tags
      //
      FormTags->Tags = AllocateZeroPool (NumberOfTags * sizeof (EFI_TAG));
      ASSERT (FormTags->Tags);
      break;

    //
    // Two types of tags constitute the One Of question: a one-of header and
    // several one-of options.
    //
    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_ORDERED_LIST_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], RawFormSet, Index, FileFormTags, CurrentVariable);

      //
      // Store away the CurrTag since what follows will be the answer that we
      // need to place into the appropriate location in the tag array
      //
      //
      // record for setting default later
      //
      QuestionIndex = (UINT16) CurrTag;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      FormTags->Tags[QuestionIndex].Flags = ((EFI_IFR_ONE_OF_OPTION *) &RawFormSet[Index])->Flags;
      CopyMem (
        &FormTags->Tags[QuestionIndex].Key,
        &((EFI_IFR_ONE_OF_OPTION *) &RawFormSet[Index])->Key,
        sizeof (UINT16)
        );
      FormTags->Tags[QuestionIndex].ResetRequired = (BOOLEAN) (FormTags->Tags[QuestionIndex].Flags & EFI_IFR_FLAG_RESET_REQUIRED);
      break;

    case EFI_IFR_CHECKBOX_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], RawFormSet, Index, FileFormTags, CurrentVariable);
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      break;

    case EFI_IFR_NUMERIC_OP:
      GetNumericHeader (&FormTags->Tags[CurrTag], RawFormSet, Index, (UINT16) 1, FileFormTags, CurrentVariable);
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      break;

    case EFI_IFR_DATE_OP:
      //
      // Date elements come in as a Year, Month, Day.  We need to process them as a country-based
      // Order.  It is much easier to do it here than anywhere else.
      //
      // For US standards - we want Month/Day/Year, thus we advance "Index" +1, +2, +0 while CurrTag is +0, +1, +2
      //
      GetNumericHeader (
        &FormTags->Tags[CurrTag],
        RawFormSet,
        (UINT16) (Index + TagLength),
        (UINT16) 0,
        FileFormTags,
        CurrentVariable
        );

      //
      // The current language selected + the Date operand
      //
      FormTags->Tags[CurrTag + 1].Operand = RawFormSet[Index];
      GetNumericHeader (
        &FormTags->Tags[CurrTag + 1],
        RawFormSet,
        (UINT16) (Index + TagLength + RawFormSet[Index + TagLength + 1]),
        (UINT16) 0,
        FileFormTags,
        CurrentVariable
        );

      //
      // The current language selected + the Date operand
      //
      FormTags->Tags[CurrTag + 2].Operand = RawFormSet[Index];
      GetNumericHeader (&FormTags->Tags[CurrTag + 2], RawFormSet, Index, (UINT16) 1, FileFormTags, CurrentVariable);

      CurrTag   = (INT16) (CurrTag + 2);

      Index     = (UINT16) (Index + TagLength);
      //
      // get the length
      //
      TagLength = RawFormSet[Index + 1];
      Index     = (UINT16) (Index + TagLength);
      //
      // get the length
      //
      TagLength = RawFormSet[Index + 1];
      break;

    case EFI_IFR_TIME_OP:
      GetNumericHeader (&FormTags->Tags[CurrTag], RawFormSet, Index, (UINT16) 0, FileFormTags, CurrentVariable);

      if (Count == 2) {
        //
        // Override the GetQuestionHeader information - date/time are treated very differently
        //
        FormTags->Tags[CurrTag].NumberOfLines = 1;
        Count = 0;
      } else {
        //
        // The premise is that every date/time op-code have 3 elements, the first 2 have 0 lines
        // associated with them, and the third has 1 line to allow to space beyond the choice.
        //
        Count++;
      }
      break;

    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], RawFormSet, Index, FileFormTags, CurrentVariable);
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      break;

    case EFI_IFR_SUPPRESS_IF_OP:
    case EFI_IFR_GRAYOUT_IF_OP:
      InconsistentTags->Operand = ((EFI_IFR_INCONSISTENT *) &RawFormSet[Index])->Header.OpCode;
      gConsistencyId++;

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->QuestionId1 = INVALID_OFFSET_VALUE;
      InconsistentTags->QuestionId2 = INVALID_OFFSET_VALUE;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_FORM_SET_OP:
      CopyMem (
        &FormTags->Tags[CurrTag].GuidValue,
        &((EFI_IFR_FORM_SET *) &RawFormSet[Index])->Guid,
        sizeof (EFI_GUID)
        );
      CopyMem (
        &FormTags->Tags[CurrTag].CallbackHandle,
        &((EFI_IFR_FORM_SET *) &RawFormSet[Index])->CallbackHandle,
        sizeof (EFI_PHYSICAL_ADDRESS)
        );
      CopyMem (&FormTags->Tags[CurrTag].Class, &((EFI_IFR_FORM_SET *) &RawFormSet[Index])->Class, sizeof (UINT8));
      CopyMem (
        &FormTags->Tags[CurrTag].SubClass,
        &((EFI_IFR_FORM_SET *) &RawFormSet[Index])->SubClass,
        sizeof (UINT8)
        );
      CopyMem (
        &FormTags->Tags[CurrTag].NvDataSize,
        &((EFI_IFR_FORM_SET *) &RawFormSet[Index])->NvDataSize,
        sizeof (UINT16)
        );
      Class     = ((EFI_IFR_FORM_SET *) &RawFormSet[Index])->Class;
      SubClass  = ((EFI_IFR_FORM_SET *) &RawFormSet[Index])->SubClass;
      //
      // If the formset has a size value, that means someone must be using this, so create a variable
      // We also shall reserve the formid of 0 for this specific purpose.
      //
      if ((FileFormTags->VariableDefinitions == NULL) && (FormTags->Tags[CurrTag].NvDataSize > 0)) {
        FileFormTags->VariableDefinitions = AllocateZeroPool (sizeof (EFI_VARIABLE_DEFINITION));
        ASSERT (FileFormTags->VariableDefinitions != NULL);
        IfrToFormTag (
          RawFormSet[Index],
          &FormTags->Tags[CurrTag],
          (VOID *) &RawFormSet[Index],
          FileFormTags->VariableDefinitions
          );
      } else {
        IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);
      }
      break;

    case EFI_IFR_BANNER_OP:
      if (gClassOfVfr == EFI_FRONT_PAGE_SUBCLASS) {
        TempValue = 0;
        CopyMem (&TempValue, &((EFI_IFR_BANNER *) &RawFormSet[Index])->Alignment, sizeof (UINT8));
        //
        // If this is the special timeout value, we will dynamically figure out where to put it
        // Also the least significant byte refers to the TimeOut desired.
        //
        if (TempValue == EFI_IFR_BANNER_TIMEOUT) {
          CopyMem (&FrontPageTimeOutTitle, &((EFI_IFR_BANNER *) &RawFormSet[Index])->Title, sizeof (UINT16));
          if (FrontPageTimeOutValue != (INT16) -1) {
            CopyMem (&FrontPageTimeOutValue, &((EFI_IFR_BANNER *) &RawFormSet[Index])->LineNumber, sizeof (UINT16));
          }
          break;
        }

        CopyMem (
          &BannerData->Banner[((EFI_IFR_BANNER *) &RawFormSet[Index])->LineNumber][
          ((EFI_IFR_BANNER *) &RawFormSet[Index])->Alignment],
          &((EFI_IFR_BANNER *) &RawFormSet[Index])->Title,
          sizeof (STRING_REF)
          );
      }
      break;

    case EFI_IFR_INCONSISTENT_IF_OP:
      CopyMem (
        &FormTags->Tags[CurrTag].Text,
        &((EFI_IFR_INCONSISTENT *) &RawFormSet[Index])->Popup,
        sizeof (UINT16)
        );
      gConsistencyId++;

      InconsistentTags->Operand = ((EFI_IFR_INCONSISTENT *) &RawFormSet[Index])->Header.OpCode;
      CopyMem (&InconsistentTags->Popup, &((EFI_IFR_INCONSISTENT *) &RawFormSet[Index])->Popup, sizeof (UINT16));

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->QuestionId1     = INVALID_OFFSET_VALUE;
      InconsistentTags->QuestionId2     = INVALID_OFFSET_VALUE;

      InconsistentTags->VariableNumber  = CurrentVariable;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_EQ_ID_VAL_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);

      InconsistentTags->Operand = ((EFI_IFR_EQ_ID_VAL *) &RawFormSet[Index])->Header.OpCode;
      CopyMem (&InconsistentTags->Value, &((EFI_IFR_EQ_ID_VAL *) &RawFormSet[Index])->Value, sizeof (UINT16));
      CopyMem (
        &InconsistentTags->QuestionId1,
        &((EFI_IFR_EQ_ID_VAL *) &RawFormSet[Index])->QuestionId,
        sizeof (UINT16)
        );

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->Width               = FormTags->Tags[CurrTag].StorageWidth;
      InconsistentTags->QuestionId2         = INVALID_OFFSET_VALUE;
      InconsistentTags->ConsistencyId       = gConsistencyId;
      FormTags->Tags[CurrTag].ConsistencyId = gConsistencyId;

      InconsistentTags->VariableNumber      = CurrentVariable;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_EQ_VAR_VAL_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);

      InconsistentTags->Operand = ((EFI_IFR_EQ_VAR_VAL *) &RawFormSet[Index])->Header.OpCode;
      CopyMem (&InconsistentTags->Value, &((EFI_IFR_EQ_VAR_VAL *) &RawFormSet[Index])->Value, sizeof (UINT16));
      CopyMem (
        &InconsistentTags->QuestionId1,
        &((EFI_IFR_EQ_VAR_VAL *) &RawFormSet[Index])->VariableId,
        sizeof (UINT16)
        );

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->QuestionId2         = INVALID_OFFSET_VALUE;
      InconsistentTags->ConsistencyId       = gConsistencyId;
      FormTags->Tags[CurrTag].ConsistencyId = gConsistencyId;

      InconsistentTags->VariableNumber      = CurrentVariable;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_EQ_ID_ID_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);

      InconsistentTags->Operand = ((EFI_IFR_EQ_ID_ID *) &RawFormSet[Index])->Header.OpCode;
      CopyMem (
        &InconsistentTags->QuestionId1,
        &((EFI_IFR_EQ_ID_ID *) &RawFormSet[Index])->QuestionId1,
        sizeof (UINT16)
        );
      CopyMem (
        &InconsistentTags->QuestionId2,
        &((EFI_IFR_EQ_ID_ID *) &RawFormSet[Index])->QuestionId2,
        sizeof (UINT16)
        );

      InconsistentTags->Width               = FormTags->Tags[CurrTag].StorageWidth;
      InconsistentTags->ConsistencyId       = gConsistencyId;
      FormTags->Tags[CurrTag].ConsistencyId = gConsistencyId;

      InconsistentTags->VariableNumber      = CurrentVariable;
      InconsistentTags->VariableNumber2     = CurrentVariable2;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_AND_OP:
    case EFI_IFR_OR_OP:
    case EFI_IFR_NOT_OP:
    case EFI_IFR_GT_OP:
    case EFI_IFR_GE_OP:
    case EFI_IFR_TRUE_OP:
    case EFI_IFR_FALSE_OP:
      InconsistentTags->Operand = ((EFI_IFR_NOT *) &RawFormSet[Index])->Header.OpCode;

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //

      //
      // Reserve INVALID_OFFSET_VALUE - 1 for TRUE or FALSE because they are inconsistency tags also, but
      // have no coresponding id. The examination of id is needed by evaluating boolean expression.
      //
      if (RawFormSet[Index] == EFI_IFR_TRUE_OP ||
          RawFormSet[Index] == EFI_IFR_FALSE_OP) {
        InconsistentTags->QuestionId1         = INVALID_OFFSET_VALUE - 1;
      } else {
        InconsistentTags->QuestionId1         = INVALID_OFFSET_VALUE;
      }
      InconsistentTags->QuestionId2         = INVALID_OFFSET_VALUE;
      InconsistentTags->ConsistencyId       = gConsistencyId;
      FormTags->Tags[CurrTag].ConsistencyId = gConsistencyId;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_EQ_ID_LIST_OP:
      IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);

      InconsistentTags->Operand = ((EFI_IFR_EQ_ID_LIST *) &RawFormSet[Index])->Header.OpCode;
      CopyMem (
        &InconsistentTags->QuestionId1,
        &((EFI_IFR_EQ_ID_LIST *) &RawFormSet[Index])->QuestionId,
        sizeof (UINT16)
        );
      CopyMem (
        &InconsistentTags->ListLength,
        &((EFI_IFR_EQ_ID_LIST *) &RawFormSet[Index])->ListLength,
        sizeof (UINT16)
        );
      InconsistentTags->ValueList = FormTags->Tags[CurrTag].IntList;

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->Width               = FormTags->Tags[CurrTag].StorageWidth;
      InconsistentTags->QuestionId2         = INVALID_OFFSET_VALUE;
      InconsistentTags->ConsistencyId       = gConsistencyId;
      FormTags->Tags[CurrTag].ConsistencyId = gConsistencyId;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_END_IF_OP:
      InconsistentTags->Operand = ((EFI_IFR_END_EXPR *) &RawFormSet[Index])->Header.OpCode;

      //
      // Since this op-code doesn't use the next field(s), initialize them with something invalid.
      // Unfortunately 0 is a valid offset value for a QuestionId
      //
      InconsistentTags->QuestionId1 = INVALID_OFFSET_VALUE;
      InconsistentTags->QuestionId2 = INVALID_OFFSET_VALUE;

      //
      // Test for an allocated buffer.  If already allocated this is due to having called this routine
      // once for sizing of the NV storage.  We then loaded the NV variable and can correctly initialize
      // the tag structure with current values from the NV
      //
      if (InconsistentTags->Next == NULL) {
        AddNextInconsistentTag (&InconsistentTags);
        break;
      }

      InconsistentTags = InconsistentTags->Next;
      break;

    case EFI_IFR_END_ONE_OF_OP:
      break;

    default:
      break;
    }
    //
    // End of switch
    //
    // Per spec., we ignore ops that we don't know how to deal with.  Skip to next tag
    //
    Index = (UINT16) (Index + TagLength);
  }
  //
  // End of Index
  //
  // When we eventually exit, make sure we mark the last tag with an op-code
  //
  FormTags->Tags[CurrTag].Operand = RawFormSet[Index];

  IfrToFormTag (RawFormSet[Index], &FormTags->Tags[CurrTag], (VOID *) &RawFormSet[Index], NULL);

  //
  // Place this as an end of the database marker
  //
  InconsistentTags->Operand = 0xFF;

  //
  // This is the Head of the linked list of pages.  Each page is an array of tags
  //
  FormTags          = &FileFormTags->FormTags;
  InconsistentTags  = FileFormTags->InconsistentTags;

  for (; InconsistentTags->Operand != 0xFF;) {
    if (InconsistentTags->QuestionId1 != INVALID_OFFSET_VALUE) {
      //
      // Search the tags for the tag which corresponds to this ID
      //
      for (CurrTag = 0; FormTags->Tags[0].Operand != EFI_IFR_END_FORM_SET_OP; CurrTag++) {
        //
        // If we hit the end of a form, go to the next set of Tags.
        // Remember - EndFormSet op-codes sit on their own page after an end form.
        //
        if (FormTags->Tags[CurrTag].Operand == EFI_IFR_END_FORM_OP) {
          //
          // Reset the CurrTag value (it will be incremented, after this case statement
          // so set to a negative one so that we get the desired effect.)  Fish can beat me later.
          //
          CurrTag   = -1;
          FormTags  = FormTags->Next;
          continue;
        }

        if (FormTags->Tags[CurrTag].Id == InconsistentTags->QuestionId1) {
          FormTags->Tags[CurrTag].Consistency++;
        }
      }
    }

    FormTags = &FileFormTags->FormTags;

    if (InconsistentTags->QuestionId2 != INVALID_OFFSET_VALUE) {
      //
      // Search the tags for the tag which corresponds to this ID
      //
      for (CurrTag = 0; FormTags->Tags[CurrTag].Operand != EFI_IFR_END_FORM_SET_OP; CurrTag++) {
        //
        // If we hit the end of a form, go to the next set of Tags.
        // Remember - EndFormSet op-codes sit on their own page after an end form.
        //
        if (FormTags->Tags[CurrTag].Operand == EFI_IFR_END_FORM_OP) {
          //
          // Reset the CurrTag value (it will be incremented, after this case statement
          // so set to a negative one so that we get the desired effect.)  Fish can beat me later.
          //
          CurrTag   = -1;
          FormTags  = FormTags->Next;
          continue;
        }

        if (FormTags->Tags[CurrTag].Id == InconsistentTags->QuestionId2) {
          FormTags->Tags[CurrTag].Consistency++;
        }
      }
    }

    InconsistentTags = InconsistentTags->Next;
  }

  return Status;
}

VOID
InitPage (
  VOID
  )
{
  CHAR16  *HomePageString;
  CHAR16  *HomeEscapeString;

  //
  // Displays the Header and Footer borders
  //
  DisplayPageFrame ();

  HomePageString    = GetToken (STRING_TOKEN (HOME_PAGE_TITLE), gHiiHandle);
  HomeEscapeString  = GetToken (STRING_TOKEN (HOME_ESCAPE_STRING), gHiiHandle);

  gST->ConOut->SetAttribute (gST->ConOut, EFI_YELLOW | EFI_BRIGHT);
  //
  //  PrintStringAt ((gScreenDimensions.RightColumn - GetStringWidth(HomePageString)/2)/2, 1, HomePageString);
  //
  PrintStringAt (
    (gScreenDimensions.RightColumn + gScreenDimensions.LeftColumn - GetStringWidth (HomePageString) / 2) / 2,
    1,
    HomePageString
    );
  PrintAt (
    gScreenDimensions.LeftColumn + 2,
    gScreenDimensions.BottomRow - 3,
    (CHAR16 *) L"%c%c%s",
    ARROW_UP,
    ARROW_DOWN,
    gMoveHighlight
    );
  PrintAt (
    gScreenDimensions.RightColumn - (GetStringWidth (HomeEscapeString) / 2) - 2,
    gScreenDimensions.BottomRow - 3,
    (CHAR16 *) L" %s",
    HomeEscapeString
    );
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  FreePool (HomeEscapeString);
  FreePool (HomePageString);

  return ;
}

CHAR16 *
GetToken (
  IN  STRING_REF                              Token,
  IN  EFI_HII_HANDLE                          HiiHandle
  )
/*++

Routine Description:

  Get the string based on the TokenID and HII Handle.

Arguments:

  Token       - The Token ID.
  HiiHandle   - Handle of Ifr to be fetched.

Returns:

  The output string.

--*/
{
  CHAR16      *Buffer;
  UINTN       BufferLength;
  EFI_STATUS  Status;

  //
  // Set default string size assumption at no more than 256 bytes
  //
  BufferLength  = 0x100;

  Buffer        = AllocateZeroPool (BufferLength);
  ASSERT (Buffer != NULL);

  Status = Hii->GetString (Hii, HiiHandle, Token, TRUE, NULL, &BufferLength, Buffer);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // Free the old pool
      //
      FreePool (Buffer);

      //
      // Allocate new pool with correct value
      //
      Buffer = AllocatePool (BufferLength);
      ASSERT (Buffer != NULL);

      Status = Hii->GetString (Hii, HiiHandle, Token, TRUE, NULL, &BufferLength, Buffer);

      if (!EFI_ERROR (Status)) {
        return Buffer;
      }
    }

    ASSERT_EFI_ERROR (Status);
  }

  return Buffer;
}

STATIC
EFI_STATUS
PopulateHomePage (
  IN UINTN                                    NumberOfIfrImages,
  IN EFI_FILE_FORM_TAGS                       *FileFormTagsHead
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  EFI_IFR_BINARY      *IfrBinary;
  CHAR16              *StringPtr;
  EFI_FILE_FORM_TAGS  *FileFormTags;
  EFI_FORM_TAGS       LocalTags;

  FileFormTags = FileFormTagsHead;

  UiInitMenu ();

  Status = EFI_SUCCESS;

  //
  // If there are no images
  //
  if (NumberOfIfrImages == 0) {
    Status = EFI_NO_MEDIA;
    return Status;
  }
  //
  // IfrBinary points to the beginning of the Binary data linked-list
  //
  IfrBinary = gBinaryDataHead;

  //
  // Print the entries which were in the default language.
  //
  for (Index = 0; Index < NumberOfIfrImages; Index++) {
    LocalTags = FileFormTags->FormTags;

    //
    // Populate the Menu
    //
    StringPtr = GetToken (IfrBinary->TitleToken, IfrBinary->Handle);

    //
    // If the default language doesn't exist, don't add a menu option yet
    //
    if (StringPtr[0] != CHAR_NULL) {
      //
      // We are NOT!! removing this StringPtr buffer via FreePool since it is being used in the menuoptions, we will do
      // it in UiFreeMenu.
      //
      UiAddMenuOption (StringPtr, IfrBinary->Handle, LocalTags.Tags, IfrBinary->FormBinary, Index);
    }
    //
    // Advance to the next HII handle
    //
    IfrBinary     = IfrBinary->Next;
    FileFormTags  = FileFormTags->NextFile;
  }

  return Status;
}

STATIC
UI_MENU_OPTION *
DisplayHomePage (
  IN UINTN                                    NumberOfIfrImages,
  IN EFI_FILE_FORM_TAGS                       *FileFormTagsHead,
  IN UINT8                                    *CallbackData
  )
{
  EFI_STATUS      Status;
  UI_MENU_OPTION  *Selection;

  //
  // This prints the basic home page template which the user sees
  //
  InitPage ();

  Status = PopulateHomePage (NumberOfIfrImages, FileFormTagsHead);

  if (EFI_ERROR (Status)) {
    Selection = NULL;
    return Selection;
  }

  Selection = UiDisplayMenu (FALSE, FileFormTagsHead, (EFI_IFR_DATA_ARRAY *) CallbackData);

  return Selection;
}

STATIC
EFI_STATUS
InitializeBinaryStructures (
  IN  EFI_HII_HANDLE                        *Handle,
  IN  BOOLEAN                               UseDatabase,
  IN  EFI_IFR_PACKET                        *Packet,
  IN  UINT8                                 *NvMapOverride,
  IN  UINTN                                 NumberOfIfrImages,
  OUT EFI_FILE_FORM_TAGS                    **FileFormTagsHead
  )
{
  UINTN                       HandleIndex;
  EFI_STATUS                  Status;
  EFI_IFR_BINARY              *BinaryData;
  EFI_FILE_FORM_TAGS          *FileFormTags;
  UINTN                       SizeOfNvStore;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_VARIABLE_DEFINITION     *VariableDefinition;
  EFI_VARIABLE_DEFINITION     *OverrideDefinition;
  VOID                        *NvMap;
  UINTN                       NvMapSize;
  EFI_HII_VARIABLE_PACK_LIST  *NvMapListHead;
  EFI_HII_VARIABLE_PACK_LIST  *NvMapListNode;

  //
  // Initialize some variables to avoid warnings
  //
  BinaryData        = NULL;
  *FileFormTagsHead = NULL;
  FileFormTags      = NULL;
  gBinaryDataHead   = NULL;
  Status            = EFI_SUCCESS;
  FormCallback      = NULL;
  NvMap             = NULL;
  NvMapSize         = 0;

  if (NumberOfIfrImages > 1) {
    NvMapOverride = NULL;
  }

  for (HandleIndex = 0; HandleIndex < NumberOfIfrImages; HandleIndex += 1) {
    //
    // If the buffers are uninitialized, allocate them, otherwise work on the ->Next members
    //
    if ((BinaryData == NULL) || (FileFormTags == NULL)) {
      //
      // Allocate memory for our Binary Data
      //
      BinaryData = AllocateZeroPool (sizeof (EFI_IFR_BINARY));
      ASSERT (BinaryData);

      //
      // Preserve the Head of what will be a linked-list.
      //
      gBinaryDataHead       = BinaryData;
      gBinaryDataHead->Next = NULL;

      if (UseDatabase) {
        Status = GetIfrBinaryData (Hii, Handle[HandleIndex], NULL, BinaryData);
      } else {
        Status = GetIfrBinaryData (Hii, Handle[HandleIndex], Packet, BinaryData);
      }
      //
      // Allocate memory for our File Form Tags
      //
      FileFormTags = AllocateZeroPool (sizeof (EFI_FILE_FORM_TAGS));
      ASSERT (FileFormTags);

      //
      // Preserve the Head of what will be a linked-list.
      //
      *FileFormTagsHead             = FileFormTags;
      (*FileFormTagsHead)->NextFile = NULL;

    } else {
      //
      // Allocate memory for our Binary Data linked-list
      // Each handle represents a Binary and we will store that data away.
      //
      BinaryData->Next = AllocateZeroPool (sizeof (EFI_IFR_BINARY));
      ASSERT (BinaryData->Next);

      BinaryData        = BinaryData->Next;
      BinaryData->Next  = NULL;

      if (UseDatabase) {
        Status = GetIfrBinaryData (Hii, Handle[HandleIndex], NULL, BinaryData);
      } else {
        Status = GetIfrBinaryData (Hii, Handle[HandleIndex], Packet, BinaryData);
      }

      if (EFI_ERROR (Status)) {
        return EFI_DEVICE_ERROR;
      }
      //
      // Allocate memory for our FileFormTags linked-list
      // Each allocation reserves handle represents a Binary and we will store that data away.
      //
      FileFormTags->NextFile = AllocateZeroPool (sizeof (EFI_FILE_FORM_TAGS));
      ASSERT (FileFormTags->NextFile);

      FileFormTags = FileFormTags->NextFile;
    }
    //
    // endif
    //
    // Tag Structure Initialization
    //
    Status              = InitializeTagStructures (BinaryData, FileFormTags);

    VariableDefinition  = FileFormTags->VariableDefinitions;

    //
    // Allocate memory for our NVRAM Maps for all of our variables
    //
    for (; VariableDefinition != NULL; VariableDefinition = VariableDefinition->Next) {
      //
      // Pad the fake variable size accordingly - this value should reflect the size of information that is not accounted by
      // the mainstream NVRAM variable such as DATE/TIME information that the browser needs to track but is saved to an RTC
      //
      VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableSize + VariableDefinition->VariableFakeSize);

      //
      // In the case where a file has no "real" NV data, we should pad the buffer accordingly
      //
      if (VariableDefinition->VariableSize == 0) {
        if (VariableDefinition->VariableFakeSize != 0) {
          VariableDefinition->NvRamMap = AllocateZeroPool (VariableDefinition->VariableFakeSize);
          ASSERT (VariableDefinition->NvRamMap != NULL);
        }
      } else {
        VariableDefinition->NvRamMap = AllocateZeroPool (VariableDefinition->VariableSize);
        ASSERT (VariableDefinition->NvRamMap != NULL);
      }

      if (VariableDefinition->VariableFakeSize != 0) {
        VariableDefinition->FakeNvRamMap = AllocateZeroPool (VariableDefinition->VariableFakeSize);
        ASSERT (VariableDefinition->FakeNvRamMap != NULL);
      }
    }

    Status = gBS->HandleProtocol (
                    (VOID *) (UINTN) FileFormTags->FormTags.Tags[0].CallbackHandle,
                    &gEfiFormCallbackProtocolGuid,
                    (VOID **) &FormCallback
                    );

    //
    // Since we might have multiple variables, if there is an NvMapOverride we need to use the EFI_VARIABLE_DEFINITION
    // information as the information that we pass back and forth.  NOTE that callbacks that are initiated will only have the
    // NVRAM data refreshed based on the op-code that initiated the callback.  In other words, we will pass to the caller a single
    // NVRAM map for a single variable based on the op-code that the user selected.
    //
    if (NvMapOverride != NULL) {
      VariableDefinition  = FileFormTags->VariableDefinitions;
      OverrideDefinition  = ((EFI_VARIABLE_DEFINITION *) NvMapOverride);

      //
      // Search through the variable definitions.  There should be sufficient passed in settings for the variable op-codes specified
      //
      for (; VariableDefinition != NULL; VariableDefinition = VariableDefinition->Next) {
        if ((!CompareMem (VariableDefinition->VariableName, L"Setup", 10)) && (VariableDefinition->Next == NULL)) {
          if (VariableDefinition->VariableSize != 0) {
            CopyMem (VariableDefinition->NvRamMap, NvMapOverride, VariableDefinition->VariableSize);
          } else {
            CopyMem (VariableDefinition->NvRamMap, NvMapOverride, VariableDefinition->VariableFakeSize);
          }
          break;
        } else {
          VariableDefinition->NvRamMap = OverrideDefinition->NvRamMap;
        }
        //
        // There should NEVER be a ->Next for VariableDefinition and a NULL ->Next for the OverrideDefinition
        //
        ASSERT (OverrideDefinition->Next);
        OverrideDefinition = OverrideDefinition->Next;
      }
    } else {
      VariableDefinition = FileFormTags->VariableDefinitions;

      //
      // Search through the variable definitions.  There should be sufficient passed in settings for the variable op-codes specified
      //
      for (; VariableDefinition != NULL; VariableDefinition = VariableDefinition->Next) {
        SizeOfNvStore = VariableDefinition->VariableSize;

        //
        // Getting the NvStore and placing it into our Global Data
        //
        if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
          Status = FormCallback->NvRead (
                                  FormCallback,
                                  VariableDefinition->VariableName,
                                  &VariableDefinition->Guid,
                                  NULL,
                                  &SizeOfNvStore,
                                  (VOID *) VariableDefinition->NvRamMap
                                  );
        } else {
          Status = gRT->GetVariable (
                          VariableDefinition->VariableName,
                          &VariableDefinition->Guid,
                          NULL,
                          &SizeOfNvStore,
                          (VOID *) VariableDefinition->NvRamMap
                          );
        }

        if (EFI_ERROR (Status)) {
          //
          // If there is a variable that exists already and it is larger than what we calculated the
          // storage needs to be, we must assume the variable size from GetVariable is correct and not
          // allow the truncation of the variable.  It is very possible that the user who created the IFR
          // we are cracking is not referring to a variable that was in a previous map, however we cannot
          // allow it's truncation.
          //
          if (Status == EFI_BUFFER_TOO_SMALL) {
            //
            // If the buffer was too small, we should have the expanded size requirement in SizeOfNvStore now.
            //
            VariableDefinition->VariableSize = (UINT16) SizeOfNvStore;

            //
            // Free the buffer that was allocated that was too small
            //
            FreePool (VariableDefinition->NvRamMap);
            FreePool (VariableDefinition->FakeNvRamMap);

            VariableDefinition->NvRamMap = AllocateZeroPool (SizeOfNvStore);
            VariableDefinition->FakeNvRamMap = AllocateZeroPool (SizeOfNvStore + VariableDefinition->VariableFakeSize);
            ASSERT (VariableDefinition->NvRamMap);
            ASSERT (VariableDefinition->FakeNvRamMap);

            if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
              Status = FormCallback->NvRead (
                                      FormCallback,
                                      VariableDefinition->VariableName,
                                      &VariableDefinition->Guid,
                                      NULL,
                                      &SizeOfNvStore,
                                      (VOID *) VariableDefinition->NvRamMap
                                      );
            } else {
              Status = gRT->GetVariable (
                              VariableDefinition->VariableName,
                              &VariableDefinition->Guid,
                              NULL,
                              &SizeOfNvStore,
                              (VOID *) VariableDefinition->NvRamMap
                              );
            }
          }
          //
          // if the variable was not found, we will retrieve default values
          //
          if (Status == EFI_NOT_FOUND) {

            if (0 == CompareMem (VariableDefinition->VariableName, L"Setup", 10)) {

              NvMapListHead = NULL;

              Status = Hii->GetDefaultImage (Hii, Handle[HandleIndex], EFI_IFR_FLAG_DEFAULT, &NvMapListHead);

              if (!EFI_ERROR (Status)) {
                ASSERT_EFI_ERROR (NULL != NvMapListHead);

                NvMapListNode = NvMapListHead;

                while (NULL != NvMapListNode) {
                  if (VariableDefinition->VariableId == NvMapListNode->VariablePack->VariableId) {
                    NvMap     = (VOID *) ((CHAR8 *) NvMapListNode->VariablePack + sizeof (EFI_HII_VARIABLE_PACK) + NvMapListNode->VariablePack->VariableNameLength);
                    NvMapSize = NvMapListNode->VariablePack->Header.Length  - sizeof (EFI_HII_VARIABLE_PACK) - NvMapListNode->VariablePack->VariableNameLength;
                    break;
                    }
                  NvMapListNode = NvMapListNode->NextVariablePack;
                }

                //
                // Free the buffer that was allocated.
                //
                FreePool (VariableDefinition->NvRamMap);
                FreePool (VariableDefinition->FakeNvRamMap);

                //
                // Allocate, copy the NvRamMap.
                //
                VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableFakeSize - VariableDefinition->VariableSize);
                VariableDefinition->VariableSize = (UINT16) NvMapSize;
                VariableDefinition->VariableFakeSize = (UINT16) (VariableDefinition->VariableFakeSize + VariableDefinition->VariableSize);

                VariableDefinition->NvRamMap = AllocateZeroPool (VariableDefinition->VariableSize);
                VariableDefinition->FakeNvRamMap = AllocateZeroPool (NvMapSize + VariableDefinition->VariableFakeSize);

                CopyMem (VariableDefinition->NvRamMap, NvMap, NvMapSize);
                FreePool (NvMapListHead);
              }

            }
            Status = EFI_SUCCESS;
          }
        }
      }
    }

    InitializeTagStructures (BinaryData, FileFormTags);
  }
  //
  // endfor
  //
  return Status;
}

STATIC
EFI_STATUS
GetIfrBinaryData (
  IN      EFI_HII_PROTOCOL *Hii,
  IN      EFI_HII_HANDLE   HiiHandle,
  IN      EFI_IFR_PACKET   *Packet,
  IN OUT  EFI_IFR_BINARY   *BinaryData
  )
/*++

Routine Description:
  Fetch the Ifr binary data.

Arguments:
  Hii         - Point to HII protocol.
  HiiHandle   - Handle of Ifr to be fetched.
  Packet      - Pointer to IFR packet.
  BinaryData  - Buffer to copy the string into

Returns:
  Returns the number of CHAR16 characters that were copied into the OutputString buffer.


--*/
{
  EFI_STATUS        Status;
  EFI_HII_PACKAGES  *PackageList;
  UINTN             BufferSize;
  VOID              *Buffer;
  UINT8             *RawFormBinary;
  EFI_IFR_FORM_SET  *FormOp;
  UINT16            Index;
  UINT16            Index2;
  UINT16            TitleToken;

  //
  // Initialize the TitleToken to 0 just in case not found
  //
  TitleToken = 0;

  //
  // Try for a 32K Buffer
  //
  BufferSize = 0x8000;

  //
  // Allocate memory for our Form binary
  //
  Buffer = AllocateZeroPool (BufferSize);
  ASSERT (Buffer);

  if (Packet == NULL) {
    Status = Hii->GetForms (Hii, HiiHandle, 0, &BufferSize, Buffer);

    if (Status == EFI_BUFFER_TOO_SMALL) {

      FreePool (Buffer);

      //
      // Allocate memory for our Form binary
      //
      Buffer = AllocatePool (BufferSize);
      ASSERT (Buffer);

      Status = Hii->GetForms (Hii, HiiHandle, 0, &BufferSize, Buffer);
    }
  } else {
    //
    // Copies the data to local usable buffer
    //
    CopyMem (Buffer, Packet->IfrData, Packet->IfrData->Header.Length);

    //
    // Register the string data with HII
    //
    PackageList = PreparePackages (2, NULL, Packet->IfrData, Packet->StringData);

    Status      = Hii->NewPack (Hii, PackageList, &HiiHandle);

    FreePool (PackageList);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // We now have the IFR binary in our Buffer
  //
  BinaryData->IfrPackage  = Buffer;
  RawFormBinary           = (UINT8 *) ((CHAR8 *) (Buffer) + sizeof (EFI_HII_PACK_HEADER));
  BinaryData->FormBinary  = (UINT8 *) ((CHAR8 *) (Buffer) + sizeof (EFI_HII_PACK_HEADER));
  BinaryData->Handle      = HiiHandle;

  //
  // If a packet was passed in, remove the string data when exiting.
  //
  if (Packet != NULL) {
    BinaryData->UnRegisterOnExit = TRUE;
  } else {
    BinaryData->UnRegisterOnExit = FALSE;
  }
  //
  // Walk through the FormSet Opcodes looking for the FormSet opcode
  // If we hit EFI_IFR_END_SET_OP we know we hit the end of the FormSet.
  //
  for (Index = 0; RawFormBinary[Index] != EFI_IFR_END_FORM_SET_OP;) {
    FormOp  = (EFI_IFR_FORM_SET *) &RawFormBinary[Index];
    Index   = (UINT16) (Index + FormOp->Header.Length);

    if (FormOp->Header.OpCode == EFI_IFR_FORM_SET_OP) {
      TitleToken = FormOp->FormSetTitle;
      //
      // If displaying FrontPage - set the flag signifying it
      //
      switch (FormOp->SubClass) {
      case EFI_FRONT_PAGE_SUBCLASS:
        FrontPageHandle = HiiHandle;

      default:
        gClassOfVfr = FormOp->SubClass;
      }
      //
      // Match GUID to find out the function key setting. If match fail, use the default setting.
      //
      for (Index2 = 0; Index2 < sizeof (gFunctionKeySettingTable) / sizeof (FUNCTIION_KEY_SETTING); Index2++) {
        if (CompareGuid ((EFI_GUID *)(UINTN)&FormOp->Guid, &(gFunctionKeySettingTable[Index2].FormSetGuid))) {
          //
          // Update the function key setting.
          //
          gFunctionKeySetting = gFunctionKeySettingTable[Index2].KeySetting;
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
    }
  }

  BinaryData->TitleToken = TitleToken;

  return Status;
}

EFI_HANDLE          PrintHandle     = NULL;
EFI_PRINT_PROTOCOL  mPrintProtocol  = { UnicodeVSPrint };

STATIC
EFI_STATUS
InstallPrint (
  VOID
  )
{
  return gBS->InstallProtocolInterface (
                &PrintHandle,
                &gEfiPrintProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &mPrintProtocol
                );
}
