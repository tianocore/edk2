/** @file
  Integer Numeric Conversion Functions.

  The atoi, atol, and atoll functions convert the initial portion of the string
  pointed to by nptr to int, long int, and long long int representation,
  respectively.  They are equivalent to:
    - atoi: (int)strtol(nptr, (char **)NULL, 10)
    - atol: strtol(nptr, (char **)NULL, 10)
    - atoll: strtoll(nptr, (char **)NULL, 10)

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>

#include  <LibConfig.h>

#include  <ctype.h>
#include  <errno.h>
#include  <limits.h>
#include  <stdlib.h>

/** The atoi function converts the initial portion of the string pointed to by
    nptr to int representation.  Except for the behavior on error, it is
    equivalent to:
      - (int)strtol(nptr, (char **)NULL, 10)

  @return   The atoi function returns the converted value.
**/
int
atoi(const char *nptr)
{
  int       Retval;
  BOOLEAN   Negative = FALSE;

  while(isspace((const unsigned char)*nptr)) ++nptr; // Skip leading spaces

  if(*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  }
  else if(*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }
  Retval = (int)AsciiStrDecimalToUintn(nptr);
  if(Negative) {
    Retval = -Retval;
  }
  return Retval;
}

/** The atol function converts the initial portion of the string pointed to by
    nptr to long int representation.  Except for the behavior on error, it is
    equivalent to:
      - strtol(nptr, (char **)NULL, 10)

  @return   The atol function returns the converted value.
**/
long int
atol(const char *nptr)
{
  long int  Retval;
  BOOLEAN   Negative = FALSE;

  while(isspace(*nptr)) ++nptr; // Skip leading spaces

  if(*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  }
  else if(*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }
  Retval = (long int)AsciiStrDecimalToUint64(nptr);
  if(Negative) {
    Retval = -Retval;
  }
  return Retval;
}

/** The atoll function converts the initial portion of the string pointed to by
    nptr to long long int representation.  Except for the behavior on error, it
    is equivalent to:
      - strtoll(nptr, (char **)NULL, 10)

  @return   The atoll function returns the converted value.
**/
long long int
atoll(const char *nptr)
{
  long long int   Retval;
  BOOLEAN         Negative = FALSE;

  while(isspace(*nptr)) ++nptr; // Skip leading spaces

  if(*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  }
  else if(*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }
  Retval = (long long int)AsciiStrDecimalToUint64(nptr);
  if(Negative) {
    Retval = -Retval;
  }
  return Retval;
}

static int
Digit2Val( int c)
{
  if(isalpha(c)) {  /* If c is one of [A-Za-z]... */
    c = toupper(c) - 7;   // Adjust so 'A' is ('9' + 1)
  }
  return c - '0';   // Value returned is between 0 and 35, inclusive.
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
strtol(const char * __restrict nptr, char ** __restrict endptr, int base)
{
  const char *pEnd;
  long        Result = 0;
  long        Previous;
  int         temp;
  BOOLEAN     Negative = FALSE;

  pEnd = nptr;

  if((base < 0) || (base == 1) || (base > 36)) {
    if(endptr != NULL) {
    *endptr = NULL;
    }
    return 0;
  }
  // Skip leading spaces.
  while(isspace(*nptr))   ++nptr;

  // Process Subject sequence: optional sign followed by digits.
  if(*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  }
  else if(*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }

  if(*nptr == '0') {  /* Might be Octal or Hex */
    if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
      if((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base = 16;  /* In case base was 0 */
      }
    }
    else {    /* Looks like Octal */
      if((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }
  if(base == 0) {   /* If still zero then must be decimal */
    base = 10;
  }
  if(*nptr  == '0') {
    for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
    pEnd = nptr;
  }

  while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
    Previous = Result;
    Result = (Result * base) + (long int)temp;
    if( Result <= Previous) {   // Detect Overflow
      if(Negative) {
        Result = LONG_MIN;
      }
      else {
        Result = LONG_MAX;
      }
      Negative = FALSE;
      errno = ERANGE;
      break;
    }
    pEnd = ++nptr;
  }
  if(Negative) {
    Result = -Result;
  }

  // Save pointer to final sequence
  if(endptr != NULL) {
    *endptr = (char *)pEnd;
  }
  return Result;
}

/** The strtoul function converts the initial portion of the string pointed to
    by nptr to unsigned long int representation.

    See the description for strtol for more information.

  @return   The strtoul function returns the converted value, if any. If no
            conversion could be performed, zero is returned. If the correct
            value is outside the range of representable values, ULONG_MAX is
            returned and the value of the macro ERANGE is stored in errno.
**/
unsigned long
strtoul(const char * __restrict nptr, char ** __restrict endptr, int base)
{
  const char     *pEnd;
  unsigned long   Result = 0;
  unsigned long   Previous;
  int             temp;

  pEnd = nptr;

  if((base < 0) || (base == 1) || (base > 36)) {
    if(endptr != NULL) {
    *endptr = NULL;
    }
    return 0;
  }
  // Skip leading spaces.
  while(isspace(*nptr))   ++nptr;

  // Process Subject sequence: optional + sign followed by digits.
  if(*nptr == '+') {
    ++nptr;
  }

  if(*nptr == '0') {  /* Might be Octal or Hex */
    if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
      if((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base = 16;  /* In case base was 0 */
      }
    }
    else {    /* Looks like Octal */
      if((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }
  if(base == 0) {   /* If still zero then must be decimal */
    base = 10;
  }
  if(*nptr  == '0') {
    for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
    pEnd = nptr;
  }

  while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
    Previous = Result;
    Result = (Result * base) + (unsigned long)temp;
    if( Result < Previous)  {   // If we overflowed
      Result = ULONG_MAX;
      errno = ERANGE;
      break;
    }
    pEnd = ++nptr;
  }

  // Save pointer to final sequence
  if(endptr != NULL) {
    *endptr = (char *)pEnd;
  }
  return Result;
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
strtoll(const char * __restrict nptr, char ** __restrict endptr, int base)
{
  const char *pEnd;
  long long   Result = 0;
  long long   Previous;
  int         temp;
  BOOLEAN     Negative = FALSE;

  pEnd = nptr;

  if((base < 0) || (base == 1) || (base > 36)) {
    if(endptr != NULL) {
    *endptr = NULL;
    }
    return 0;
  }
  // Skip leading spaces.
  while(isspace(*nptr))   ++nptr;

  // Process Subject sequence: optional sign followed by digits.
  if(*nptr == '+') {
    Negative = FALSE;
    ++nptr;
  }
  else if(*nptr == '-') {
    Negative = TRUE;
    ++nptr;
  }

  if(*nptr == '0') {  /* Might be Octal or Hex */
    if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
      if((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base = 16;  /* In case base was 0 */
      }
    }
    else {    /* Looks like Octal */
      if((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }
  if(base == 0) {   /* If still zero then must be decimal */
    base = 10;
  }
  if(*nptr  == '0') {
    for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
    pEnd = nptr;
  }

  while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
    Previous = Result;
    Result = (Result * base) + (long long int)temp;
    if( Result <= Previous) {   // Detect Overflow
      if(Negative) {
        Result = LLONG_MIN;
      }
      else {
        Result = LLONG_MAX;
      }
      Negative = FALSE;
      errno = ERANGE;
      break;
    }
    pEnd = ++nptr;
  }
  if(Negative) {
    Result = -Result;
  }

  // Save pointer to final sequence
  if(endptr != NULL) {
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
strtoull(const char * __restrict nptr, char ** __restrict endptr, int base)
{
  const char           *pEnd;
  unsigned long long    Result = 0;
  unsigned long long    Previous;
  int                   temp;

  pEnd = nptr;

  if((base < 0) || (base == 1) || (base > 36)) {
    if(endptr != NULL) {
    *endptr = NULL;
    }
    return 0;
  }
  // Skip leading spaces.
  while(isspace(*nptr))   ++nptr;

  // Process Subject sequence: optional + sign followed by digits.
  if(*nptr == '+') {
    ++nptr;
  }

  if(*nptr == '0') {  /* Might be Octal or Hex */
    if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
      if((base == 0) || (base == 16)) {
        nptr += 2;  /* Skip the "0X"      */
        base = 16;  /* In case base was 0 */
      }
    }
    else {    /* Looks like Octal */
      if((base == 0) || (base == 8)) {
        ++nptr;     /* Skip the leading "0" */
        base = 8;   /* In case base was 0   */
      }
    }
  }
  if(base == 0) {   /* If still zero then must be decimal */
    base = 10;
  }
  if(*nptr  == '0') {
    for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
    pEnd = nptr;
  }

  while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
    Previous = Result;
    Result = (Result * base) + (unsigned long long)temp;
    if( Result < Previous)  {   // If we overflowed
      Result = ULLONG_MAX;
      errno = ERANGE;
      break;
    }
    pEnd = ++nptr;
  }

  // Save pointer to final sequence
  if(endptr != NULL) {
    *endptr = (char *)pEnd;
  }
  return Result;
}
