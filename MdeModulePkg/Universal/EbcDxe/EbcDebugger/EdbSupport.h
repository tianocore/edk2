/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbSupport.h

Abstract:


--*/

#ifndef _EFI_EDB_SUPPORT_H_
#define _EFI_EDB_SUPPORT_H_

#include <Uefi.h>

#define EFI_DEBUG_PROMPT_STRING      L"EDB > "
#define EFI_DEBUG_PROMPT_COLUMN      5
#define EFI_DEBUG_INPUS_BUFFER_SIZE  64

#define EFI_DEBUGGER_LINE_NUMBER_IN_PAGE  0x10

#define EFI_DEBUG_MAX_PRINT_BUFFER   (80 * 4)

UINTN
EFIAPI
Xtoi (
  CHAR16  *str
  );

UINT64
EFIAPI
LXtoi (
  CHAR16  *str
  );

UINTN
EFIAPI
Atoi (
  CHAR16  *str
  );

UINTN
EFIAPI
AsciiXtoi (
  CHAR8  *str
  );

UINTN
EFIAPI
AsciiAtoi (
  CHAR8  *str
  );

INTN
EFIAPI
StrCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  );

INTN
EFIAPI
StriCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2
  );

INTN
EFIAPI
StriCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  );

BOOLEAN
EFIAPI
StrEndWith (
  IN CHAR16                       *Str,
  IN CHAR16                       *SubStr
  );

CHAR16 *
EFIAPI
StrDuplicate (
  IN CHAR16   *Src
  );

CHAR16 *
EFIAPI
StrGetNewTokenLine (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  );

CHAR16 *
EFIAPI
StrGetNextTokenLine (
  IN CHAR16                       *CharSet
  );

CHAR16 *
EFIAPI
StrGetNewTokenField (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  );

CHAR16 *
EFIAPI
StrGetNextTokenField (
  IN CHAR16                       *CharSet
  );

VOID
EFIAPI
PatchForStrTokenAfter (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  );

VOID
EFIAPI
PatchForStrTokenBefore (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  );

CHAR8 *
EFIAPI
AsciiStrGetNewTokenLine (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  );

CHAR8 *
EFIAPI
AsciiStrGetNextTokenLine (
  IN CHAR8                       *CharSet
  );

CHAR8 *
EFIAPI
AsciiStrGetNewTokenField (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  );

CHAR8 *
EFIAPI
AsciiStrGetNextTokenField (
  IN CHAR8                       *CharSet
  );

VOID
EFIAPI
PatchForAsciiStrTokenAfter (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  );

VOID
EFIAPI
PatchForAsciiStrTokenBefore (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  );

//
// Shell Library
//
VOID
EFIAPI
Input (
  IN CHAR16    *Prompt OPTIONAL,
  OUT CHAR16   *InStr,
  IN UINTN     StrLen
  );

BOOLEAN
EFIAPI
SetPageBreak (
  VOID
  );

UINTN
EFIAPI
EDBPrint (
  IN CONST CHAR16  *Format,
  ...
  );

UINTN
EFIAPI
EDBSPrint (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  CONST CHAR16  *Format,
  ...
  );

UINTN
EFIAPI
EDBSPrintWithOffset (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  UINTN         Offset,
  IN  CONST CHAR16  *Format,
  ...
  );

EFI_STATUS
EFIAPI
ReadFileToBuffer (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *FileName,
  OUT UINTN                       *BufferSize,
  OUT VOID                        **Buffer,
  IN  BOOLEAN                     ScanFs
  );

CHAR16 *
EFIAPI
GetFileNameUnderDir (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *DirName,
  IN  CHAR16                      *FileName,
  IN OUT UINTN                    *Index
  );

#endif
