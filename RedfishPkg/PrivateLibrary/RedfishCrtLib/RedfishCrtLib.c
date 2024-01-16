/** @file
  CRT wrapper functions for system call,the string operation functions
  are remodeled after edk2-libc.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/RedfishCrtLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SortLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

int   errno            = 0;
char  errnum_message[] = "We don't support to map errnum to the error message on edk2 Redfish\n";

// This is required to keep VC++ happy if you use floating-point
int  _fltused = 1;

/**
  Determine if a particular character is an alphanumeric character
  @return  Returns 1 if c is an alphanumeric character, otherwise returns 0.
**/
int
isalnum (
  int  c
  )
{
  //
  // <alnum> ::= [0-9] | [a-z] | [A-Z]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (('a' <= (c)) && ((c) <= 'z')) ||
          (('A' <= (c)) && ((c) <= 'Z')));
}

/**
  Determine if a particular character is a digital character

  @return  Returns 1 if c is an digital character, otherwise returns 0.
**/
int
isdchar (
  int  c
  )
{
  //
  // [0-9] | [e +-.]
  //
  return ((('0' <= (c)) && ((c) <= '9')) ||
          (c == 'e') || (c == 'E') ||
          (c == '+') || (c == '-') ||
          (c == '.'));
}

/**
  Determine if a particular character is a space character

  @return  Returns 1 if c is a space character
**/
int
isspace (
  int  c
  )
{
  //
  // <space> ::= [ ]
  //
  return ((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n') || ((c) == '\v')  || ((c) == '\f');
}

/**
  Allocates memory blocks
*/
void *
malloc (
  size_t  size
  )
{
  return AllocatePool ((UINTN)size);
}

/**
  De-allocates or frees a memory block
*/
void
free (
  void  *ptr
  )
{
  //
  // In Standard C, free() handles a null pointer argument transparently. This
  // is not true of FreePool() below, so protect it.
  //
  if (ptr != NULL) {
    FreePool (ptr);
  }
}

/**
  NetBSD Compatibility Function strdup creates a duplicate copy of a string.

  @return  Returns the pointer to duplicated string.
**/
char *
strdup (
  const char  *str
  )
{
  size_t  len;
  char    *copy;

  len = strlen (str) + 1;
  if ((copy = malloc (len)) == NULL) {
    return (NULL);
  }

  memcpy (copy, str, len);
  return (copy);
}

/** The toupper function converts a lowercase letter to a corresponding
    uppercase letter.

    @param[in]    c   The character to be converted.

    @return   If the argument is a character for which islower is true and
              there are one or more corresponding characters, as specified by
              the current locale, for which isupper is true, the toupper
              function returns one of the corresponding characters (always the
              same one for any given locale); otherwise, the argument is
              returned unchanged.
**/
int
toupper (
  IN  int  c
  )
{
  if ((c >= 'a') && (c <= 'z')) {
    c = c - ('a' - 'A');
  }

  return c;
}

/**
  Digit to a value.

  @return  Returns the value of digit.
**/
int
Digit2Val (
  int  c
  )
{
  if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) {
    /* If c is one of [A-Za-z]... */
    c = toupper (c) - 7;   // Adjust so 'A' is ('9' + 1)
  }

  return c - '0';   // Value returned is between 0 and 35, inclusive.
}

/** The strtoll function converts the initial portion of the string pointed to
    by nptr to long long int representation.

    See the description for strtol for more information.

  @return   The strtoll function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, LLONG_MIN or
            LLONG_MAX is returned (according to the sign of the value, if any),
            and the value of the macro ERANGE is stored in errno.
**/
long long
strtoll (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  const char  *pEnd;
  long long   Result = 0;
  long long   Previous;
  int         temp;
  BOOLEAN     Negative = FALSE;

  pEnd = nptr;

  if ((base < 0) || (base == 1) || (base > 36)) {
    if (endptr != NULL) {
      *endptr = NULL;
    }

    return 0;
  }

  // Skip leading spaces.
  while (isspace (*nptr)) {
    ++nptr;
  }

  // Process Subject sequence: optional sign followed by digits.
  if (*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  } else if (*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }

  if (*nptr == '0') {
    /* Might be Octal or Hex */
    if (toupper (nptr[1]) == 'X') {
      /* Looks like Hex */
      if ((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base  = 16; /* In case base was 0 */
      }
    } else {
      /* Looks like Octal */
      if ((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }

  if (base == 0) {
    /* If still zero then must be decimal */
    base = 10;
  }

  if (*nptr  == '0') {
    for ( ; *nptr == '0'; ++nptr) {
      /* Skip any remaining leading zeros */
    }

    pEnd = nptr;
  }

  while ( isalnum (*nptr) && ((temp = Digit2Val (*nptr)) < base)) {
    Previous = Result;
    Result   = MultS64x64 (Result, base) + (long long int)temp;
    if ( Result <= Previous) {
      // Detect Overflow
      if (Negative) {
        Result = LLONG_MIN;
      } else {
        Result = LLONG_MAX;
      }

      Negative = FALSE;
      errno    = ERANGE;
      break;
    }

    pEnd = ++nptr;
  }

  if (Negative) {
    Result = -Result;
  }

  // Save pointer to final sequence
  if (endptr != NULL) {
    *endptr = (char *)pEnd;
  }

  return Result;
}

/** The strtol, strtoll, strtoul, and strtoull functions convert the initial
    portion of the string pointed to by nptr to long int, long long int,
    unsigned long int, and unsigned long long int representation, respectively.
    First, they decompose the input string into three parts: an initial,
    possibly empty, sequence of white-space characters (as specified by the
    isspace function), a subject sequence resembling an integer represented in
    some radix determined by the value of base, and a final string of one or
    more unrecognized characters, including the terminating null character of
    the input string. Then, they attempt to convert the subject sequence to an
    integer, and return the result.

    If the value of base is zero, the expected form of the subject sequence is
    that of an integer constant, optionally preceded
    by a plus or minus sign, but not including an integer suffix. If the value
    of base is between 2 and 36 (inclusive), the expected form of the subject
    sequence is a sequence of letters and digits representing an integer with
    the radix specified by base, optionally preceded by a plus or minus sign,
    but not including an integer suffix. The letters from a (or A) through z
    (or Z) are ascribed the values 10 through 35; only letters and digits whose
    ascribed values are less than that of base are permitted. If the value of
    base is 16, the characters 0x or 0X may optionally precede the sequence of
    letters and digits, following the sign if present.

    The subject sequence is defined as the longest initial subsequence of the
    input string, starting with the first non-white-space character, that is of
    the expected form. The subject sequence contains no characters if the input
    string is empty or consists entirely of white space, or if the first
    non-white-space character is other than a sign or a permissible letter or digit.

    If the subject sequence has the expected form and the value of base is
    zero, the sequence of characters starting with the first digit is
    interpreted as an integer constant. If the subject sequence has the
    expected form and the value of base is between 2 and 36, it is used as the
    base for conversion, ascribing to each letter its value as given above. If
    the subject sequence begins with a minus sign, the value resulting from the
    conversion is negated (in the return type). A pointer to the final string
    is stored in the object pointed to by endptr, provided that endptr is
    not a null pointer.

    In other than the "C" locale, additional locale-specific subject sequence
    forms may be accepted.

    If the subject sequence is empty or does not have the expected form, no
    conversion is performed; the value of nptr is stored in the object pointed
    to by endptr, provided that endptr is not a null pointer.

  @return   The strtol, strtoll, strtoul, and strtoull functions return the
            converted value, if any. If no conversion could be performed, zero
            is returned. If the correct value is outside the range of
            representable values, LONG_MIN, LONG_MAX, LLONG_MIN, LLONG_MAX,
            ULONG_MAX, or ULLONG_MAX is returned (according to the return type
            and sign of the value, if any), and the value of the macro ERANGE
            is stored in errno.
**/
long
strtol (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  const char  *pEnd;
  long        Result = 0;
  long        Previous;
  int         temp;
  BOOLEAN     Negative = FALSE;

  pEnd = nptr;

  if ((base < 0) || (base == 1) || (base > 36)) {
    if (endptr != NULL) {
      *endptr = NULL;
    }

    return 0;
  }

  // Skip leading spaces.
  while (isspace (*nptr)) {
    ++nptr;
  }

  // Process Subject sequence: optional sign followed by digits.
  if (*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  } else if (*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }

  if (*nptr == '0') {
    /* Might be Octal or Hex */
    if (toupper (nptr[1]) == 'X') {
      /* Looks like Hex */
      if ((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base  = 16; /* In case base was 0 */
      }
    } else {
      /* Looks like Octal */
      if ((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }

  if (base == 0) {
    /* If still zero then must be decimal */
    base = 10;
  }

  if (*nptr  == '0') {
    for ( ; *nptr == '0'; ++nptr) {
      /* Skip any remaining leading zeros */
    }

    pEnd = nptr;
  }

  while ( isalnum (*nptr) && ((temp = Digit2Val (*nptr)) < base)) {
    Previous = Result;
    Result   = (Result * base) + (long int)temp;
    if ( Result <= Previous) {
      // Detect Overflow
      if (Negative) {
        Result = LONG_MIN;
      } else {
        Result = LONG_MAX;
      }

      Negative = FALSE;
      errno    = ERANGE;
      break;
    }

    pEnd = ++nptr;
  }

  if (Negative) {
    Result = -Result;
  }

  // Save pointer to final sequence
  if (endptr != NULL) {
    *endptr = (char *)pEnd;
  }

  return Result;
}

/** The strtoull function converts the initial portion of the string pointed to
    by nptr to unsigned long long int representation.

    See the description for strtol for more information.

  @return   The strtoull function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, ULLONG_MAX is
            returned and the value of the macro ERANGE is stored in errno.
**/
unsigned long long
strtoull (
  const char  *nptr,
  char        **endptr,
  int         base
  )
{
  const char          *pEnd;
  unsigned long long  Result = 0;
  unsigned long long  Previous;
  int                 temp;

  pEnd = nptr;

  if ((base < 0) || (base == 1) || (base > 36)) {
    if (endptr != NULL) {
      *endptr = NULL;
    }

    return 0;
  }

  // Skip leading spaces.
  while (isspace (*nptr)) {
    ++nptr;
  }

  // Process Subject sequence: optional + sign followed by digits.
  if (*nptr == '+') {
    ++nptr;
  }

  if (*nptr == '0') {
    /* Might be Octal or Hex */
    if (toupper (nptr[1]) == 'X') {
      /* Looks like Hex */
      if ((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base  = 16; /* In case base was 0 */
      }
    } else {
      /* Looks like Octal */
      if ((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }

  if (base == 0) {
    /* If still zero then must be decimal */
    base = 10;
  }

  if (*nptr  == '0') {
    for ( ; *nptr == '0'; ++nptr) {
      /* Skip any remaining leading zeros */
    }

    pEnd = nptr;
  }

  while ( isalnum (*nptr) && ((temp = Digit2Val (*nptr)) < base)) {
    Previous = Result;
    Result   = DivU64x32 (Result, base) + (unsigned long long)temp;
    if ( Result < Previous) {
      // If we overflowed
      Result = ULLONG_MAX;
      errno  = ERANGE;
      break;
    }

    pEnd = ++nptr;
  }

  // Save pointer to final sequence
  if (endptr != NULL) {
    *endptr = (char *)pEnd;
  }

  return Result;
}

/**
  edk2 Jansson port does not support doubles, simply return integer part.

  These conversion functions convert the initial portion of the string
  pointed to by nptr to double, float, and long double representation,
  respectively.

  The strtod(), strtof(), and strtold() functions return the converted
  value, if any.

  If endptr is not NULL, a pointer to the character after the last charac-
  ter used in the conversion is stored in the location referenced by
  endptr.

  If no conversion is performed, zero is returned and the value of nptr is
  stored in the location referenced by endptr.

  If the correct value would cause overflow, plus or minus HUGE_VAL,
  HUGE_VALF, or HUGE_VALL is returned (according to the sign and type of
  the return value), and ERANGE is stored in errno.  If the correct value
  would cause underflow, zero is returned and ERANGE is stored in errno.

  @return  Integer part of decimal number.
**/
double
strtod (
  const char *__restrict  nptr,
  char **__restrict       endptr
  )
{
  UINTN  Data;
  UINTN  StrLen;

  Data   = 0;
  StrLen = 0;

  if (nptr == NULL) {
    return (double)0;
  }

  AsciiStrDecimalToUintnS (nptr, NULL, &Data);
  DEBUG ((DEBUG_WARN, "%a: \"%a\" We don't support double type on edk2 yet. Only integer part is returned: %d\n", __func__, nptr, Data));

  //
  // Force endptr to the last position of nptr because caller may
  // check endptr and raise assertion. We don't support floating
  // number in edk2 so this prevents unecessary assertion from happening.
  //
  if (endptr != NULL) {
    StrLen  = AsciiStrLen (nptr);
    *endptr = (char *__restrict)nptr + StrLen;
  }

  return (double)Data;
}

static UINT8  BitMask[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

#define WHICH8(c)     ((unsigned char)(c) >> 3)
#define WHICH_BIT(c)  (BitMask[((c) & 0x7)])
#define BITMAP64  ((UINT64 *)bitmap)

static
void
BuildBitmap (
  unsigned char  *bitmap,
  const char     *s2,
  int            n
  )
{
  unsigned char  bit;
  int            index;

  // Initialize bitmap.  Bit 0 is always 1 which corresponds to '\0'
  for (BITMAP64[0] = index = 1; index < n; index++) {
    BITMAP64[index] = 0;
  }

  // Set bits in bitmap corresponding to the characters in s2
  for ( ; *s2 != '\0'; s2++) {
    index         = WHICH8 (*s2);
    bit           = WHICH_BIT (*s2);
    bitmap[index] = bitmap[index] | bit;
  }
}

/** The strpbrk function locates the first occurrence in the string pointed to
    by s1 of any character from the string pointed to by s2.

    @return   The strpbrk function returns a pointer to the character, or a
              null pointer if no character from s2 occurs in s1.
**/
char *
strpbrk (
  const char  *s1,
  const char  *s2
  )
{
  UINT8  bitmap[(((UCHAR_MAX + 1) / CHAR_BIT) + (CHAR_BIT - 1)) & ~7U];
  UINT8  bit;
  int    index;

  BuildBitmap (bitmap, s2, sizeof (bitmap) / sizeof (UINT64));

  for ( ; *s1 != '\0'; ++s1) {
    index = WHICH8 (*s1);
    bit   = WHICH_BIT (*s1);
    if ((bitmap[index] & bit) != 0) {
      return (char *)s1;
    }
  }

  return NULL;
}

/** The strerror function maps the number in errnum to a message string.
    Typically, the values for errnum come from errno, but strerror shall map
    any value of type int to a message.

    The implementation shall behave as if no library function calls the
    strerror function.

    @return   The strerror function returns a pointer to the string, the
              contents of which are locale specific.  The array pointed to
              shall not be modified by the program, but may be overwritten by
              a subsequent call to the strerror function.
**/
char *
strerror (
  int  errnum
  )
{
  return errnum_message;
}

/**
  Allocate and zero-initialize array.
**/
void *
calloc (
  size_t  Num,
  size_t  Size
  )
{
  void    *RetVal;
  size_t  NumSize;

  NumSize = Num * Size;
  RetVal  = NULL;
  if (NumSize != 0) {
    RetVal = malloc (NumSize);
    if ( RetVal != NULL) {
      (VOID)ZeroMem (RetVal, NumSize);
    }
  }

  DEBUG ((DEBUG_POOL, "0x%p = calloc(%d, %d)\n", RetVal, Num, Size));

  return RetVal;
}

//
//  The arrays give the cumulative number of days up to the first of the
//  month number used as the index (1 -> 12) for regular and leap years.
//  The value at index 13 is for the whole year.
//
UINTN  CumulativeDays[2][14] = {
  {
    0,
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
  },
  {
    0,
    0,
    31,
    31 + 29,
    31 + 29 + 31,
    31 + 29 + 31 + 30,
    31 + 29 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
  }
};

#define IsLeap(y)  (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define SECSPERMIN   (60)
#define SECSPERHOUR  (60 * 60)
#define SECSPERDAY   (24 * SECSPERHOUR)

/**
  Get the system time as seconds elapsed since midnight, January 1, 1970.
**/
time_t
time (
  time_t  *timer
  )
{
  EFI_TIME  Time;
  time_t    CalTime;
  UINTN     Year;

  //
  // Get the current time and date information
  //
  gRT->GetTime (&Time, NULL);

  //
  // Years Handling
  // UTime should now be set to 00:00:00 on Jan 1 of the current year.
  //
  for (Year = 1970, CalTime = 0; Year != Time.Year; Year++) {
    CalTime = CalTime + (time_t)(CumulativeDays[IsLeap (Year)][13] * SECSPERDAY);
  }

  //
  // Add in number of seconds for current Month, Day, Hour, Minute, Seconds, and TimeZone adjustment
  //
  CalTime = CalTime +
            (time_t)((Time.TimeZone != EFI_UNSPECIFIED_TIMEZONE) ? (Time.TimeZone * 60) : 0) +
            (time_t)(CumulativeDays[IsLeap (Time.Year)][Time.Month] * SECSPERDAY) +
            (time_t)(((Time.Day > 0) ? Time.Day - 1 : 0) * SECSPERDAY) +
            (time_t)(Time.Hour * SECSPERHOUR) +
            (time_t)(Time.Minute * 60) +
            (time_t)Time.Second;

  if (timer != NULL) {
    *timer = CalTime;
  }

  return CalTime;
}

/**
  Performs a quick sort
**/
void
qsort (
  void *base,
  size_t num,
  size_t width,
  int ( *compare )(const void *, const void *)
  )
{
  ASSERT (base    != NULL);
  ASSERT (compare != NULL);

  PerformQuickSort (base, (UINTN)num, (UINTN)width, (SORT_COMPARE)compare);
  return;
}

/**
  Get character from stream, we don't support file operastion on edk2 JSON library.

  @return Returns the character currently pointed by the internal file position indicator of the specified stream

**/
int
fgetc (
  FILE  *_File
  )
{
  return EOF;
}

/**
  Open stream file, we don't support file operastion on edk2 JSON library.

  @return 0 Unsupported

**/
FILE *
fopen (
  const char  *filename,
  const char  *mode
  )
{
  return NULL;
}

/**
  Read stream from file, we don't support file operastion on edk2 JSON library.

  @return 0 Unsupported

**/
size_t
fread (
  void    *ptr,
  size_t  size,
  size_t  count,
  FILE    *stream
  )
{
  return 0;
}

/**
  Write stream from file, we don't support file operastion on edk2 JSON library.

  @return 0 Unsupported

**/
size_t
fwrite (
  const void  *ptr,
  size_t      size,
  size_t      count,
  FILE        *stream
  )
{
  return 0;
}

/**
  Close file, we don't support file operastion on edk2 JSON library.

  @return 0 Unsupported

**/
int
fclose (
  FILE  *stream
  )
{
  return EOF;
}

/**
  Write the formatted string to file, we don't support file operastion on edk2 JSON library.

  @return 0 Unsupported

**/
int
fprintf (
  FILE        *stream,
  const char  *format,
  ...
  )
{
  return -1;
}

/**
  This function check if this is the formating string specifier.

  @param[in]      FormatString     A Null-terminated ASCII format string.
  @param[in,out]  CurrentPosition  The starting position at the given string to check for
                                   "[flags][width][.precision][length]s" string specifier.
  @param[in]      StrLength        Maximum string length.

  @return BOOLEAN   TRUE means this is the formating string specifier. CurrentPosition is
                    returned at the position of "s".
                    FALSE means this is not the formating string specifier.. CurrentPosition is
                    returned at the position of failed character.

**/
BOOLEAN
CheckFormatingString (
  IN     CONST CHAR8  *FormatString,
  IN OUT UINTN        *CurrentPosition,
  IN     UINTN        StrLength
  )
{
  CHAR8  FormatStringParamater;

  while (*(FormatString + *CurrentPosition) != 's') {
    //
    // Loop until reach character 's' if the formating string is
    // compliant with "[flags][width][.precision][length]" format for
    // the string specifier.
    //
    FormatStringParamater = *(FormatString + *CurrentPosition);
    if ((FormatStringParamater != '-') &&
        (FormatStringParamater != '+') &&
        (FormatStringParamater != '*') &&
        (FormatStringParamater != '.') &&
        !(((UINTN)FormatStringParamater >= (UINTN)'0') && ((UINTN)FormatStringParamater <= (UINTN)'9'))
        )
    {
      return FALSE;
    }

    (*CurrentPosition)++;
    if (*CurrentPosition >= StrLength) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  This function clones *FormatString however replaces "%s" with "%a" in the
  returned string.

  @param[in] A Null-terminated ASCII format string.

  @return The new format string. Caller has to free the memory of this string
          using FreePool().

**/
CHAR8 *
ReplaceUnicodeToAsciiStrFormat (
  IN CONST CHAR8  *FormatString
  )
{
  UINTN    FormatStrSize;
  UINTN    FormatStrIndex;
  UINTN    FormatStrSpecifier;
  BOOLEAN  PercentageMark;
  CHAR8    *TempFormatBuffer;
  BOOLEAN  IsFormatString;

  //
  // Error checking.
  //
  if (FormatString == NULL) {
    return NULL;
  }

  FormatStrSize = AsciiStrSize (FormatString);
  if (FormatStrSize == 0) {
    return NULL;
  }

  TempFormatBuffer = AllocatePool (FormatStrSize); // Allocate memory for the
                                                   // new string.
  if (TempFormatBuffer == NULL) {
    return NULL;
  }

  //
  // Clone *FormatString but replace "%s" wih "%a".
  // "%%" is not considered as the format tag.
  //
  PercentageMark = FALSE;
  FormatStrIndex = 0;
  while (FormatStrIndex < FormatStrSize) {
    if (PercentageMark == TRUE) {
      //
      // Previous character is "%".
      //
      PercentageMark = FALSE;
      if (*(FormatString + FormatStrIndex) != '%') {
        // Check if this is double "%".
        FormatStrSpecifier = FormatStrIndex;
        //
        // Check if this is the formating string specifier.
        //
        IsFormatString = CheckFormatingString (FormatString, &FormatStrSpecifier, FormatStrSize);
        if ((FormatStrSpecifier - FormatStrIndex) != 0) {
          CopyMem (
            (VOID *)(TempFormatBuffer + FormatStrIndex),
            (VOID *)(FormatString + FormatStrIndex),
            FormatStrSpecifier - FormatStrIndex
            );
        }

        FormatStrIndex = FormatStrSpecifier;
        if (IsFormatString == TRUE) {
          //
          // Replace 's' with 'a' which is printed in ASCII
          // format on edk2 environment.
          //
          *(TempFormatBuffer + FormatStrSpecifier) = 'a';
          FormatStrIndex++;
        }

        continue;
      }

      goto ContinueCheck;
    }

    if (*(FormatString + FormatStrIndex) == '%') {
      //
      // This character is "%", set the flag.
      //
      PercentageMark = TRUE;
    }

ContinueCheck:
    //
    // Clone character to the new string and advance FormatStrIndex
    // to process next character.
    //
    *(TempFormatBuffer + FormatStrIndex) = *(FormatString + FormatStrIndex);
    FormatStrIndex++;
  }

  return TempFormatBuffer;
}

/**
  This is the Redfish version of CRT vsnprintf function, this function replaces "%s" to
  "%a" before invoking AsciiVSPrint(). That is because "%s" is unicode base on edk2
  environment however "%s" is ascii code base on vsnprintf().
  See definitions of AsciiVSPrint() for the details.

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated ASCII format string.
  @param  Marker          VA_LIST marker for the variable argument list.

  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
RedfishAsciiVSPrint (
  OUT CHAR8        *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  IN  VA_LIST      Marker
  )
{
  CHAR8  *TempFormatBuffer;
  UINTN  LenStrProduced;

  //
  // Looking for "%s" in the format string and replace it
  // with "%a" for printing ASCII code characters on edk2
  // environment.
  //
  TempFormatBuffer = ReplaceUnicodeToAsciiStrFormat (FormatString);
  if (TempFormatBuffer == NULL) {
    return 0;
  }

  LenStrProduced = AsciiVSPrint (StartOfBuffer, BufferSize, (CONST CHAR8 *)TempFormatBuffer, Marker);
  FreePool (TempFormatBuffer);
  return LenStrProduced;
}

/**
  This is the Redfish version of CRT snprintf function, this function replaces "%s" to
  "%a" before invoking AsciiSPrint(). That is because "%s" is unicode base on edk2
  environment however "%s" is ascii code base on snprintf().
  See definitions of AsciiSPrint() for the details.

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated ASCII format string.
  @param  ...             Variable argument list whose contents are accessed based on the
                          format string specified by FormatString.

  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
RedfishAsciiSPrint (
  OUT CHAR8        *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  ...
  )
{
  VA_LIST  Marker;
  UINTN    LenStrProduced;

  VA_START (Marker, FormatString);
  LenStrProduced = RedfishAsciiVSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
  return LenStrProduced;
}
