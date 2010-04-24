/** @file

  The lite print protocol defines only one print function to 
  print the format unicode string.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PPRINT_H__
#define __PPRINT_H__

#define EFI_PRINT_PROTOCOL_GUID  \
   { 0xdf2d868e, 0x32fc, 0x4cf0, {0x8e, 0x6b, 0xff, 0xd9, 0x5d, 0x13, 0x43, 0xd0} }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_PRINT_PROTOCOL  EFI_PRINT_PROTOCOL;

/**
  Produces a Null-terminated Unicode string in an output buffer, based on 
  a Null-terminated Unicode format string and a VA_LIST argument list.
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.  
  The Unicode string is produced by parsing the format string specified by FormatString.  
  Arguments are pulled from the variable argument list specified by Marker based on the 
  contents of the format string.  
  The number of Unicode characters in the produced output buffer is returned, not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced, and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If BufferSize > 1 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
typedef
UINTN
(EFIAPI *UNI_VSPRINT)(
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

/**
   EFI_PRINT_PROTOCOL provides one service to produce a Null-terminated Unicode string,
   based on a Null-terminated Unicode format string and a VA_LIST argument list, and fills into 
   the buffer as output.
**/
struct _EFI_PRINT_PROTOCOL {
  UNI_VSPRINT               VSPrint;
};

extern EFI_GUID gEfiPrintProtocolGuid;

#endif
