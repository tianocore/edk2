/** @file
  Formatting functions used in parser definitions

  Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FIELD_FORMAT_HELPER_H_
#define FIELD_FORMAT_HELPER_H_

#include <Library/UefiLib.h>
#include <Uefi.h>

/**
  This function traces 1 byte of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
DumpUint8 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *Ptr);
}

/**
  This function traces 2 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
DumpUint16 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *(UINT16*)Ptr);
}

/**
  This function traces 4 bytes of data as specified in the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
DumpUint32 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  Print (Format, *(UINT32*)Ptr);
}

/**
  This function traces 8 bytes of data as specified by the format string.

  @param [in] Format  The format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
DumpUint64 (
  IN CONST CHAR16* Format,
  IN UINT8*        Ptr
  )
{
  // Some fields are not aligned and this causes alignment faults
  // on ARM platforms if the compiler generates LDRD instructions.
  // Perform word access so that LDRD instructions are not generated.
  UINT64 Val;

  Val = *(UINT32*)(Ptr + sizeof (UINT32));

  Val = LShiftU64(Val,32);
  Val |= (UINT64)*(UINT32*)Ptr;

  Print (Format, Val);
}

/**
  This function traces 3 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
Dump3Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  Print (
    (Format != NULL) ? Format : (CONST CHAR16*) L"%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2]
    );
}

/**
  This function traces 4 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
Dump4Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  Print (
    (Format != NULL) ? Format : (CONST CHAR16*) L"%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3]
    );
}

/**
  This function traces 6 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
Dump6Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  Print (
    (Format != NULL) ? Format : (CONST CHAR16*) L"%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5]
    );
}

/**
  This function traces 8 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
Dump8Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN UINT8*        Ptr
  )
{
  Print (
    (Format != NULL) ? Format : (CONST CHAR16*) L"%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7]
    );
}

/**
  This function traces 12 characters which can be optionally
  formated using the format string if specified.

  If no format string is specified the Format must be NULL.

  @param [in] Format  Optional format string for tracing the data.
  @param [in] Ptr     Pointer to the start of the buffer.
**/
static
inline
VOID
EFIAPI
Dump12Chars (
  IN CONST CHAR16* Format OPTIONAL,
  IN       UINT8*  Ptr
  )
{
  Print (
    (Format != NULL) ? Format : (CONST CHAR16*) L"%c%c%c%c%c%c%c%c%c%c%c%c",
    Ptr[0],
    Ptr[1],
    Ptr[2],
    Ptr[3],
    Ptr[4],
    Ptr[5],
    Ptr[6],
    Ptr[7],
    Ptr[8],
    Ptr[9],
    Ptr[10],
    Ptr[11]
    );
}

#endif
