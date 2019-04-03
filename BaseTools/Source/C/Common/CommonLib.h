/** @file
Common library assistance routines.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_COMMON_LIB_H
#define _EFI_COMMON_LIB_H

#include <Common/UefiBaseTypes.h>
#include <Common/BuildVersion.h>
#include <assert.h>
#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination

#define MAX_LONG_FILE_PATH 500

#define MAX_UINT64 ((UINT64)0xFFFFFFFFFFFFFFFFULL)
#define MAX_UINT32 ((UINT32)0xFFFFFFFF)
#define MAX_UINT16  ((UINT16)0xFFFF)
#define MAX_UINT8   ((UINT8)0xFF)
#define ARRAY_SIZE(Array) (sizeof (Array) / sizeof ((Array)[0]))
#define ASCII_RSIZE_MAX 1000000
#undef RSIZE_MAX
#define RSIZE_MAX 1000000

#define IS_COMMA(a)                ((a) == L',')
#define IS_HYPHEN(a)               ((a) == L'-')
#define IS_DOT(a)                  ((a) == L'.')
#define IS_LEFT_PARENTH(a)         ((a) == L'(')
#define IS_RIGHT_PARENTH(a)        ((a) == L')')
#define IS_SLASH(a)                ((a) == L'/')
#define IS_NULL(a)                 ((a) == L'\0')

#define ASSERT(x) assert(x)

#ifdef __cplusplus
extern "C" {
#endif

//
// Function declarations
//
VOID
PeiZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
;

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
;

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
;

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
;

INTN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
;

EFI_STATUS
GetFileImage (
  IN CHAR8    *InputFileName,
  OUT CHAR8   **InputFileImage,
  OUT UINT32  *BytesRead
  )
;

EFI_STATUS
PutFileImage (
  IN CHAR8    *OutputFileName,
  IN CHAR8    *OutputFileImage,
  IN UINT32   BytesToWrite
  )
;
/*++

Routine Description:

  This function opens a file and writes OutputFileImage into the file.

Arguments:

  OutputFileName     The name of the file to write.
  OutputFileImage    A pointer to the memory buffer.
  BytesToWrite       The size of the memory buffer.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     No resource to complete operations.

**/

UINT8
CalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
;

UINT8
CalculateSum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
;

UINT16
CalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
;

UINT16
CalculateSum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
;

EFI_STATUS
PrintGuid (
  IN EFI_GUID                     *Guid
  )
;

#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination
EFI_STATUS
PrintGuidToBuffer (
  IN EFI_GUID     *Guid,
  IN OUT UINT8    *Buffer,
  IN UINT32       BufferLen,
  IN BOOLEAN      Uppercase
  )
;

CHAR8 *
LongFilePath (
 IN CHAR8 *FileName
);

UINTN
StrLen (
  CONST CHAR16   *String
  );

VOID *
AllocateCopyPool (
  UINTN       AllocationSize,
  CONST VOID  *Buffer
  );

INTN
StrnCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString,
  UINTN                     Length
  );

RETURN_STATUS
StrToGuid (
  CONST CHAR16       *String,
  EFI_GUID               *Guid
  );

RETURN_STATUS
StrHexToBytes (
  CONST CHAR16       *String,
  UINTN              Length,
  UINT8              *Buffer,
  UINTN              MaxBufferSize
  );

UINTN
InternalHexCharToUintn (
  CHAR16                    Char
  );

VOID *
InternalAllocateCopyPool (
   UINTN            AllocationSize,
   CONST VOID       *Buffer
  );

BOOLEAN
InternalIsDecimalDigitCharacter (
        CHAR16                    Char
  );

UINT32
SwapBytes32 (
        UINT32                    Value
  );

UINT16
SwapBytes16 (
        UINT16                    Value
  );

EFI_GUID *
CopyGuid (
   EFI_GUID       *DestinationGuid,
   CONST EFI_GUID  *SourceGuid
  );

UINT64
WriteUnaligned64 (
   UINT64                    *Buffer,
   UINT64                    Value
  );

UINT64
ReadUnaligned64 (
   CONST UINT64              *Buffer
  );

UINTN
StrSize (
  CONST CHAR16              *String
  );

UINT64
StrHexToUint64 (
  CONST CHAR16             *String
  );

UINT64
StrDecimalToUint64 (
  CONST CHAR16              *String
  );

RETURN_STATUS
StrHexToUint64S (
    CONST CHAR16       *String,
    CHAR16             **EndPointer,
    UINT64             *Data
  );

RETURN_STATUS
StrDecimalToUint64S (
    CONST CHAR16             *String,
         CHAR16             **EndPointer,  OPTIONAL
         UINT64             *Data
  );

VOID *
ReallocatePool (
   UINTN  OldSize,
   UINTN  NewSize,
   VOID   *OldBuffer  OPTIONAL
  );

VOID *
InternalReallocatePool (
   UINTN            OldSize,
   UINTN            NewSize,
   VOID             *OldBuffer  OPTIONAL
  );

VOID *
InternalAllocateZeroPool (
   UINTN            AllocationSize
  ) ;

VOID *
InternalAllocatePool (
   UINTN            AllocationSize
  );

UINTN
StrnLenS (
   CONST CHAR16              *String,
   UINTN                     MaxSize
  );

CHAR16
InternalCharToUpper (
        CHAR16                    Char
  );

INTN
StrCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString
  );

UINT64
SwapBytes64 (
  UINT64                    Value
  );

UINT64
InternalMathSwapBytes64 (
  UINT64                    Operand
  );

RETURN_STATUS
StrToIpv4Address (
  CONST CHAR16       *String,
  CHAR16             **EndPointer,
  EFI_IPv4_ADDRESS       *Address,
  UINT8              *PrefixLength
  );

RETURN_STATUS
StrToIpv6Address (
  CONST CHAR16       *String,
  CHAR16             **EndPointer,
  EFI_IPv6_ADDRESS       *Address,
  UINT8              *PrefixLength
  );

RETURN_STATUS
StrCpyS (
  CHAR16       *Destination,
  UINTN        DestMax,
  CONST CHAR16 *Source
  );

RETURN_STATUS
UnicodeStrToAsciiStrS (
  CONST CHAR16              *Source,
  CHAR8                     *Destination,
  UINTN                     DestMax
  );
VOID *
AllocatePool (
  UINTN  AllocationSize
  );

UINT16
WriteUnaligned16 (
  UINT16                    *Buffer,
  UINT16                    Value
  );

UINT16
ReadUnaligned16 (
  CONST UINT16              *Buffer
  );

VOID *
AllocateZeroPool (
  UINTN  AllocationSize
  );

BOOLEAN
InternalIsHexaDecimalDigitCharacter (
  CHAR16                    Char
  );

BOOLEAN
InternalSafeStringIsOverlap (
  IN VOID    *Base1,
  IN UINTN   Size1,
  IN VOID    *Base2,
  IN UINTN   Size2
  );

BOOLEAN
InternalSafeStringNoStrOverlap (
  IN CHAR16  *Str1,
  IN UINTN   Size1,
  IN CHAR16  *Str2,
  IN UINTN   Size2
  );

BOOLEAN
IsHexStr (
   CHAR16   *Str
  );

UINTN
Strtoi (
   CHAR16  *Str
  );

VOID
Strtoi64 (
    CHAR16  *Str,
   UINT64  *Data
  );

VOID
StrToAscii (
       CHAR16 *Str,
    CHAR8  **AsciiStr
  );

CHAR16 *
SplitStr (
    CHAR16 **List,
       CHAR16 Separator
  );

/*++

Routine Description:
  Convert FileName to the long file path, which can support larger than 260 length.

Arguments:
  FileName         - FileName.

Returns:
  LongFilePath      A pointer to the converted long file path.

--*/

#ifdef __cplusplus
}
#endif

#ifdef __GNUC__
#include <stdio.h>
#include <sys/stat.h>
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define strcmpi strcasecmp
size_t _filelength(int fd);
#ifndef __CYGWIN__
char *strlwr(char *s);
#endif
#endif

//
// On windows, mkdir only has one parameter.
// On unix, it has two parameters
//
#if defined(__GNUC__)
#define mkdir(dir, perm) mkdir(dir, perm)
#else
#define mkdir(dir, perm) mkdir(dir)
#endif

#endif
