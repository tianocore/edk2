/** @file
  Safe String functions.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>

#define RSIZE_MAX  (PcdGet32 (PcdMaximumUnicodeStringLength))

#define ASCII_RSIZE_MAX  (PcdGet32 (PcdMaximumAsciiStringLength))

#define SAFE_STRING_CONSTRAINT_CHECK(Expression, Status)  \
  do { \
    ASSERT (Expression); \
    if (!(Expression)) { \
      return Status; \
    } \
  } while (FALSE)

/**
  Returns if 2 memory blocks are overlapped.

  @param  Base1  Base address of 1st memory block.
  @param  Size1  Size of 1st memory block.
  @param  Base2  Base address of 2nd memory block.
  @param  Size2  Size of 2nd memory block.

  @retval TRUE  2 memory blocks are overlapped.
  @retval FALSE 2 memory blocks are not overlapped.
**/
BOOLEAN
InternalSafeStringIsOverlap (
  IN VOID    *Base1,
  IN UINTN   Size1,
  IN VOID    *Base2,
  IN UINTN   Size2
  )
{
  if ((((UINTN)Base1 >= (UINTN)Base2) && ((UINTN)Base1 < (UINTN)Base2 + Size2)) ||
      (((UINTN)Base2 >= (UINTN)Base1) && ((UINTN)Base2 < (UINTN)Base1 + Size1))) {
    return TRUE;
  }
  return FALSE;
}

/**
  Returns if 2 Unicode strings are not overlapped.

  @param  Str1   Start address of 1st Unicode string.
  @param  Size1  The number of char in 1st Unicode string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Unicode string.
  @param  Size2  The number of char in 2nd Unicode string,
                 including terminating null char.

  @retval TRUE  2 Unicode strings are NOT overlapped.
  @retval FALSE 2 Unicode strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoStrOverlap (
  IN CHAR16  *Str1,
  IN UINTN   Size1,
  IN CHAR16  *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1 * sizeof(CHAR16), Str2, Size2 * sizeof(CHAR16));
}

/**
  Returns if 2 Ascii strings are not overlapped.

  @param  Str1   Start address of 1st Ascii string.
  @param  Size1  The number of char in 1st Ascii string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Ascii string.
  @param  Size2  The number of char in 2nd Ascii string,
                 including terminating null char.

  @retval TRUE  2 Ascii strings are NOT overlapped.
  @retval FALSE 2 Ascii strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoAsciiStrOverlap (
  IN CHAR8   *Str1,
  IN UINTN   Size1,
  IN CHAR8   *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1, Str2, Size2);
}

/**
  Returns the length of a Null-terminated Unicode string.

  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String   A pointer to a Null-terminated Unicode string.
  @param  MaxSize  The maximum number of Destination Unicode
                   char, including terminating null char.

  @retval 0        If String is NULL.
  @retval MaxSize  If there is no null character in the first MaxSize characters of String.
  @return The number of characters that percede the terminating null character.

**/
UINTN
EFIAPI
StrnLenS (
  IN CONST CHAR16              *String,
  IN UINTN                     MaxSize
  )
{
  UINTN                             Length;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // If String is a null pointer, then the StrnLenS function returns zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the StrnLenS function returns the number of characters that precede the
  // terminating null character. If there is no null character in the first MaxSize characters of
  // String then StrnLenS returns MaxSize. At most the first MaxSize characters of String shall
  // be accessed by StrnLenS.
  //
  for (Length = 0; (Length < MaxSize) && (*String != 0); String++, Length++) {
    ;
  }
  return Length;
}

/**
  Copies the string pointed to by Source (including the terminating null char)
  to the array pointed to by Destination.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrCpyS (
  OUT CHAR16       *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR16 *Source
  )
{
  UINTN            SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Copies not more than Length successive char from the string pointed to by
  Source to the array pointed to by Destination. If no null char is copied from
  Source, then Destination[Length] is always set to null.

  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.
  @param  Length                   The maximum number of Unicode characters to copy.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than 
                                   MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrnCpyS (
  OUT CHAR16       *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR16 *Source,
  IN  UINTN        Length
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than RSIZE_MAX
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  if (Length >= DestMax) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrnCpyS function copies not more than Length successive characters (characters that
  // follow a null character are not copied) from the array pointed to by Source to the array
  // pointed to by Destination. If no null character was copied from Source, then Destination[Length] is set to a null
  // character.
  //
  while ((*Source != 0) && (SourceLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends a copy of the string pointed to by Source (including the terminating
  null char) to the end of the string pointed to by Destination.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than 
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrCatS (
  IN OUT CHAR16       *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR16 *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // Let CopyLen denote the value DestMax - StrnLenS(Destination, DestMax) upon entry to StrCatS.
  //
  DestLen = StrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than StrnLenS(Source, CopyLen).
  //
  SourceLen = StrnLenS (Source, CopyLen);
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends not more than Length successive char from the string pointed to by
  Source to the end of the string pointed to by Destination. If no null char is
  copied from Source, then Destination[StrLen(Destination) + Length] is always
  set to null.

  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Unicode string.
  @param  DestMax                  The maximum number of Destination Unicode
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Unicode string.
  @param  Length                   The maximum number of Unicode characters to copy.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumUnicodeStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
StrnCatS (
  IN OUT CHAR16       *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR16 *Source,
  IN     UINTN        Length
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // Let CopyLen denote the value DestMax - StrnLenS(Destination, DestMax) upon entry to StrnCatS.
  //
  DestLen = StrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. If Length is not less than CopyLen, then CopyLen shall be greater than StrnLenS(Source, CopyLen).
  //
  SourceLen = StrnLenS (Source, CopyLen);
  if (Length >= CopyLen) {
    SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrnCatS function appends not more than Length successive characters (characters
  // that follow a null character are not copied) from the array pointed to by Source to the end of
  // the string pointed to by Destination. The initial character from Source overwrites the null character at
  // the end of Destination. If no null character was copied from Source, then Destination[DestMax-CopyLen+Length] is set to
  // a null character.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (SourceLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Returns the length of a Null-terminated Ascii string.

  @param  String   A pointer to a Null-terminated Ascii string.
  @param  MaxSize  The maximum number of Destination Ascii
                   char, including terminating null char.

  @retval 0        If String is NULL.
  @retval MaxSize  If there is no null character in the first MaxSize characters of String.
  @return The number of characters that percede the terminating null character.

**/
UINTN
EFIAPI
AsciiStrnLenS (
  IN CONST CHAR8               *String,
  IN UINTN                     MaxSize
  )
{
  UINTN                             Length;

  //
  // If String is a null pointer, then the AsciiStrnLenS function returns zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the AsciiStrnLenS function returns the number of characters that precede the
  // terminating null character. If there is no null character in the first MaxSize characters of
  // String then AsciiStrnLenS returns MaxSize. At most the first MaxSize characters of String shall
  // be accessed by AsciiStrnLenS.
  //
  for (Length = 0; (Length < MaxSize) && (*String != 0); String++, Length++) {
    ;
  }
  return Length;
}

/**
  Copies the string pointed to by Source (including the terminating null char)
  to the array pointed to by Destination.

  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source
  )
{
  UINTN            SourceLen;
  
  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Copies not more than Length successive char from the string pointed to by
  Source to the array pointed to by Destination. If no null char is copied from
  Source, then Destination[Length] is always set to null.

  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than 
                                   MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrnCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source,
  IN  UINTN        Length
  )
{
  UINTN            SourceLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
  if (Length >= DestMax) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCpyS function copies not more than Length successive characters (characters that
  // follow a null character are not copied) from the array pointed to by Source to the array
  // pointed to by Destination. If no null character was copied from Source, then Destination[Length] is set to a null
  // character.
  //
  while ((*Source != 0) && (SourceLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends a copy of the string pointed to by Source (including the terminating
  null char) to the end of the string pointed to by Destination.

  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than 
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, CopyLen);
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends not more than Length successive char from the string pointed to by
  Source to the end of the string pointed to by Destination. If no null char is
  copied from Source, then Destination[StrLen(Destination) + Length] is always
  set to null.

  If an error would be returned, then the function will also ASSERT().

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than 
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
EFIAPI
AsciiStrnCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source,
  IN     UINTN        Length
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;
  
  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrnCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. If Length is not less than CopyLen, then CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, CopyLen);
  if (Length >= CopyLen) {
    SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCatS function appends not more than Length successive characters (characters
  // that follow a null character are not copied) from the array pointed to by Source to the end of
  // the string pointed to by Destination. The initial character from Source overwrites the null character at
  // the end of Destination. If no null character was copied from Source, then Destination[DestMax-CopyLen+Length] is set to
  // a null character.
  //
  Destination = Destination + DestLen;
  while ((*Source != 0) && (SourceLen > 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}
