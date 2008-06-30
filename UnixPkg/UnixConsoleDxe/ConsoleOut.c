/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ConsoleOut.c

Abstract:

  Console based on Posix APIs. 

  This file creates an Posix window and attaches a SimpleTextOut protocol.

--*/

#include "Console.h"
//
// Private worker functions.
//

#if 0
STATIC
VOID
UnixSimpleTextOutScrollScreen (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Console
  );

#endif
STATIC
VOID
UnixSimpleTextOutPutChar (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA     *Console,
  IN      CHAR16                              Char
  );

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutSetAttribute (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *This,
  IN UINTN                          Attribute
  );
STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutSetMode (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL           *This,
  IN UINTN                                  ModeNumber
  );

//
// Modeule Global for Simple Text Out Mode.
//
#define MAX_SIMPLE_TEXT_OUT_MODE  \
        (sizeof(mUnixSimpleTextOutSupportedModes)/sizeof(UNIX_SIMPLE_TEXT_OUT_MODE))

STATIC UNIX_SIMPLE_TEXT_OUT_MODE  mUnixSimpleTextOutSupportedModes[] = {
  { 80, 25 },         
#if 0
  { 80, 50 },         
  { 80, 43 },         
  { 100, 100 },       
  { 100, 999 }         
#endif
};

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutReset (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  UnixSimpleTextOutSetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK));

  UnixSimpleTextOutSetMode (This, 0);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutOutputString (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;
  CHAR16                          *Str;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  for (Str = String; *Str != '\0'; Str++) {
    switch (*Str) {
#if 0
    case '\n':
      if (Private->Position.Y == (Private->MaxScreenSize.Y - 1)) {
        UnixSimpleTextOutScrollScreen (Private);
      }

      if (Private->Position.Y < (Private->MaxScreenSize.Y - 1)) {
        Private->Position.Y++;
        This->Mode->CursorRow++;
      }
      break;

    case '\r':
      Private->Position.X      = 0;
      This->Mode->CursorColumn  = 0;
      break;

    case '\b':
      if (Private->Position.X > 0) {
        Private->Position.X--;
        This->Mode->CursorColumn--;
      }
      break;

#endif
    default:
      UnixSimpleTextOutPutChar (Private, *Str);
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
UnixSimpleTextOutPutChar (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA   *Console,
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
  char c = Char;
  Console->UnixThunk->Write (1, &c, 1);
}

#if 0
STATIC
VOID
UnixSimpleTextOutScrollScreen (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Console
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
}
#endif


STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutTestString (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  //
  // BugBug: The correct answer would be a function of what code pages
  //         are currently loaded? For now we will just return success.
  //
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutQueryMode (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber > MAX_SIMPLE_TEXT_OUT_MODE) {
    return EFI_INVALID_PARAMETER;
  }

  *Columns  = mUnixSimpleTextOutSupportedModes[ModeNumber].ColumnsX;
  *Rows     = mUnixSimpleTextOutSupportedModes[ModeNumber].RowsY;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutSetMode (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber > MAX_SIMPLE_TEXT_OUT_MODE) {
    return EFI_INVALID_PARAMETER;
  }


  This->Mode->Mode = (INT32) ModeNumber;

  This->EnableCursor (This, TRUE);
  This->ClearScreen (This);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutSetAttribute (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private               = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

#if 0
  Private->Attribute    = (WORD) Attribute;
#endif
  This->Mode->Attribute = (INT32) Attribute;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutClearScreen (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;
  //  DWORD                           ConsoleWindow;

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

  This->SetCursorPosition (This, 0, 0);
  Private->UnixThunk->Write (1, "\e[2J", 4);


#if 0
  Private->UnixThunk->FillConsoleOutputCharacter (
                        Private->NtOutHandle,
                        ' ',
                        Private->MaxScreenSize.X * Private->MaxScreenSize.Y,
                        Private->Possition,
                        &ConsoleWindow
                        );
  Private->UnixThunk->FillConsoleOutputAttribute (
                        Private->NtOutHandle,
                        Private->Attribute,
                        Private->MaxScreenSize.X * Private->MaxScreenSize.Y,
                        Private->Possition,
                        &ConsoleWindow
                        );
#endif

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutSetCursorPosition (
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
  char buf[12];
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  Private                   = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);

#if 0
  Private->Position.X      = (WORD) Column;
#endif
  This->Mode->CursorColumn  = (INT32) Column;

#if 0
  Private->Position.Y      = (WORD) Row;
#endif
  This->Mode->CursorRow     = (INT32) Row;
#if 0
  Private->UnixThunk->SetConsoleCursorPosition (Private->NtOutHandle, Private->Possition);
#endif

  buf[0] = '\e';
  buf[1] = '[';
  buf[2] = '0' + ((Row / 100) % 10);
  buf[3] = '0' + ((Row / 10) % 10);
  buf[4] = '0' + ((Row / 1) % 10);
  buf[5] = ';';
  buf[6] = '0' + ((Column / 100) % 10);
  buf[7] = '0' + ((Column / 10) % 10);
  buf[8] = '0' + ((Column / 1) % 10);
  buf[9] = 'H';
  Private->UnixThunk->Write (1, buf, 10);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
UnixSimpleTextOutEnableCursor (
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
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;
#if 0
  CONSOLE_CURSOR_INFO             Info;
#endif

  Private                   = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (This);
  Private->CursorEnable     = Enable;
  This->Mode->CursorVisible = Enable;

#if 0
  Private->UnixThunk->GetConsoleCursorInfo (Private->NtOutHandle, &Info);
  Info.bVisible = Enable;
  Private->UnixThunk->SetConsoleCursorInfo (Private->NtOutHandle, &Info);
#endif

  return EFI_SUCCESS;
}

EFI_STATUS
UnixSimpleTextOutOpenWindow (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private
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

  //WindowName          = Private->UnixIo->EnvString;
#if 0
  Private->Attribute  = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
  if (*WindowName == '?') {
    Private->Attribute  = BACKGROUND_RED | FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN;
    WindowName          = L"EFI Emulator Error Console";
  }
#endif
    WindowName          = L"EFI Emulator Error Console";

  AddUnicodeString (
    "eng",
    gUnixConsoleComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WindowName
    );

  //
  // Fill in protocol member functions
  //
  SimpleTextOut                     = &Private->SimpleTextOut;
  SimpleTextOut->Reset              = UnixSimpleTextOutReset;
  SimpleTextOut->OutputString       = UnixSimpleTextOutOutputString;
  SimpleTextOut->TestString         = UnixSimpleTextOutTestString;
  SimpleTextOut->QueryMode          = UnixSimpleTextOutQueryMode;
  SimpleTextOut->SetMode            = UnixSimpleTextOutSetMode;
  SimpleTextOut->SetAttribute       = UnixSimpleTextOutSetAttribute;
  SimpleTextOut->ClearScreen        = UnixSimpleTextOutClearScreen;
  SimpleTextOut->SetCursorPosition  = UnixSimpleTextOutSetCursorPosition;
  SimpleTextOut->EnableCursor       = UnixSimpleTextOutEnableCursor;

  //
  // Initialize SimpleTextOut protocol mode structure
  //
  SimpleTextOut->Mode             = &Private->SimpleTextOutMode;
  SimpleTextOut->Mode->MaxMode    = MAX_SIMPLE_TEXT_OUT_MODE;
  SimpleTextOut->Mode->Attribute  = 0; //FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

#if 0
  //
  // Open the window an initialize it!
  //
  Private->NtOutHandle = Private->UnixThunk->CreateConsoleScreenBuffer (
                                                GENERIC_WRITE | GENERIC_READ,
                                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                                NULL,
                                                CONSOLE_TEXTMODE_BUFFER,
                                                NULL
                                                );
  Private->UnixThunk->SetConsoleTitle (WindowName);
#endif

  return SimpleTextOut->SetMode (SimpleTextOut, 0);
}

EFI_STATUS
UnixSimpleTextOutCloseWindow (
  IN OUT  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Console
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
#if 0
  Console->UnixThunk->CloseHandle (Console->NtOutHandle);
#endif
  return EFI_SUCCESS;
}
