/** @file

 Common library.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _COMMON_LIB_H_
#define _COMMON_LIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include "CommonLib.h"
#include <Common/UefiBaseTypes.h>

#define MAX_QUI_PARAM_LEN              2000
#define ERROR_INFO_LENGTH              400
#define MAX_STR_LEN_FOR_PICK_UQI       200
#define MAX_PLATFORM_DEFAULT_ID_NUM    1000
#define _MAX_BUILD_VERSION             100
#define _MAXIMUM_SECTION_FILE_NUM      1000

#ifndef _MAX_PATH
#define _MAX_PATH 500
#endif

///
/// Variable attributes.
///
#define EFI_VARIABLE_NON_VOLATILE       0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x00000004

///
/// This attribute is identified by the mnemonic 'HR'
/// elsewhere in this specification.
///
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS   0x00000010

#define VARSTORE_LIST_TYPE         0x0000000000000001ULL
#define EFI_VARSTORE_LIST_TYPE     0x0000000000000002ULL
#define PLATFORM_DEFAULT_ID_TYPE   0x0000000000000004ULL
#define UQI_LIST_TYPE              0x0000000000000008ULL
#define HII_OBJ_LIST_TYPE          0x0000000000000010ULL

///
/// LIST_ENTRY structure definition.
///
typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY  *ForwardLink;
  struct _LIST_ENTRY  *BackLink;
} LIST_ENTRY;

#define CR(Record, TYPE, Field, TestSignature)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))
#define AllocateZeroPool(a)  calloc(a, sizeof (CHAR8))
#define FreePool(a) free(a)
#define CopyMem(a, b, c)  memcpy(a, b, c)
#define ZeroMem(a, b) memset(a, 0, b)
#define CompareMem(a, b, c)  memcmp(a, b, c)
#define AllocatePool(a)  malloc(a)

/**
  Returns a 16-bit signature built from 2 ASCII characters.

  This macro returns a 16-bit value built from the two ASCII characters specified
  by A and B.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.

  @return A 16-bit value built from the two ASCII characters specified by A and B.

**/
#define SIGNATURE_16(A, B)        ((A) | (B << 8))

/**
  Returns a 32-bit signature built from 4 ASCII characters.

  This macro returns a 32-bit value built from the four ASCII characters specified
  by A, B, C, and D.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.

  @return A 32-bit value built from the two ASCII characters specified by A, B,
          C and D.

**/
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

#define ASSERT_UNICODE_BUFFER(Buffer) ASSERT ((((UINTN) (Buffer)) & 0x01) == 0)

/**
  Returns an argument of a specified type from a variable argument list and updates
  the pointer to the variable argument list to point to the next argument.

  This function returns an argument of the type specified by TYPE from the beginning
  of the variable argument list specified by Marker.  Marker is then updated to point
  to the next argument in the variable argument list.  The method for computing the
  pointer to the next argument in the argument list is CPU specific following the EFIAPI ABI.

  @param   Marker   The pointer to the beginning of a variable argument list.
  @param   TYPE     The type of argument to retrieve from the beginning
                    of the variable argument list.

  @return  An argument of the type specified by TYPE.

**/
#define BASE_ARG(Marker, TYPE)   (*(TYPE *) ((Marker += _BASE_INT_SIZE_OF (TYPE)) - _BASE_INT_SIZE_OF (TYPE)))

///
/// Define the maximum number of characters that are required to
/// encode with a NULL terminator a decimal, hexadecimal, GUID,
/// or TIME value.
///
///  Maximum Length Decimal String     = 28
///    "-9,223,372,036,854,775,808"
///  Maximum Length Hexadecimal String = 17
///    "FFFFFFFFFFFFFFFF"
///  Maximum Length GUID               = 37
///    "00000000-0000-0000-0000-000000000000"
///  Maximum Length TIME               = 18
///    "12/12/2006  12:12"
///
#define MAXIMUM_VALUE_CHARACTERS  38

///
/// Pointer to the start of a variable argument list stored in a memory buffer. Same as UINT8 *.
///
typedef UINTN  *BASE_LIST;

/**
  Returns the size of a data type in sizeof(UINTN) units rounded up to the nearest UINTN boundary.

  @param  TYPE  The date type to determine the size of.

  @return The size of TYPE in sizeof (UINTN) units rounded up to the nearest UINTN boundary.
**/
#define _BASE_INT_SIZE_OF(TYPE) ((sizeof (TYPE) + sizeof (UINTN) - 1) / sizeof (UINTN))

//
// Print primitives
//
#define PREFIX_SIGN           BIT1
#define PREFIX_BLANK          BIT2
#define LONG_TYPE             BIT4
#define OUTPUT_UNICODE        BIT6
#define FORMAT_UNICODE        BIT8
#define PAD_TO_WIDTH          BIT9
#define ARGUMENT_UNICODE      BIT10
#define PRECISION             BIT11
#define ARGUMENT_REVERSED     BIT12
#define COUNT_ONLY_NO_PRINT   BIT13

///
/// Flags bitmask values use in UnicodeValueToString() and
/// AsciiValueToString()
///
#define LEFT_JUSTIFY      0x01
#define COMMA_TYPE        0x08
#define PREFIX_ZERO       0x20
#define RADIX_HEX         0x80

//
// Record date and time information
//
typedef struct {
  UINT16  Year;
  UINT8   Month;
  UINT8   Day;
  UINT8   Hour;
  UINT8   Minute;
  UINT8   Second;
  UINT8   Pad1;
  UINT32  Nanosecond;
  INT16   TimeZone;
  UINT8   Daylight;
  UINT8   Pad2;
} TIME;


/**
  Copies one Null-terminated Unicode string to another Null-terminated Unicode
  string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then return NULL.
  If Destination is not aligned on a 16-bit boundary, then return NULL.

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.

  @return Destination.

**/
CHAR16 *
StrCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  );

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then return 0.

  @param  String  A pointer to a Null-terminated Unicode string.

  @return The length of String.

**/
UINTN
FceStrLen (
  IN      CONST CHAR16              *String
  );

/**
  Returns the size of a Null-terminated Unicode string in bytes, including the
  Null terminator.

  This function returns the size, in bytes, of the Null-terminated Unicode string
  specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  String  A pointer to a Null-terminated Unicode string.

  @return The size of String.

**/
UINTN
FceStrSize (
  IN      CONST CHAR16              *String
  );

/**
  Compares two Null-terminated Unicode strings, and returns the difference
  between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. If FirstString is identical to
  SecondString, then 0 is returned. Otherwise, the value returned is the first
  mismatched Unicode character in SecondString subtracted from the first
  mismatched Unicode character in FirstString.

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.

  @retval 0      FirstString is identical to SecondString.
  @return others FirstString is not identical to SecondString.

**/
INTN
FceStrCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString
  );

/**
  Concatenates one Null-terminated Unicode string to another Null-terminated
  Unicode string, and returns the concatenated Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination. The Null-terminated concatenated
  Unicode String is returned. If Source and Destination overlap, then the
  results are undefined.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.

  @return Destination.

**/
CHAR16 *
StrCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  );

/**
  Returns the first occurrence of a Null-terminated Unicode sub-string
  in a Null-terminated Unicode string.

  This function scans the contents of the Null-terminated Unicode string
  specified by String and returns the first occurrence of SearchString.
  If SearchString is not found in String, then NULL is returned.  If
  the length of SearchString is zero, then String is
  returned.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If SearchString is NULL, then ASSERT().
  If SearchString is not aligned on a 16-bit boundary, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and SearchString
  or String contains more than PcdMaximumUnicodeStringLength Unicode
  characters, not including the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.
  @param  SearchString    A pointer to a Null-terminated Unicode string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @return others          If there is a match.

**/
CHAR16 *
StrStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  );

/**
  Convert a Null-terminated Unicode decimal string to a value of
  type UINT64.

  This function returns a value of type UINT64 by interpreting the contents
  of the Unicode string specified by String as a decimal number. The format
  of the input Unicode string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The
  function will ignore the pad space, which includes spaces or
  tab characters, before [decimal digits]. The running zero in the
  beginning of [decimal digits] will be ignored. Then, the function
  stops at the first character that is a not a valid decimal character
  or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits,
  then 0 is returned.
  If the number represented by String overflows according
  to the range defined by UINT64, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains
  more than PcdMaximumUnicodeStringLength Unicode characters, not including
  the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
FceStrDecimalToUint64 (
  IN      CONST CHAR16              *String
  );


/**
  Convert one Null-terminated ASCII string to a Null-terminated
  Unicode string and returns the Unicode string.

  This function converts the contents of the ASCII string Source to the Unicode
  string Destination, and returns Destination.  The function terminates the
  Unicode string Destination by appending a Null-terminator character at the end.
  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((AsciiStrLen (Source) + 1) * sizeof (CHAR16)) in bytes.

  @param  Source        A pointer to a Null-terminated ASCII string.
  @param  Destination   A pointer to a Null-terminated Unicode string.

  @return Destination.
  @return NULL          If Destination or Source is NULL, return NULL.

**/
CHAR16 *
AsciiStrToUnicodeStr (
  IN      CONST CHAR8               *Source,
  OUT     CHAR16                    *Destination
  );

/**
  Worker function that produces a Null-terminated string in an output buffer
  based on a Null-terminated format string and variable argument list.

  VSPrint function to process format and place the results in Buffer. Since a
  VA_LIST is used this routine allows the nesting of Vararg routines. Thus
  this is the main print working routine

  @param  StartOfBuffer The character buffer to print the results of the parsing
                        of Format into.
  @param  BufferSize    The maximum number of characters to put into buffer.
                        Zero means no limit.
  @param  Flags         Initial flags value.
                        Can only have FORMAT_UNICODE and OUTPUT_UNICODE set
  @param  FormatString  A Null-terminated format string.
  @param  ...           The variable argument list.

  @return The number of characters printed.

**/
UINTN
BasePrintLibSPrint (
  OUT CHAR8        *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *FormatString,
  ...
  );

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated
  Unicode format string and variable argument list.

  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the format string.
  The number of Unicode characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If BufferSize > 1 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the
                          format string specified by FormatString.

  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
UnicodeSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  ...
  );

/**
  Convert a Null-terminated Unicode string to a Null-terminated
  ASCII string and returns the ASCII string.

  This function converts the content of the Unicode string Source
  to the ASCII string Destination by copying the lower 8 bits of
  each Unicode character. It returns Destination. The function terminates
  the ASCII string Destination  by appending a Null-terminator character
  at the end. The caller is responsible to make sure Destination points
  to a buffer with size equal or greater than (FceStrLen (Source) + 1) in bytes.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().

  If any Unicode characters in Source contain non-zero value in
  the upper 8 bits, then ASSERT().

  @param  Source        Pointer to a Null-terminated Unicode string.
  @param  Destination   Pointer to a Null-terminated ASCII string.

  @reture Destination

**/
CHAR8 *
UnicodeStrToAsciiStr (
  IN      CONST CHAR16             *Source,
  OUT           CHAR8              *Destination
  );

/**
  Allocate new memory and then copy the Unicode string Source to Destination.

  @param  Dest                   Location to copy string
  @param  Src                    String to copy

**/
VOID
NewStringCpy (
  IN OUT CHAR16       **Dest,
  IN CHAR16           *Src
  );

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type UINT64.

  This function returns a value of type UINT64 by interpreting the contents
  of the Unicode string specified by String as a hexadecimal number.
  The format of the input Unicode string String is

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix.
  If "x" appears in the input string, it must be prefixed with at least one 0.
  The function will ignore the pad space, which includes spaces or tab characters,
  before [zeros], [x] or [hexadecimal digit]. The running zero before [x] or
  [hexadecimal digit] will be ignored. Then, the decoding starts after [x] or the
  first valid hexadecimal digit. Then, the function stops at the first character that is
  a not a valid hexadecimal character or NULL, whichever one comes first.

  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If String has only pad spaces, then zero is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits,
  then zero is returned.
  If the number represented by String overflows according to the range defined by
  UINT64, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
FceStrHexToUint64 (
  IN      CONST CHAR16             *String
  );


CHAR16
ToUpper (
  CHAR16  a
  );

CHAR16
ToLower (
  CHAR16  a
  );

/**
  Performs a case-insensitive comparison between a Null-terminated
  Unicode pattern string and a Null-terminated Unicode string.

  @param  String   - A pointer to a Null-terminated Unicode string.
  @param  Pattern  - A pointer to a Null-terminated Unicode pattern string.


  @retval TRUE     - Pattern was found in String.
  @retval FALSE    - Pattern was not found in String.

**/
BOOLEAN
MetaiMatch (
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  );

/**
  Multiplies a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 64-bit unsigned result.

  This function multiplies the 64-bit unsigned value Multiplicand by the 32-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 32-bit unsigned value.

  @return Multiplicand * Multiplier.

**/
UINT64
MultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  );

/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 64-bit unsigned result.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. This
  function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend / Divisor

**/
UINT64
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );

/**
  Shifts a 64-bit integer left between 0 and 63 bits. The low bits are filled
  with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the left by Count bits. The
  low Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift left.
  @param  Count   The number of bits to shift left.

  @return Operand << Count.

**/
UINT64
LShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );

/**
  Shifts a 64-bit integer right between 0 and 63 bits. This high bits are
  filled with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand >> Count.

**/
UINT64
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );


/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 64-bit unsigned result and an optional 32-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 32-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.
  @param  Remainder A pointer to a 32-bit unsigned value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
UINT64
DivU64x32Remainder (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder
  );

/**
  Copies a buffer to an allocated buffer.

  Allocates the number bytes specified by AllocationSize, copies allocationSize bytes
  from Buffer to the newly allocated buffer, and returns a pointer to the allocated
  buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
FceAllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  );

/**
  Initializes the head node of a doubly-linked list, and returns the pointer to
  the head node of the doubly-linked list.

  Initializes the forward and backward links of a new linked list. After
  initializing a linked list with this function, the other linked list
  functions may be used to add and remove nodes from the linked list. It is up
  to the caller of this function to allocate the memory for ListHead.

  If ListHead is NULL, then ASSERT().

  @param  ListHead  A pointer to the head node of a new doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InitializeListHead (
  IN OUT  LIST_ENTRY                *ListHead
  );

/**
  Adds a node to the beginning of a doubly-linked list, and returns the pointer
  to the head node of the doubly-linked list.

  Adds the node Entry at the beginning of the doubly-linked list denoted by
  ListHead, and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.
  @param  Entry     A pointer to a node that is to be inserted at the beginning
                    of a doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InsertHeadList (
  IN OUT  LIST_ENTRY                *ListHead,
  IN OUT  LIST_ENTRY                *Entry
  );

/**
  Adds a node to the end of a doubly-linked list, and returns the pointer to
  the head node of the doubly-linked list.

  Adds the node Entry to the end of the doubly-linked list denoted by ListHead,
  and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.
  @param  Entry     A pointer to a node that is to be added at the end of the
                    doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InsertTailList (
  IN OUT  LIST_ENTRY                *ListHead,
  IN OUT  LIST_ENTRY                *Entry
  );

/**
  Retrieves the first node of a doubly-linked list.

  Returns the first node of a doubly-linked list.  List must have been
  initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().
  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.

  @return The first node of a doubly-linked list.
  @retval NULL  The list is empty.

**/
LIST_ENTRY *
GetFirstNode (
  IN      CONST LIST_ENTRY          *List
  );

/**
  Retrieves the next node of a doubly-linked list.

  Returns the node of a doubly-linked list that follows Node.
  List must have been initialized with INTIALIZE_LIST_HEAD_VARIABLE()
  or InitializeListHead().  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and List contains more than
  PcdMaximumLinkedListLenth nodes, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @return A pointer to the next node if one exists. Otherwise List is returned.

**/
LIST_ENTRY *
GetNextNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );

/**
  Retrieves the previous node of a doubly-linked list.

  Returns the node of a doubly-linked list that precedes Node.
  List must have been initialized with INTIALIZE_LIST_HEAD_VARIABLE()
  or InitializeListHead().  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and List contains more than
  PcdMaximumLinkedListLenth nodes, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @return A pointer to the previous node if one exists. Otherwise List is returned.

**/
LIST_ENTRY *
GetPreviousNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );

/**
  Checks to see if a doubly-linked list is empty or not.

  Checks to see if the doubly-linked list is empty. If the linked list contains
  zero nodes, this function returns TRUE. Otherwise, it returns FALSE.

  If ListHead is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.

  @retval TRUE  The linked list is empty.
  @retval FALSE The linked list is not empty.

**/
BOOLEAN
IsListEmpty (
  IN      CONST LIST_ENTRY          *ListHead
  );

/**
  Determines if a node in a doubly-linked list is the head node of a the same
  doubly-linked list.  This function is typically used to terminate a loop that
  traverses all the nodes in a doubly-linked list starting with the head node.

  Returns TRUE if Node is equal to List.  Returns FALSE if Node is one of the
  nodes in the doubly-linked list specified by List.  List must have been
  initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead(),
  then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List and Node is not
  equal to List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @retval TRUE  Node is the head of the doubly-linked list pointed by List.
  @retval FALSE Node is not the head of the doubly-linked list pointed by List.

**/
BOOLEAN
IsNull (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );

/**
  Determines if a node the last node in a doubly-linked list.

  Returns TRUE if Node is the last node in the doubly-linked list specified by
  List. Otherwise, FALSE is returned. List must have been initialized with
  INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @retval TRUE  Node is the last node in the linked list.
  @retval FALSE Node is not the last node in the linked list.

**/
BOOLEAN
IsNodeAtEnd (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );

/**
  Removes a node from a doubly-linked list, and returns the node that follows
  the removed node.

  Removes the node Entry from a doubly-linked list. It is up to the caller of
  this function to release the memory used by this node if that is required. On
  exit, the node following Entry in the doubly-linked list is returned. If
  Entry is the only node in the linked list, then the head node of the linked
  list is returned.

  If Entry is NULL, then ASSERT().
  If Entry is the head node of an empty list, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list containing Entry, including the Entry node, is greater than
  or equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  Entry A pointer to a node in a linked list.

  @return Entry.

**/
LIST_ENTRY *
RemoveEntryList (
  IN      CONST LIST_ENTRY          *Entry
  );

#endif
