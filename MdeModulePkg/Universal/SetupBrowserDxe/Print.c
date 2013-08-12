/** @file
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


Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"

/**
  The internal function prints to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  protocol instance.

  @param Column          The position of the output string.
  @param Row             The position of the output string.
  @param Out             The EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param Fmt             The format string.
  @param Args            The additional argument for the variables in the format string.

  @return Number of Unicode character printed.

**/
UINTN
PrintInternal (
  IN UINTN                            Column,
  IN UINTN                            Row,
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Out,
  IN CHAR16                           *Fmt,
  IN VA_LIST                          Args
  )
{
  CHAR16  *Buffer;
  CHAR16  *BackupBuffer;
  UINTN   Index;
  UINTN   PreviousIndex;
  UINTN   Count;

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

  UnicodeVSPrint (Buffer, 0x10000, Fmt, Args);

  Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;

  Out->SetAttribute (Out, Out->Mode->Attribute);

  Index         = 0;
  PreviousIndex = 0;
  Count         = 0;

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
    Count += StrLen (&BackupBuffer[PreviousIndex]);

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
  Count += StrLen (&BackupBuffer[PreviousIndex]);

  FreePool (Buffer);
  FreePool (BackupBuffer);
  return Count;
}


/**
  Prints a formatted unicode string to the default console.

  @param  Fmt        Format string
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console.

**/
UINTN
EFIAPI
ConsolePrint (
  IN CHAR16   *Fmt,
  ...
  )
{
  VA_LIST Args;
  UINTN   LengthOfPrinted;

  VA_START (Args, Fmt);
  LengthOfPrinted = PrintInternal ((UINTN) -1, (UINTN) -1, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}


/**
  Prints a unicode string to the default console,
  using L"%s" format.

  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
PrintString (
  IN CHAR16       *String
  )
{
  return ConsolePrint (L"%s", String);
}


/**
  Prints a chracter to the default console,
  using L"%c" format.

  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
PrintChar (
  CHAR16       Character
  )
{
  return ConsolePrint (L"%c", Character);
}


/**
  Prints a formatted unicode string to the default console, at
  the supplied cursor position.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Fmt        Format string.
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintAt (
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *Fmt,
  ...
  )
{
  VA_LIST Args;
  UINTN   LengthOfPrinted;

  VA_START (Args, Fmt);
  LengthOfPrinted = PrintInternal (Column, Row, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}


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
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *String
  )
{
  return PrintAt (Column, Row, L"%s", String);
}


/**
  Prints a chracter to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
PrintCharAt (
  IN UINTN     Column,
  IN UINTN     Row,
  CHAR16       Character
  )
{
  return PrintAt (Column, Row, L"%c", Character);
}
