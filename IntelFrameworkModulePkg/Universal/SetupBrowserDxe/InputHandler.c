/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  InputHandler.C

Abstract:

  Implementation for handling user input from the User Interface

Revision History

--*/

#include "Setup.h"
#include "Ui.h"
#include "Colors.h"

#define EFI_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))

EFI_STATUS
ReadString(
  IN  UI_MENU_OPTION              *MenuOption,
  OUT CHAR16                      *StringPtr
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;
  CHAR16        NullCharacter;
  UINTN         ScreenSize;
  EFI_TAG       *Tag;
  CHAR16        Space[2];
  CHAR16        KeyPad[2];
  BOOLEAN       SelectionComplete;
  CHAR16        *TempString;
  CHAR16        *BufferedString;
  UINTN         Index;
  UINTN         Count;
  UINTN         Start;
  UINTN         Top;
  CHAR16        *PromptForDataString;
  UINTN         DimensionsWidth;
  UINTN         DimensionsHeight;
  BOOLEAN       CursorVisible;

  DimensionsWidth     = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight    = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  PromptForDataString = GetToken (STRING_TOKEN (PROMPT_FOR_DATA), gHiiHandle);

  NullCharacter       = CHAR_NULL;
  ScreenSize          = GetStringWidth (PromptForDataString) / 2;
  Tag                 = MenuOption->ThisTag;
  Space[0]            = L' ';
  Space[1]            = CHAR_NULL;
  SelectionComplete   = FALSE;

  TempString          = AllocateZeroPool (MenuOption->ThisTag->Maximum * 2);
  ASSERT (TempString);

  if (ScreenSize < (Tag->Maximum / (UINTN) 2)) {
    ScreenSize = Tag->Maximum / 2;
  }

  if ((ScreenSize + 2) > DimensionsWidth) {
    ScreenSize = DimensionsWidth - 2;
  }

  BufferedString = AllocateZeroPool (ScreenSize * 2);
  ASSERT (BufferedString);

  Start = (DimensionsWidth - ScreenSize - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - 6) / 2) + gScreenDimensions.TopRow - 1;

  //
  // Display prompt for string
  //
  CreatePopUp (ScreenSize, 4, &NullCharacter, PromptForDataString, Space, &NullCharacter);

  FreePool (PromptForDataString);

  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));

  CursorVisible = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  do {
    Status = WaitForKeyStroke (&Key);

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    switch (Key.UnicodeChar) {
    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
        break;

      case SCAN_RIGHT:
        break;

      case SCAN_ESC:
        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      if (GetStringWidth (StringPtr) >= MenuOption->ThisTag->Minimum) {
        SelectionComplete = TRUE;
        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_SUCCESS;
      } else {
        ScreenSize = GetStringWidth (gMiniString) / 2;
        CreatePopUp (ScreenSize, 4, &NullCharacter, gMiniString, gPressEnter, &NullCharacter);
        //
        // Simply create a popup to tell the user that they had typed in too few characters.
        // To save code space, we can then treat this as an error and return back to the menu.
        //
        do {
          Status = WaitForKeyStroke (&Key);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;
      }

      break;

    case CHAR_BACKSPACE:
      if (StringPtr[0] != CHAR_NULL) {
        for (Index = 0; StringPtr[Index] != CHAR_NULL; Index++) {
          TempString[Index] = StringPtr[Index];
        }
        //
        // Effectively truncate string by 1 character
        //
        TempString[Index - 1] = CHAR_NULL;
        StrCpy (StringPtr, TempString);
      }

    default:
      //
      // If it is the beginning of the string, don't worry about checking maximum limits
      //
      if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        StrnCpy (StringPtr, &Key.UnicodeChar, 1);
        StrnCpy (TempString, &Key.UnicodeChar, 1);
      } else if ((GetStringWidth (StringPtr) < MenuOption->ThisTag->Maximum) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        KeyPad[0] = Key.UnicodeChar;
        KeyPad[1] = CHAR_NULL;
        StrCat (StringPtr, KeyPad);
        StrCat (TempString, KeyPad);
      }
      //
      // If the width of the input string is now larger than the screen, we nee to
      // adjust the index to start printing portions of the string
      //
      SetUnicodeMem (BufferedString, ScreenSize - 1, L' ');

      PrintStringAt (Start + 1, Top + 3, BufferedString);

      if ((GetStringWidth (StringPtr) / 2) > (DimensionsWidth - 2)) {
        Index = (GetStringWidth (StringPtr) / 2) - DimensionsWidth + 2;
      } else {
        Index = 0;
      }

      for (Count = 0; Index + 1 < GetStringWidth (StringPtr) / 2; Index++, Count++) {
        BufferedString[Count] = StringPtr[Index];
      }

      PrintStringAt (Start + 1, Top + 3, BufferedString);
      break;
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + GetStringWidth (StringPtr) / 2, Top + 3);
  } while (!SelectionComplete);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
  return Status;
}

EFI_STATUS
ReadPassword (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     PromptForPassword,
  IN  EFI_TAG                     *Tag,
  IN  EFI_IFR_DATA_ARRAY          *PageData,
  IN  BOOLEAN                     SecondEntry,
  IN  EFI_FILE_FORM_TAGS          *FileFormTags,
  OUT CHAR16                      *StringPtr
  )
{
  EFI_STATUS                  Status;
  UINTN                       ScreenSize;
  CHAR16                      NullCharacter;
  CHAR16                      Space[2];
  EFI_INPUT_KEY               Key;
  CHAR16                      KeyPad[2];
  UINTN                       Index;
  UINTN                       Start;
  UINTN                       Top;
  CHAR16                      *TempString;
  CHAR16                      *TempString2;
  BOOLEAN                     Confirmation;
  BOOLEAN                     ConfirmationComplete;
  EFI_HII_CALLBACK_PACKET     *Packet;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_VARIABLE_DEFINITION     *VariableDefinition;
  UINTN                       DimensionsWidth;
  UINTN                       DimensionsHeight;
  EFI_IFR_DATA_ENTRY          *DataEntry;
  UINTN                       WidthOfString;

  DimensionsWidth       = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight      = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  VariableDefinition    = NULL;
  NullCharacter         = CHAR_NULL;
  Space[0]              = L' ';
  Space[1]              = CHAR_NULL;
  Confirmation          = FALSE;
  ConfirmationComplete  = FALSE;
  Status                = EFI_SUCCESS;
  FormCallback          = NULL;
  Packet                = NULL;

  //
  // Remember that dynamic pages in an environment where all pages are not
  // dynamic require us to call back to the user to give them an opportunity
  // to register fresh information in the HII database so that we can extract it.
  //
  Status = gBS->HandleProtocol (
                  (VOID *) (UINTN) MenuOption->Tags[0].CallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  (VOID **) &FormCallback
                  );

  TempString  = AllocateZeroPool (MenuOption->ThisTag->Maximum * 2);
  TempString2 = AllocateZeroPool (MenuOption->ThisTag->Maximum * 2);

  ASSERT (TempString);
  ASSERT (TempString2);

  if (Tag->Flags & EFI_IFR_FLAG_INTERACTIVE) {
    //
    // Password requires a callback to determine if a password exists
    //
    DataEntry = (EFI_IFR_DATA_ENTRY *) (PageData + 1);
    DataEntry->OpCode  = EFI_IFR_PASSWORD_OP;
    DataEntry->Length  = 3;

    ExtractRequestedNvMap (FileFormTags, Tag->VariableNumber, &VariableDefinition);

    //
    // The user is about to be prompted with a password field, Data = 0 (Return Status determines the type of prompt)
    //
    DataEntry->Data  = (VOID *) (UINTN) (UINT8) (0 + SecondEntry * 2);
    PageData->NvRamMap    = VariableDefinition->NvRamMap;

    if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
      Status = FormCallback->Callback (
                              FormCallback,
                              Tag->Key,
                              PageData,
                              &Packet
                              );
    }
    //
    // If error on return, continue with the reading of a typed in password to verify user knows password
    // If no error, there is no password set, so prompt for new password
    // if the previous callback was to verify the user knew password, and user typed it correctly - should return no error
    //
    if (!EFI_ERROR (Status)) {
      PromptForPassword = FALSE;

      //
      // Simulate this as the second entry into this routine for an interactive behavior
      //
      SecondEntry = TRUE;
    } else if (Status == EFI_NOT_READY) {
Error:
      if (Packet != NULL) {
        //
        // Upon error, we will likely receive a string to print out
        // Display error popup
        //
        WidthOfString = GetStringWidth (Packet->String);
        ScreenSize = EFI_MAX(WidthOfString, GetStringWidth (gPressEnter)) / 2;
        CreatePopUp (ScreenSize, 4, &NullCharacter, Packet->String, gPressEnter, &NullCharacter);
        FreePool (Packet);

        do {
          Status = WaitForKeyStroke (&Key);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      }

      Status = EFI_NOT_READY;
      goto Done;
    }
  }

  do {
    //
    // Display PopUp Screen
    //
    ScreenSize = GetStringWidth (gPromptForNewPassword) / 2;
    if (GetStringWidth (gConfirmPassword) / 2 > ScreenSize) {
      ScreenSize = GetStringWidth (gConfirmPassword) / 2;
    }

    Start = (DimensionsWidth - ScreenSize - 4) / 2 + gScreenDimensions.LeftColumn + 2;
    Top   = ((DimensionsHeight - 6) / 2) + gScreenDimensions.TopRow - 1;

    if (!Confirmation) {
      if (PromptForPassword) {
        CreatePopUp (ScreenSize, 4, &NullCharacter, gPromptForPassword, Space, &NullCharacter);
      } else {
        CreatePopUp (ScreenSize, 4, &NullCharacter, gPromptForNewPassword, Space, &NullCharacter);
      }
    } else {
      CreatePopUp (ScreenSize, 4, &NullCharacter, gConfirmPassword, Space, &NullCharacter);
      StringPtr[0] = CHAR_NULL;
    }

    do {
      Status = WaitForKeyStroke (&Key);

      switch (Key.UnicodeChar) {
      case CHAR_NULL:
        if (Key.ScanCode == SCAN_ESC) {
          return EFI_NOT_READY;
        }

        ConfirmationComplete = FALSE;
        break;

      case CHAR_CARRIAGE_RETURN:
        if (Tag->Flags & EFI_IFR_FLAG_INTERACTIVE) {
          //
          // User just typed a string in
          //
          DataEntry = (EFI_IFR_DATA_ENTRY *) (PageData + 1);
          DataEntry->OpCode = EFI_IFR_PASSWORD_OP;

          //
          // If the user just typed in a password, Data = 1
          // If the user just typed in a password to confirm the previous password, Data = 2
          //
          if (!Confirmation) {
            DataEntry->Length  = 3;
            DataEntry->Data    = (VOID *) (UINTN) (UINT8) (1 + SecondEntry * 2);

            if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
              Status = FormCallback->Callback (
                                      FormCallback,
                                      Tag->Key,
                                      PageData,
                                      &Packet
                                      );
            }

            DataEntry->Length  = sizeof (EFI_IFR_DATA_ENTRY);
            DataEntry->Data    = (VOID *) TempString;
          } else {
            DataEntry->Length  = 3;
            DataEntry->Data    = (VOID *) (UINTN) (UINT8) (2 + SecondEntry * 2);

            if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
              Status = FormCallback->Callback (
                                      FormCallback,
                                      Tag->Key,
                                      PageData,
                                      &Packet
                                      );
            }

            DataEntry->Length  = sizeof (EFI_IFR_DATA_ENTRY);
            DataEntry->Data    = (VOID *) TempString2;
          }

          if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
            Status = FormCallback->Callback (
                                    FormCallback,
                                    Tag->Key,
                                    PageData,
                                    &Packet
                                    );
          }
          //
          // If this was the confirmation round of callbacks
          // and an error comes back, display an error
          //
          if (Confirmation) {
            if (EFI_ERROR (Status)) {
              if (Packet->String == NULL) {
                WidthOfString = GetStringWidth (gConfirmError);
                ScreenSize = EFI_MAX (WidthOfString, GetStringWidth (gPressEnter)) / 2;
                CreatePopUp (ScreenSize, 4, &NullCharacter, gConfirmError, gPressEnter, &NullCharacter);
              } else {
                WidthOfString = GetStringWidth (Packet->String);
                ScreenSize = EFI_MAX (WidthOfString, GetStringWidth (gPressEnter)) / 2;
                CreatePopUp (ScreenSize, 4, &NullCharacter, Packet->String, gPressEnter, &NullCharacter);
                FreePool (Packet);
              }

              StringPtr[0] = CHAR_NULL;
              do {
                Status = WaitForKeyStroke (&Key);

                if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
                  Status = EFI_NOT_READY;
                  goto Done;
                }
              } while (1);
            } else {
                Status = EFI_NOT_READY;
                goto Done;
            }
          } else {
            //
            // User typed a string in and it wasn't valid somehow from the callback
            // For instance, callback may have said that some invalid characters were contained in the string
            //
            if (Status == EFI_NOT_READY) {
              goto Error;
            }

            if (PromptForPassword && EFI_ERROR (Status)) {
              Status = EFI_DEVICE_ERROR;
              goto Done;
            }
          }
        }

        if (Confirmation) {
          //
          // Compare tempstring and tempstring2, if the same, return with StringPtr success
          // Otherwise, kick and error box, and return an error
          //
          if (StrCmp (TempString, TempString2) == 0) {
            Status = EFI_SUCCESS;
            goto Done;
          } else {
            WidthOfString = GetStringWidth (gConfirmError);
            ScreenSize = EFI_MAX (WidthOfString, GetStringWidth (gPressEnter)) / 2;
            CreatePopUp (ScreenSize, 4, &NullCharacter, gConfirmError, gPressEnter, &NullCharacter);
            StringPtr[0] = CHAR_NULL;
            do {
              Status = WaitForKeyStroke (&Key);
              if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
                Status = EFI_DEVICE_ERROR;
                goto Done;
              }
            } while (1);
          }
        }

        if (PromptForPassword) {
          //
          // I was asked for a password, return it back in StringPtr
          //
          Status = EFI_SUCCESS;
          goto Done;
        } else {
          //
          // If the two passwords were not the same kick an error popup
          //
          Confirmation          = TRUE;
          ConfirmationComplete  = TRUE;
          break;
        }

      case CHAR_BACKSPACE:
        if (StringPtr[0] != CHAR_NULL) {
          if (!Confirmation) {
            for (Index = 0; StringPtr[Index] != CHAR_NULL; Index++) {
              TempString[Index] = StringPtr[Index];
            }
            //
            // Effectively truncate string by 1 character
            //
            TempString[Index - 1] = CHAR_NULL;
            StrCpy (StringPtr, TempString);
          } else {
            for (Index = 0; StringPtr[Index] != CHAR_NULL; Index++) {
              TempString2[Index] = StringPtr[Index];
            }
            //
            // Effectively truncate string by 1 character
            //
            TempString2[Index - 1] = CHAR_NULL;
            StrCpy (StringPtr, TempString2);
          }

          ConfirmationComplete = FALSE;
        } else {
          ConfirmationComplete = FALSE;
        }

      //
      // Must be a character we are interested in!
      //
      default:
        if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
          if (!Confirmation) {
            StrnCpy (StringPtr, &Key.UnicodeChar, 1);
            StrnCpy (TempString, &Key.UnicodeChar, 1);
          } else {
            StrnCpy (StringPtr, &Key.UnicodeChar, 1);
            StrnCpy (TempString2, &Key.UnicodeChar, 1);
            ConfirmationComplete = FALSE;
          }
        } else if ((GetStringWidth (StringPtr) / 2 <= (UINTN) (MenuOption->ThisTag->Maximum - 1) / 2) &&
                 (Key.UnicodeChar != CHAR_BACKSPACE)
                ) {
          KeyPad[0] = Key.UnicodeChar;
          KeyPad[1] = CHAR_NULL;
          if (!Confirmation) {
            StrCat (StringPtr, KeyPad);
            StrCat (TempString, KeyPad);
          } else {
            StrCat (StringPtr, KeyPad);
            StrCat (TempString2, KeyPad);
          }
        }

        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
        for (Index = 1; Index < ScreenSize; Index++) {
          PrintCharAt (Start + Index, Top + 3, L' ');
        }

        gST->ConOut->SetCursorPosition (
                      gST->ConOut,
                      (DimensionsWidth - GetStringWidth (StringPtr) / 2) / 2 + gScreenDimensions.LeftColumn,
                      Top + 3
                      );
        for (Index = 0; Index + 1 < GetStringWidth (StringPtr) / 2; Index++) {
          PrintChar (L'*');
        }

        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        break;
      }
      //
      // end switch
      //
    } while (!ConfirmationComplete);

  } while (1);

Done:
  FreePool (TempString);
  FreePool (TempString2);
  return Status;
}

VOID
EncodePassword (
  IN  CHAR16                      *Password,
  IN  UINT8                       MaxSize
  )
{
  UINTN   Index;
  UINTN   Loop;
  CHAR16  *Buffer;
  CHAR16  *Key;

  Key     = (CHAR16 *) L"MAR10648567";
  Buffer  = AllocateZeroPool (MaxSize);

  ASSERT (Buffer);

  for (Index = 0; Key[Index] != 0; Index++) {
    for (Loop = 0; Loop < (UINT8) (MaxSize / 2); Loop++) {
      Buffer[Loop] = (CHAR16) (Password[Loop] ^ Key[Index]);
    }
  }

  CopyMem (Password, Buffer, MaxSize);

  FreePool (Buffer);
  return ;
}

EFI_STATUS
GetNumericInput (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  BOOLEAN                     ManualInput,
  IN  EFI_TAG                     *Tag,
  IN  UINTN                       NumericType,
  OUT UINT16                      *Value
  )
/*++

Routine Description:

  This routine reads a numeric value from the user input.

Arguments:

  MenuOption       -  Pointer to the current input menu.

  FileFormTagsHead -  Pointer to the root of formset.

  ManualInput      -  If the input is manual or not.

  Tag              -  Pointer to all the attributes and values associated with a tag.

  Value            -  Pointer to the numeric value that is going to be read.

Returns:

  EFI_SUCCESS       - If numerical input is read successfully
  EFI_DEVICE_ERROR  - If operation fails

--*/
{
  EFI_INPUT_KEY           Key;
  BOOLEAN                 SelectionComplete;
  UINTN                   Column;
  UINTN                   Row;
  CHAR16                  FormattedNumber[6];
  UINTN                   PreviousNumber[6];
  INTN                    Number;
  UINTN                   Count;
  UINT16                  BackupValue;
  STRING_REF              PopUp;
  CHAR16                  NullCharacter;
  CHAR16                  *StringPtr;
  EFI_FILE_FORM_TAGS      *FileFormTags;
  EFI_VARIABLE_DEFINITION *VariableDefinition;
  UINTN                   Loop;

  NullCharacter     = CHAR_NULL;
  StringPtr         = NULL;
  Column            = MenuOption->OptCol;
  Row               = MenuOption->Row;
  Number            = 0;
  PreviousNumber[0] = 0;
  Count             = 0;
  SelectionComplete = FALSE;
  BackupValue       = Tag->Value;
  FileFormTags      = FileFormTagsHead;

  if (ManualInput) {
    PrintAt (Column, Row, (CHAR16 *) L"[     ]");
    Column++;
    if (Tag->Operand != EFI_IFR_TIME_OP) {
      *Value = BackupValue;
    }
  }
  //
  // First time we enter this handler, we need to check to see if
  // we were passed an increment or decrement directive
  //
  do {
    Key.UnicodeChar = CHAR_NULL;
    if (gDirection != 0) {
      Key.ScanCode  = gDirection;
      gDirection    = 0;
      goto TheKey2;
    }

    WaitForKeyStroke (&Key);

TheKey2:
    switch (Key.UnicodeChar) {
    case '+':
    case '-':
      if ((Tag->Operand == EFI_IFR_DATE_OP) || (Tag->Operand == EFI_IFR_TIME_OP)) {
        Key.UnicodeChar = CHAR_NULL;
        if (Key.UnicodeChar == '+') {
          Key.ScanCode = SCAN_RIGHT;
        } else {
          Key.ScanCode = SCAN_LEFT;
        }

        goto TheKey2;
      }
      break;

    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
      case SCAN_RIGHT:
        if ((Tag->Operand == EFI_IFR_DATE_OP) || (Tag->Operand == EFI_IFR_TIME_OP)) {
          //
          // By setting this value, we will return back to the caller.
          // We need to do this since an auto-refresh will destroy the adjustment
          // based on what the real-time-clock is showing.  So we always commit
          // upon changing the value.
          //
          gDirection = SCAN_DOWN;
        }

        if (!ManualInput) {
          Tag->Value = *Value;
          if (Key.ScanCode == SCAN_LEFT) {
            Number = *Value - Tag->Step;
            if (Number < Tag->Minimum) {
              Number = Tag->Minimum;
            }
          } else if (Key.ScanCode == SCAN_RIGHT) {
            Number = *Value + Tag->Step;
            if (Number > Tag->Maximum) {
              Number = Tag->Maximum;
            }
          }

          Tag->Value  = (UINT16) Number;
          *Value      = (UINT16) Number;
          UnicodeValueToString (
            FormattedNumber,
            FALSE,
            (UINTN) Number,
            (sizeof (FormattedNumber) / sizeof (FormattedNumber[0]))
            );
          Number = (UINT16) GetStringWidth (FormattedNumber);

          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
          if ((Tag->Operand == EFI_IFR_DATE_OP) || (Tag->Operand == EFI_IFR_TIME_OP)) {
            for (Loop = 0; Loop < (UINTN) ((Number >= 8) ? 4 : 2); Loop++) {
              PrintAt (MenuOption->OptCol + Loop, MenuOption->Row, (CHAR16 *) L" ");
            }
          } else {
            for (Loop = 0; Loop < gOptionBlockWidth; Loop++) {
              PrintAt (MenuOption->OptCol + Loop, MenuOption->Row, (CHAR16 *) L" ");
            }
          }

          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_HIGHLIGHT | FIELD_BACKGROUND_HIGHLIGHT);

          if ((MenuOption->Col + gPromptBlockWidth + 1) == MenuOption->OptCol) {
            PrintCharAt (MenuOption->OptCol, Row, LEFT_NUMERIC_DELIMITER);
            Column = MenuOption->OptCol + 1;
          }
          //
          // If Number looks like "3", convert it to "03/"
          //
          if (Number == 4 && (NumericType == DATE_NUMERIC)) {
            FormattedNumber[3]  = FormattedNumber[1];
            FormattedNumber[2]  = DATE_SEPARATOR;
            FormattedNumber[1]  = FormattedNumber[0];
            FormattedNumber[0]  = L'0';
            Number              = 8;
          }
          //
          // If Number looks like "13", convert it to "13/"
          //
          if (Number == 6 && (NumericType == DATE_NUMERIC)) {
            FormattedNumber[3]  = FormattedNumber[2];
            FormattedNumber[2]  = DATE_SEPARATOR;
            Number              = 8;
          }

          if (Number == 4 &&
              (NumericType == TIME_NUMERIC) &&
              (MenuOption->Col + gPromptBlockWidth + 8) != MenuOption->OptCol
              ) {
            FormattedNumber[3]  = FormattedNumber[1];
            FormattedNumber[2]  = TIME_SEPARATOR;
            FormattedNumber[1]  = FormattedNumber[0];
            FormattedNumber[0]  = L'0';
            Number              = 8;
          }

          if (Number == 4 &&
              (NumericType == TIME_NUMERIC) &&
              (MenuOption->Col + gPromptBlockWidth + 8) == MenuOption->OptCol
              ) {
            FormattedNumber[3]  = FormattedNumber[1];
            FormattedNumber[2]  = RIGHT_NUMERIC_DELIMITER;
            FormattedNumber[1]  = FormattedNumber[0];
            FormattedNumber[0]  = L'0';
            Number              = 8;
          }

          PrintStringAt (Column, Row, FormattedNumber);
          if (Number == 10 && (NumericType == DATE_NUMERIC)) {
            PrintChar (RIGHT_NUMERIC_DELIMITER);
          }

          if (NumericType == REGULAR_NUMERIC) {
            PrintChar (RIGHT_NUMERIC_DELIMITER);
          }
        }
        break;

      case SCAN_UP:
      case SCAN_DOWN:
        goto EnterCarriageReturn;

      case SCAN_ESC:
        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

EnterCarriageReturn:

    case CHAR_CARRIAGE_RETURN:
      //
      // Check to see if the Value is something reasonable against consistency limitations.
      // If not, let's kick the error specified.
      //
      //
      // This gives us visibility to the FileFormTags->NvRamMap to check things
      // ActiveIfr is a global maintained by the menuing code to ensure that we
      // are pointing to the correct formset's file data.
      //
      for (Count = 0; Count < gActiveIfr; Count++) {
        FileFormTags = FileFormTags->NextFile;
      }

      ExtractRequestedNvMap (FileFormTags, Tag->VariableNumber, &VariableDefinition);

      CopyMem (&VariableDefinition->NvRamMap[Tag->StorageStart], &Tag->Value, Tag->StorageWidth);

      //
      // Data associated with a NULL device (in the fake NV storage)
      //
      if (Tag->StorageWidth == (UINT16) 0) {
        CopyMem (&VariableDefinition->FakeNvRamMap[Tag->StorageStart], &Tag->Value, 2);
      }
      //
      // If a late check is required save off the information.  This is used when consistency checks
      // are required, but certain values might be bound by an impossible consistency check such as
      // if two questions are bound by consistency checks and each only has two possible choices, there
      // would be no way for a user to switch the values.  Thus we require late checking.
      //
      if (Tag->Flags & EFI_IFR_FLAG_LATE_CHECK) {
        CopyMem (&Tag->OldValue, &BackupValue, Tag->StorageWidth);
      } else {
        //
        // In theory, passing the value and the Id are sufficient to determine what needs
        // to be done.  The Id is the key to look for the entry needed in the Inconsistency
        // database.  That will yields operand and ID data - and since the ID's correspond
        // to the NV storage, we can determine the values for other IDs there.
        //
        if (ValueIsNotValid (TRUE, 0, Tag, FileFormTags, &PopUp)) {
          if (PopUp == 0x0000) {
            SelectionComplete = TRUE;
            break;
          }

          StringPtr = GetToken (PopUp, MenuOption->Handle);

          CreatePopUp (GetStringWidth (StringPtr) / 2, 3, &NullCharacter, StringPtr, &NullCharacter);

          do {
            WaitForKeyStroke (&Key);

            switch (Key.UnicodeChar) {

            case CHAR_CARRIAGE_RETURN:
              SelectionComplete = TRUE;
              FreePool (StringPtr);
              break;

            default:
              break;
            }
          } while (!SelectionComplete);

          Tag->Value  = BackupValue;
          *Value      = BackupValue;

          CopyMem (&VariableDefinition->NvRamMap[Tag->StorageStart], &Tag->Value, Tag->StorageWidth);

          //
          // Data associated with a NULL device (in the fake NV storage)
          //
          if (Tag->StorageWidth == (UINT16) 0) {
            CopyMem (&VariableDefinition->FakeNvRamMap[Tag->StorageStart], &Tag->Value, 2);
          }

          return EFI_DEVICE_ERROR;
        }
      }

      return EFI_SUCCESS;
      break;

    case CHAR_BACKSPACE:
      if (ManualInput) {
        if (Count == 0) {
          break;
        }
        //
        // Remove a character
        //
        Number  = PreviousNumber[Count - 1];
        *Value  = (UINT16) Number;
        UpdateStatusBar (INPUT_ERROR, Tag->Flags, FALSE);
        Count--;
        Column--;
        PrintAt (Column, Row, (CHAR16 *) L" ");
      }
      break;

    default:
      if (ManualInput) {
        if (Key.UnicodeChar > L'9' || Key.UnicodeChar < L'0') {
          UpdateStatusBar (INPUT_ERROR, Tag->Flags, TRUE);
          break;
        }
        //
        // If Count 0-4 is complete, there is no way more is valid
        //
        if (Count > 4) {
          break;
        }
        //
        // Someone typed something valid!
        //
        if (Count != 0) {
          Number = Number * 10 + (Key.UnicodeChar - L'0');
        } else {
          Number = Key.UnicodeChar - L'0';
        }

        if (Number > Tag->Maximum) {
          UpdateStatusBar (INPUT_ERROR, Tag->Flags, TRUE);
          Number = PreviousNumber[Count];
          break;
        } else {
          UpdateStatusBar (INPUT_ERROR, Tag->Flags, FALSE);
        }

        Count++;

        PreviousNumber[Count] = Number;
        *Value                = (UINT16) Number;
        Tag->Value            = (UINT16) Number;

        PrintCharAt (Column, Row, Key.UnicodeChar);
        Column++;
      }
      break;
    }
  } while (!SelectionComplete);
  return EFI_SUCCESS;
}
//
// Notice that this is at least needed for the ordered list manipulation.
// Left/Right doesn't make sense for this op-code
//
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  EFI_TAG                     *Tag,
  IN  UINTN                       ValueCount,
  OUT UINT16                      *Value,
  OUT UINT16                      *KeyValue
  )
{
  EFI_INPUT_KEY Key;
  UINTN         Index;
  UINTN         TempIndex;
  CHAR16        *StringPtr;
  CHAR16        *TempStringPtr;
  UINT16        Token;
  UINTN         Index2;
  UINTN         TopOptionIndex;
  UINTN         HighlightPosition;
  UINTN         Start;
  UINTN         End;
  UINTN         Top;
  UINTN         Bottom;
  UINT16        TempValue;
  UINTN         Count;
  UINTN         PopUpMenuLines;
  UINTN         MenuLinesInView;
  UINTN         PopUpWidth;
  CHAR16        Character;
  BOOLEAN       FirstOptionFoundFlag;
  INT32         SavedAttribute;
  EFI_TAG       TagBackup;
  UINT8         *ValueArray;
  UINT8         *ValueArrayBackup;
  UINT8         ValueBackup;
  BOOLEAN       Initialized;
  BOOLEAN       KeyInitialized;
  BOOLEAN       ShowDownArrow;
  BOOLEAN       ShowUpArrow;
  UINTN         DimensionsWidth;

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;

  TempValue         = 0;
  TempIndex         = 0;
  ValueArray        = (UINT8 *) Value;
  ValueArrayBackup  = NULL;
  Initialized       = FALSE;
  KeyInitialized    = FALSE;
  ShowDownArrow     = FALSE;
  ShowUpArrow       = FALSE;

  if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
    ValueArrayBackup = AllocateZeroPool (Tag->StorageWidth);
    ASSERT (ValueArrayBackup != NULL);
    CopyMem (ValueArrayBackup, ValueArray, ValueCount);
    TempValue = *(UINT8 *) (ValueArray);
    if (ValueArray[0] != 0x00) {
      Initialized = TRUE;
    }

    for (Index = 0; ValueArray[Index] != 0x00; Index++)
      ;
    ValueCount = Index;
  } else {
    TempValue = *Value;
  }

  Count                 = 0;
  PopUpWidth            = 0;

  FirstOptionFoundFlag  = FALSE;

  StringPtr             = AllocateZeroPool ((gOptionBlockWidth + 1) * 2);
  ASSERT (StringPtr);

  //
  // Initialization for "One of" pop-up menu
  //
  //
  // Get the number of one of options present and its size
  //
  for (Index = MenuOption->TagIndex; MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP; Index++) {
    if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP &&
        !MenuOption->Tags[Index].Suppress) {
      if (!FirstOptionFoundFlag) {
        FirstOptionFoundFlag  = TRUE;
      }

      Count++;
      Token = MenuOption->Tags[Index].Text;

      //
      // If this is an ordered list that is initialized
      //
      if (Initialized) {
        for (ValueBackup = (UINT8) MenuOption->TagIndex;
             MenuOption->Tags[ValueBackup].Operand != EFI_IFR_END_OP;
             ValueBackup++
            ) {
          if (MenuOption->Tags[ValueBackup].Value == ((UINT8 *) ValueArrayBackup)[Index - MenuOption->TagIndex - 1]) {
            StringPtr = GetToken (MenuOption->Tags[ValueBackup].Text, MenuOption->Handle);
            break;
          }
        }
      } else {
        StringPtr = GetToken (Token, MenuOption->Handle);
      }

      if (StrLen (StringPtr) > PopUpWidth) {
        PopUpWidth = StrLen (StringPtr);
      }

      FreePool (StringPtr);
    }
  }
  //
  // Perform popup menu initialization.
  //
  PopUpMenuLines  = Count;
  PopUpWidth      = PopUpWidth + POPUP_PAD_SPACE_COUNT;

  SavedAttribute  = gST->ConOut->Mode->Attribute;
  gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);

  if ((PopUpWidth + POPUP_FRAME_WIDTH) > DimensionsWidth) {
    PopUpWidth = DimensionsWidth - POPUP_FRAME_WIDTH;
  }

  Start           = (DimensionsWidth - PopUpWidth - POPUP_FRAME_WIDTH) / 2 + gScreenDimensions.LeftColumn;
  End             = Start + PopUpWidth + POPUP_FRAME_WIDTH;
  Top             = gScreenDimensions.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT;
  Bottom          = gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT;

  MenuLinesInView = Bottom - Top - 1;
  if (MenuLinesInView >= PopUpMenuLines) {
    Top     = Top + (MenuLinesInView - PopUpMenuLines) / 2;
    Bottom  = Top + PopUpMenuLines + 1;
  } else {
    TempValue     = MenuOption->Tags[MenuOption->TagIndex + 1].Value;
    ShowDownArrow = TRUE;
  }

  TopOptionIndex    = 1;
  HighlightPosition = 0;
  do {
    if (Initialized) {
      for (Index = MenuOption->TagIndex, Index2 = 0; Index2 < ValueCount; Index++, Index2++) {
        //
        // Set the value for the item we are looking for
        //
        Count = ValueArrayBackup[Index2];

        //
        // If we hit the end of the Array, we are complete
        //
        if (Count == 0) {
          break;
        }

        if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
          for (ValueBackup = (UINT8) MenuOption->TagIndex;
               MenuOption->Tags[ValueBackup].Operand != EFI_IFR_END_ONE_OF_OP;
               ValueBackup++
              ) {
            //
            // We just found what we are looking for
            //
            if (MenuOption->Tags[ValueBackup].Value == Count) {
              //
              // As long as the two indexes aren't the same, we have
              // two different op-codes we need to swap internally
              //
              if (Index != ValueBackup) {
                //
                // Backup destination tag, then copy source to destination, then copy backup to source location
                //
                CopyMem (&TagBackup, &MenuOption->Tags[Index], sizeof (EFI_TAG));
                CopyMem (&MenuOption->Tags[Index], &MenuOption->Tags[ValueBackup], sizeof (EFI_TAG));
                CopyMem (&MenuOption->Tags[ValueBackup], &TagBackup, sizeof (EFI_TAG));
              } else {
                //
                // If the indexes are the same, then the op-code is where he belongs
                //
              }
            }
          }
        } else {
          //
          // Since this wasn't an option op-code (likely the ordered list op-code) decerement Index2
          //
          Index2--;
        }
      }
    }
    //
    // Clear that portion of the screen
    //
    ClearLines (Start, End, Top, Bottom, POPUP_TEXT | POPUP_BACKGROUND);

    //
    // Draw "One of" pop-up menu
    //
    Character = (CHAR16) BOXDRAW_DOWN_RIGHT;
    PrintCharAt (Start, Top, Character);
    for (Index = Start; Index + 2 < End; Index++) {
      if ((ShowUpArrow) && ((Index + 1) == (Start + End) / 2)) {
        Character = (CHAR16) GEOMETRICSHAPE_UP_TRIANGLE;
      } else {
        Character = (CHAR16) BOXDRAW_HORIZONTAL;
      }

      PrintChar (Character);
    }

    Character = (CHAR16) BOXDRAW_DOWN_LEFT;
    PrintChar (Character);
    Character = (CHAR16) BOXDRAW_VERTICAL;
    for (Index = Top + 1; Index < Bottom; Index++) {
      PrintCharAt (Start, Index, Character);
      PrintCharAt (End - 1, Index, Character);
    }
    //
    // Display the One of options
    //
    Index2 = Top + 1;
    for (Index = MenuOption->TagIndex + TopOptionIndex;
         (MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP) && (Index2 < Bottom);
         Index++
        ) {
      if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
        Token = MenuOption->Tags[Index].Text;
        if (Initialized) {
          for (ValueBackup = (UINT8) MenuOption->TagIndex;
               MenuOption->Tags[ValueBackup].Operand != EFI_IFR_END_ONE_OF_OP;
               ValueBackup++
              ) {
            if (MenuOption->Tags[ValueBackup].Value == ((UINT8 *) ValueArrayBackup)[Index - MenuOption->TagIndex - 1]) {
              StringPtr = GetToken (MenuOption->Tags[ValueBackup].Text, MenuOption->Handle);
              break;
            }
          }
        } else {
          ValueBackup = (UINT8) Index;
          StringPtr   = GetToken (Token, MenuOption->Handle);
        }
        //
        // If the string occupies multiple lines, truncate it to fit in one line,
        // and append a "..." for indication.
        //
        if (StrLen (StringPtr) > (PopUpWidth - 1)) {
          TempStringPtr = AllocateZeroPool (sizeof (CHAR16) * (PopUpWidth - 1));
          ASSERT (TempStringPtr != NULL);
          CopyMem (TempStringPtr, StringPtr, (sizeof (CHAR16) * (PopUpWidth - 5)));
          FreePool (StringPtr);
          StringPtr = TempStringPtr;
          StrCat (StringPtr, (CHAR16 *) L"...");
        }
        //
        // Code to display the text should go here. Follwed by the [*]
        //
        if (MenuOption->Tags[ValueBackup].Suppress == TRUE) {
          //
          // Don't show the one, so decrease the Index2 for balance
          //
          Index2--;
        } else if (MenuOption->Tags[ValueBackup].GrayOut == TRUE) {
          //
          // Gray Out the one
          //
          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_GRAYED | POPUP_BACKGROUND);
          PrintStringAt (Start + 2, Index2, StringPtr);
          gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
        } else if (MenuOption->Tags[ValueBackup].Value == TempValue) {
          //
          // Highlight the selected one
          //
          gST->ConOut->SetAttribute (gST->ConOut, PICKLIST_HIGHLIGHT_TEXT | PICKLIST_HIGHLIGHT_BACKGROUND);
          PrintStringAt (Start + 2, Index2, StringPtr);
          gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
          HighlightPosition = Index2;
        } else {
          gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
          PrintStringAt (Start + 2, Index2, StringPtr);
        }

        FreePool (StringPtr);
        Index2 = Index2 + 1;
      }
    }

    Character = (CHAR16) BOXDRAW_UP_RIGHT;
    PrintCharAt (Start, Bottom, Character);
    for (Index = Start; Index + 2 < End; Index++) {
      if ((ShowDownArrow) && ((Index + 1) == (Start + End) / 2)) {
        Character = (CHAR16) GEOMETRICSHAPE_DOWN_TRIANGLE;
      } else {
        Character = (CHAR16) BOXDRAW_HORIZONTAL;
      }

      PrintChar (Character);
    }

    Character = (CHAR16) BOXDRAW_UP_LEFT;
    PrintChar (Character);
    //
    // Get User selection and change TempValue if necessary
    //
    //
    // Stop: One of pop-up menu
    //
    Key.UnicodeChar = CHAR_NULL;
    if ((gDirection == SCAN_UP) || (gDirection == SCAN_DOWN)) {
      Key.ScanCode  = gDirection;
      gDirection    = 0;
      goto TheKey;
    }

    if (!KeyInitialized) {
      if (MenuOption->ThisTag->Operand == EFI_IFR_ONE_OF_OP) {
        *KeyValue = MenuOption->Tags[MenuOption->TagIndex + 1].Key;
      } else {
        *KeyValue = MenuOption->ThisTag->Key;
      }

      KeyInitialized = TRUE;
    }

    WaitForKeyStroke (&Key);

TheKey:
    switch (Key.UnicodeChar) {
    case '+':
    case '-':
      //
      // If an ordered list op-code, we will allow for a popup of +/- keys
      // to create an ordered list of items
      //
      if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
        if (Key.UnicodeChar == '+') {
          if ((TopOptionIndex > 1) && (HighlightPosition == (Top + 1))) {
            //
            // Highlight reaches the top of the popup window, scroll one menu item.
            //
            TopOptionIndex--;
            ShowDownArrow = TRUE;
          }

          if (TopOptionIndex == 1) {
            ShowUpArrow = FALSE;
          }
        } else {
          if (((TopOptionIndex + MenuLinesInView) <= PopUpMenuLines) && (HighlightPosition == (Bottom - 1))) {
            //
            // Highlight reaches the bottom of the popup window, scroll one menu item.
            //
            TopOptionIndex++;
            ShowUpArrow = TRUE;
          }

          if ((TopOptionIndex + MenuLinesInView) == (PopUpMenuLines + 1)) {
            ShowDownArrow = FALSE;
          }
        }

        for (Index = MenuOption->TagIndex + TopOptionIndex;
             MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP;
             Index++
            ) {
          if (MenuOption->Tags[Index].Operand == EFI_IFR_ORDERED_LIST_OP) {
            continue;
          }

          if (Key.UnicodeChar == '+') {
            TempIndex = Index - 1;
          } else {
            TempIndex = Index + 1;
          }
          //
          // Is this the current tag we are on?
          //
          if (MenuOption->Tags[Index].Value == TempValue) {
            //
            // Is this prior tag a valid choice?  If not, bail out
            //
            if (MenuOption->Tags[TempIndex].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
              //
              // Copy the destination tag to the local variable
              //
              CopyMem (&TagBackup, &MenuOption->Tags[TempIndex], sizeof (EFI_TAG));
              //
              // Copy the current tag to the tag location before us
              //
              CopyMem (&MenuOption->Tags[TempIndex], &MenuOption->Tags[Index], sizeof (EFI_TAG));
              //
              // Copy the backed up tag to the current location
              //
              CopyMem (&MenuOption->Tags[Index], &TagBackup, sizeof (EFI_TAG));

              //
              // Adjust the array of values
              //
              for (Index = 0; Index < ValueCount; Index++) {
                if (ValueArrayBackup[Index] == (UINT8) TempValue) {
                  if (Key.UnicodeChar == '+') {
                    if (Index == 0) {
                      //
                      // It is the top of the array already
                      //
                      break;
                    }

                    TempIndex = Index - 1;
                  } else {
                    if ((Index + 1) == ValueCount) {
                      //
                      // It is the bottom of the array already
                      //
                      break;
                    }

                    TempIndex = Index + 1;
                  }

                  ValueBackup                 = ValueArrayBackup[TempIndex];
                  ValueArrayBackup[TempIndex] = ValueArrayBackup[Index];
                  ValueArrayBackup[Index]     = ValueBackup;
                  Initialized                 = TRUE;
                  break;
                }
              }
              break;
            } else {
              break;
            }
          }
        }
      }
      break;

    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_UP:
      case SCAN_DOWN:
        if (Key.ScanCode == SCAN_UP) {
          if ((TopOptionIndex > 1) && (HighlightPosition == (Top + 1))) {
            //
            // Highlight reaches the top of the popup window, scroll one menu item.
            //
            TopOptionIndex--;
            ShowDownArrow = TRUE;
          }

          if (TopOptionIndex == 1) {
            ShowUpArrow = FALSE;
          }
        } else {
          if (((TopOptionIndex + MenuLinesInView) <= PopUpMenuLines) && (HighlightPosition == (Bottom - 1))) {
            //
            // Highlight reaches the bottom of the popup window, scroll one menu item.
            //
            TopOptionIndex++;
            ShowUpArrow = TRUE;
          }

          if ((TopOptionIndex + MenuLinesInView) == (PopUpMenuLines + 1)) {
            ShowDownArrow = FALSE;
          }
        }

        for (Index = MenuOption->TagIndex + TopOptionIndex;
             MenuOption->Tags[Index].Operand != EFI_IFR_END_ONE_OF_OP;
             Index++
            ) {
          if (MenuOption->Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
            if (Initialized) {
              for (Index = 0; (ValueArrayBackup[Index] != TempValue) && (Index < ValueCount); Index++)
                ;

              //
              // Did we hit the end of the array?  Either get the first TempValue or the next one
              //
              if (Key.ScanCode == SCAN_UP) {
                if (Index == 0) {
                  TempValue = ValueArrayBackup[0];
                } else {
                  TempValue = ValueArrayBackup[Index - 1];
                }
              } else {
                if ((Index + 1) == ValueCount) {
                  TempValue = ValueArrayBackup[Index];
                } else {
                  TempValue = ValueArrayBackup[Index + 1];
                }
              }
              break;
            } else {
              if (Key.ScanCode == SCAN_UP) {
                TempIndex = Index - 1;

                //
                // Keep going until meets meaningful tag.
                //
                while ((MenuOption->Tags[TempIndex].Operand != EFI_IFR_ONE_OF_OPTION_OP  &&
                         MenuOption->Tags[TempIndex].Operand != EFI_IFR_ONE_OF_OP        &&
                         MenuOption->Tags[TempIndex].Operand != EFI_IFR_END_ONE_OF_OP)
                       ||
                       (MenuOption->Tags[TempIndex].Operand == EFI_IFR_ONE_OF_OPTION_OP  &&
                         (MenuOption->Tags[TempIndex].Suppress || MenuOption->Tags[TempIndex].GrayOut))) {
                  TempIndex--;
                }
              } else {
                TempIndex = Index + 1;

                //
                // Keep going until meets meaningful tag.
                //
                while ((MenuOption->Tags[TempIndex].Operand != EFI_IFR_ONE_OF_OPTION_OP  &&
                         MenuOption->Tags[TempIndex].Operand != EFI_IFR_ONE_OF_OP        &&
                         MenuOption->Tags[TempIndex].Operand != EFI_IFR_END_ONE_OF_OP)
                       ||
                       (MenuOption->Tags[TempIndex].Operand == EFI_IFR_ONE_OF_OPTION_OP  &&
                         (MenuOption->Tags[TempIndex].Suppress || MenuOption->Tags[TempIndex].GrayOut))) {
                  TempIndex++;
                }
              }
              //
              // The option value is the same as what is stored in NV store.  This is where we take action
              //
              if (MenuOption->Tags[Index].Value == TempValue) {
                //
                // Only if the previous op-code is an option can we select it, otherwise we are at the left-most option
                //
                if (MenuOption->Tags[TempIndex].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
                  TempValue = MenuOption->Tags[TempIndex].Value;
                  *KeyValue = MenuOption->Tags[TempIndex].Key;
                } else {
                  TempValue = MenuOption->Tags[Index].Value;
                  *KeyValue = MenuOption->Tags[Index].Key;
                }
                break;
              }
            }
          }
        }
        break;

      case SCAN_ESC:
        gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);
        if (ValueArrayBackup != NULL) {
          FreePool (ValueArrayBackup);
        }

        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      //
      // return the current selection
      //
      if (Tag->Operand == EFI_IFR_ORDERED_LIST_OP) {
        CopyMem (ValueArray, ValueArrayBackup, ValueCount);
        FreePool (ValueArrayBackup);
      } else {
        *Value = TempValue;
      }

      goto Done;

    default:
      break;
    }
  } while (1);

Done:
  gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);
  return EFI_SUCCESS;
}

EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY           *Key
  )
{
  EFI_STATUS  Status;

  do {
    UiWaitForSingleEvent (gST->ConIn->WaitForKey, 0);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
  } while (EFI_ERROR(Status));

  return Status;
}
