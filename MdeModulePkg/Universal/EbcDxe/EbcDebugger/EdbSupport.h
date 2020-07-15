/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _EFI_EDB_SUPPORT_H_
#define _EFI_EDB_SUPPORT_H_

#include <Uefi.h>

#define EFI_DEBUG_PROMPT_STRING      L"EDB > "
#define EFI_DEBUG_PROMPT_COLUMN      5
#define EFI_DEBUG_INPUS_BUFFER_SIZE  64

#define EFI_DEBUGGER_LINE_NUMBER_IN_PAGE  0x10

#define EFI_DEBUG_MAX_PRINT_BUFFER   (80 * 4)

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINTN
EFIAPI
Xtoi (
  CHAR16  *Str
  );

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINT64
EFIAPI
LXtoi (
  CHAR16  *Str
  );

/**

  Convert hex string to uint.

  @param Str  -  The string

**/
UINTN
EFIAPI
Atoi (
  CHAR16  *Str
  );

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINTN
EFIAPI
AsciiXtoi (
  CHAR8  *Str
  );

/**

  Convert hex string to uint.

  @param Str  -  The string

**/
UINTN
EFIAPI
AsciiAtoi (
  CHAR8  *Str
  );

/**
  Compare the Unicode and Ascii string pointed by String to the string pointed by String2.

  @param String - Unicode String to process

  @param String2 - Ascii string to process

  @return Return a positive integer if String is lexicall greater than String2; Zero if
  the two strings are identical; and a negative interger if String is lexically
  less than String2.

**/
INTN
EFIAPI
StrCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  );

/**

  Compare the Unicode string pointed by String to the string pointed by String2.

  @param  String - Unicode String to process
  @param  String2 - Unicode string to process

  @return Return a positive integer if String is lexically greater than String2; Zero if
  the two strings are identical; and a negative integer if String is lexically
  less than String2.

**/
INTN
EFIAPI
StriCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2
  );

/**

  Compare the Unicode and Ascii string pointed by String to the string pointed by String2.

  @param  String - Unicode String to process
  @param  String2 - Ascii string to process

  @return Return a positive integer if String is lexically greater than String2; Zero if
  the two strings are identical; and a negative integer if String is lexically
  less than String2.

**/
INTN
EFIAPI
StriCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  );

/**

  Verify if the string is end with the sub string.

  @param  Str - The string where to search the sub string
  @param  SubStr - The substring.

**/
BOOLEAN
EFIAPI
StrEndWith (
  IN CHAR16                       *Str,
  IN CHAR16                       *SubStr
  );

/**
  Duplicate a string.

  @param  Src  The string to be duplicated.

**/
CHAR16 *
EFIAPI
StrDuplicate (
  IN CHAR16   *Src
  );

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNewTokenLine (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  );

/**

  Find the next token after one or more specified characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNextTokenLine (
  IN CHAR16                       *CharSet
  );

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNewTokenField (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  );

/**

  Find the next token after one specificed characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNextTokenField (
  IN CHAR16                       *CharSet
  );

/**

  Patch a character to the end of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForStrTokenAfter (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  );

/**
  Patch a character at the beginning of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForStrTokenBefore (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  );

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNewTokenLine (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  );

/**

  Find the next token after one or more specified characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNextTokenLine (
  IN CHAR8                       *CharSet
  );

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNewTokenField (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  );

/**

  Find the next token after one specificed characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNextTokenField (
  IN CHAR8                       *CharSet
  );

/**

  Patch a character to the end of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForAsciiStrTokenAfter (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  );

/**
  Patch a character at the beginning of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForAsciiStrTokenBefore (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  );

/**

  Shell Library.
  Get user input.

  @param  Prompt    The prompt string.
  @param  InStr     Point to the input string.
  @param  StrLen    The max length of string user can input.

**/
VOID
EFIAPI
Input (
  IN CHAR16    *Prompt OPTIONAL,
  OUT CHAR16   *InStr,
  IN UINTN     StrLen
  );

/**

  SetPageBreak.

**/
BOOLEAN
EFIAPI
SetPageBreak (
  VOID
  );

/**
  Print a Unicode string to the output device.

  @param  Format    A Null-terminated Unicode format string.
  @param  ...       The variable argument list that contains pointers to Null-
                    terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBPrint (
  IN CONST CHAR16  *Format,
  ...
  );

/**
  Print a Unicode string to the output buffer.

  @param  Buffer          A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  Format          A Null-terminated Unicode format string.
  @param  ...             The variable argument list that contains pointers to Null-
                          terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBSPrint (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  CONST CHAR16  *Format,
  ...
  );

/**
  Print a Unicode string to the output buffer with specified offset..

  @param  Buffer          A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  Offset          The offset of the buffer.
  @param  Format          A Null-terminated Unicode format string.
  @param  ...             The variable argument list that contains pointers to Null-
                          terminated Unicode strings to be printed

**/
UINTN
EFIAPI
EDBSPrintWithOffset (
  OUT CHAR16        *Buffer,
  IN  INTN          BufferSize,
  IN  UINTN         Offset,
  IN  CONST CHAR16  *Format,
  ...
  );

/**

  Read a file.
  If ScanFs is FLASE, it will use DebuggerPrivate->Vol as default Fs.
  If ScanFs is TRUE, it will scan all FS and check the file.
  If there is only one file match the name, it will be read.
  If there is more than one file match the name, it will return Error.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  FileName        - The file to be read.
  @param  BufferSize      - The file buffer size
  @param  Buffer          - The file buffer
  @param  ScanFs          - Need Scan all FS

  @retval EFI_SUCCESS    - read file successfully
  @retval EFI_NOT_FOUND  - file not found
  @retval EFI_NO_MAPPING - there is duplicated files found

**/
EFI_STATUS
EFIAPI
ReadFileToBuffer (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *FileName,
  OUT UINTN                       *BufferSize,
  OUT VOID                        **Buffer,
  IN  BOOLEAN                     ScanFs
  );

/**

  Get file name under this dir with index

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  DirName         - The dir to be read.
  @param  FileName        - The file name pattern under this dir
  @param  Index           - The file index under this dir

  @return File Name which match the pattern and index.

**/
CHAR16 *
EFIAPI
GetFileNameUnderDir (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *DirName,
  IN  CHAR16                      *FileName,
  IN OUT UINTN                    *Index
  );

#endif
