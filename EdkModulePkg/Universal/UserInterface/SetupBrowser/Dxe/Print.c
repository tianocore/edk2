/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Print.c

Abstract:

  Basic Ascii AvSPrintf() function named VSPrint(). VSPrint() enables very
  simple implemenation of SPrint() and Print() to support debug.

  You can not Print more than EFI_DRIVER_LIB_MAX_PRINT_BUFFER characters at a
  time. This makes the implementation very simple.

  VSPrint, Print, SPrint format specification has the follwoing form

  %type

  type:
    'S','s' - argument is an Unicode string
    'c' - argument is an ascii character
    '%' - Print a %

--*/

#include "Print.h"

STATIC
UINTN
_IPrint (
  IN UINTN                            Column,
  IN UINTN                            Row,
  IN EFI_SIMPLE_TEXT_OUT_PROTOCOL     *Out,
  IN CHAR16                           *fmt,
  IN VA_LIST                          args
  )
//
// Display string worker for: Print, PrintAt, IPrint, IPrintAt
//
{
  CHAR16  *Buffer;
  CHAR16  *BackupBuffer;
  UINTN   Index;
  UINTN   PreviousIndex;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer        = AllocateZeroPool (0x10000);
  BackupBuffer  = AllocateZeroPool (0x10000);
  ASSERT (Buffer);
  ASSERT (BackupBuffer);

  if (Column != (UINTN) -1) {
    Out->SetCursorPosition (Out, Column, Row);
  }

  UnicodeVSPrint (Buffer, 0x10000, fmt, args);

  Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;

  Out->SetAttribute (Out, Out->Mode->Attribute);

  Index         = 0;
  PreviousIndex = 0;

  do {
    for (; (Buffer[Index] != NARROW_CHAR) && (Buffer[Index] != WIDE_CHAR) && (Buffer[Index] != 0); Index++) {
      BackupBuffer[Index] = Buffer[Index];
    }

    if (Buffer[Index] == 0) {
      break;
    }
    //
    // Null-terminate the temporary string
    //
    BackupBuffer[Index] = 0;

    //
    // Print this out, we are about to switch widths
    //
    Out->OutputString (Out, &BackupBuffer[PreviousIndex]);

    //
    // Preserve the current index + 1, since this is where we will start printing from next
    //
    PreviousIndex = Index + 1;

    //
    // We are at a narrow or wide character directive.  Set attributes and strip it and print it
    //
    if (Buffer[Index] == NARROW_CHAR) {
      //
      // Preserve bits 0 - 6 and zero out the rest
      //
      Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;
      Out->SetAttribute (Out, Out->Mode->Attribute);
    } else {
      //
      // Must be wide, set bit 7 ON
      //
      Out->Mode->Attribute = Out->Mode->Attribute | EFI_WIDE_ATTRIBUTE;
      Out->SetAttribute (Out, Out->Mode->Attribute);
    }

    Index++;

  } while (Buffer[Index] != 0);

  //
  // We hit the end of the string - print it
  //
  Out->OutputString (Out, &BackupBuffer[PreviousIndex]);

  FreePool (Buffer);
  FreePool (BackupBuffer);
  return EFI_SUCCESS;
}

UINTN
Print (
  IN CHAR16   *fmt,
  ...
  )
/*++

Routine Description:

    Prints a formatted unicode string to the default console

Arguments:

    fmt         - Format string

Returns:

    Length of string printed to the console

--*/
{
  VA_LIST args;

  VA_START (args, fmt);
  return _IPrint ((UINTN) -1, (UINTN) -1, gST->ConOut, fmt, args);
}

UINTN
PrintString (
  CHAR16       *String
  )
/*++

Routine Description:

  Prints a unicode string to the default console,
  using L"%s" format.

Arguments:

  String      - String pointer.

Returns:

  Length of string printed to the console

--*/
{
  return Print ((CHAR16 *) L"%s", String);
}

UINTN
PrintChar (
  CHAR16       Character
  )
/*++

Routine Description:

  Prints a chracter to the default console,
  using L"%c" format.

Arguments:

  Character   - Character to print.

Returns:

  Length of string printed to the console.

--*/
{
  return Print ((CHAR16 *) L"%c", Character);
}

UINTN
PrintAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *fmt,
  ...
  )
/*++

Routine Description:

  Prints a formatted unicode string to the default console, at
  the supplied cursor position

Arguments:

  Column, Row - The cursor position to print the string at

  fmt         - Format string

Returns:

  Length of string printed to the console

--*/
{
  VA_LIST args;

  VA_START (args, fmt);
  return _IPrint (Column, Row, gST->ConOut, fmt, args);
}

UINTN
PrintStringAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       *String
  )
/*++

Routine Description:

  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

Arguments:

  Column, Row - The cursor position to print the string at

  String      - String pointer.

Returns:

  Length of string printed to the console

--*/
{
  return PrintAt (Column, Row, (CHAR16 *) L"%s", String);
}

UINTN
PrintCharAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       Character
  )
/*++

Routine Description:

  Prints a chracter to the default console, at
  the supplied cursor position, using L"%c" format.

Arguments:

  Column, Row - The cursor position to print the string at

  Character   - Character to print.

Returns:

  Length of string printed to the console.

--*/
{
  return PrintAt (Column, Row, (CHAR16 *) L"%c", Character);
}


