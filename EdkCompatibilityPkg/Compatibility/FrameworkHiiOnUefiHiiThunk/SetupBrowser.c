/** @file
Framework to UEFI 2.1 Setup Browser Thunk. The file consume EFI_FORM_BROWSER2_PROTOCOL
to produce a EFI_FORM_BROWSER_PROTOCOL.

Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"
#include "SetupBrowser.h"

EFI_HII_HANDLE gStringPackHandle = NULL;
BOOLEAN mFrontPageDisplayed = FALSE;
//
// 106F3545-B788-4cb5-9D2A-CE0CDB208DF5
//
EFI_GUID gEfiHiiThunkProducerGuid = { 0x106f3545, 0xb788, 0x4cb5, { 0x9d, 0x2a, 0xce, 0xc, 0xdb, 0x20, 0x8d, 0xf5 } }; 


/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (gStringPackHandle, Id, NULL);
}

/**

  Show progress bar with title above it. It only works in Graphics mode.


  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS       Success update the progress bar

**/
EFI_STATUS
PlatformBdsShowProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL          *UgaDraw;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          BlockNum;
  UINTN                          PosX;
  UINTN                          PosY;
  UINTN                          Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  UgaDraw = NULL;
  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );
  }
  if (EFI_ERROR (Status) || (GraphicsOutput == NULL && UgaDraw == NULL)) {
    return EFI_UNSUPPORTED;
  }

  SizeOfX = 0;
  SizeOfY = 0;
  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  } else {
    Status = UgaDraw->GetMode (
                        UgaDraw,
                        &SizeOfX,
                        &SizeOfY,
                        &ColorDepth,
                        &RefreshRate
                        );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  BlockWidth  = SizeOfX / 100;
  BlockHeight = SizeOfY / 50;

  BlockNum    = Progress;

  PosX        = 0;
  PosY        = SizeOfY * 48 / 50;

  if (BlockNum == 0) {
    //
    // Clear progress area
    //
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);

    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &Color,
                          EfiBltVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &Color,
                          EfiUgaVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_UGA_PIXEL)
                          );
    }
  }
  //
  // Show progress by drawing blocks
  //
  for (Index = PreviousValue; Index < BlockNum; Index++) {
    PosX = Index * BlockWidth;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &ProgressColor,
                          EfiBltVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &ProgressColor,
                          EfiUgaVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_UGA_PIXEL)
                          );
    }
  }

  PrintXY (
    (SizeOfX - StrLen (Title) * EFI_GLYPH_WIDTH) / 2,
    PosY - EFI_GLYPH_HEIGHT - 1,
    &TitleForeground,
    &TitleBackground,
    Title
    );

  return EFI_SUCCESS;
}

/**
  Function waits for a given event to fire, or for an optional timeout to expire.


  @param Event           The event to wait for
                        
  @param Timeout         An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS       Event fired before Timeout expired.
  @retval  EFI_TIME_OUT      Timout expired before Event fired..

**/
EFI_STATUS
WaitForSingleEvent (
  IN EFI_EVENT                  Event,
  IN UINT64                     Timeout OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
            TimerEvent,
            TimerRelative,
            Timeout
            );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  Function show progress bar to wait for user input.


  @param TimeoutDefault  - The fault time out value before the system
                         continue to boot.

  @retval  EFI_SUCCESS       User pressed some key except "Enter"
  @retval  EFI_TIME_OUT      Timout expired or user press "Enter"

**/
EFI_STATUS
ShowProgress (
  IN UINT16                       TimeoutDefault
  )
{
  EFI_STATUS                    Status;
  CHAR16                        *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  EFI_INPUT_KEY                 Key;
  UINT16                        TimeoutRemain;

  if (TimeoutDefault == 0) {
    return EFI_TIMEOUT;
  }

  DEBUG ((EFI_D_INFO, "\n\nStart showing progress bar... Press any key to stop it! ...Zzz....\n"));
  
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  //
  // Clear the progress status bar first
  //
  TmpStr = GetStringById (STRING_TOKEN (STR_START_BOOT_OPTION));
  if (TmpStr != NULL) {
    PlatformBdsShowProgress (Foreground, Background, TmpStr, Color, 0, 0);
  }

  TimeoutRemain = TimeoutDefault;
  while (TimeoutRemain != 0) {
    DEBUG ((EFI_D_INFO, "Showing progress bar...Remaining %d second!\n", TimeoutRemain));
    
    Status = WaitForSingleEvent (gST->ConIn->WaitForKey, ONE_SECOND);
    if (Status != EFI_TIMEOUT) {
      break;
    }
    TimeoutRemain--;

    //
    // Show progress
    //
    if (TmpStr != NULL) {
      PlatformBdsShowProgress (
        Foreground,
        Background,
        TmpStr,
        Color,
        ((TimeoutDefault - TimeoutRemain) * 100 / TimeoutDefault),
        0
        );
    }
  }
  gBS->FreePool (TmpStr);

  //
  // Timeout expired
  //
  if (TimeoutRemain == 0) {
    return EFI_TIMEOUT;
  }

  //
  // User pressed some key
  //
  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
    //
    // User pressed enter, equivalent to select "continue"
    //
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Return the default value for system Timeout variable.

  @return Timeout value.

**/
UINT16
EFIAPI
GetTimeout (
  VOID
  )
{
  return PcdGet16 (PcdPlatformBootTimeOut);
}


/**
  This is the Framework Setup Browser interface which displays a FormSet.

  @param This           The EFI_FORM_BROWSER_PROTOCOL context.
  @param UseDatabase    TRUE if the FormSet is from HII database. The Thunk implementation
                        only support UseDatabase is TRUE.
  @param Handle         The Handle buffer.
  @param HandleCount    The number of Handle in the Handle Buffer. It must be 1 for this implementation.
  @param Packet         The pointer to data buffer containing IFR and String package. Not supported.
  @param CallbackHandle Not supported.
  @param NvMapOverride  The buffer is used only when there is no NV variable to define the 
                        current settings and the caller needs to provide to the browser the
                        current settings for the the "fake" NV variable. If used, no saving of
                        an NV variable is possbile. This parameter is also ignored if Handle is NULL.
  @param ScreenDimensions 
                        Allows the browser to be called so that it occupies a portion of the physical 
                        screen instead of dynamically determining the screen dimensions.
  @param ResetRequired  This BOOLEAN value denotes whether a reset is required based on the data that 
                        might have been changed. The ResetRequired parameter is primarily applicable 
                        for configuration applications, and is an optional parameter.

  @retval EFI_SUCCESS             If the Formset is displayed correctly.
  @retval EFI_UNSUPPORTED         If UseDatabase is FALSE or HandleCount is not 1.
  @retval EFI_INVALID_PARAMETER   If the *Handle passed in is not found in the database.
**/
EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL         *This,
  IN  BOOLEAN                           UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE          *Handle,
  IN  UINTN                             HandleCount,
  IN  EFI_IFR_PACKET                    *Packet, OPTIONAL
  IN  EFI_HANDLE                        CallbackHandle, OPTIONAL
  IN  UINT8                             *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR   *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                           *ResetRequired OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_BROWSER_ACTION_REQUEST        ActionRequest;
  HII_THUNK_CONTEXT                 *ThunkContext;
  HII_THUNK_PRIVATE_DATA            *Private;
  EFI_FORMBROWSER_THUNK_PRIVATE_DATA *BrowserPrivate;

  if (!UseDatabase) {
    //
    // ThunkSendForm only support displays forms registered into the HII database.
    //
    return EFI_UNSUPPORTED;
  }

  if (HandleCount != 1 ) {
    return EFI_UNSUPPORTED;
  }

  BrowserPrivate = EFI_FORMBROWSER_THUNK_PRIVATE_DATA_FROM_THIS (This);
  Private = BrowserPrivate->ThunkPrivate;

  ThunkContext = FwHiiHandleToThunkContext (Private, *Handle);
  if (ThunkContext == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Following UEFI spec to do auto booting after a time-out. This feature is implemented 
  // in Framework Setup Browser and moved to MdeModulePkg/Universal/BdsDxe. The auto booting is
  // moved here in HII Thunk module. 
  //
  if (CompareGuid (&gFrameworkBdsFrontPageFormsetGuid, &ThunkContext->FormSet->Guid) && !mFrontPageDisplayed) {
    //
    // Send form is called before entering the 
    //
    mFrontPageDisplayed = TRUE;
    Status = ShowProgress (GetTimeout ());

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  
  if (NvMapOverride != NULL) {
    ThunkContext->NvMapOverride = NvMapOverride;
  }

  Status = mFormBrowser2Protocol->SendForm (
                                    mFormBrowser2Protocol,
                                    &ThunkContext->UefiHiiHandle,
                                    1,
                                    NULL,
                                    0,
                                    (EFI_SCREEN_DESCRIPTOR *) ScreenDimensions,
                                    &ActionRequest
                                    );

  if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
    *ResetRequired = TRUE;
  }
  
  return Status;
}

/** 

  Rountine used to display a generic dialog interface and return 
  the Key or Input from user input.

  @param LinesNumber   The number of lines for the dialog box.
  @param HotKey        Defines if a single character is parsed (TRUE) and returned in KeyValue
                       or if a string is returned in StringBuffer.
  @param MaximumStringSize The maximum size in bytes of a typed-in string.
  @param StringBuffer  On return contains the typed-in string if HotKey is FALSE.
  @param Key           The EFI_INPUT_KEY value returned if HotKey is TRUE.
  @param FirstString   The pointer to the first string in the list of strings
                       that comprise the dialog box.
  @param ...           A series of NumberOfLines text strings that will be used
                       to construct the dialog box.
  @retval EFI_SUCCESS  The dialog is created successfully and user interaction was received.
  @retval EFI_DEVICE_ERROR The user typed in an ESC.
  @retval EFI_INVALID_PARAMETER One of the parameters was invalid.(StringBuffer == NULL && HotKey == FALSE).
**/
EFI_STATUS
EFIAPI 
ThunkCreatePopUp (
  IN  UINTN                           LinesNumber,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *Key,
  IN  CHAR16                          *FirstString,
  ...
  )
{
  VA_LIST                          Args;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SavedConsoleMode;
  UINTN                            Columns;
  UINTN                            Rows;
  UINTN                            Column;
  UINTN                            Row;
  UINTN                            NumberOfLines;
  UINTN                            MaxLength;
  CHAR16                           *String;
  UINTN                            Length;
  CHAR16                           *Line;
  UINTN                            EventIndex;

  if (!HotKey) {
    return EFI_UNSUPPORTED;
  }
  
  if (MaximumStringSize == 0) {
    //
    // Blank strint to output
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the length of the longest line in the popup and the the total 
  // number of lines in the popup
  //
  MaxLength = StrLen (FirstString);
  NumberOfLines = 1;
  VA_START (Args, FirstString);
  while ((String = VA_ARG (Args, CHAR16 *)) != NULL) {
    MaxLength = MAX (MaxLength, StrLen (String));
    NumberOfLines++;
  }
  VA_END (Args);

  //
  // If the total number of lines in the popup is not same to the input NumberOfLines
  // the parameter is not valid. Not check.
  //
  //  if (NumberOfLines != LinesNumber) {
  //    return EFI_INVALID_PARAMETER;
  //  }

  //
  // If the maximum length of all the strings is not same to the input MaximumStringSize
  // the parameter is not valid. Not check.
  //
  // if (MaxLength != MaximumStringSize) {
  //   return EFI_INVALID_PARAMETER;
  // }

  //
  // Cache a pointer to the Simple Text Output Protocol in the EFI System Table
  //
  ConOut = gST->ConOut;
  
  //
  // Save the current console cursor position and attributes
  //
  CopyMem (&SavedConsoleMode, ConOut->Mode, sizeof (SavedConsoleMode));

  //
  // Retrieve the number of columns and rows in the current console mode
  //
  ConOut->QueryMode (ConOut, SavedConsoleMode.Mode, &Columns, &Rows);

  //
  // Disable cursor and set the foreground and background colors specified by Attribute
  //
  ConOut->EnableCursor (ConOut, FALSE);
  ConOut->SetAttribute (ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

  //
  // Limit NumberOfLines to height of the screen minus 3 rows for the box itself
  //
  NumberOfLines = MIN (NumberOfLines, Rows - 3);

  //
  // Limit MaxLength to width of the screen minus 2 columns for the box itself
  //
  MaxLength = MIN (MaxLength, Columns - 2);

  //
  // Compute the starting row and starting column for the popup
  //
  Row    = (Rows - (NumberOfLines + 3)) / 2;
  Column = (Columns - (MaxLength + 2)) / 2;

  //
  // Allocate a buffer for a single line of the popup with borders and a Null-terminator
  //
  Line = AllocateZeroPool ((MaxLength + 3) * sizeof (CHAR16));
  ASSERT (Line != NULL);

  //
  // Draw top of popup box   
  //
  SetMem16 (Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_DOWN_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_DOWN_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition (ConOut, Column, Row++);
  ConOut->OutputString (ConOut, Line);

  //
  // Draw middle of the popup with strings
  //
  VA_START (Args, FirstString);
  String = FirstString;
  while ((String != NULL) && (NumberOfLines > 0)) {
    Length = StrLen (String);
    SetMem16 (Line, (MaxLength + 2) * 2, L' ');
    if (Length <= MaxLength) {
      //
      // Length <= MaxLength
      //
      CopyMem (Line + 1 + (MaxLength - Length) / 2, String , Length * sizeof (CHAR16));
    } else {
      //
      // Length > MaxLength
      //
      CopyMem (Line + 1, String + (Length - MaxLength) / 2 , MaxLength * sizeof (CHAR16));
    }
    Line[0]             = BOXDRAW_VERTICAL;
    Line[MaxLength + 1] = BOXDRAW_VERTICAL;
    Line[MaxLength + 2] = L'\0';
    ConOut->SetCursorPosition (ConOut, Column, Row++);
    ConOut->OutputString (ConOut, Line);
    String = VA_ARG (Args, CHAR16 *);
    NumberOfLines--;
  }
  VA_END (Args);

  //
  // Draw bottom of popup box
  //
  SetMem16 (Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
  Line[0]             = BOXDRAW_UP_RIGHT;
  Line[MaxLength + 1] = BOXDRAW_UP_LEFT;
  Line[MaxLength + 2] = L'\0';
  ConOut->SetCursorPosition (ConOut, Column, Row++);
  ConOut->OutputString (ConOut, Line);

  //
  // Free the allocated line buffer
  //
  FreePool (Line);

  //
  // Restore the cursor visibility, position, and attributes
  //
  ConOut->EnableCursor      (ConOut, SavedConsoleMode.CursorVisible);
  ConOut->SetCursorPosition (ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
  ConOut->SetAttribute      (ConOut, SavedConsoleMode.Attribute);

  //
  // Wait for a keystroke
  //
  if (Key != NULL) {
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
  }
  
  return EFI_SUCCESS;
}

/** 

  Initialize string packages in HII database.

**/
VOID
InitSetBrowserStrings (
  VOID
  )
{
  //
  // Initialize strings to HII database
  //
  gStringPackHandle = HiiAddPackages (
                        &gEfiHiiThunkProducerGuid,
                        NULL,
                        STRING_ARRAY_NAME,
                        NULL
                        );
  ASSERT (gStringPackHandle != NULL);
}
