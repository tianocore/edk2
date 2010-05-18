/** @file

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleFileParsing.h

Abstract:

  Function prototypes and defines for the simple file parsing routines.

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
