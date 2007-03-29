/**@file
	Implementation for handling the User Interface option processing.

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

EFI_STATUS
ExtractRequestedNvMap (
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  UINT16                      VariableId,
  OUT EFI_VARIABLE_DEFINITION     **VariableDefinition
  )
{
  *VariableDefinition = FileFormTags->VariableDefinitions;

  //
  // Extract the data from the NV variable - consumer will free the buffer.
  //
  for (; *VariableDefinition != NULL; *VariableDefinition = (*VariableDefinition)->Next) {
    //
    // If there is a variable with this ID return with EFI_SUCCESS
    //
    if (!CompareMem (&(*VariableDefinition)->VariableId, &VariableId, sizeof (UINT16))) {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
ExtractNvValue (
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  UINT16                      VariableId,
  IN  UINT16                      VariableSize,
  IN  UINT16                      OffsetValue,
  OUT VOID                        **Buffer
  )
{
  EFI_STATUS              Status;
  EFI_VARIABLE_DEFINITION *VariableDefinition;

  Status = ExtractRequestedNvMap (FileFormTags, VariableId, &VariableDefinition);

  if (!EFI_ERROR (Status)) {
    //
    // Allocate sufficient space for the data and copy it into the outgoing buffer
    //
    if (VariableSize != 0) {
      *Buffer = AllocateZeroPool (VariableSize);
      ASSERT (*Buffer != NULL);
      CopyMem (*Buffer, &VariableDefinition->NvRamMap[OffsetValue], VariableSize);
    }
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
VOID
AdjustNvMap (
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  IN  UI_MENU_OPTION              *MenuOption
  )
{
  CHAR8                   *NvRamMap;
  UINTN                   SizeRequired;
  UINTN                   Index;
  UINTN                   CachedStart;
  EFI_VARIABLE_DEFINITION *VariableDefinition;

  CachedStart   = 0;

  SizeRequired  = MenuOption->ThisTag->StorageStart + MenuOption->ThisTag->StorageWidth;

  ExtractRequestedNvMap (FileFormTags, MenuOption->Tags->VariableNumber, &VariableDefinition);

  //
  // We arrived here because the current NvRamMap is too small for the new op-code to store things and
  // we need to adjust the buffer to support this.
  //
  NvRamMap = AllocateZeroPool (SizeRequired + 1);
  ASSERT (NvRamMap != NULL);

  //
  // Copy current NvRamMap to the new NvRamMap
  //
  CopyMem (NvRamMap, VariableDefinition->NvRamMap, VariableDefinition->VariableFakeSize);

  //
  // Remember, the only time we come here is because we are in the NVPlus section of the NvRamMap
  //
  for (Index = MenuOption->TagIndex;
       (MenuOption->Tags[Index].Operand != EFI_IFR_END_FORM_OP) && (MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP);
       Index++
      ) {

    switch (MenuOption->Tags[Index].Operand) {
    case EFI_IFR_ORDERED_LIST_OP:
    case EFI_IFR_ONE_OF_OP:
      CachedStart = MenuOption->Tags[Index].StorageStart;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      if (MenuOption->Tags[Index].Flags & EFI_IFR_FLAG_DEFAULT) {
        CopyMem (&NvRamMap[CachedStart], &MenuOption->Tags[Index].Value, 2);
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      CopyMem (&NvRamMap[MenuOption->Tags[Index].StorageStart], &MenuOption->Tags[Index].Flags, 1);
      break;

    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_DATE_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_STRING_OP:
    case EFI_IFR_PASSWORD_OP:
      CopyMem (
        &NvRamMap[MenuOption->Tags[Index].StorageStart],
        &MenuOption->Tags[Index].Value,
        MenuOption->Tags[Index].StorageWidth
        );
      break;

    }
  }

  FreePool (VariableDefinition->NvRamMap);
  VariableDefinition->NvRamMap          = NvRamMap;
  VariableDefinition->VariableFakeSize  = (UINT16) SizeRequired;
}

EFI_STATUS
ProcessOptions (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  EFI_IFR_DATA_ARRAY          *PageData,
  OUT CHAR16                      **OptionString
  )
{
  EFI_STATUS                  Status;
  CHAR16                      *StringPtr;
  UINTN                       Index;
  UINTN                       CachedIndex;
  EFI_FILE_FORM_TAGS          *FileFormTags;
  EFI_TAG                     *Tag;
  CHAR16                      FormattedNumber[6];
  UINT16                      Number;
  UINT16                      Value;
  UINT16                      *ValueArray;
  UINT16                      *NvRamMap;
  CHAR8                       *TmpNvRamMap;
  UINTN                       Default;
  UINTN                       StringCount;
  CHAR16                      Character[2];
  UINTN                       Count;
  EFI_TIME                    Time;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  STRING_REF                  PopUp;
  CHAR16                      NullCharacter;
  EFI_INPUT_KEY               Key;
  EFI_VARIABLE_DEFINITION     *VariableDefinition;
  BOOLEAN                     OrderedList;
  BOOLEAN                     Initialized;
  UINT16                      KeyValue;
  BOOLEAN                     Skip;

  FileFormTags = FileFormTagsHead;

  for (Index = 0; Index < MenuOption->IfrNumber; Index++) {
    FileFormTags = FileFormTags->NextFile;
  }

  OrderedList         = FALSE;
  Initialized         = FALSE;
  ValueArray          = NULL;
  VariableDefinition  = NULL;
  Skip                = FALSE;

  ZeroMem (&Time, sizeof (EFI_TIME));

  StringPtr = (CHAR16 *) L"\0";
  Tag       = MenuOption->ThisTag;
  ExtractRequestedNvMap (FileFormTags, Tag->VariableNumber, &VariableDefinition);

  if (Tag->StorageStart > VariableDefinition->VariableSize) {
    NvRamMap = (UINT16 *) &VariableDefinition->FakeNvRamMap[Tag->StorageStart];
  } else {
    NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
  }

  StringCount   = 0;
  Character[1]  = 0;
  Count         = 0;
  Default       = 0;
  NullCharacter = CHAR_NULL;
  FormCallback  = NULL;

  if (MenuOption->ThisTag->Operand == EFI_IFR_ORDERED_LIST_OP) {
    OrderedList = TRUE;
    if (((UINT8 *) NvRamMap)[0] != 0x00) {
      Initialized = TRUE;
    }
  }

  ZeroMem (FormattedNumber, 12);

  Status = gBS->HandleProtocol (
                  (VOID *) (UINTN) FileFormTags->FormTags.Tags[0].CallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  (VOID **) &FormCallback
                  );

  if (*OptionString != NULL) {
    FreePool (*OptionString);
    *OptionString = NULL;
  }

  switch (Tag->Operand) {

  case EFI_IFR_ORDERED_LIST_OP:
  case EFI_IFR_ONE_OF_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    CachedIndex = MenuOption->TagIndex;

    //
    // search for EFI_IFR_ONE_OF_OPTION_OP until you hit the EFI_IFR_END_ONE_OF_OP,
    // each of the .Text in the options are going to be what gets displayed.  Break each into 26 char chunks
    // when hit right/left arrow allows for selection - then repopulate Tag[TagIndex] with the choice
    //
    for (Index = MenuOption->TagIndex; MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP; Index++) {
      //
      // We found an option - which assumedly has a string.  We will eventually have to support
      // wrapping of strings.  For now, let's pretend they don't wrap and code that up.
      //
      // Count how many strings there are
      //
      if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
        //
        // If one of the options for the one-of has an interactive flag, back-define the oneof to have one too
        //
        if (MenuOption->Tags[Index].Flags & EFI_IFR_FLAG_INTERACTIVE) {
          MenuOption->Tags[CachedIndex].Flags = (UINT8) (MenuOption->Tags[CachedIndex].Flags | EFI_IFR_FLAG_INTERACTIVE);
        }

        StringCount++;
      }
    }
    //
    // We now know how many strings we will have, so we can allocate the
    // space required for the array or strings.
    //
    *OptionString = AllocateZeroPool (StringCount * (gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    //
    // Add left delimeter to string
    //
    *OptionString[0] = LEFT_ONEOF_DELIMITER;

    //
    // Retrieve the current OneOf value
    //
    if (Selected) {
      //
      // Auto selection from list
      //
      Value = 0;
      //
      // Copy current setting to the seed Value
      //
      if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
        ValueArray = AllocateZeroPool (MenuOption->ThisTag->StorageWidth);
        ASSERT (ValueArray != NULL);
        CopyMem (ValueArray, NvRamMap, MenuOption->ThisTag->StorageWidth);
      } else {
        CopyMem (&Value, NvRamMap, MenuOption->ThisTag->StorageWidth);
        CopyMem (gPreviousValue, NvRamMap, MenuOption->ThisTag->StorageWidth);
      }

      Number = Value;
      if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
        Status = GetSelectionInputPopUp (MenuOption, Tag, MenuOption->ThisTag->StorageWidth, ValueArray, &KeyValue);
      } else {
        Status = GetSelectionInputPopUp (MenuOption, Tag, 1, &Value, &KeyValue);
      }

      if (!EFI_ERROR (Status)) {
        if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
          CopyMem (NvRamMap, ValueArray, MenuOption->ThisTag->StorageWidth);
          FreePool (ValueArray);
        } else {
          //
          // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
          //
          CopyMem (NvRamMap, &Value, Tag->StorageWidth);
          MenuOption->ThisTag->Key = KeyValue;
        }
        //
        // If a late check is required save off the information.  This is used when consistency checks
        // are required, but certain values might be bound by an impossible consistency check such as
        // if two questions are bound by consistency checks and each only has two possible choices, there
        // would be no way for a user to switch the values.  Thus we require late checking.
        //
        if (Tag->Flags & EFI_IFR_FLAG_LATE_CHECK) {
          CopyMem (&Tag->OldValue, &Value, Tag->StorageWidth);
        } else {
          //
          // In theory, passing the value and the Id are sufficient to determine what needs
          // to be done.  The Id is the key to look for the entry needed in the Inconsistency
          // database.  That will yields operand and ID data - and since the ID's correspond
          // to the NV storage, we can determine the values for other IDs there.
          //
          if (ValueIsNotValid (TRUE, 0, Tag, FileFormTags, &PopUp)) {
            if (PopUp == 0x0000) {
              //
              // Restore Old Value
              //
              if (!Tag->Suppress && !Tag->GrayOut) {
                CopyMem (NvRamMap, &Number, MenuOption->ThisTag->StorageWidth);
              }
              break;
            }

            StringPtr = GetToken (PopUp, MenuOption->Handle);

            CreatePopUp (GetStringWidth (StringPtr) / 2, 3, &NullCharacter, StringPtr, &NullCharacter);

            do {
              Status = WaitForKeyStroke (&Key);

              switch (Key.UnicodeChar) {

              case CHAR_CARRIAGE_RETURN:
                //
                // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
                //
                CopyMem (NvRamMap, &Number, MenuOption->ThisTag->StorageWidth);
                FreePool (StringPtr);
                break;

              default:
                break;
              }
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
          }
        }

        UpdateStatusBar (NV_UPDATE_REQUIRED, Tag->Flags, TRUE);
      } else {
        if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
          FreePool (ValueArray);
        }

        return EFI_SUCCESS;
      }
    } else {
      for (Index = MenuOption->TagIndex; MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP; Index++) {
        //
        // We found an option - which assumedly has a string.  We will eventually have to support
        // wrapping of strings.  For now, let's pretend they don't wrap and code that up.
        //
        if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
          if (OrderedList) {
            if (!Initialized) {
              //
              // If the first entry is invalid, then the "default" settings are based on what is reflected
              // in the order of the op-codes
              //
              ((UINT8 *) NvRamMap)[Index - MenuOption->TagIndex - 1] = (UINT8) MenuOption->Tags[Index].Value;
            }
            //
            // Only display 3 lines of stuff at most
            //
            if ((Index - MenuOption->TagIndex) > ORDERED_LIST_SIZE) {
              break;
            }

            if (((Index - MenuOption->TagIndex) != 1) && !Skip) {
              Character[0] = LEFT_ONEOF_DELIMITER;
              NewStrCat (OptionString[0], Character);
            }

            MenuOption->ThisTag->NumberOfLines = (UINT16) (Index - MenuOption->TagIndex);
            if (!Initialized) {
              StringPtr = GetToken (MenuOption->Tags[Index].Text, MenuOption->Handle);
            } else {
              for (Value = (UINT16) (MenuOption->TagIndex + 1);
                   MenuOption->Tags[Value].Operand != EFI_IFR_END_ONE_OF_OP;
                   Value++
                  ) {
                if (MenuOption->Tags[Value].Value == ((UINT8 *) NvRamMap)[Index - MenuOption->TagIndex - 1]) {
                  StringPtr = GetToken (MenuOption->Tags[Value].Text, MenuOption->Handle);
                  break;
                }
              }

              if (MenuOption->Tags[Value].Operand == EFI_IFR_END_ONE_OF_OP) {
                Skip = TRUE;
                continue;
              }
            }

            Skip = FALSE;
            NewStrCat (OptionString[0], StringPtr);
            Character[0] = RIGHT_ONEOF_DELIMITER;
            NewStrCat (OptionString[0], Character);
            Character[0] = CHAR_CARRIAGE_RETURN;
            NewStrCat (OptionString[0], Character);

            //
            // Remove Buffer allocated for StringPtr after it has been used.
            //
            FreePool (StringPtr);
          } else {
            //
            // The option value is the same as what is stored in NV store.  Print this.
            //
            if (!CompareMem (&(MenuOption->Tags[Index].Value), NvRamMap, MenuOption->ThisTag->StorageWidth)) {
              StringPtr = GetToken (MenuOption->Tags[Index].Text, MenuOption->Handle);
              NewStrCat (OptionString[0], StringPtr);
              Character[0] = RIGHT_ONEOF_DELIMITER;
              NewStrCat (OptionString[0], Character);
              //
              // Remove Buffer allocated for StringPtr after it has been used.
              //
              FreePool (StringPtr);
              Default = 0;
              break;
            }

            if ((MenuOption->Tags[Index].Flags & EFI_IFR_FLAG_DEFAULT) == 1) {
              Default = MenuOption->Tags[Index].Text;
              Value   = MenuOption->Tags[Index].Value;
            };
          }
        }
      }
      //
      // We didn't find a value that matched a setting in the NVRAM Map - display default - set default
      //
      if (Default != 0) {
        //
        // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
        //
        CopyMem (NvRamMap, &Value, MenuOption->ThisTag->StorageWidth);

        StringPtr = GetToken ((UINT16) Default, MenuOption->Handle);
        NewStrCat (OptionString[0], StringPtr);
        Character[0] = RIGHT_ONEOF_DELIMITER;
        NewStrCat (OptionString[0], Character);
        //
        // Remove Buffer allocated for StringPtr after it has been used.
        //
        FreePool (StringPtr);
      }
    }
    break;

  case EFI_IFR_CHECKBOX_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    Default = Tag->Flags & 1;
    //
    // If hit spacebar, set or unset Tag[TagIndex].Flags based on it's previous value - BOOLEAN
    //
    *OptionString = AllocateZeroPool ((gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    //
    // Since Checkboxes are BOOLEAN values, bit 0 of the Flags bit defines the default option, therefore, if
    // the default option (only one option for checkboxes) is on, then the default value is on.  Tag.Default is not
    // an active field for Checkboxes.
    //
    StrnCpy (OptionString[0], (CHAR16 *) LEFT_CHECKBOX_DELIMITER, 1);

    //
    // Since this is a BOOLEAN operation, flip bit 0 upon selection
    //
    if (Selected) {
      Tag->Value          = (UINT16) (Tag->Value ^ 1);
      *(UINT8 *) NvRamMap = (UINT8) (Tag->Value & 1);
      UpdateStatusBar (NV_UPDATE_REQUIRED, Tag->Flags, TRUE);
    }

    if ((*(UINT8 *) NvRamMap & 1) == 0x01) {
      NewStrCat (OptionString[0], (CHAR16 *) CHECK_ON);
      //
      // If someone reset default variables - we may need to reload from our NvMapping....
      //
      Tag->Value = *(UINT8 *) NvRamMap;
    } else {
      //
      // If someone reset default variables - we may need to reload from our NvMapping....
      //
      NewStrCat (OptionString[0], (CHAR16 *) CHECK_OFF);
      Tag->Value = *(UINT8 *) NvRamMap;
    }

    NewStrCat (OptionString[0], (CHAR16 *) RIGHT_CHECKBOX_DELIMITER);
    NewStrCat (OptionString[0], StringPtr);
    break;

  case EFI_IFR_NUMERIC_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    *OptionString = AllocateZeroPool ((gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    //
    // Add left delimeter to string
    //
    *OptionString[0] = LEFT_NUMERIC_DELIMITER;

    //
    // Retrieve the current numeric value
    //
    if (Selected) {
      //
      // Go ask for input
      //
      if (Tag->Step == 0) {
        //
        // Manual Input
        //
        Status = GetNumericInput (MenuOption, FileFormTagsHead, TRUE, Tag, REGULAR_NUMERIC, &Number);
        if (!EFI_ERROR (Status)) {
          CopyMem (gPreviousValue, NvRamMap, MenuOption->ThisTag->StorageWidth);
          UpdateStatusBar (NV_UPDATE_REQUIRED, Tag->Flags, TRUE);

          //
          // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
          //
          CopyMem (NvRamMap, &Number, MenuOption->ThisTag->StorageWidth);
        } else {
          return EFI_SUCCESS;
        }
      } else {
        //
        // Auto selection from list
        //
        if ((((Tag->StorageWidth == 1) && (UINT8) (*NvRamMap) > Tag->Maximum) || ((UINT8) (*NvRamMap) < Tag->Minimum)) ||
            (((Tag->StorageWidth == 2) && *NvRamMap > Tag->Maximum) || (*NvRamMap < Tag->Minimum))
            ) {
          //
          // Seed Number with valid value if currently invalid
          //
          Number = Tag->Default;
        } else {
          if (Tag->StorageWidth == 1) {
            Number = (UINT8) (*NvRamMap);
          } else {
            Number = *NvRamMap;
          }
        }

        Status = GetNumericInput (MenuOption, FileFormTagsHead, FALSE, Tag, REGULAR_NUMERIC, &Number);
        if (!EFI_ERROR (Status)) {
          CopyMem (gPreviousValue, NvRamMap, MenuOption->ThisTag->StorageWidth);
          UpdateStatusBar (NV_UPDATE_REQUIRED, Tag->Flags, TRUE);

          //
          // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
          //
          CopyMem (NvRamMap, &Number, MenuOption->ThisTag->StorageWidth);
        } else {
          return EFI_SUCCESS;
        }
      }
    } else {
      if (((Tag->StorageWidth == 1) && (UINT8) (*NvRamMap) <= Tag->Maximum && (UINT8) (*NvRamMap) >= Tag->Minimum) ||
          ((Tag->StorageWidth == 2) && *NvRamMap <= Tag->Maximum && *NvRamMap >= Tag->Minimum)
          ) {
        if (Tag->StorageWidth == 1) {
          Number = (UINT8) (*NvRamMap);
        } else {
          Number = *NvRamMap;
        }
        UnicodeValueToString (
          FormattedNumber,
          FALSE,
          (UINTN) Number,
          (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
          );
        Number = (UINT16) GetStringWidth (FormattedNumber);
        StrnCpy (OptionString[0] + 1, FormattedNumber, Number);
      } else {
        //
        // If *NvRamMap isn't within parameters, set it to within parameters
        //
        //
        // Since the value can be one byte long or two bytes long, do a CopyMem based on StorageWidth
        //
        CopyMem (NvRamMap, &Tag->Default, MenuOption->ThisTag->StorageWidth);
        Number = Tag->Default;

        UnicodeValueToString (
          FormattedNumber,
          FALSE,
          (UINTN) Number,
          (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
          );
        Number = (UINT16) GetStringWidth (FormattedNumber);
        StrnCpy (OptionString[0] + 1, FormattedNumber, Number);
      }

      *(OptionString[0] + Number / 2) = RIGHT_NUMERIC_DELIMITER;
      NewStrCat (OptionString[0] + (Number / 2) + 1, StringPtr);
    }
    break;

  case EFI_IFR_DATE_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    Status = gRT->GetTime (&Time, NULL);
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    //
    // This for loop advances Index till it points immediately after a date entry.  We can then
    // subtract MenuOption->TagIndex from Index and find out relative to the start of the Date
    // structure which field we were in.  For instance, if TagIndex was 52, and we advanced Index
    // to 53 and found it to no longer point to a date operand, we were pointing to the last of 3
    // date operands.
    //
    //
    // This has BUGBUG potential....fix this - if someone wants to ask two DATE questions in a row.....code
    // against such silliness.
    //
    // Also, we want to internationalize the order of the date information.  We need to code for it as well.
    //
    for (Index = MenuOption->TagIndex; MenuOption->Tags[Index].Operand == EFI_IFR_DATE_OP; Index++)
      ;

    //
    // Count 0 = We entered on the first Date operand
    // Count 1 = We entered on the second Date operand
    // Count 2 = We entered on the third Date operand
    //
    Count = 3 - (Index - MenuOption->TagIndex);
    if (Count > 2) {
      return EFI_SUCCESS;
    }
    //
    // This is similar to numerics, except for the following:
    // We will under normal circumstances get 3 consecutive calls
    // to process this opcodes data.
    //
    *OptionString = AllocateZeroPool ((gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    switch (Count) {
    case 0:
      if (Selected) {
        Number = (UINT16) Time.Month;

        if (Tag->Step == 0) {
          MenuOption->OptCol++;
          Status = GetNumericInput (MenuOption, FileFormTagsHead, TRUE, Tag, DATE_NUMERIC, &Number);
        } else {
          //
          // Seed value with current setting
          //
          Tag->Value  = (UINT16) Time.Month;
          Status      = GetNumericInput (MenuOption, FileFormTagsHead, FALSE, Tag, DATE_NUMERIC, &Number);
        }

        if (!EFI_ERROR (Status)) {
          Time.Month = (UINT8) Number;
          gRT->SetTime (&Time);
        }
      }

      VariableDefinition->FakeNvRamMap[Tag->Id] = Time.Month;
      *OptionString[0]                          = LEFT_NUMERIC_DELIMITER;

      UnicodeValueToString (
        FormattedNumber,
        FALSE,
        (UINTN) Time.Month,
        (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
        );
      Number = (UINT16) GetStringWidth (FormattedNumber);

      if (Number == 4) {
        FormattedNumber[2]  = FormattedNumber[1];
        FormattedNumber[1]  = FormattedNumber[0];
        FormattedNumber[0]  = L'0';
        Number              = 6;
      }

      StrnCpy (OptionString[0] + 1, FormattedNumber, Number);
      *(OptionString[0] + Number / 2) = DATE_SEPARATOR;
      StrCat (OptionString[0] + (Number / 2) + 1, StringPtr);
      break;

    case 1:
      if (Selected) {
        Number = (UINT16) Time.Day;

        if (Tag->Step == 0) {
          Status = GetNumericInput (MenuOption, FileFormTagsHead, TRUE, Tag, DATE_NUMERIC, &Number);
        } else {
          //
          // Seed value with current setting
          //
          Tag->Value  = (UINT16) Time.Day;
          Status      = GetNumericInput (MenuOption, FileFormTagsHead, FALSE, Tag, DATE_NUMERIC, &Number);
        }

        if (!EFI_ERROR (Status)) {
          Time.Day = (UINT8) Number;
          gRT->SetTime (&Time);
        }
      }

      VariableDefinition->FakeNvRamMap[Tag->Id] = Time.Day;
      SetUnicodeMem (OptionString[0], 4, L' ');

      UnicodeValueToString (
        FormattedNumber,
        FALSE,
        (UINTN) Time.Day,
        (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
        );
      Number = (UINT16) GetStringWidth (FormattedNumber);
      if (Number == 4) {
        FormattedNumber[2]  = FormattedNumber[1];
        FormattedNumber[1]  = FormattedNumber[0];
        FormattedNumber[0]  = L'0';
        Number              = 6;
      }

      StrnCpy (OptionString[0] + 4, FormattedNumber, Number);
      *(OptionString[0] + Number / 2 + 3) = DATE_SEPARATOR;
      StrCat (OptionString[0] + (Number / 2) + 4, StringPtr);
      break;

    case 2:
      if (Selected) {
        Number = (UINT16) Time.Year;

        if (Tag->Step == 0) {
          Status = GetNumericInput (MenuOption, FileFormTagsHead, TRUE, Tag, DATE_NUMERIC, &Number);
        } else {
          //
          // Seed value with current setting
          //
          Status = GetNumericInput (MenuOption, FileFormTagsHead, FALSE, Tag, DATE_NUMERIC, &Number);
        }

        if (!EFI_ERROR (Status)) {
          Time.Year = (UINT16) Number;
          gRT->SetTime (&Time);
        }
      }

      Tag->Value  = (UINT16) Time.Year;
      VariableDefinition->FakeNvRamMap[Tag->Id]     = (UINT8) Tag->Value;
      VariableDefinition->FakeNvRamMap[Tag->Id + 1] = (UINT8) (Tag->Value >> 8);
      SetUnicodeMem (OptionString[0], 7, L' ');
      UnicodeValueToString (
        FormattedNumber,
        FALSE,
        (UINTN) Time.Year,
        (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
        );
      Number = (UINT16) GetStringWidth (FormattedNumber);
      StrnCpy (OptionString[0] + 7, FormattedNumber, Number);
      *(OptionString[0] + Number / 2 + 6) = RIGHT_NUMERIC_DELIMITER;
      StrCat (OptionString[0] + (Number / 2) + 7, StringPtr);
      break;
    }

    break;

  //
  // BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG
  // We need to add code to support the NVRam storage version of Date - this is the 1% case where someone
  // might want to set an alarm and actually preserve the data in NVRam so a driver can pick up the instruction
  // BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG
  //
  case EFI_IFR_TIME_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    Status = gRT->GetTime (&Time, NULL);
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    //
    // This is similar to numerics, except for the following:
    // We will under normal circumstances get 3 consecutive calls
    // to process this opcodes data.
    //
    *OptionString = AllocateZeroPool ((gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    //
    // This for loop advances Index till it points immediately after a date entry.  We can then
    // subtract MenuOption->TagIndex from Index and find out relative to the start of the Date
    // structure which field we were in.  For instance, if TagIndex was 52, and we advanced Index
    // to 53 and found it to no longer point to a date operand, we were pointing to the last of 3
    // date operands.
    //
    for (Index = MenuOption->TagIndex; MenuOption->Tags[Index].Operand == EFI_IFR_TIME_OP; Index++)
      ;
    //
    // Count 0 = We entered on the first Date operand
    // Count 1 = We entered on the second Date operand
    // Count 2 = We entered on the third Date operand
    //
    Count = 3 - (Index - MenuOption->TagIndex);
    if (Count > 2) {
      return EFI_SUCCESS;
    }

    switch (Count) {
    case 0:
      Number = Time.Hour;
      break;

    case 1:
      Number = Time.Minute;
      break;

    case 2:
      Number = Time.Second;
    }
    //
    // Retrieve the current numeric value
    //
    if (Selected) {
      //
      // Go ask for input
      //
      if (Tag->Step == 0) {
        //
        // Manual Input
        //
        Status = GetNumericInput (MenuOption, FileFormTagsHead, TRUE, Tag, TIME_NUMERIC, &Number);
        if (!EFI_ERROR (Status)) {
          *NvRamMap       = Number;
          Time.Nanosecond = 0;
          gRT->SetTime (&Time);
        } else {
          return EFI_SUCCESS;
        }
      } else {
        //
        // Auto selection from list
        //
        Status = GetNumericInput (MenuOption, FileFormTagsHead, FALSE, Tag, TIME_NUMERIC, &Number);
        if (!EFI_ERROR (Status)) {
          *NvRamMap = Number;
        } else {
          return EFI_SUCCESS;
        }
      }

      switch (Count) {
      case 0:
        Time.Hour = (UINT8) Number;
        break;

      case 1:
        Time.Minute = (UINT8) Number;
        break;

      case 2:
        Time.Second = (UINT8) Number;
      }

      Time.Nanosecond = 0;
      gRT->SetTime (&Time);
    } else {
      switch (Count) {
      case 0:
        *OptionString[0] = LEFT_NUMERIC_DELIMITER;
        UnicodeValueToString (
          FormattedNumber,
          FALSE,
          (UINTN) Time.Hour,
          (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
          );
        Number = (UINT16) GetStringWidth (FormattedNumber);
        if (Number == 4) {
          FormattedNumber[2]  = FormattedNumber[1];
          FormattedNumber[1]  = FormattedNumber[0];
          FormattedNumber[0]  = L'0';
          Number              = 6;
        }

        StrnCpy (OptionString[0] + 1, FormattedNumber, Number);
        *(OptionString[0] + Number / 2) = TIME_SEPARATOR;
        StrCat (OptionString[0] + (Number / 2) + 1, StringPtr);
        break;

      case 1:
        SetUnicodeMem (OptionString[0], 4, L' ');
        UnicodeValueToString (
          FormattedNumber,
          FALSE,
          (UINTN) Time.Minute,
          (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
          );
        Number = (UINT16) GetStringWidth (FormattedNumber);
        if (Number == 4) {
          FormattedNumber[2]  = FormattedNumber[1];
          FormattedNumber[1]  = FormattedNumber[0];
          FormattedNumber[0]  = L'0';
          Number              = 6;
        }

        StrnCpy (OptionString[0] + 4, FormattedNumber, Number);
        *(OptionString[0] + Number / 2 + 3) = TIME_SEPARATOR;
        StrCat (OptionString[0] + (Number / 2) + 4, StringPtr);
        break;

      case 2:
        SetUnicodeMem (OptionString[0], 7, L' ');
        UnicodeValueToString (
          FormattedNumber,
          FALSE,
          (UINTN) Time.Second,
          (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
          );
        Number = (UINT16) GetStringWidth (FormattedNumber);
        if (Number == 4) {
          FormattedNumber[2]  = FormattedNumber[1];
          FormattedNumber[1]  = FormattedNumber[0];
          FormattedNumber[0]  = L'0';
          Number              = 6;
        }

        StrnCpy (OptionString[0] + 7, FormattedNumber, Number);
        *(OptionString[0] + Number / 2 + 6) = RIGHT_NUMERIC_DELIMITER;
        StrCat (OptionString[0] + (Number / 2) + 7, StringPtr);
        break;
      }
      //
      // BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG
      // We need to add code to support the NVRam storage version of Date - this is the 1% case where someone
      // might want to set an alarm and actually preserve the data in NVRam so a driver can pick up the instruction
      // BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG BUGBUG
      //
    }
    break;

  case EFI_IFR_STRING_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    *OptionString = AllocateZeroPool ((gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow);
    ASSERT (*OptionString);

    if (Selected) {
      StringPtr = AllocateZeroPool (Tag->Maximum);
      ASSERT (StringPtr);

      Status = ReadString (MenuOption, StringPtr);

      if (!EFI_ERROR (Status)) {
        CopyMem (gPreviousValue, NvRamMap, MenuOption->ThisTag->StorageWidth);
        CopyMem (&VariableDefinition->NvRamMap[Tag->StorageStart], StringPtr, Tag->StorageWidth);

        UpdateStatusBar (NV_UPDATE_REQUIRED, Tag->Flags, TRUE);
      }

      FreePool (StringPtr);
      return Status;
    } else {
      for (Index = 0; Index < gOptionBlockWidth; Index++) {
        if (VariableDefinition->NvRamMap[Tag->StorageStart + (Index * 2)] != 0x0000) {
          CopyMem (OptionString[0] + Index, &VariableDefinition->NvRamMap[Tag->StorageStart + (Index * 2)], 2);
        } else {
          if (Index == 0) {
            *(OptionString[0] + Index)      = '_';
            *(OptionString[0] + 1 + Index)  = 0;
          }
          break;
        }
      }

      return Status;
    }

  case EFI_IFR_PASSWORD_OP:
    //
    // If the op-code we are looking at is larger than the latest created NvMap - we likely encountered a dynamically
    // created entry which has an expanded NvMap requirement.  We won't save this information - but we need to adjust
    // the NvMap so that we can properly display the information
    //
    if ((UINTN) (Tag->StorageStart + Tag->StorageWidth) > VariableDefinition->VariableFakeSize) {
      AdjustNvMap (FileFormTags, MenuOption);
      NvRamMap = (UINT16 *) &VariableDefinition->NvRamMap[Tag->StorageStart];
    }

    if (Selected) {
      StringPtr = AllocateZeroPool (Tag->Maximum);
      ASSERT (StringPtr);

      //
      // If interactive, read the password and do the appropriate callbacks in that routine.
      // Since interactive passwords assume to handle the password data in a separate variable
      // storage, we don't need to do more than what is below for password callbacks
      //
      if (Tag->Flags & EFI_IFR_FLAG_INTERACTIVE) {
        MenuOption->Tags[0].CallbackHandle  = FileFormTags->FormTags.Tags[0].CallbackHandle;
        Status = ReadPassword (MenuOption, TRUE, Tag, PageData, FALSE, FileFormTags, StringPtr);
        ZeroMem (StringPtr, Tag->Maximum);

        if (EFI_ERROR (Status)) {
          if (Status == EFI_NOT_READY) {
            FreePool (StringPtr);
            return EFI_SUCCESS;
          }
        }

        Status = ReadPassword (MenuOption, TRUE, Tag, PageData, TRUE, FileFormTags, StringPtr);
        FreePool (StringPtr);
        return EFI_SUCCESS;
      }

      for (Index = 0; Index < Tag->Maximum; Index++) {
        if (VariableDefinition->NvRamMap[Tag->StorageStart + Index] != 0x00) {
          //
          // There is something there!  Prompt for password
          //
          Status = ReadPassword (MenuOption, TRUE, Tag, PageData, FALSE, FileFormTags, StringPtr);
          if (EFI_ERROR (Status)) {
            FreePool (StringPtr);
            return EFI_SUCCESS;
          }

          if (Tag->Encoding == 1) {
            EncodePassword (StringPtr, (UINT8) Tag->Maximum);
            Status = CompareMem (StringPtr, &VariableDefinition->NvRamMap[Tag->StorageStart], Tag->Maximum);
          } else {
            Status = CompareMem (StringPtr, &VariableDefinition->NvRamMap[Tag->StorageStart], Tag->Maximum);
          }

          if (Status != 0) {
            FreePool (StringPtr);
            return EFI_SUCCESS;
          } else {
            break;
          }
        }
      }
      //
      // Clean the string
      //
      ZeroMem (StringPtr, Tag->Maximum);

      //
      // No password set!  Go ahead and prompt the user for a password.
      //
      Status = ReadPassword (MenuOption, FALSE, Tag, PageData, FALSE, FileFormTags, StringPtr);

      if (EFI_ERROR (Status)) {
        //
        // User couldn't figure out how to type two identical passwords
        //
        FreePool (StringPtr);
        return EFI_SUCCESS;
      }
      //
      // Very simple example of how one MIGHT do password encoding
      //
      if (Tag->Encoding == 1) {
        EncodePassword (StringPtr, (UINT8) Tag->Maximum);
      }

      TmpNvRamMap = AllocatePool (VariableDefinition->VariableSize);
      ASSERT (TmpNvRamMap != NULL);

      Count = VariableDefinition->VariableSize;

      if ((FormCallback != NULL) && (FormCallback->NvRead != NULL)) {
        Status = FormCallback->NvRead (
                                FormCallback,
                                VariableDefinition->VariableName,
                                &VariableDefinition->Guid,
                                NULL,
                                &Count,
                                (VOID *) TmpNvRamMap
                                );
      } else {
        Status = gRT->GetVariable (
                        VariableDefinition->VariableName,
                        &VariableDefinition->Guid,
                        NULL,
                        &Count,
                        (VOID *) TmpNvRamMap
                        );
      }

      CopyMem (&VariableDefinition->NvRamMap[Tag->StorageStart], StringPtr, Tag->StorageWidth);
      CopyMem (&TmpNvRamMap[Tag->StorageStart], StringPtr, Tag->StorageWidth);

      if ((FormCallback != NULL) && (FormCallback->NvWrite != NULL)) {
        Status = FormCallback->NvWrite (
                                FormCallback,
                                VariableDefinition->VariableName,
                                &VariableDefinition->Guid,
                                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                VariableDefinition->VariableSize,
                                (VOID *) TmpNvRamMap,
                                &gResetRequired
                                );
      } else {
        Status = gRT->SetVariable (
                        VariableDefinition->VariableName,
                        &VariableDefinition->Guid,
                        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                        VariableDefinition->VariableSize,
                        (VOID *) TmpNvRamMap
                        );
      }

      FreePool (TmpNvRamMap);
      FreePool (StringPtr);
      break;
    }

  default:
    break;
  }

  return EFI_SUCCESS;
}

/**
  Split StringPtr to several lines of strings stored in FormattedString and the glyph width of
  each line cannot exceed gHelpBlockWidth.

  @param StringPtr          The pointer of string
  @param FormattedString    The pointer of format string
  @param RowCount           The count of row

**/
VOID
ProcessHelpString (
  IN  CHAR16  *StringPtr,
  OUT CHAR16  **FormattedString,
  IN  UINTN   RowCount
  )
{
  CONST UINTN BlockWidth = (UINTN) gHelpBlockWidth - 1;
  UINTN AllocateSize;
  //
  // [PrevCurrIndex, CurrIndex) forms a range of a screen-line
  //
  UINTN CurrIndex;
  UINTN PrevCurrIndex;
  UINTN LineCount;
  UINTN VirtualLineCount;
  //
  // GlyphOffset stores glyph width of current screen-line
  //
  UINTN GlyphOffset;
  //
  // GlyphWidth equals to 2 if we meet width directive
  //
  UINTN GlyphWidth;
  //
  // during scanning, we remember the position of last space character
  // in case that if next word cannot put in current line, we could restore back to the position
  // of last space character
  // while we should also remmeber the glyph width of the last space character for restoring
  //
  UINTN LastSpaceIndex;
  UINTN LastSpaceGlyphWidth;
  //
  // every time we begin to form a new screen-line, we should remember glyph width of single character
  // of last line
  //
  UINTN LineStartGlyphWidth;
  UINTN *IndexArray;
  UINTN *OldIndexArray;

  //
  // every three elements of IndexArray form a screen-line of string:[ IndexArray[i*3], IndexArray[i*3+1] )
  // IndexArray[i*3+2] stores the initial glyph width of single character. to save this is because we want
  // to bring the width directive of the last line to current screen-line.
  // e.g.: "\wideabcde ... fghi", if "fghi" also has width directive but is splitted to the next screen-line
  // different from that of "\wideabcde", we should remember the width directive.
  //
  AllocateSize  = 0x20;
  IndexArray    = AllocatePool (AllocateSize * sizeof (UINTN) * 3);

  if (*FormattedString != NULL) {
    FreePool (*FormattedString);
    *FormattedString = NULL;
  }

  for (PrevCurrIndex = 0, CurrIndex  = 0, LineCount   = 0, LastSpaceIndex = 0,
       IndexArray[0] = 0, GlyphWidth = 1, GlyphOffset = 0, LastSpaceGlyphWidth = 1, LineStartGlyphWidth = 1;
       (StringPtr[CurrIndex] != CHAR_NULL);
       CurrIndex ++) {

    if (LineCount == AllocateSize) {
      AllocateSize   += 0x10;
      OldIndexArray  =  IndexArray;
      IndexArray     =  AllocatePool (AllocateSize * sizeof (UINTN) * 3);
      CopyMem (IndexArray, OldIndexArray, LineCount * sizeof (UINTN) * 3);
      if (OldIndexArray != NULL) {
        FreePool (OldIndexArray);
      }
    }

    switch (StringPtr[CurrIndex]) {

      case NARROW_CHAR:
      case WIDE_CHAR:
        GlyphWidth = ((StringPtr[CurrIndex] == WIDE_CHAR) ? 2 : 1);
        if (CurrIndex == 0) {
          LineStartGlyphWidth = GlyphWidth;
        }
        break;

      //
      // char is '\n'
      // "\r\n" isn't handled here, handled by case CHAR_CARRIAGE_RETURN
      //
      case CHAR_LINEFEED:
        //
        // Store a range of string as a line
        //
        IndexArray[LineCount*3]   = PrevCurrIndex;
        IndexArray[LineCount*3+1] = CurrIndex;
        IndexArray[LineCount*3+2] = LineStartGlyphWidth;
        LineCount ++;
        //
        // Reset offset and save begin position of line
        //
        GlyphOffset = 0;
        LineStartGlyphWidth = GlyphWidth;
        PrevCurrIndex = CurrIndex + 1;
        break;

      //
      // char is '\r'
      // "\r\n" and "\r" both are handled here
      //
      case CHAR_CARRIAGE_RETURN:
        if (StringPtr[CurrIndex + 1] == CHAR_LINEFEED) {
          //
          // next char is '\n'
          //
          IndexArray[LineCount*3]   = PrevCurrIndex;
          IndexArray[LineCount*3+1] = CurrIndex;
          IndexArray[LineCount*3+2] = LineStartGlyphWidth;
          LineCount ++;
          CurrIndex ++;
        }
        GlyphOffset = 0;
        LineStartGlyphWidth = GlyphWidth;
        PrevCurrIndex = CurrIndex + 1;
        break;

      //
      // char is space or other char
      //
      default:
        GlyphOffset     += GlyphWidth;
        if (GlyphOffset >= BlockWidth) {
          if (LastSpaceIndex > PrevCurrIndex) {
            //
            // LastSpaceIndex points to space inside current screen-line,
            // restore to LastSpaceIndex
            // (Otherwise the word is too long to fit one screen-line, just cut it)
            //
            CurrIndex  = LastSpaceIndex;
            GlyphWidth = LastSpaceGlyphWidth;
          } else if (GlyphOffset > BlockWidth) {
            //
            // the word is too long to fit one screen-line and we don't get the chance
            // of GlyphOffset == BlockWidth because GlyphWidth = 2
            //
            CurrIndex --;
          }

          IndexArray[LineCount*3]   = PrevCurrIndex;
          IndexArray[LineCount*3+1] = CurrIndex + 1;
          IndexArray[LineCount*3+2] = LineStartGlyphWidth;
          LineStartGlyphWidth = GlyphWidth;
          LineCount ++;
          //
          // Reset offset and save begin position of line
          //
          GlyphOffset                 = 0;
          PrevCurrIndex               = CurrIndex + 1;
        }

        //
        // LastSpaceIndex: remember position of last space
        //
        if (StringPtr[CurrIndex] == CHAR_SPACE) {
          LastSpaceIndex      = CurrIndex;
          LastSpaceGlyphWidth = GlyphWidth;
        }
        break;
    }
  }

  if (GlyphOffset > 0) {
    IndexArray[LineCount*3]   = PrevCurrIndex;
    IndexArray[LineCount*3+1] = CurrIndex;
    IndexArray[LineCount*3+2] = GlyphWidth;
    LineCount ++;
  }

  if (LineCount == 0) {
    //
    // in case we meet null string
    //
    IndexArray[0] = 0;
    IndexArray[1] = 1;
    //
    // we assume null string's glyph width is 1
    //
    IndexArray[1] = 1;
    LineCount ++;
  }

  VirtualLineCount = RowCount * (LineCount / RowCount + (LineCount % RowCount > 0));
  *FormattedString = AllocateZeroPool (VirtualLineCount * (BlockWidth + 1) * sizeof (CHAR16) * 2);

  for (CurrIndex = 0; CurrIndex < LineCount; CurrIndex ++) {
    *(*FormattedString + CurrIndex * 2 * (BlockWidth + 1)) = (CHAR16)((IndexArray[CurrIndex*3+2] == 2) ? WIDE_CHAR : NARROW_CHAR);
    StrnCpy (
      *FormattedString + CurrIndex * 2 * (BlockWidth + 1) + 1,
      StringPtr + IndexArray[CurrIndex*3],
      IndexArray[CurrIndex*3+1]-IndexArray[CurrIndex*3]
      );
  }

  if (IndexArray != NULL) {
    FreePool (IndexArray);
  }
}

VOID
IfrToFormTag (
  IN  UINT8               OpCode,
  IN  EFI_TAG             *TargetTag,
  IN  VOID                *FormData,
  EFI_VARIABLE_DEFINITION *VariableDefinitionsHead
  )
{
  UINT16                  TempValue;
  CHAR16                  *VariableName;
  CHAR8                   *AsciiString;
  EFI_VARIABLE_DEFINITION *VariableDefinitions;
  EFI_VARIABLE_DEFINITION *PreviousVariableDefinitions;
  STATIC UINT16           VariableSize;
  EFI_GUID                Guid;
  STATIC UINT16           CurrentVariable;
  STATIC UINT16           CurrentVariable2;
  UINTN                   Index;

  switch (OpCode) {
  case EFI_IFR_FORM_OP:
    CopyMem (&TargetTag->Id, &((EFI_IFR_FORM *) FormData)->FormId, sizeof (UINT16));
    CopyMem (&TargetTag->Text, &((EFI_IFR_FORM *) FormData)->FormTitle, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    if (VariableDefinitionsHead != NULL) {
      VariableName = AllocateZeroPool (12);
      ASSERT (VariableName != NULL);
      CopyMem (VariableName, L"Setup", 12);
      VariableDefinitionsHead->VariableName = VariableName;
      VariableDefinitionsHead->VariableSize = VariableSize;
      CopyMem (&VariableDefinitionsHead->Guid, &Guid, sizeof (EFI_GUID));
    }
    break;

  case EFI_IFR_SUBTITLE_OP:
    TargetTag->NumberOfLines = 1;
    CopyMem (&TargetTag->Text, &((EFI_IFR_SUBTITLE *) FormData)->SubTitle, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_TEXT_OP:
    TargetTag->NumberOfLines = 1;
    CopyMem (&TargetTag->Text, &((EFI_IFR_TEXT *) FormData)->Text, sizeof (UINT16));
    CopyMem (&TargetTag->Help, &((EFI_IFR_TEXT *) FormData)->Help, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;

    //
    // To optimize the encoding size, certain opcodes have optional fields such as those
    // inside the if() statement.  If the encoded length is the complete size, then we
    // know we have valid data encoded that we want to integrate
    //
    if (((EFI_IFR_TEXT *) FormData)->Header.Length == sizeof (EFI_IFR_TEXT)) {
      //
      // Text has no help associated with it, but in case there is a second entry due to
      // dynamic/interactive flags being active, bring this data over.
      //
      CopyMem (&TargetTag->TextTwo, &((EFI_IFR_TEXT *) FormData)->TextTwo, sizeof (UINT16));
      TargetTag->Flags = ((EFI_IFR_TEXT *) FormData)->Flags;
      CopyMem (&TargetTag->Key, &((EFI_IFR_TEXT *) FormData)->Key, sizeof (UINT16));
    }
    break;

  case EFI_IFR_ONE_OF_OPTION_OP:
    CopyMem (&TargetTag->Text, &((EFI_IFR_ONE_OF_OPTION *) FormData)->Option, sizeof (UINT16));
    CopyMem (&TargetTag->Value, &((EFI_IFR_ONE_OF_OPTION *) FormData)->Value, sizeof (UINT16));
    TargetTag->Flags = ((EFI_IFR_ONE_OF_OPTION *) FormData)->Flags;
    CopyMem (&TargetTag->Key, &((EFI_IFR_ONE_OF_OPTION *) FormData)->Key, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_CHECKBOX_OP:
    TargetTag->Flags          = ((EFI_IFR_CHECKBOX *) FormData)->Flags;
    TargetTag->ResetRequired  = (BOOLEAN) (TargetTag->Flags & EFI_IFR_FLAG_RESET_REQUIRED);
    CopyMem (&TargetTag->Key, &((EFI_IFR_CHECKBOX *) FormData)->Key, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_NUMERIC_OP:
    TargetTag->Flags = ((EFI_IFR_NUMERIC *) FormData)->Flags;
    CopyMem (&TargetTag->Key, &((EFI_IFR_NUMERIC *) FormData)->Key, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_STRING_OP:
    //
    // Convert EFI_IFR_STRING.MinSize and EFI_IFR_STRING.MaxSize to actual minimum and maximum bytes
    // and store to EFI_TAG.Minimum and EFI_TAG.Maximum
    //
    TempValue = 0;
    CopyMem (&TempValue, &((EFI_IFR_STRING *) FormData)->MinSize, sizeof (UINT8));
    TempValue = (UINT16) (TempValue * 2);
    CopyMem (&TargetTag->Minimum, &TempValue, sizeof (UINT16));

    TempValue = 0;
    CopyMem (&TempValue, &((EFI_IFR_STRING *) FormData)->MaxSize, sizeof (UINT8));
    TempValue = (UINT16) (TempValue * 2);
    CopyMem (&TargetTag->Maximum, &TempValue, sizeof (UINT16));
    CopyMem (&TargetTag->StorageWidth, &TempValue, sizeof (UINT16));
    TargetTag->Flags          = (UINT8) (((EFI_IFR_STRING *) FormData)->Flags);
    TargetTag->ResetRequired  = (BOOLEAN) (TargetTag->Flags & EFI_IFR_FLAG_RESET_REQUIRED);
    CopyMem (&TargetTag->Key, &((EFI_IFR_STRING *) FormData)->Key, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_PASSWORD_OP:
    TempValue = 0;
    CopyMem (&TempValue, &((EFI_IFR_PASSWORD *) FormData)->MinSize, sizeof (UINT8));
    TempValue = (UINT16) (TempValue * 2);
    CopyMem (&TargetTag->Minimum, &TempValue, sizeof (UINT16));

    TempValue = 0;
    CopyMem (&TempValue, &((EFI_IFR_PASSWORD *) FormData)->MaxSize, sizeof (UINT8));
    TempValue = (UINT16) (TempValue * 2);
    CopyMem (&TargetTag->Maximum, &TempValue, sizeof (UINT16));
    CopyMem (&TargetTag->StorageWidth, &TempValue, sizeof (UINT16));
    TargetTag->Flags          = ((EFI_IFR_PASSWORD *) FormData)->Flags;
    TargetTag->ResetRequired  = (BOOLEAN) (TargetTag->Flags & EFI_IFR_FLAG_RESET_REQUIRED);
    CopyMem (&TargetTag->Key, &((EFI_IFR_PASSWORD *) FormData)->Key, sizeof (UINT16));
    CopyMem (&TargetTag->Encoding, &((EFI_IFR_PASSWORD *) FormData)->Encoding, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_VARSTORE_OP:
    //
    // It should NEVER be NULL
    //
    if (VariableDefinitionsHead == NULL) {
      break;
    }

    VariableDefinitions = VariableDefinitionsHead;

    //
    // Advance VariableDefinitions to the last entry
    //
    for (; VariableDefinitions != NULL; VariableDefinitions = VariableDefinitions->Next) {
      PreviousVariableDefinitions = VariableDefinitions;
      //
      // If there is a variable with this GUID and ID already, we need to bail out
      //
      if (!CompareMem (&VariableDefinitions->Guid, &((EFI_IFR_VARSTORE *) FormData)->Guid, sizeof (EFI_GUID)) &&
          !CompareMem (&VariableDefinitions->VariableId, &((EFI_IFR_VARSTORE *) FormData)->VarId, sizeof (UINT16))
            ) {
        return ;
      }

      if (VariableDefinitions->Next == NULL) {
        break;
      }
    }
    //
    // If the last entry has a variable in it already, allocate a new entry and use it
    //
    if (VariableDefinitions->VariableName != NULL) {
      VariableDefinitions->Next = AllocateZeroPool (sizeof (EFI_VARIABLE_DEFINITION));
      ASSERT (VariableDefinitions->Next != NULL);
      PreviousVariableDefinitions   = VariableDefinitions;
      VariableDefinitions           = VariableDefinitions->Next;
      VariableDefinitions->Previous = PreviousVariableDefinitions;
    }
    //
    // Copy the Variable data to our linked list
    //
    CopyMem (&VariableDefinitions->VariableId, &((EFI_IFR_VARSTORE *) FormData)->VarId, sizeof (UINT16));
    CopyMem (&VariableDefinitions->VariableSize, &((EFI_IFR_VARSTORE *) FormData)->Size, sizeof (UINT16));
    CopyMem (&VariableDefinitions->Guid, &((EFI_IFR_VARSTORE *) FormData)->Guid, sizeof (EFI_GUID));

    //
    // The ASCII String which is immediately past the EFI_IFR_VARSTORE is inferred by the structure definition
    // due to it being variable sized.  There are rules preventing it from being > 40 characters long and should
    // be enforced by the compiler.
    //
    AsciiString                       = (CHAR8 *) (&((EFI_IFR_VARSTORE *) FormData)->Size);
    AsciiString                       = AsciiString + 2;
    VariableDefinitions->VariableName = AllocateZeroPool ((AsciiStrLen (AsciiString) + 1) * 2);
    ASSERT (VariableDefinitions->VariableName != NULL);
    for (Index = 0; AsciiString[Index] != 0; Index++) {
      VariableDefinitions->VariableName[Index] = (CHAR16) AsciiString[Index];
    }

    VariableDefinitions->VariableName[Index] = 0;

    //
    // Propogate the tag information for this op-code
    //
    CopyMem (&TargetTag->VariableNumber, &((EFI_IFR_VARSTORE *) FormData)->VarId, sizeof (UINT16));
    CopyMem (&TargetTag->GuidValue, &((EFI_IFR_VARSTORE *) FormData)->Guid, sizeof (EFI_GUID));
    CopyMem (&TargetTag->StorageWidth, &((EFI_IFR_VARSTORE *) FormData)->Size, sizeof (UINT16));
    CopyMem (&TargetTag->Maximum, &((EFI_IFR_VARSTORE *) FormData)->Size, sizeof (UINT16));
    break;

  case EFI_IFR_VARSTORE_SELECT_OP:
    CopyMem (&TargetTag->VariableNumber, &((EFI_IFR_VARSTORE_SELECT *) FormData)->VarId, sizeof (UINT16));
    CopyMem (&CurrentVariable, &((EFI_IFR_VARSTORE_SELECT *) FormData)->VarId, sizeof (UINT16));
    CurrentVariable2 = CurrentVariable;
    break;

  case EFI_IFR_VARSTORE_SELECT_PAIR_OP:
    CopyMem (&TargetTag->VariableNumber, &((EFI_IFR_VARSTORE_SELECT_PAIR *) FormData)->VarId, sizeof (UINT16));
    CopyMem (
      &TargetTag->VariableNumber2,
      &((EFI_IFR_VARSTORE_SELECT_PAIR *) FormData)->SecondaryVarId,
      sizeof (UINT16)
      );
    CopyMem (&CurrentVariable, &((EFI_IFR_VARSTORE_SELECT_PAIR *) FormData)->VarId, sizeof (UINT16));
    CopyMem (&CurrentVariable2, &((EFI_IFR_VARSTORE_SELECT_PAIR *) FormData)->SecondaryVarId, sizeof (UINT16));
    break;

  case EFI_IFR_REF_OP:
    TargetTag->NumberOfLines = 1;
    CopyMem (&TargetTag->Id, &((EFI_IFR_REF *) FormData)->FormId, sizeof (UINT16));
    CopyMem (&TargetTag->Key, &((EFI_IFR_REF *) FormData)->Key, sizeof (UINT16));
    CopyMem (&TargetTag->Text, &((EFI_IFR_REF *) FormData)->Prompt, sizeof (UINT16));
    CopyMem (&TargetTag->Help, &((EFI_IFR_REF *) FormData)->Help, sizeof (UINT16));
    TargetTag->Flags          = ((EFI_IFR_REF *) FormData)->Flags;
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_EQ_ID_VAL_OP:
    CopyMem (&TargetTag->Value, &((EFI_IFR_EQ_ID_VAL *) FormData)->Value, sizeof (UINT16));
    CopyMem (&TargetTag->Id, &((EFI_IFR_EQ_ID_VAL *) FormData)->QuestionId, sizeof (UINT16));
    TargetTag->StorageWidth   = ((EFI_IFR_EQ_ID_VAL *) FormData)->Width;
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_EQ_VAR_VAL_OP:
    CopyMem (&TargetTag->Value, &((EFI_IFR_EQ_VAR_VAL *) FormData)->Value, sizeof (UINT16));
    CopyMem (&TargetTag->Id, &((EFI_IFR_EQ_VAR_VAL *) FormData)->VariableId, sizeof (UINT16));
    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_EQ_ID_ID_OP:
    CopyMem (&TargetTag->Id, &((EFI_IFR_EQ_ID_ID *) FormData)->QuestionId1, sizeof (UINT16));
    CopyMem (&TargetTag->Id2, &((EFI_IFR_EQ_ID_ID *) FormData)->QuestionId2, sizeof (UINT16));
    TargetTag->StorageWidth   = ((EFI_IFR_EQ_ID_ID *) FormData)->Width;
    TargetTag->VariableNumber = CurrentVariable;
    TargetTag->VariableNumber = CurrentVariable2;
    break;

  case EFI_IFR_EQ_ID_LIST_OP:
    CopyMem (&TargetTag->Id, &((EFI_IFR_EQ_ID_LIST *) FormData)->QuestionId, sizeof (UINT16));
    CopyMem (&TargetTag->Id2, &((EFI_IFR_EQ_ID_LIST *) FormData)->ListLength, sizeof (UINT16));
    TargetTag->StorageWidth = ((EFI_IFR_EQ_ID_LIST *) FormData)->Width;

    TargetTag->IntList      = AllocateZeroPool (TargetTag->Id2 * sizeof (UINT16));
    ASSERT (TargetTag->IntList);

    for (TempValue = 0; TempValue < TargetTag->Id2; TempValue++) {
      CopyMem (
        &TargetTag->IntList[TempValue],
        &((EFI_IFR_EQ_ID_LIST *) FormData)->ValueList[TempValue],
        sizeof (UINT16)
        );
    }

    TargetTag->VariableNumber = CurrentVariable;
    break;

  case EFI_IFR_FORM_SET_OP:
    CopyMem (&VariableSize, &((EFI_IFR_FORM_SET *) FormData)->NvDataSize, sizeof (UINT16));
    CopyMem (&Guid, &((EFI_IFR_FORM_SET *) FormData)->Guid, sizeof (EFI_GUID));
    //
    // If there is a size specified in the formste, we will establish a "default" variable
    //
    if (VariableDefinitionsHead != NULL) {
      VariableName = AllocateZeroPool (12);
      ASSERT (VariableName != NULL);
      CopyMem (VariableName, L"Setup", 12);
      VariableDefinitionsHead->VariableName = VariableName;
      VariableDefinitionsHead->VariableSize = VariableSize;
      CopyMem (&VariableDefinitionsHead->Guid, &Guid, sizeof (EFI_GUID));
    }
    break;

  case EFI_IFR_END_FORM_SET_OP:
    CurrentVariable   = 0;
    CurrentVariable2  = 0;
    break;
  }

  return ;
}
