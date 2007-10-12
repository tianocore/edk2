/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Print.h

Abstract:

  This file defines the Print protocol

--*/

#ifndef __PPRINT_H__
#define __PPRINT_H__

#define EFI_PRINT_PROTOCOL_GUID  \
   { 0x5bcc3dbc, 0x8c57, 0x450a, { 0xbb, 0xc, 0xa1, 0xc0, 0xbd, 0xde, 0x48, 0xc } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_PRINT_PROTOCOL  EFI_PRINT_PROTOCOL;


typedef
UINTN
(EFIAPI *UNI_VSPRINT) (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

typedef
UINTN
(EFIAPI *UNI_VSPRINT_ASCII) (
  OUT CHAR16       *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  IN  VA_LIST      Marker
  );

typedef
UINTN
(EFIAPI *VALUE_TO_UNISTRING) (
  IN OUT CHAR16  *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width
  );

typedef
UINTN
(EFIAPI *ASCII_VSPRINT) (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  VA_LIST       Marker
  );

typedef
UINTN
(EFIAPI *ASCII_VSPRINT_UNI) (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

typedef
UINTN
(EFIAPI *VALUE_TO_ASCIISTRING) (
  IN OUT CHAR8  *Buffer,
  IN UINTN      Flags,
  IN INT64      Value,
  IN UINTN      Width
  );

struct _EFI_PRINT_PROTOCOL {
  UNI_VSPRINT               VSPrint;
  UNI_VSPRINT_ASCII         UniVSPrintAscii;
  VALUE_TO_UNISTRING        UniValueToString;                         
  ASCII_VSPRINT             AsciiVSPrint;          
  ASCII_VSPRINT_UNI         AsciiVSPrintUni;
  VALUE_TO_ASCIISTRING      AsciiValueToString;
};

extern EFI_GUID gEfiPrintProtocolGuid;

#endif
