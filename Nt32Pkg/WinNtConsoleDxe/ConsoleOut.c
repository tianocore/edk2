/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ConsoleOut.c

Abstract:

  Console based on Win32 APIs. 

  This file creates an Win32 window and attaches a SimpleTextOut protocol.

--*/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/SimpleTextIn.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>


#include "Console.h"
//
// Private worker functions.
//

STATIC
VOID
WinNtSimpleTextOutScrollScreen (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Console
  );

STATIC
VOID
WinNtSimpleTextOutPutChar (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA     *Console,
  IN      CHAR16                              Char
  );

//
// Modeule Global for Simple Text Out Mode.
//
#define MAX_SIMPLE_TEXT_OUT_MODE  \
        (sizeof(mWinNtSimpleTextOutSupportedModes)/sizeof(WIN_NT_SIMPLE_TEXT_OUT_MODE))

STATIC WIN_NT_SIMPLE_TEXT_OUT_MODE  mWinNtSimpleTextOutSupportedModes[] = {
  { 80, 25 },         
  { 80, 50 },         
  { 80, 43 },         
  { 100, 100 },       
  { 100, 999 }         
};

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutReset (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL         *This,
  IN BOOLEAN                              ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  WinNtSimpleTextOutSetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK));

  WinNtSimpleTextOutSetMode (This, 0);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN CHAR16                                 *String
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  String  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;
  CHAR16                          *Str;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  for (Str = String; *Str != '\0'; Str++) {
    switch (*Str) {
    case '\n':
      if (Private->Possition.Y == (Private->MaxScreenSize.Y - 1)) {
        WinNtSimpleTextOutScrollScreen (Private);
      }

      if (Private->Possition.Y < (Private->MaxScreenSize.Y - 1)) {
        Private->Possition.Y++;
        This->Mode->CursorRow++;
      }
      break;

    case '\r':
      Private->Possition.X      = 0;
      This->Mode->CursorColumn  = 0;
      break;

    case '\b':
      if (Private->Possition.X > 0) {
        Private->Possition.X--;
        This->Mode->CursorColumn--;
      }
      break;

    default:
      WinNtSimpleTextOutPutChar (Private, *Str);
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
WinNtSimpleTextOutPutChar (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA   *Console,
  IN      CHAR16                            Char
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Console - TODO: add argument description
  Char    - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  SMALL_RECT  Region;
  COORD       StrCoordinate;
  COORD       StrSize;
  CHAR_INFO   CharInfo;
  BOOL        Flag;

  CharInfo.Char.UnicodeChar = Char;
  CharInfo.Attributes       = Console->Attribute;

  StrSize.X                 = 1;
  StrSize.Y                 = 1;
  StrCoordinate.X           = 0;
  StrCoordinate.Y           = 0;

  Region.Left               = (INT16) Console->Possition.X;
  Region.Top                = (INT16) Console->Possition.Y;
  Region.Right              = (INT16) (Console->Possition.X + 1);
  Region.Bottom             = (INT16) Console->Possition.Y;

  Console->WinNtThunk->WriteConsoleOutput (
                        Console->NtOutHandle,
                        &CharInfo,
                        StrSize,
                        StrCoordinate,
                        &Region
                        );

  if (Console->Possition.X >= (Console->MaxScreenSize.X - 1)) {
    //
    // If you print off the end wrap around
    //
    Console->SimpleTextOut.OutputString (&Console->SimpleTextOut, L"\n\r");
  } else {
    Console->Possition.X++;
    Console->SimpleTextOut.Mode->CursorColumn++;
  }

  Flag = Console->WinNtThunk->SetConsoleCursorPosition (Console->NtOutHandle, Console->Possition);
}

STATIC
VOID
WinNtSimpleTextOutScrollScreen (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Console
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Console - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  SMALL_RECT  Scroll;
  CHAR_INFO   CharInfo;
  COORD       Origin;

  CharInfo.Char.UnicodeChar = ' ';
  CharInfo.Attributes       = Console->Attribute;

  Origin.X                  = 0;
  Origin.Y                  = 0;

  Scroll.Top                = 1;
  Scroll.Left               = 0;
  Scroll.Right              = (INT16) Console->MaxScreenSize.X;
  Scroll.Bottom             = (INT16) Console->MaxScreenSize.Y;

  Console->WinNtThunk->ScrollConsoleScreenBuffer (
                        Console->NtOutHandle,
                        &Scroll,
                        NULL,
                        Origin,
                        &CharInfo
                        );
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutTestString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN CHAR16                                 *String
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  String  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  //
  // BugBug: The correct answer would be a function of what code pages
  //         are currently loaded? For now we will just return success.
  //
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutQueryMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN UINTN                                  ModeNumber,
  OUT UINTN                                 *Columns,
  OUT UINTN                                 *Rows
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  ModeNumber  - TODO: add argument description
  Columns     - TODO: add argument description
  Rows        - TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber > MAX_SIMPLE_TEXT_OUT_MODE) {
    return EFI_INVALID_PARAMETER;
  }

  *Columns  = mWinNtSimpleTextOutSupportedModes[ModeNumber].ColumnsX;
  *Rows     = mWinNtSimpleTextOutSupportedModes[ModeNumber].RowsY;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN UINTN                                  ModeNumber
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  ModeNumber  - TODO: add argument description

Returns:

  EFI_INVALID_PARAMETER - TODO: Add description for return value
  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber > MAX_SIMPLE_TEXT_OUT_MODE) {
    return EFI_INVALID_PARAMETER;
  }

  Private->MaxScreenSize.X  = (WORD) mWinNtSimpleTextOutSupportedModes[ModeNumber].ColumnsX;
  Private->MaxScreenSize.Y  = (WORD) mWinNtSimpleTextOutSupportedModes[ModeNumber].RowsY;

  Private->WinNtThunk->SetConsoleScreenBufferSize (Private->NtOutHandle, Private->MaxScreenSize);
  Private->WinNtThunk->SetConsoleActiveScreenBuffer (Private->NtOutHandle);

  This->Mode->Mode = (INT32) ModeNumber;

  This->EnableCursor (This, TRUE);
  This->ClearScreen (This);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          Attribute
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This      - TODO: add argument description
  Attribute - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private               = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  Private->Attribute    = (WORD) Attribute;
  This->Mode->Attribute = (INT32) Attribute;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutClearScreen (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;
  DWORD                           ConsoleWindow;

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  This->SetCursorPosition (This, 0, 0);

  Private->WinNtThunk->FillConsoleOutputCharacter (
                        Private->NtOutHandle,
                        ' ',
                        Private->MaxScreenSize.X * Private->MaxScreenSize.Y,
                        Private->Possition,
                        &ConsoleWindow
                        );
  Private->WinNtThunk->FillConsoleOutputAttribute (
                        Private->NtOutHandle,
                        Private->Attribute,
                        Private->MaxScreenSize.X * Private->MaxScreenSize.Y,
                        Private->Possition,
                        &ConsoleWindow
                        );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutSetCursorPosition (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN UINTN                                  Column,
  IN UINTN                                  Row
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Column  - TODO: add argument description
  Row     - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private                   = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  Private->Possition.X      = (WORD) Column;
  This->Mode->CursorColumn  = (INT32) Column;

  Private->Possition.Y      = (WORD) Row;
  This->Mode->CursorRow     = (INT32) Row;
  Private->WinNtThunk->SetConsoleCursorPosition (Private->NtOutHandle, Private->Possition);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
WinNtSimpleTextOutEnableCursor (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN BOOLEAN                        Enable
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This    - TODO: add argument description
  Enable  - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;
  CONSOLE_CURSOR_INFO             Info;

  Private                   = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);
  Private->CursorEnable     = Enable;
  This->Mode->CursorVisible = Enable;

  Private->WinNtThunk->GetConsoleCursorInfo (Private->NtOutHandle, &Info);
  Info.bVisible = Enable;
  Private->WinNtThunk->SetConsoleCursorInfo (Private->NtOutHandle, &Info);

  return EFI_SUCCESS;
}

EFI_STATUS
WinNtSimpleTextOutOpenWindow (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Private - TODO: add argument description

Returns:

  TODO: add return values

--*/
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOut;
  CHAR16                        *WindowName;

  WindowName          = Private->WinNtIo->EnvString;
  Private->Attribute  = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
  if (*WindowName == '?') {
    Private->Attribute  = BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
    WindowName          = L"EFI Emulator Error Console";
  }

  AddUnicodeString2 (
    "eng",
    gWinNtConsoleComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WindowName,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gWinNtConsoleComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    WindowName,
    FALSE
    );


  //
  // Fill in protocol member functions
  //
  SimpleTextOut                     = &Private->SimpleTextOut;
  SimpleTextOut->Reset              = WinNtSimpleTextOutReset;
  SimpleTextOut->OutputString       = WinNtSimpleTextOutOutputString;
  SimpleTextOut->TestString         = WinNtSimpleTextOutTestString;
  SimpleTextOut->QueryMode          = WinNtSimpleTextOutQueryMode;
  SimpleTextOut->SetMode            = WinNtSimpleTextOutSetMode;
  SimpleTextOut->SetAttribute       = WinNtSimpleTextOutSetAttribute;
  SimpleTextOut->ClearScreen        = WinNtSimpleTextOutClearScreen;
  SimpleTextOut->SetCursorPosition  = WinNtSimpleTextOutSetCursorPosition;
  SimpleTextOut->EnableCursor       = WinNtSimpleTextOutEnableCursor;

  //
  // Initialize SimpleTextOut protocol mode structure
  //
  SimpleTextOut->Mode             = &Private->SimpleTextOutMode;
  SimpleTextOut->Mode->MaxMode    = MAX_SIMPLE_TEXT_OUT_MODE;
  SimpleTextOut->Mode->Attribute  = (INT32) Private->Attribute;

  //
  // Open the window an initialize it!
  //
  Private->NtOutHandle = Private->WinNtThunk->CreateConsoleScreenBuffer (
                                                GENERIC_WRITE | GENERIC_READ,
                                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                NULL,
                                                CONSOLE_TEXTMODE_BUFFER,
                                                NULL
                                                );
  Private->WinNtThunk->SetConsoleTitle (WindowName);

  return SimpleTextOut->SetMode (SimpleTextOut, 0);
}

EFI_STATUS
WinNtSimpleTextOutCloseWindow (
  IN OUT  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Console
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Console - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  Console->WinNtThunk->CloseHandle (Console->NtOutHandle);
  return EFI_SUCCESS;
}
