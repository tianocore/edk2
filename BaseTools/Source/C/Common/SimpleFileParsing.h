/** @file
Function prototypes and defines for the simple file parsing routines.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SIMPLE_FILE_PARSING_H_
#define _SIMPLE_FILE_PARSING_H_

#include <Common/UefiBaseTypes.h>

STATUS
SFPInit (
  VOID
  )
;

STATUS
SFPOpenFile (
  CHAR8    *FileName
  )
;

BOOLEAN
SFPIsKeyword (
  CHAR8 *Str
  )
;

BOOLEAN
SFPIsToken (
  CHAR8 *Str
  )
;

BOOLEAN
SFPGetNextToken (
  CHAR8  *Str,
  UINTN  Len
  )
;

BOOLEAN
SFPGetGuidToken (
  CHAR8 *Str,
  UINT32 Len
  )
;

#define PARSE_GUID_STYLE_5_FIELDS 0

BOOLEAN
SFPGetGuid (
  INTN        GuidStyle,
  EFI_GUID    *Value
  )
;

BOOLEAN
SFPSkipToToken (
  CHAR8 *Str
  )
;

BOOLEAN
SFPGetNumber (
  UINTN   *Value
  )
;

BOOLEAN
SFPGetQuotedString (
  CHAR8      *Str,
  INTN       Length
  )
;

BOOLEAN
SFPIsEOF (
  VOID
  )
;

STATUS
SFPCloseFile (
  VOID
  )
;

UINTN
SFPGetLineNumber (
  VOID
  )
;

CHAR8  *
SFPGetFileName (
  VOID
  )
;

#endif // #ifndef _SIMPLE_FILE_PARSING_H_
