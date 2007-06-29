/** @file
  Mde UEFI library API implemention.
  Print to StdErr or ConOut defined in EFI_SYSTEM_TABLE

  Copyright (c) 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//
#include "UefiLibInternal.h"

/**
  Internal function which prints a formatted Unicode string to the console output device
  specified by Console

  This function prints a formatted Unicode string to the console output device
  specified by Console and returns the number of Unicode characters that printed
  to it.  If the length of the formatted Unicode string is greater than PcdUefiLibMaxPrintBufferSize,
  then only the first PcdUefiLibMaxPrintBufferSize characters are sent to Console.

  @param Format   Null-terminated Unicode format string.
  @param Console  The output console.
  @param Marker   VA_LIST marker for the variable argument list.

  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

**/

STATIC
UINTN
InternalPrint (
  IN  CONST CHAR16                     *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  UINTN   Return;
  CHAR16  *Buffer;
  UINTN   BufferSize;

  ASSERT (Format != NULL);
  ASSERT (((UINTN) Format & 0x01) == 0);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrint (Buffer, BufferSize, Format, Marker);

  if (Console != NULL) {
    //
    // To be extra safe make sure Console has been initialized
    //
    Console->OutputString (Console, Buffer);
  }

  FreePool (Buffer);

  return Return;
}

/**
  Prints a formatted Unicode string to the console output device specified by
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of Unicode
  characters that printed to ConOut.  If the length of the formatted Unicode
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.

  @param Format   Null-terminated Unicode format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

**/
UINTN
EFIAPI
Print (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = InternalPrint (Format, gST->ConOut, Marker);

  VA_END (Marker);

  return Return;
}

/**
  Prints a formatted Unicode string to the console output device specified by
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of Unicode
  characters that printed to StdErr.  If the length of the formatted Unicode
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.

  @param Format   Null-terminated Unicode format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

**/

UINTN
EFIAPI
ErrorPrint (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = InternalPrint( Format, gST->StdErr, Marker);

  VA_END (Marker);

  return Return;
}


/**
  Internal function which prints a formatted ASCII string to the console output device
  specified by Console

  This function prints a formatted ASCII string to the console output device
  specified by Console and returns the number of ASCII characters that printed
  to it.  If the length of the formatted ASCII string is greater than PcdUefiLibMaxPrintBufferSize,
  then only the first PcdUefiLibMaxPrintBufferSize characters are sent to Console.

  @param Format   Null-terminated ASCII format string.
  @param Console  The output console.
  @param Marker   VA_LIST marker for the variable argument list.

  If Format is NULL, then ASSERT().

**/

STATIC
UINTN
AsciiInternalPrint (
  IN  CONST CHAR8                      *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  UINTN   Return;
  CHAR16  *Buffer;
  UINTN   BufferSize;

  ASSERT (Format != NULL);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrintAsciiFormat (Buffer, BufferSize, Format, Marker);

  if (Console != NULL) {
    //
    // To be extra safe make sure Console has been initialized
    //
    Console->OutputString (Console, Buffer);
  }

  FreePool (Buffer);

  return Return;
}

/**
  Prints a formatted ASCII string to the console output device specified by
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of ASCII
  characters that printed to ConOut.  If the length of the formatted ASCII
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.

  @param Format   Null-terminated ASCII format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

**/
UINTN
EFIAPI
AsciiPrint (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = AsciiInternalPrint( Format, gST->ConOut, Marker);

  VA_END (Marker);

  return Return;
}

/**
  Prints a formatted ASCII string to the console output device specified by
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of ASCII
  characters that printed to StdErr.  If the length of the formatted ASCII
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.

  @param Format   Null-terminated ASCII format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

**/
UINTN
EFIAPI
AsciiErrorPrint (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = AsciiInternalPrint( Format, gST->StdErr, Marker);

  VA_END (Marker);

  return Return;
}

