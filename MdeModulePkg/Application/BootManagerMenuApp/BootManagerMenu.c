/** @file
  The application to show the Boot Manager Menu.

Copyright (c) 2011 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootManagerMenu.h"

EFI_HII_HANDLE  gStringPackHandle;

BOOLEAN  mModeInitialized = FALSE;

//
// Boot video resolution and text mode.
//
UINT32  mBootHorizontalResolution = 0;
UINT32  mBootVerticalResolution   = 0;
UINT32  mBootTextModeColumn       = 0;
UINT32  mBootTextModeRow          = 0;
//
// BIOS setup video resolution and text mode.
//
UINT32  mSetupTextModeColumn       = 0;
UINT32  mSetupTextModeRow          = 0;
UINT32  mSetupHorizontalResolution = 0;
UINT32  mSetupVerticalResolution   = 0;

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at
  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
PrintStringAt (
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *String
  )
{
  UINTN       ScreenWidth;
  UINTN       ScreenRows;
  CHAR16      *TurncateString;
  EFI_STATUS  Status;
  UINTN       ShowingLength;

  gST->ConOut->SetCursorPosition (gST->ConOut, Column, Row);

  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &ScreenWidth,
                 &ScreenRows
                 );

  if ((Column > (ScreenWidth - 1)) || (Row > (ScreenRows - 1))) {
    return 0;
  }

  if ((StrLen (String) + Column) > (ScreenWidth - 1)) {
    //
    // |      - ScreenWidth -       |
    // ...Column.....................
    // TurncateString length should leave one character for draw box and
    // require one character for string end.
    //
    ShowingLength  = ScreenWidth - Column - 1;
    TurncateString = AllocatePool ((ShowingLength + 1) * sizeof (CHAR16));

    if (TurncateString == NULL) {
      return 0;
    }

    Status = StrnCpyS (TurncateString, ShowingLength + 1, String, ShowingLength - 3);

    if (EFI_ERROR (Status)) {
      FreePool (TurncateString);
      return 0;
    }

    *(TurncateString + ShowingLength - 3) = L'.';
    *(TurncateString + ShowingLength - 2) = L'.';
    *(TurncateString + ShowingLength - 1) = L'.';
    *(TurncateString + ShowingLength)     = L'\0';
    ShowingLength                         = Print (L"%s", TurncateString);
    FreePool (TurncateString);
    return ShowingLength;
  } else {
    return Print (L"%s", String);
  }
}

/**
  Prints a character to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
PrintCharAt (
  IN UINTN  Column,
  IN UINTN  Row,
  CHAR16    Character
  )
{
  UINTN  ScreenWidth;
  UINTN  ScreenRows;

  gST->ConOut->SetCursorPosition (gST->ConOut, Column, Row);

  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &ScreenWidth,
                 &ScreenRows
                 );

  if ((Column > (ScreenWidth - 1)) || (Row > (ScreenRows - 1))) {
    return 0;
  }

  return Print (L"%c", Character);
}

/**
  Count the storage space of a Unicode string which uses current language to get
  from input string ID.

  @param StringId          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetLineWidth (
  IN EFI_STRING_ID  StringId
  )
{
  UINTN       Index;
  UINTN       IncrementValue;
  EFI_STRING  String;
  UINTN       LineWidth;

  LineWidth = 0;
  String    = HiiGetString (gStringPackHandle, StringId, NULL);

  if (String != NULL) {
    Index          = 0;
    IncrementValue = 1;

    do {
      //
      // Advance to the null-terminator or to the first width directive
      //
      for ( ;
            (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
            Index++, LineWidth = LineWidth + IncrementValue
            )
      {
      }

      //
      // We hit the null-terminator, we now have a count
      //
      if (String[Index] == 0) {
        break;
      }

      //
      // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
      // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
      //
      if (String[Index] == NARROW_CHAR) {
        //
        // Skip to the next character
        //
        Index++;
        IncrementValue = 1;
      } else {
        //
        // Skip to the next character
        //
        Index++;
        IncrementValue = 2;
      }
    } while (String[Index] != 0);

    FreePool (String);
  }

  return LineWidth;
}

/**
  This function uses calculate the boot menu location, size and scroll bar information.

  @param  BootMenuData            The boot menu data to be processed.

  @return EFI_SUCCESS             calculate boot menu information successful.
  @retval EFI_INVALID_PARAMETER   Input parameter is invalid

**/
EFI_STATUS
InitializeBootMenuScreen (
  IN OUT  BOOT_MENU_POPUP_DATA  *BootMenuData
  )
{
  UINTN  MaxStrWidth;
  UINTN  StrWidth;
  UINTN  Index;
  UINTN  Column;
  UINTN  Row;
  UINTN  MaxPrintRows;
  UINTN  UnSelectableItmes;

  if (BootMenuData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get maximum string width
  //
  MaxStrWidth = 0;
  for (Index = 0; Index < TITLE_TOKEN_COUNT; Index++) {
    StrWidth    = GetLineWidth (BootMenuData->TitleToken[Index]);
    MaxStrWidth = MaxStrWidth > StrWidth ? MaxStrWidth : StrWidth;
  }

  for (Index = 0; Index < BootMenuData->ItemCount; Index++) {
    StrWidth    = GetLineWidth (BootMenuData->PtrTokens[Index]);
    MaxStrWidth = MaxStrWidth > StrWidth ? MaxStrWidth : StrWidth;
  }

  for (Index = 0; Index < HELP_TOKEN_COUNT; Index++) {
    StrWidth    = GetLineWidth (BootMenuData->HelpToken[Index]);
    MaxStrWidth = MaxStrWidth > StrWidth ? MaxStrWidth : StrWidth;
  }

  //
  // query current row and column to calculate boot menu location
  //
  gST->ConOut->QueryMode (
                 gST->ConOut,
                 gST->ConOut->Mode->Mode,
                 &Column,
                 &Row
                 );

  MaxPrintRows      = Row - 6;
  UnSelectableItmes = TITLE_TOKEN_COUNT + 2 + HELP_TOKEN_COUNT + 2;
  if (MaxStrWidth + 8 > Column) {
    BootMenuData->MenuScreen.Width = Column;
  } else {
    BootMenuData->MenuScreen.Width = MaxStrWidth + 8;
  }

  if (BootMenuData->ItemCount + UnSelectableItmes > MaxPrintRows) {
    BootMenuData->MenuScreen.Height                   = MaxPrintRows;
    BootMenuData->ScrollBarControl.HasScrollBar       = TRUE;
    BootMenuData->ScrollBarControl.ItemCountPerScreen = MaxPrintRows - UnSelectableItmes;
    BootMenuData->ScrollBarControl.FirstItem          = 0;
    BootMenuData->ScrollBarControl.LastItem           = MaxPrintRows - UnSelectableItmes - 1;
  } else {
    BootMenuData->MenuScreen.Height                   = BootMenuData->ItemCount + UnSelectableItmes;
    BootMenuData->ScrollBarControl.HasScrollBar       = FALSE;
    BootMenuData->ScrollBarControl.ItemCountPerScreen = BootMenuData->ItemCount;
    BootMenuData->ScrollBarControl.FirstItem          = 0;
    BootMenuData->ScrollBarControl.LastItem           = BootMenuData->ItemCount - 1;
  }

  BootMenuData->MenuScreen.StartCol = (Column -  BootMenuData->MenuScreen.Width) / 2;
  BootMenuData->MenuScreen.StartRow = (Row -  BootMenuData->MenuScreen.Height) / 2;

  return EFI_SUCCESS;
}

/**
  This function uses check boot option is wheher setup application or no

  @param   BootOption   Pointer to EFI_BOOT_MANAGER_LOAD_OPTION array.

  @retval  TRUE         This boot option is setup application.
  @retval  FALSE        This boot options isn't setup application

**/
BOOLEAN
IsBootManagerMenu (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootManagerMenu;

  Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  if (!EFI_ERROR (Status)) {
    EfiBootManagerFreeLoadOption (&BootManagerMenu);
  }

  return (BOOLEAN)(!EFI_ERROR (Status) && (BootOption->OptionNumber == BootManagerMenu.OptionNumber));
}

/**
  Return whether to ignore the boot option.

  @param BootOption  Pointer to EFI_BOOT_MANAGER_LOAD_OPTION to check.

  @retval TRUE  Ignore the boot option.
  @retval FALSE Do not ignore the boot option.
**/
BOOLEAN
IgnoreBootOption (
  IN   EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;

  //
  // Ignore myself.
  //
  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageDevicePathProtocolGuid, (VOID **)&ImageDevicePath);
  ASSERT_EFI_ERROR (Status);
  if (CompareMem (BootOption->FilePath, ImageDevicePath, GetDevicePathSize (ImageDevicePath)) == 0) {
    return TRUE;
  }

  //
  // Do not ignore Boot Manager Menu.
  //
  if (IsBootManagerMenu (BootOption)) {
    return FALSE;
  }

  //
  // Ignore the hidden/inactive boot option.
  //
  if (((BootOption->Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption->Attributes & LOAD_OPTION_ACTIVE) == 0)) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function uses to initialize boot menu data

  @param   BootOption             Pointer to EFI_BOOT_MANAGER_LOAD_OPTION array.
  @param   BootOptionCount        Number of boot option.
  @param   BootMenuData           The Input BootMenuData to be initialized.

  @retval  EFI_SUCCESS            Initialize boot menu data successful.
  @retval  EFI_INVALID_PARAMETER  Input parameter is invalid.

**/
EFI_STATUS
InitializeBootMenuData (
  IN   EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption,
  IN   UINTN                         BootOptionCount,
  OUT  BOOT_MENU_POPUP_DATA          *BootMenuData
  )
{
  UINTN  Index;
  UINTN  StrIndex;

  if ((BootOption == NULL) || (BootMenuData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  BootMenuData->TitleToken[0] = STRING_TOKEN (STR_BOOT_POPUP_MENU_TITLE_STRING);
  BootMenuData->PtrTokens     = AllocateZeroPool (BootOptionCount * sizeof (EFI_STRING_ID));
  ASSERT (BootMenuData->PtrTokens != NULL);

  //
  // Skip boot option which created by BootNext Variable
  //
  for (StrIndex = 0, Index = 0; Index < BootOptionCount; Index++) {
    if (IgnoreBootOption (&BootOption[Index])) {
      continue;
    }

    ASSERT (BootOption[Index].Description != NULL);
    BootMenuData->PtrTokens[StrIndex++] = HiiSetString (
                                            gStringPackHandle,
                                            0,
                                            BootOption[Index].Description,
                                            NULL
                                            );
  }

  BootMenuData->ItemCount    = StrIndex;
  BootMenuData->HelpToken[0] = STRING_TOKEN (STR_BOOT_POPUP_MENU_HELP1_STRING);
  BootMenuData->HelpToken[1] = STRING_TOKEN (STR_BOOT_POPUP_MENU_HELP2_STRING);
  BootMenuData->HelpToken[2] = STRING_TOKEN (STR_BOOT_POPUP_MENU_HELP3_STRING);
  InitializeBootMenuScreen (BootMenuData);
  BootMenuData->SelectItem = 0;
  return EFI_SUCCESS;
}

/**
  This function uses input select item to highlight selected item
  and set current selected item in BootMenuData

  @param  WantSelectItem          The user wants to select item.
  @param  BootMenuData            The boot menu data to be processed

  @return EFI_SUCCESS             Highlight selected item and update current selected
                                  item successful
  @retval EFI_INVALID_PARAMETER   Input parameter is invalid
**/
EFI_STATUS
BootMenuSelectItem (
  IN     UINTN                 WantSelectItem,
  IN OUT BOOT_MENU_POPUP_DATA  *BootMenuData
  )
{
  INT32       SavedAttribute;
  EFI_STRING  String;
  UINTN       StartCol;
  UINTN       StartRow;
  UINTN       PrintCol;
  UINTN       PrintRow;
  UINTN       TopShadeNum;
  UINTN       LowShadeNum;
  UINTN       FirstItem;
  UINTN       LastItem;
  UINTN       ItemCountPerScreen;
  UINTN       Index;
  BOOLEAN     RePaintItems;

  if ((BootMenuData == NULL) || (WantSelectItem >= BootMenuData->ItemCount)) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (BootMenuData->ItemCount != 0);
  SavedAttribute = gST->ConOut->Mode->Attribute;
  RePaintItems   = FALSE;
  StartCol       = BootMenuData->MenuScreen.StartCol;
  StartRow       = BootMenuData->MenuScreen.StartRow;
  //
  // print selectable items again and adjust scroll bar if need
  //
  if (BootMenuData->ScrollBarControl.HasScrollBar &&
      ((WantSelectItem < BootMenuData->ScrollBarControl.FirstItem) ||
       (WantSelectItem > BootMenuData->ScrollBarControl.LastItem) ||
       (WantSelectItem == BootMenuData->SelectItem)))
  {
    ItemCountPerScreen = BootMenuData->ScrollBarControl.ItemCountPerScreen;
    //
    // Set first item and last item
    //
    if (WantSelectItem < BootMenuData->ScrollBarControl.FirstItem) {
      BootMenuData->ScrollBarControl.FirstItem = WantSelectItem;
      BootMenuData->ScrollBarControl.LastItem  = WantSelectItem + ItemCountPerScreen - 1;
    } else if (WantSelectItem > BootMenuData->ScrollBarControl.LastItem) {
      BootMenuData->ScrollBarControl.FirstItem = WantSelectItem - ItemCountPerScreen + 1;
      BootMenuData->ScrollBarControl.LastItem  = WantSelectItem;
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);
    FirstItem   = BootMenuData->ScrollBarControl.FirstItem;
    LastItem    = BootMenuData->ScrollBarControl.LastItem;
    TopShadeNum = 0;
    if (FirstItem != 0) {
      TopShadeNum = (FirstItem * ItemCountPerScreen) / BootMenuData->ItemCount;
      if ((FirstItem * ItemCountPerScreen) % BootMenuData->ItemCount != 0) {
        TopShadeNum++;
      }

      PrintCol = StartCol  + BootMenuData->MenuScreen.Width - 2;
      PrintRow = StartRow + TITLE_TOKEN_COUNT + 2;
      for (Index = 0; Index < TopShadeNum; Index++, PrintRow++) {
        PrintCharAt (PrintCol, PrintRow, BLOCKELEMENT_LIGHT_SHADE);
      }
    }

    LowShadeNum = 0;
    if (LastItem != BootMenuData->ItemCount - 1) {
      LowShadeNum = ((BootMenuData->ItemCount - 1 - LastItem) * ItemCountPerScreen) / BootMenuData->ItemCount;
      if (((BootMenuData->ItemCount - 1 - LastItem) * ItemCountPerScreen) % BootMenuData->ItemCount != 0) {
        LowShadeNum++;
      }

      PrintCol = StartCol  + BootMenuData->MenuScreen.Width - 2;
      PrintRow = StartRow + TITLE_TOKEN_COUNT + 2 + ItemCountPerScreen - LowShadeNum;
      for (Index = 0; Index < LowShadeNum; Index++, PrintRow++) {
        PrintCharAt (PrintCol, PrintRow, BLOCKELEMENT_LIGHT_SHADE);
      }
    }

    PrintCol = StartCol  + BootMenuData->MenuScreen.Width - 2;
    PrintRow = StartRow + TITLE_TOKEN_COUNT + 2 + TopShadeNum;
    for (Index = TopShadeNum; Index < ItemCountPerScreen - LowShadeNum; Index++, PrintRow++) {
      PrintCharAt (PrintCol, PrintRow, BLOCKELEMENT_FULL_BLOCK);
    }

    //
    // Clear selectable items first
    //
    PrintCol = StartCol  + 1;
    PrintRow = StartRow + TITLE_TOKEN_COUNT + 2;
    String   = AllocateZeroPool ((BootMenuData->MenuScreen.Width - 2) * sizeof (CHAR16));
    ASSERT (String != NULL);
    for (Index = 0; Index < BootMenuData->MenuScreen.Width - 3; Index++) {
      String[Index] = 0x20;
    }

    for (Index = 0; Index < ItemCountPerScreen; Index++) {
      PrintStringAt (PrintCol, PrintRow + Index, String);
    }

    FreePool (String);
    //
    // print selectable items
    //
    for (Index = 0; Index < ItemCountPerScreen; Index++, PrintRow++) {
      String = HiiGetString (gStringPackHandle, BootMenuData->PtrTokens[Index + FirstItem], NULL);
      PrintStringAt (PrintCol, PrintRow, String);
      FreePool (String);
    }

    RePaintItems = TRUE;
  }

  //
  // if Want Select and selected item isn't the same and doesn't re-draw selectable
  // items, clear select item
  //
  FirstItem = BootMenuData->ScrollBarControl.FirstItem;
  if ((WantSelectItem != BootMenuData->SelectItem) && !RePaintItems) {
    gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);
    String   = HiiGetString (gStringPackHandle, BootMenuData->PtrTokens[BootMenuData->SelectItem], NULL);
    PrintCol = StartCol  + 1;
    PrintRow = StartRow + 3 + BootMenuData->SelectItem - FirstItem;
    PrintStringAt (PrintCol, PrintRow, String);
    FreePool (String);
  }

  //
  // Print want to select item
  //
  gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLACK);
  String   = HiiGetString (gStringPackHandle, BootMenuData->PtrTokens[WantSelectItem], NULL);
  PrintCol = StartCol  + 1;
  PrintRow = StartRow + TITLE_TOKEN_COUNT + 2 + WantSelectItem - FirstItem;
  PrintStringAt (PrintCol, PrintRow, String);
  FreePool (String);

  gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);
  BootMenuData->SelectItem = WantSelectItem;
  return EFI_SUCCESS;
}

/**
  This function uses to draw boot popup menu

  @param   BootMenuData           The Input BootMenuData to be processed.

  @retval  EFI_SUCCESS            Draw boot popup menu successful.

**/
EFI_STATUS
DrawBootPopupMenu (
  IN  BOOT_MENU_POPUP_DATA  *BootMenuData
  )
{
  EFI_STRING  String;
  UINTN       Index;
  UINTN       Width;
  UINTN       StartCol;
  UINTN       StartRow;
  UINTN       PrintRow;
  UINTN       PrintCol;
  UINTN       LineWidth;
  INT32       SavedAttribute;
  UINTN       ItemCountPerScreen;

  gST->ConOut->ClearScreen (gST->ConOut);

  SavedAttribute = gST->ConOut->Mode->Attribute;
  gST->ConOut->SetAttribute (gST->ConOut, EFI_WHITE | EFI_BACKGROUND_BLUE);
  Width              = BootMenuData->MenuScreen.Width;
  StartCol           = BootMenuData->MenuScreen.StartCol;
  StartRow           = BootMenuData->MenuScreen.StartRow;
  ItemCountPerScreen = BootMenuData->ScrollBarControl.ItemCountPerScreen;
  PrintRow           = StartRow;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  //
  // Draw Boot popup menu screen
  //
  PrintCharAt (StartCol, PrintRow, BOXDRAW_DOWN_RIGHT);
  for (Index = 1; Index < Width - 1; Index++) {
    PrintCharAt (StartCol + Index, PrintRow, BOXDRAW_HORIZONTAL);
  }

  PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_DOWN_LEFT);

  //
  // Draw the screen for title
  //
  String = AllocateZeroPool ((Width - 1) * sizeof (CHAR16));
  ASSERT (String != NULL);
  for (Index = 0; Index < Width - 2; Index++) {
    String[Index] = 0x20;
  }

  for (Index = 0; Index < TITLE_TOKEN_COUNT; Index++) {
    PrintRow++;
    PrintCharAt (StartCol, PrintRow, BOXDRAW_VERTICAL);
    PrintStringAt (StartCol + 1, PrintRow, String);
    PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_VERTICAL);
  }

  PrintRow++;
  PrintCharAt (StartCol, PrintRow, BOXDRAW_VERTICAL_RIGHT);
  for (Index = 1; Index < Width - 1; Index++) {
    PrintCharAt (StartCol + Index, PrintRow, BOXDRAW_HORIZONTAL);
  }

  PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_VERTICAL_LEFT);

  //
  // Draw screen for selectable items
  //
  for (Index = 0; Index < ItemCountPerScreen; Index++) {
    PrintRow++;
    PrintCharAt (StartCol, PrintRow, BOXDRAW_VERTICAL);
    PrintStringAt (StartCol + 1, PrintRow, String);
    PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_VERTICAL);
  }

  PrintRow++;
  PrintCharAt (StartCol, PrintRow, BOXDRAW_VERTICAL_RIGHT);
  for (Index = 1; Index < Width - 1; Index++) {
    PrintCharAt (StartCol + Index, PrintRow, BOXDRAW_HORIZONTAL);
  }

  PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_VERTICAL_LEFT);

  //
  // Draw screen for Help
  //
  for (Index = 0; Index < HELP_TOKEN_COUNT; Index++) {
    PrintRow++;
    PrintCharAt (StartCol, PrintRow, BOXDRAW_VERTICAL);
    PrintStringAt (StartCol + 1, PrintRow, String);
    PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_VERTICAL);
  }

  FreePool (String);

  PrintRow++;
  PrintCharAt (StartCol, PrintRow, BOXDRAW_UP_RIGHT);
  for (Index = 1; Index < Width - 1; Index++) {
    PrintCharAt (StartCol + Index, PrintRow, BOXDRAW_HORIZONTAL);
  }

  PrintCharAt (StartCol + Width - 1, PrintRow, BOXDRAW_UP_LEFT);

  //
  // print title strings
  //
  PrintRow = StartRow + 1;
  for (Index = 0; Index < TITLE_TOKEN_COUNT; Index++, PrintRow++) {
    String    = HiiGetString (gStringPackHandle, BootMenuData->TitleToken[Index], NULL);
    LineWidth = GetLineWidth (BootMenuData->TitleToken[Index]);
    PrintCol  = StartCol + (Width - LineWidth) / 2;
    PrintStringAt (PrintCol, PrintRow, String);
    FreePool (String);
  }

  //
  // print selectable items
  //
  PrintCol = StartCol + 1;
  PrintRow = StartRow + TITLE_TOKEN_COUNT + 2;
  for (Index = 0; Index < ItemCountPerScreen; Index++, PrintRow++) {
    String = HiiGetString (gStringPackHandle, BootMenuData->PtrTokens[Index], NULL);
    PrintStringAt (PrintCol, PrintRow, String);
    FreePool (String);
  }

  //
  // Print Help strings
  //
  PrintRow++;
  for (Index = 0; Index < HELP_TOKEN_COUNT; Index++, PrintRow++) {
    String    = HiiGetString (gStringPackHandle, BootMenuData->HelpToken[Index], NULL);
    LineWidth = GetLineWidth (BootMenuData->HelpToken[Index]);
    PrintCol  = StartCol + (Width - LineWidth) / 2;
    PrintStringAt (PrintCol, PrintRow, String);
    FreePool (String);
  }

  //
  // Print scroll bar if has scroll bar
  //
  if (BootMenuData->ScrollBarControl.HasScrollBar) {
    PrintCol = StartCol + Width - 2;
    PrintRow = StartRow + 2;
    PrintCharAt (PrintCol, PrintRow, GEOMETRICSHAPE_UP_TRIANGLE);
    PrintCharAt (PrintCol + 1, PrintRow, BOXDRAW_VERTICAL);
    PrintRow += (ItemCountPerScreen + 1);
    PrintCharAt (PrintCol, PrintRow, GEOMETRICSHAPE_DOWN_TRIANGLE);
    PrintCharAt (PrintCol + 1, PrintRow, BOXDRAW_VERTICAL);
  }

  gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);
  //
  // Print Selected item
  //
  BootMenuSelectItem (BootMenuData->SelectItem, BootMenuData);
  return EFI_SUCCESS;
}

/**
  This function uses to boot from selected item

  @param   BootOptions            Pointer to EFI_BOOT_MANAGER_LOAD_OPTION array.
  @param   BootOptionCount        Number of boot option.
  @param   SelectItem             Current selected item.
**/
VOID
BootFromSelectOption (
  IN   EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  IN   UINTN                         BootOptionCount,
  IN   UINTN                         SelectItem
  )
{
  UINTN  ItemNum;
  UINTN  Index;

  ASSERT (BootOptions != NULL);

  for (ItemNum = 0, Index = 0; Index < BootOptionCount; Index++) {
    if (IgnoreBootOption (&BootOptions[Index])) {
      continue;
    }

    if (ItemNum++ == SelectItem) {
      EfiBootManagerBoot (&BootOptions[Index]);
      break;
    }
  }
}

/**
  This function will change video resolution and text mode
  according to defined setup mode or defined boot mode

  @param  IsSetupMode   Indicate mode is changed to setup mode or boot mode.

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others             Mode failed to be changed.

**/
EFI_STATUS
EFIAPI
BdsSetConsoleMode (
  BOOLEAN  IsSetupMode
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINT32                                NewHorizontalResolution;
  UINT32                                NewVerticalResolution;
  UINT32                                NewColumns;
  UINT32                                NewRows;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;

  MaxGopMode  = 0;
  MaxTextMode = 0;

  //
  // Get current video resolution and text mode
  //
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (IsSetupMode) {
    //
    // The required resolution and text mode is setup mode.
    //
    NewHorizontalResolution = mSetupHorizontalResolution;
    NewVerticalResolution   = mSetupVerticalResolution;
    NewColumns              = mSetupTextModeColumn;
    NewRows                 = mSetupTextModeRow;
  } else {
    //
    // The required resolution and text mode is boot mode.
    //
    NewHorizontalResolution = mBootHorizontalResolution;
    NewVerticalResolution   = mBootVerticalResolution;
    NewColumns              = mBootTextModeColumn;
    NewRows                 = mBootTextModeRow;
  }

  if (GraphicsOutput != NULL) {
    MaxGopMode = GraphicsOutput->Mode->MaxMode;
  }

  if (SimpleTextOut != NULL) {
    MaxTextMode = SimpleTextOut->Mode->MaxMode;
  }

  //
  // 1. If current video resolution is same with required video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with required text mode, text mode need not be changed.
  //    1.2. If current text mode is different from required text mode, text mode need be changed.
  // 2. If current video resolution is different from required video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                               GraphicsOutput,
                               ModeNumber,
                               &SizeOfInfo,
                               &Info
                               );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution))
      {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution))
        {
          //
          // Current resolution is same with required resolution, check if text mode need be set
          //
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
            //
            // If current text mode is same with required text mode. Do nothing
            //
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            //
            // If current text mode is different from required text mode.  Set new video mode
            //
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR (Status)) {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows)) {
                  //
                  // Required text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  //
                  // Update text mode PCD.
                  //
                  Status = PcdSet32S (PcdConOutColumn, mSetupTextModeColumn);
                  ASSERT_EFI_ERROR (Status);
                  Status = PcdSet32S (PcdConOutRow, mSetupTextModeRow);
                  ASSERT_EFI_ERROR (Status);
                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }

            if (Index == MaxTextMode) {
              //
              // If required text mode is not supported, return error.
              //
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }

      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    //
    // If the resolution is not supported, return error.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Set PCD to Inform GraphicsConsole to change video resolution.
  // Set PCD to Inform Consplitter to change text mode.
  //
  Status = PcdSet32S (PcdVideoHorizontalResolution, NewHorizontalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdVideoVerticalResolution, NewVerticalResolution);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutColumn, NewColumns);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdConOutRow, NewRows);
  ASSERT_EFI_ERROR (Status);

  //
  // Video mode is changed, so restart graphics console driver and higher level driver.
  // Reconnect graphics console driver and higher level driver.
  // Locate all the handles with GOP protocol and reconnect it.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
    }

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }
  }

  return EFI_SUCCESS;
}

/**
  Display the boot popup menu and allow user select boot item.

  @param   ImageHandle     The image handle.
  @param   SystemTable     The system table.

  @retval  EFI_SUCCESS          Boot from selected boot option, and return success from boot option
  @retval  EFI_NOT_FOUND        User select to enter setup or can not find boot option

**/
EFI_STATUS
EFIAPI
BootManagerMenuEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION     *BootOption;
  UINTN                            BootOptionCount;
  EFI_STATUS                       Status;
  BOOT_MENU_POPUP_DATA             BootMenuData;
  UINTN                            Index;
  EFI_INPUT_KEY                    Key;
  BOOLEAN                          ExitApplication;
  UINTN                            SelectItem;
  EFI_BOOT_LOGO_PROTOCOL           *BootLogo;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOut;
  UINTN                            BootTextColumn;
  UINTN                            BootTextRow;

  //
  // Set Logo status invalid when boot manager menu is launched
  //
  BootLogo = NULL;
  Status   = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **)&BootLogo);
  if (!EFI_ERROR (Status) && (BootLogo != NULL)) {
    Status = BootLogo->SetBootLogo (BootLogo, NULL, 0, 0, 0, 0);
    ASSERT_EFI_ERROR (Status);
  }

  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  gStringPackHandle = HiiAddPackages (
                        &gEfiCallerIdGuid,
                        gImageHandle,
                        BootManagerMenuAppStrings,
                        NULL
                        );
  ASSERT (gStringPackHandle != NULL);

  //
  // Connect all prior to entering the platform setup menu.
  //
  EfiBootManagerConnectAll ();
  EfiBootManagerRefreshAllBootOption ();

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  if (!mModeInitialized) {
    //
    // After the console is ready, get current video resolution
    // and text mode before launching setup at first time.
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
      GraphicsOutput = NULL;
    }

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    (VOID **)&SimpleTextOut
                    );
    if (EFI_ERROR (Status)) {
      SimpleTextOut = NULL;
    }

    if (GraphicsOutput != NULL) {
      //
      // Get current video resolution and text mode.
      //
      mBootHorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
      mBootVerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
    }

    if (SimpleTextOut != NULL) {
      Status = SimpleTextOut->QueryMode (
                                SimpleTextOut,
                                SimpleTextOut->Mode->Mode,
                                &BootTextColumn,
                                &BootTextRow
                                );
      mBootTextModeColumn = (UINT32)BootTextColumn;
      mBootTextModeRow    = (UINT32)BootTextRow;
    }

    //
    // Get user defined text mode for setup.
    //
    mSetupHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
    mSetupVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
    mSetupTextModeColumn       = PcdGet32 (PcdSetupConOutColumn);
    mSetupTextModeRow          = PcdGet32 (PcdSetupConOutRow);
    mModeInitialized           = TRUE;
  }

  //
  // Set back to conventional setup resolution
  //
  BdsSetConsoleMode (TRUE);

  //
  // Initialize Boot menu data
  //
  Status = InitializeBootMenuData (BootOption, BootOptionCount, &BootMenuData);
  //
  // According to boot menu data to draw boot popup menu
  //
  DrawBootPopupMenu (&BootMenuData);

  //
  // check user input to determine want to re-draw or boot from user selected item
  //
  ExitApplication = FALSE;
  while (!ExitApplication) {
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (!EFI_ERROR (Status)) {
      switch (Key.UnicodeChar) {
        case CHAR_NULL:
          switch (Key.ScanCode) {
            case SCAN_UP:
              SelectItem = BootMenuData.SelectItem == 0 ? BootMenuData.ItemCount - 1 : BootMenuData.SelectItem - 1;
              BootMenuSelectItem (SelectItem, &BootMenuData);
              break;

            case SCAN_DOWN:
              SelectItem = BootMenuData.SelectItem == BootMenuData.ItemCount - 1 ? 0 : BootMenuData.SelectItem + 1;
              BootMenuSelectItem (SelectItem, &BootMenuData);
              break;

            case SCAN_ESC:
              gST->ConOut->ClearScreen (gST->ConOut);
              ExitApplication = TRUE;
              //
              // Set boot resolution for normal boot
              //
              BdsSetConsoleMode (FALSE);
              break;

            default:
              break;
          }

          break;

        case CHAR_CARRIAGE_RETURN:
          gST->ConOut->ClearScreen (gST->ConOut);
          //
          // Set boot resolution for normal boot
          //
          BdsSetConsoleMode (FALSE);
          BootFromSelectOption (BootOption, BootOptionCount, BootMenuData.SelectItem);
          //
          // Back to boot manager menu again, set back to setup resolution
          //
          BdsSetConsoleMode (TRUE);
          DrawBootPopupMenu (&BootMenuData);
          break;

        default:
          break;
      }
    }
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);
  FreePool (BootMenuData.PtrTokens);

  HiiRemovePackages (gStringPackHandle);

  return Status;
}
