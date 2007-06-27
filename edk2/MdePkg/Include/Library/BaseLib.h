/** @file
  Memory-only library functions with no library constructor/destructor

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_LIB__
#define __BASE_LIB__

//
// Definitions for architecture specific types
// These include SPIN_LOCK and BASE_LIBRARY_JUMP_BUFFER
//

//
// SPIN_LOCK
//
typedef volatile UINTN              SPIN_LOCK;

#if   defined (MDE_CPU_IA32)
//
// IA32 context buffer used by SetJump() and LongJump()
//
typedef struct {
  UINT32                            Ebx;
  UINT32                            Esi;
  UINT32                            Edi;
  UINT32                            Ebp;
  UINT32                            Esp;
  UINT32                            Eip;
} BASE_LIBRARY_JUMP_BUFFER;

#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT 8

#elif defined (MDE_CPU_IPF)

//
// IPF context buffer used by SetJump() and LongJump()
//
typedef struct {
  UINT64                            F2[2];
  UINT64                            F3[2];
  UINT64                            F4[2];
  UINT64                            F5[2];
  UINT64                            F16[2];
  UINT64                            F17[2];
  UINT64                            F18[2];
  UINT64                            F19[2];
  UINT64                            F20[2];
  UINT64                            F21[2];
  UINT64                            F22[2];
  UINT64                            F23[2];
  UINT64                            F24[2];
  UINT64                            F25[2];
  UINT64                            F26[2];
  UINT64                            F27[2];
  UINT64                            F28[2];
  UINT64                            F29[2];
  UINT64                            F30[2];
  UINT64                            F31[2];
  UINT64                            R4;
  UINT64                            R5;
  UINT64                            R6;
  UINT64                            R7;
  UINT64                            SP;
  UINT64                            BR0;
  UINT64                            BR1;
  UINT64                            BR2;
  UINT64                            BR3;
  UINT64                            BR4;
  UINT64                            BR5;
  UINT64                            InitialUNAT;
  UINT64                            AfterSpillUNAT;
  UINT64                            PFS;
  UINT64                            BSP;
  UINT64                            Predicates;
  UINT64                            LoopCount;
  UINT64                            FPSR;
} BASE_LIBRARY_JUMP_BUFFER;

#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT 0x10

#elif defined (MDE_CPU_X64)
//
// X64 context buffer used by SetJump() and LongJump()
//
typedef struct {
  UINT64                            Rbx;
  UINT64                            Rsp;
  UINT64                            Rbp;
  UINT64                            Rdi;
  UINT64                            Rsi;
  UINT64                            R12;
  UINT64                            R13;
  UINT64                            R14;
  UINT64                            R15;
  UINT64                            Rip;
} BASE_LIBRARY_JUMP_BUFFER;

#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT 8

#elif defined (MDE_CPU_EBC)
//
// EBC context buffer used by SetJump() and LongJump()
//
typedef struct {
  UINT64                            R0;
  UINT64                            R1;
  UINT64                            R2;
  UINT64                            R3;
  UINT64                            IP;
} BASE_LIBRARY_JUMP_BUFFER;

#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT 8

#else
#error Unknown Processor Type
#endif

//
// String Services
//

/**
  Copies one Null-terminated Unicode string to another Null-terminated Unicode
  string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destiantion

**/
CHAR16 *
EFIAPI
StrCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  );


/**
  Copies one Null-terminated Unicode string with a maximum length to another
  Null-terminated Unicode string with a maximum length and returns the new
  Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. At most, Length Unicode
  characters are copied from Source to Destination. If Length is 0, then
  Destination is returned unmodified. If Length is greater that the number of
  Unicode characters in Source, then Destination is padded with Null Unicode
  characters. If Source and Destination overlap, then the results are
  undefined.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit bounadry, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to copy.

  @return Destination

**/
CHAR16 *
EFIAPI
StrnCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length
  );


/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  String  Pointer to a Null-terminated Unicode string.

  @return The length of String.

**/
UINTN
EFIAPI
StrLen (
  IN      CONST CHAR16              *String
  );


/**
  Returns the size of a Null-terminated Unicode string in bytes, including the
  Null terminator.

  This function returns the size, in bytes, of the Null-terminated Unicode
  string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  String  Pointer to a Null-terminated Unicode string.

  @return The size of String.

**/
UINTN
EFIAPI
StrSize (
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

  If FirstString is NULL, then ASSERT().
  If FirstString is not aligned on a 16-bit boundary, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If SecondString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

**/
INTN
EFIAPI
StrCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString
  );


/**
  Compares two Null-terminated Unicode strings with maximum lengths, and
  returns the difference between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. At most, Length Unicode
  characters will be compared. If Length is 0, then 0 is returned. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched Unicode character in SecondString
  subtracted from the first mismatched Unicode character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and FirstString is not aligned on a 16-bit bounadary, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If Length > 0 and SecondString is not aligned on a 16-bit bounadary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.
  @param  Length        Maximum number of Unicode characters to compare.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

**/
INTN
EFIAPI
StrnCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString,
  IN      UINTN                     Length
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
  If Destination is not aligned on a 16-bit bounadary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit bounadary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destination

**/
CHAR16 *
EFIAPI
StrCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  );


/**
  Concatenates one Null-terminated Unicode string with a maximum length to the
  end of another Null-terminated Unicode string, and returns the concatenated
  Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination, and Destination is returned. At
  most, Length Unicode characters are concatenated from Source to the end of
  Destination, and Destination is always Null-terminated. If Length is 0, then
  Destination is returned unmodified. If Source and Destination overlap, then
  the results are undefined.

  If Destination is NULL, then ASSERT().
  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to concatenate from
                      Source.

  @return Destination

**/
CHAR16 *
EFIAPI
StrnCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length
  );

/**
  Returns the first occurance of a Null-terminated Unicode sub-string
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
  characters not including the Null-terminator, then ASSERT().

  @param  String				  Pointer to a Null-terminated Unicode string.
  @param  SearchString	Pointer to a Null-terminated Unicode string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @retval !NULL           If there is a match.

**/
CHAR16 *
EFIAPI
StrStr (
  IN      CONST CHAR16      	      *String,
  IN      CONST CHAR16      	      *SearchString
  );

/**
  Convert a Null-terminated Unicode decimal string to a value of
  type UINTN.

  This function returns a value of type UINTN by interpreting the contents
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
  to the range defined by UINTN, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains
  more than PcdMaximumUnicodeStringLength Unicode characters not including
  the Null-terminator, then ASSERT().

  @param  String			    Pointer to a Null-terminated Unicode string.

  @retval UINTN

**/
UINTN
EFIAPI
StrDecimalToUintn (
  IN      CONST CHAR16      	      *String
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
  more than PcdMaximumUnicodeStringLength Unicode characters not including
  the Null-terminator, then ASSERT().

  @param  String			    Pointer to a Null-terminated Unicode string.

  @retval UINT64

**/
UINT64
EFIAPI
StrDecimalToUint64 (
  IN      CONST CHAR16      	      *String
  );
 

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type UINTN.

  This function returns a value of type UINTN by interpreting the contents
  of the Unicode string specified by String as a hexadecimal number.
  The format of the input Unicode string String is:

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
  UINTN, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator,
  then ASSERT().

  @param  String			    Pointer to a Null-terminated Unicode string.

  @retval UINTN

**/
UINTN
EFIAPI
StrHexToUintn (
  IN      CONST CHAR16      	      *String
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
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator,
  then ASSERT().

  @param  String			    Pointer to a Null-terminated Unicode string.

  @retval UINT64

**/
UINT64
EFIAPI
StrHexToUint64 (
  IN      CONST CHAR16      	      *String
  );


/**
  Convert one Null-terminated Unicode string to a Null-terminated
  ASCII string and returns the ASCII string.

  This function converts the content of the Unicode string Source
  to the ASCII string Destination by copying the lower 8 bits of
  each Unicode character. It returns Destination.

  If any Unicode characters in Source contain non-zero value in
  the upper 8 bits, then ASSERT().

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and Source contains
  more than PcdMaximumUnicodeStringLength Unicode characters not including
  the Null-terminator, then ASSERT().

  If PcdMaximumAsciiStringLength is not zero, and Source contains more
  than PcdMaximumAsciiStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Source        Pointer to a Null-terminated Unicode string.
  @param  Destination   Pointer to a Null-terminated ASCII string.

  @reture Destination

**/
CHAR8 *
EFIAPI
UnicodeStrToAsciiStr (
  IN      CONST CHAR16      	      *Source,
  OUT 	  CHAR8  	                  *Destination
  );


/**
  Copies one Null-terminated ASCII string to another Null-terminated ASCII
  string and returns the new ASCII string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

  @return Destination

**/
CHAR8 *
EFIAPI
AsciiStrCpy (
  OUT     CHAR8                     *Destination,
  IN      CONST CHAR8               *Source
  );


/**
  Copies one Null-terminated ASCII string with a maximum length to another
  Null-terminated ASCII string with a maximum length and returns the new ASCII
  string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. At most, Length ASCII characters
  are copied from Source to Destination. If Length is 0, then Destination is
  returned unmodified. If Length is greater that the number of ASCII characters
  in Source, then Destination is padded with Null ASCII characters. If Source
  and Destination overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to copy.

  @return Destination

**/
CHAR8 *
EFIAPI
AsciiStrnCpy (
  OUT     CHAR8                     *Destination,
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length
  );


/**
  Returns the length of a Null-terminated ASCII string.

  This function returns the number of ASCII characters in the Null-terminated
  ASCII string specified by String.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String  Pointer to a Null-terminated ASCII string.

  @return The length of String.

**/
UINTN
EFIAPI
AsciiStrLen (
  IN      CONST CHAR8               *String
  );


/**
  Returns the size of a Null-terminated ASCII string in bytes, including the
  Null terminator.

  This function returns the size, in bytes, of the Null-terminated ASCII string
  specified by String.

  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String  Pointer to a Null-terminated ASCII string.

  @return The size of String.

**/
UINTN
EFIAPI
AsciiStrSize (
  IN      CONST CHAR8               *String
  );


/**
  Compares two Null-terminated ASCII strings, and returns the difference
  between the first mismatched ASCII characters.

  This function compares the Null-terminated ASCII string FirstString to the
  Null-terminated ASCII string SecondString. If FirstString is identical to
  SecondString, then 0 is returned. Otherwise, the value returned is the first
  mismatched ASCII character in SecondString subtracted from the first
  mismatched ASCII character in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

**/
INTN
EFIAPI
AsciiStrCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString
  );


/**
  Performs a case insensitive comparison of two Null-terminated ASCII strings,
  and returns the difference between the first mismatched ASCII characters.

  This function performs a case insensitive comparison of the Null-terminated
  ASCII string FirstString to the Null-terminated ASCII string SecondString. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched lower case ASCII character in
  SecondString subtracted from the first mismatched lower case ASCII character
  in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString using case insensitive
              comparisons.
  @retval !=0 FirstString is not identical to SecondString using case
              insensitive comparisons.

**/
INTN
EFIAPI
AsciiStriCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString
  );


/**
  Compares two Null-terminated ASCII strings with maximum lengths, and returns
  the difference between the first mismatched ASCII characters.

  This function compares the Null-terminated ASCII string FirstString to the
  Null-terminated ASCII  string SecondString. At most, Length ASCII characters
  will be compared. If Length is 0, then 0 is returned. If FirstString is
  identical to SecondString, then 0 is returned. Otherwise, the value returned
  is the first mismatched ASCII character in SecondString subtracted from the
  first mismatched ASCII character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

**/
INTN
EFIAPI
AsciiStrnCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString,
  IN      UINTN                     Length
  );


/**
  Concatenates one Null-terminated ASCII string to another Null-terminated
  ASCII string, and returns the concatenated ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents of
  Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination. The Null-terminated concatenated ASCII
  String is returned.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

  @return Destination

**/
CHAR8 *
EFIAPI
AsciiStrCat (
  IN OUT CHAR8    *Destination,
  IN CONST CHAR8  *Source
  );


/**
  Concatenates one Null-terminated ASCII string with a maximum length to the
  end of another Null-terminated ASCII string, and returns the concatenated
  ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents
  of Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination, and Destination is returned. At most,
  Length ASCII characters are concatenated from Source to the end of
  Destination, and Destination is always Null-terminated. If Length is 0, then
  Destination is returned unmodified. If Source and Destination overlap, then
  the results are undefined.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters not including the Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to concatenate from
                      Source.

  @return Destination

**/
CHAR8 *
EFIAPI
AsciiStrnCat (
  IN OUT  CHAR8                     *Destination,
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length
  );


/**
  Returns the first occurance of a Null-terminated ASCII sub-string
  in a Null-terminated ASCII string.

  This function scans the contents of the ASCII string specified by String
  and returns the first occurrence of SearchString. If SearchString is not
  found in String, then NULL is returned. If the length of SearchString is zero,
  then String is returned.

  If String is NULL, then ASSERT().
  If SearchString is NULL, then ASSERT().

  If PcdMaximumAsciiStringLength is not zero, and SearchString or
  String contains more than PcdMaximumAsciiStringLength Unicode characters
  not including the Null-terminator, then ASSERT().

  @param  String				  Pointer to a Null-terminated ASCII string.
  @param  SearchString	  Pointer to a Null-terminated ASCII string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @retval !NULL           If there is a match.

**/
CHAR8 *
EFIAPI
AsciiStrStr (
  IN      CONST CHAR8      	        *String,
  IN      CONST CHAR8      	        *SearchString
  );


/**
  Convert a Null-terminated ASCII decimal string to a value of type
  UINTN.

  This function returns a value of type UINTN by interpreting the contents
  of the ASCII string String as a decimal number. The format of the input
  ASCII string String is:

                    [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before the digits.
  The running zero in the beginning of [decimal digits] will be ignored. Then, the
  function stops at the first character that is a not a valid decimal character or
  Null-terminator, whichever on comes first.

  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, then 0 is returned.
  If the number represented by String overflows according to the range defined by
  UINTN, then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String			    Pointer to a Null-terminated ASCII string.

  @retval UINTN

**/
UINTN
EFIAPI
AsciiStrDecimalToUintn (
  IN      CONST CHAR8               *String
  );


/**
  Convert a Null-terminated ASCII decimal string to a value of type
  UINT64.

  This function returns a value of type UINT64 by interpreting the contents
  of the ASCII string String as a decimal number. The format of the input
  ASCII string String is:

                    [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before the digits.
  The running zero in the beginning of [decimal digits] will be ignored. Then, the
  function stops at the first character that is a not a valid decimal character or
  Null-terminator, whichever on comes first.

  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, then 0 is returned.
  If the number represented by String overflows according to the range defined by
  UINT64, then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String			    Pointer to a Null-terminated ASCII string.

  @retval UINT64

**/
UINT64
EFIAPI
AsciiStrDecimalToUint64 (
  IN      CONST CHAR8       	      *String
  );


/**
  Convert a Null-terminated ASCII hexadecimal string to a value of type UINTN.

  This function returns a value of type UINTN by interpreting the contents of
  the ASCII string String as a hexadecimal number. The format of the input ASCII
  string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If "x"
  appears in the input string, it must be prefixed with at least one 0. The function
  will ignore the pad space, which includes spaces or tab characters, before [zeros],
  [x] or [hexadecimal digits]. The running zero before [x] or [hexadecimal digits]
  will be ignored. Then, the decoding starts after [x] or the first valid hexadecimal
  digit. Then, the function stops at the first character that is a not a valid
  hexadecimal character or Null-terminator, whichever on comes first.

  If String has only pad spaces, then 0 is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, then
  0 is returned.

  If the number represented by String overflows according to the range defined by UINTN,
  then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero,
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including
  the Null-terminator, then ASSERT().

  @param  String			    Pointer to a Null-terminated ASCII string.

  @retval UINTN

**/
UINTN
EFIAPI
AsciiStrHexToUintn (
  IN      CONST CHAR8       	      *String
  );


/**
  Convert a Null-terminated ASCII hexadecimal string to a value of type UINT64.

  This function returns a value of type UINT64 by interpreting the contents of
  the ASCII string String as a hexadecimal number. The format of the input ASCII
  string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If "x"
  appears in the input string, it must be prefixed with at least one 0. The function
  will ignore the pad space, which includes spaces or tab characters, before [zeros],
  [x] or [hexadecimal digits]. The running zero before [x] or [hexadecimal digits]
  will be ignored. Then, the decoding starts after [x] or the first valid hexadecimal
  digit. Then, the function stops at the first character that is a not a valid
  hexadecimal character or Null-terminator, whichever on comes first.

  If String has only pad spaces, then 0 is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, then
  0 is returned.

  If the number represented by String overflows according to the range defined by UINT64,
  then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero,
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including
  the Null-terminator, then ASSERT().

  @param  String			    Pointer to a Null-terminated ASCII string.

  @retval UINT64

**/
UINT64
EFIAPI
AsciiStrHexToUint64 (
  IN      CONST CHAR8      	        *String
  );


/**
  Convert one Null-terminated ASCII string to a Null-terminated
  Unicode string and returns the Unicode string.

  This function converts the contents of the ASCII string Source to the Unicode
  string Destination, and returns Destination.  The function terminates the
  Unicode string Destination by appending a Null-terminator character at the end.
  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((AsciiStrLen (Source) + 1) * sizeof (CHAR16)) in bytes.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  Source        Pointer to a Null-terminated ASCII string.
  @param  Destination   Pointer to a Null-terminated Unicode string.

  @reture Destination

**/
CHAR16 *
EFIAPI
AsciiStrToUnicodeStr (
  IN      CONST CHAR8       	      *Source,
  OUT 	  CHAR16  	                *Destination
  );


/**
  Converts an 8-bit value to an 8-bit BCD value.

  Converts the 8-bit value specified by Value to BCD. The BCD value is
  returned.

  If Value >= 100, then ASSERT().

  @param  Value The 8-bit value to convert to BCD. Range 0..99.

  @return The BCD value

**/
UINT8
EFIAPI
DecimalToBcd8 (
  IN      UINT8                     Value
  );


/**
  Converts an 8-bit BCD value to an 8-bit value.

  Converts the 8-bit BCD value specified by Value to an 8-bit value. The 8-bit
  value is returned.

  If Value >= 0xA0, then ASSERT().
  If (Value & 0x0F) >= 0x0A, then ASSERT().

  @param  Value The 8-bit BCD value to convert to an 8-bit value.

  @return The 8-bit value is returned.

**/
UINT8
EFIAPI
BcdToDecimal8 (
  IN      UINT8                     Value
  );


//
// Linked List Functions and Macros
//

/**
  Initializes the head node of a doubly linked list that is declared as a
  global variable in a module.

  Initializes the forward and backward links of a new linked list. After
  initializing a linked list with this macro, the other linked list functions
  may be used to add and remove nodes from the linked list. This macro results
  in smaller executables by initializing the linked list in the data section,
  instead if calling the InitializeListHead() function to perform the
  equivalent operation.

  @param  ListHead  The head note of a list to initiailize.

**/
#define INITIALIZE_LIST_HEAD_VARIABLE(ListHead)  {&ListHead, &ListHead}


/**
  Initializes the head node of a doubly linked list, and returns the pointer to
  the head node of the doubly linked list.

  Initializes the forward and backward links of a new linked list. After
  initializing a linked list with this function, the other linked list
  functions may be used to add and remove nodes from the linked list. It is up
  to the caller of this function to allocate the memory for ListHead.

  If ListHead is NULL, then ASSERT().

  @param  ListHead  A pointer to the head node of a new doubly linked list.

  @return ListHead

**/
LIST_ENTRY *
EFIAPI
InitializeListHead (
  IN      LIST_ENTRY                *ListHead
  );


/**
  Adds a node to the beginning of a doubly linked list, and returns the pointer
  to the head node of the doubly linked list.

  Adds the node Entry at the beginning of the doubly linked list denoted by
  ListHead, and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly linked list.
  @param  Entry     A pointer to a node that is to be inserted at the beginning
                    of a doubly linked list.

  @return ListHead

**/
LIST_ENTRY *
EFIAPI
InsertHeadList (
  IN      LIST_ENTRY                *ListHead,
  IN      LIST_ENTRY                *Entry
  );


/**
  Adds a node to the end of a doubly linked list, and returns the pointer to
  the head node of the doubly linked list.

  Adds the node Entry to the end of the doubly linked list denoted by ListHead,
  and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly linked list.
  @param  Entry     A pointer to a node that is to be added at the end of the
                    doubly linked list.

  @return ListHead

**/
LIST_ENTRY *
EFIAPI
InsertTailList (
  IN      LIST_ENTRY                *ListHead,
  IN      LIST_ENTRY                *Entry
  );


/**
  Retrieves the first node of a doubly linked list.

  Returns the first node of a doubly linked list. List must have been
  initialized with InitializeListHead(). If List is empty, then NULL is
  returned.

  If List is NULL, then ASSERT().
  If List was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  List  A pointer to the head node of a doubly linked list.

  @return The first node of a doubly linked list.
  @retval NULL  The list is empty.

**/
LIST_ENTRY *
EFIAPI
GetFirstNode (
  IN      CONST LIST_ENTRY          *List
  );


/**
  Retrieves the next node of a doubly linked list.

  Returns the node of a doubly linked list that follows Node. List must have
  been initialized with InitializeListHead(). If List is empty, then List is
  returned.

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and List contains more than
  PcdMaximumLinkedListLenth nodes, then ASSERT().
  If Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly linked list.
  @param  Node  A pointer to a node in the doubly linked list.

  @return Pointer to the next node if one exists. Otherwise a null value which
          is actually List is returned.

**/
LIST_ENTRY *
EFIAPI
GetNextNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );


/**
  Checks to see if a doubly linked list is empty or not.

  Checks to see if the doubly linked list is empty. If the linked list contains
  zero nodes, this function returns TRUE. Otherwise, it returns FALSE.

  If ListHead is NULL, then ASSERT().
  If ListHead was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly linked list.

  @retval TRUE  The linked list is empty.
  @retval FALSE The linked list is not empty.

**/
BOOLEAN
EFIAPI
IsListEmpty (
  IN      CONST LIST_ENTRY          *ListHead
  );


/**
  Determines if a node in a doubly linked list is null.

  Returns FALSE if Node is one of the nodes in the doubly linked list specified
  by List. Otherwise, TRUE is returned. List must have been initialized with
  InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If Node is not a node in List and Node is not equal to List, then ASSERT().

  @param  List  A pointer to the head node of a doubly linked list.
  @param  Node  A pointer to a node in the doubly linked list.

  @retval TRUE  Node is one of the nodes in the doubly linked list.
  @retval FALSE Node is not one of the nodes in the doubly linked list.

**/
BOOLEAN
EFIAPI
IsNull (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );


/**
  Determines if a node the last node in a doubly linked list.

  Returns TRUE if Node is the last node in the doubly linked list specified by
  List. Otherwise, FALSE is returned. List must have been initialized with
  InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly linked list.
  @param  Node  A pointer to a node in the doubly linked list.

  @retval TRUE  Node is the last node in the linked list.
  @retval FALSE Node is not the last node in the linked list.

**/
BOOLEAN
EFIAPI
IsNodeAtEnd (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  );


/**
  Swaps the location of two nodes in a doubly linked list, and returns the
  first node after the swap.

  If FirstEntry is identical to SecondEntry, then SecondEntry is returned.
  Otherwise, the location of the FirstEntry node is swapped with the location
  of the SecondEntry node in a doubly linked list. SecondEntry must be in the
  same double linked list as FirstEntry and that double linked list must have
  been initialized with InitializeListHead(). SecondEntry is returned after the
  nodes are swapped.

  If FirstEntry is NULL, then ASSERT().
  If SecondEntry is NULL, then ASSERT().
  If SecondEntry and FirstEntry are not in the same linked list, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list containing the FirstEntry and SecondEntry nodes, including
  the FirstEntry and SecondEntry nodes, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  FirstEntry  A pointer to a node in a linked list.
  @param  SecondEntry A pointer to another node in the same linked list.

**/
LIST_ENTRY *
EFIAPI
SwapListEntries (
  IN      LIST_ENTRY                *FirstEntry,
  IN      LIST_ENTRY                *SecondEntry
  );


/**
  Removes a node from a doubly linked list, and returns the node that follows
  the removed node.

  Removes the node Entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by this node if that is required. On
  exit, the node following Entry in the doubly linked list is returned. If
  Entry is the only node in the linked list, then the head node of the linked
  list is returned.

  If Entry is NULL, then ASSERT().
  If Entry is the head node of an empty list, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list containing Entry, including the Entry node, is greater than
  or equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  Entry A pointer to a node in a linked list

  @return Entry

**/
LIST_ENTRY *
EFIAPI
RemoveEntryList (
  IN      CONST LIST_ENTRY          *Entry
  );

//
// Math Services
//

/**
  Shifts a 64-bit integer left between 0 and 63 bits. The low bits are filled
  with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the left by Count bits. The
  low Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift left.
  @param  Count   The number of bits to shift left.

  @return Operand << Count

**/
UINT64
EFIAPI
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

  @return Operand >> Count

**/
UINT64
EFIAPI
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );


/**
  Shifts a 64-bit integer right between 0 and 63 bits. The high bits are filled
  with original integer's bit 63. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to bit 63 of Operand.  The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand >> Count

**/
UINT64
EFIAPI
ARShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );


/**
  Rotates a 32-bit integer left between 0 and 31 bits, filling the low bits
  with the high bits that were rotated.

  This function rotates the 32-bit value Operand to the left by Count bits. The
  low Count bits are fill with the high Count bits of Operand. The rotated
  value is returned.

  If Count is greater than 31, then ASSERT().

  @param  Operand The 32-bit operand to rotate left.
  @param  Count   The number of bits to rotate left.

  @return Operand <<< Count

**/
UINT32
EFIAPI
LRotU32 (
  IN      UINT32                    Operand,
  IN      UINTN                     Count
  );


/**
  Rotates a 32-bit integer right between 0 and 31 bits, filling the high bits
  with the low bits that were rotated.

  This function rotates the 32-bit value Operand to the right by Count bits.
  The high Count bits are fill with the low Count bits of Operand. The rotated
  value is returned.

  If Count is greater than 31, then ASSERT().

  @param  Operand The 32-bit operand to rotate right.
  @param  Count   The number of bits to rotate right.

  @return Operand >>> Count

**/
UINT32
EFIAPI
RRotU32 (
  IN      UINT32                    Operand,
  IN      UINTN                     Count
  );


/**
  Rotates a 64-bit integer left between 0 and 63 bits, filling the low bits
  with the high bits that were rotated.

  This function rotates the 64-bit value Operand to the left by Count bits. The
  low Count bits are fill with the high Count bits of Operand. The rotated
  value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to rotate left.
  @param  Count   The number of bits to rotate left.

  @return Operand <<< Count

**/
UINT64
EFIAPI
LRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );


/**
  Rotates a 64-bit integer right between 0 and 63 bits, filling the high bits
  with the high low bits that were rotated.

  This function rotates the 64-bit value Operand to the right by Count bits.
  The high Count bits are fill with the low Count bits of Operand. The rotated
  value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to rotate right.
  @param  Count   The number of bits to rotate right.

  @return Operand >>> Count

**/
UINT64
EFIAPI
RRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  );


/**
  Returns the bit position of the lowest bit set in a 32-bit value.

  This function computes the bit position of the lowest bit set in the 32-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 31 is returned.

  @param  Operand The 32-bit operand to evaluate.

  @return Position of the lowest bit set in Operand if found.
  @retval -1 Operand is zero.

**/
INTN
EFIAPI
LowBitSet32 (
  IN      UINT32                    Operand
  );


/**
  Returns the bit position of the lowest bit set in a 64-bit value.

  This function computes the bit position of the lowest bit set in the 64-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 63 is returned.

  @param  Operand The 64-bit operand to evaluate.

  @return Position of the lowest bit set in Operand if found.
  @retval -1  Operand is zero.

**/
INTN
EFIAPI
LowBitSet64 (
  IN      UINT64                    Operand
  );


/**
  Returns the bit position of the highest bit set in a 32-bit value. Equivalent
  to log2(x).

  This function computes the bit position of the highest bit set in the 32-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 31 is returned.

  @param  Operand The 32-bit operand to evaluate.

  @return Position of the highest bit set in Operand if found.
  @retval -1  Operand is zero.

**/
INTN
EFIAPI
HighBitSet32 (
  IN      UINT32                    Operand
  );


/**
  Returns the bit position of the highest bit set in a 64-bit value. Equivalent
  to log2(x).

  This function computes the bit position of the highest bit set in the 64-bit
  value specified by Operand. If Operand is zero, then -1 is returned.
  Otherwise, a value between 0 and 63 is returned.

  @param  Operand The 64-bit operand to evaluate.

  @return Position of the highest bit set in Operand if found.
  @retval -1  Operand is zero.

**/
INTN
EFIAPI
HighBitSet64 (
  IN      UINT64                    Operand
  );


/**
  Returns the value of the highest bit set in a 32-bit value. Equivalent to
  1 << HighBitSet32(x).

  This function computes the value of the highest bit set in the 32-bit value
  specified by Operand. If Operand is zero, then zero is returned.

  @param  Operand The 32-bit operand to evaluate.

  @return 1 << HighBitSet32(Operand)
  @retval 0 Operand is zero.

**/
UINT32
EFIAPI
GetPowerOfTwo32 (
  IN      UINT32                    Operand
  );


/**
  Returns the value of the highest bit set in a 64-bit value. Equivalent to
  1 << HighBitSet64(x).

  This function computes the value of the highest bit set in the 64-bit value
  specified by Operand. If Operand is zero, then zero is returned.

  @param  Operand The 64-bit operand to evaluate.

  @return 1 << HighBitSet64(Operand)
  @retval 0 Operand is zero.

**/
UINT64
EFIAPI
GetPowerOfTwo64 (
  IN      UINT64                    Operand
  );


/**
  Switches the endianess of a 16-bit integer.

  This function swaps the bytes in a 16-bit unsigned value to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Operand A 16-bit unsigned value.

  @return The byte swaped Operand.

**/
UINT16
EFIAPI
SwapBytes16 (
  IN      UINT16                    Value
  );


/**
  Switches the endianess of a 32-bit integer.

  This function swaps the bytes in a 32-bit unsigned value to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Operand A 32-bit unsigned value.

  @return The byte swaped Operand.

**/
UINT32
EFIAPI
SwapBytes32 (
  IN      UINT32                    Value
  );


/**
  Switches the endianess of a 64-bit integer.

  This function swaps the bytes in a 64-bit unsigned value to switch the value
  from little endian to big endian or vice versa. The byte swapped value is
  returned.

  @param  Operand A 64-bit unsigned value.

  @return The byte swaped Operand.

**/
UINT64
EFIAPI
SwapBytes64 (
  IN      UINT64                    Value
  );


/**
  Multiples a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 64-bit unsigned result.

  This function multiples the 64-bit unsigned value Multiplicand by the 32-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  If the result overflows, then ASSERT().

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 32-bit unsigned value.

  @return Multiplicand * Multiplier

**/
UINT64
EFIAPI
MultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  );


/**
  Multiples a 64-bit unsigned integer by a 64-bit unsigned integer and
  generates a 64-bit unsigned result.

  This function multiples the 64-bit unsigned value Multiplicand by the 64-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  If the result overflows, then ASSERT().

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 64-bit unsigned value.

  @return Multiplicand * Multiplier

**/
UINT64
EFIAPI
MultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  );


/**
  Multiples a 64-bit signed integer by a 64-bit signed integer and generates a
  64-bit signed result.

  This function multiples the 64-bit signed value Multiplicand by the 64-bit
  signed value Multiplier and generates a 64-bit signed result. This 64-bit
  signed result is returned.

  If the result overflows, then ASSERT().

  @param  Multiplicand  A 64-bit signed value.
  @param  Multiplier    A 64-bit signed value.

  @return Multiplicand * Multiplier

**/
INT64
EFIAPI
MultS64x64 (
  IN      INT64                     Multiplicand,
  IN      INT64                     Multiplier
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
EFIAPI
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  );


/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 32-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 32-bit remainder. This function
  returns the 32-bit unsigned remainder.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend % Divisor

**/
UINT32
EFIAPI
ModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
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
EFIAPI
DivU64x32Remainder (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder  OPTIONAL
  );


/**
  Divides a 64-bit unsigned integer by a 64-bit unsigned integer and generates
  a 64-bit unsigned result and an optional 64-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 64-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 64-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 64-bit unsigned value.
  @param  Remainder A pointer to a 64-bit unsigned value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
UINT64
EFIAPI
DivU64x64Remainder (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder  OPTIONAL
  );


/**
  Divides a 64-bit signed integer by a 64-bit signed integer and generates a
  64-bit signed result and a optional 64-bit signed remainder.

  This function divides the 64-bit signed value Dividend by the 64-bit signed
  value Divisor and generates a 64-bit signed quotient. If Remainder is not
  NULL, then the 64-bit signed remainder is returned in Remainder. This
  function returns the 64-bit signed quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit signed value.
  @param  Divisor   A 64-bit signed value.
  @param  Remainder A pointer to a 64-bit signed value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
INT64
EFIAPI
DivS64x64Remainder (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
  );


/**
  Reads a 16-bit value from memory that may be unaligned.

  This function returns the 16-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 16-bit value that may be unaligned.

  @return *Uint16

**/
UINT16
EFIAPI
ReadUnaligned16 (
  IN      CONST UINT16              *Uint16
  );


/**
  Writes a 16-bit value to memory that may be unaligned.

  This function writes the 16-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 16-bit value that may be unaligned.
  @param  Value   16-bit value to write to Buffer.

  @return Value

**/
UINT16
EFIAPI
WriteUnaligned16 (
  OUT     UINT16                    *Uint16,
  IN      UINT16                    Value
  );


/**
  Reads a 24-bit value from memory that may be unaligned.

  This function returns the 24-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 24-bit value that may be unaligned.

  @return The value read.

**/
UINT32
EFIAPI
ReadUnaligned24 (
  IN      CONST UINT32              *Buffer
  );


/**
  Writes a 24-bit value to memory that may be unaligned.

  This function writes the 24-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 24-bit value that may be unaligned.
  @param  Value   24-bit value to write to Buffer.

  @return The value written.

**/
UINT32
EFIAPI
WriteUnaligned24 (
  OUT     UINT32                    *Buffer,
  IN      UINT32                    Value
  );


/**
  Reads a 32-bit value from memory that may be unaligned.

  This function returns the 32-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 32-bit value that may be unaligned.

  @return *Uint32

**/
UINT32
EFIAPI
ReadUnaligned32 (
  IN      CONST UINT32              *Uint32
  );


/**
  Writes a 32-bit value to memory that may be unaligned.

  This function writes the 32-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 32-bit value that may be unaligned.
  @param  Value   32-bit value to write to Buffer.

  @return Value

**/
UINT32
EFIAPI
WriteUnaligned32 (
  OUT     UINT32                    *Uint32,
  IN      UINT32                    Value
  );


/**
  Reads a 64-bit value from memory that may be unaligned.

  This function returns the 64-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 64-bit value that may be unaligned.

  @return *Uint64

**/
UINT64
EFIAPI
ReadUnaligned64 (
  IN      CONST UINT64              *Uint64
  );


/**
  Writes a 64-bit value to memory that may be unaligned.

  This function writes the 64-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  Pointer to a 64-bit value that may be unaligned.
  @param  Value   64-bit value to write to Buffer.

  @return Value

**/
UINT64
EFIAPI
WriteUnaligned64 (
  OUT     UINT64                    *Uint64,
  IN      UINT64                    Value
  );


//
// Bit Field Functions
//

/**
  Returns a bit field from an 8-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.

  @return The bit field read.

**/
UINT8
EFIAPI
BitFieldRead8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to an 8-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 8-bit value is
  returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  Value     New value of the bit field.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldWrite8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     Value
  );


/**
  Reads a bit field from an 8-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise inclusive OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  OrData    The value to OR with the read value from the value

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     OrData
  );


/**
  Reads a bit field from an 8-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the read value from the value.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldAnd8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData
  );


/**
  Reads a bit field from an 8-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise
  inclusive OR with value specified by OrData. All other bits in Operand are
  preserved. The new 8-bit value is returned.

  If 8-bit operations are not supported, then ASSERT().
  If StartBit is greater than 7, then ASSERT().
  If EndBit is greater than 7, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..7.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..7.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 8-bit value.

**/
UINT8
EFIAPI
BitFieldAndThenOr8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT8                     AndData,
  IN      UINT8                     OrData
  );


/**
  Returns a bit field from a 16-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.

  @return The bit field read.

**/
UINT16
EFIAPI
BitFieldRead16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to a 16-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 16-bit value is
  returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  Value     New value of the bit field.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldWrite16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    Value
  );


/**
  Reads a bit field from a 16-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise inclusive OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  OrData    The value to OR with the read value from the value

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    OrData
  );


/**
  Reads a bit field from a 16-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the read value from the value

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldAnd16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData
  );


/**
  Reads a bit field from a 16-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise
  inclusive OR with value specified by OrData. All other bits in Operand are
  preserved. The new 16-bit value is returned.

  If 16-bit operations are not supported, then ASSERT().
  If StartBit is greater than 15, then ASSERT().
  If EndBit is greater than 15, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..15.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..15.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 16-bit value.

**/
UINT16
EFIAPI
BitFieldAndThenOr16 (
  IN      UINT16                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT16                    AndData,
  IN      UINT16                    OrData
  );


/**
  Returns a bit field from a 32-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.

  @return The bit field read.

**/
UINT32
EFIAPI
BitFieldRead32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to a 32-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 32-bit value is
  returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  Value     New value of the bit field.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldWrite32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  );


/**
  Reads a bit field from a 32-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise inclusive OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  OrData    The value to OR with the read value from the value

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  );


/**
  Reads a bit field from a 32-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the value

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldAnd32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  );


/**
  Reads a bit field from a 32-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise
  inclusive OR with value specified by OrData. All other bits in Operand are
  preserved. The new 32-bit value is returned.

  If 32-bit operations are not supported, then ASSERT().
  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 32-bit value.

**/
UINT32
EFIAPI
BitFieldAndThenOr32 (
  IN      UINT32                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  );


/**
  Returns a bit field from a 64-bit value.

  Returns the bitfield specified by the StartBit and the EndBit from Operand.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.

  @return The bit field read.

**/
UINT64
EFIAPI
BitFieldRead64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to a 64-bit value, and returns the result.

  Writes Value to the bit field specified by the StartBit and the EndBit in
  Operand. All other bits in Operand are preserved. The new 64-bit value is
  returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  Value     New value of the bit field.

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldWrite64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    Value
  );


/**
  Reads a bit field from a 64-bit value, performs a bitwise OR, and returns the
  result.

  Performs a bitwise inclusive OR between the bit field specified by StartBit
  and EndBit in Operand and the value specified by OrData. All other bits in
  Operand are preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  OrData    The value to OR with the read value from the value

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    OrData
  );


/**
  Reads a bit field from a 64-bit value, performs a bitwise AND, and returns
  the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData. All other bits in Operand are
  preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the value

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldAnd64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData
  );


/**
  Reads a bit field from a 64-bit value, performs a bitwise AND followed by a
  bitwise OR, and returns the result.

  Performs a bitwise AND between the bit field specified by StartBit and EndBit
  in Operand and the value specified by AndData, followed by a bitwise
  inclusive OR with value specified by OrData. All other bits in Operand are
  preserved. The new 64-bit value is returned.

  If 64-bit operations are not supported, then ASSERT().
  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Operand   Operand on which to perform the bitfield operation.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the value.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The new 64-bit value.

**/
UINT64
EFIAPI
BitFieldAndThenOr64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  );


//
// Base Library Synchronization Functions
//

/**
  Retrieves the architecture specific spin lock alignment requirements for
  optimal spin lock performance.

  This function retrieves the spin lock alignment requirements for optimal
  performance on a given CPU architecture. The spin lock alignment must be a
  power of two and is returned by this function. If there are no alignment
  requirements, then 1 must be returned. The spin lock synchronization
  functions must function correctly if the spin lock size and alignment values
  returned by this function are not used at all. These values are hints to the
  consumers of the spin lock synchronization functions to obtain optimal spin
  lock performance.

  @return The architecture specific spin lock alignment.

**/
UINTN
EFIAPI
GetSpinLockProperties (
  VOID
  );


/**
  Initializes a spin lock to the released state and returns the spin lock.

  This function initializes the spin lock specified by SpinLock to the released
  state, and returns SpinLock. Optimal performance can be achieved by calling
  GetSpinLockProperties() to determine the size and alignment requirements for
  SpinLock.

  If SpinLock is NULL, then ASSERT().

  @param  SpinLock  A pointer to the spin lock to initialize to the released
                    state.

  @return SpinLock

**/
SPIN_LOCK *
EFIAPI
InitializeSpinLock (
  IN      SPIN_LOCK                 *SpinLock
  );


/**
  Waits until a spin lock can be placed in the acquired state.

  This function checks the state of the spin lock specified by SpinLock. If
  SpinLock is in the released state, then this function places SpinLock in the
  acquired state and returns SpinLock. Otherwise, this function waits
  indefinitely for the spin lock to be released, and then places it in the
  acquired state and returns SpinLock. All state transitions of SpinLock must
  be performed using MP safe mechanisms.

  If SpinLock is NULL, then ASSERT().
  If SpinLock was not initialized with InitializeSpinLock(), then ASSERT().
  If PcdSpinLockTimeout is not zero, and SpinLock is can not be acquired in
  PcdSpinLockTimeout microseconds, then ASSERT().

  @param  SpinLock  A pointer to the spin lock to place in the acquired state.

  @return SpinLock

**/
SPIN_LOCK *
EFIAPI
AcquireSpinLock (
  IN      SPIN_LOCK                 *SpinLock
  );


/**
  Attempts to place a spin lock in the acquired state.

  This function checks the state of the spin lock specified by SpinLock. If
  SpinLock is in the released state, then this function places SpinLock in the
  acquired state and returns TRUE. Otherwise, FALSE is returned. All state
  transitions of SpinLock must be performed using MP safe mechanisms.

  If SpinLock is NULL, then ASSERT().
  If SpinLock was not initialized with InitializeSpinLock(), then ASSERT().

  @param  SpinLock  A pointer to the spin lock to place in the acquired state.

  @retval TRUE  SpinLock was placed in the acquired state.
  @retval FALSE SpinLock could not be acquired.

**/
BOOLEAN
EFIAPI
AcquireSpinLockOrFail (
  IN      SPIN_LOCK                 *SpinLock
  );


/**
  Releases a spin lock.

  This function places the spin lock specified by SpinLock in the release state
  and returns SpinLock.

  If SpinLock is NULL, then ASSERT().
  If SpinLock was not initialized with InitializeSpinLock(), then ASSERT().

  @param  SpinLock  A pointer to the spin lock to release.

  @return SpinLock

**/
SPIN_LOCK *
EFIAPI
ReleaseSpinLock (
  IN      SPIN_LOCK                 *SpinLock
  );


/**
  Performs an atomic increment of an 32-bit unsigned integer.

  Performs an atomic increment of the 32-bit unsigned integer specified by
  Value and returns the incremented value. The increment operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  If Value is NULL, then ASSERT().

  @param  Value A pointer to the 32-bit value to increment.

  @return The incremented value.

**/
UINT32
EFIAPI
InterlockedIncrement (
  IN      UINT32                    *Value
  );


/**
  Performs an atomic decrement of an 32-bit unsigned integer.

  Performs an atomic decrement of the 32-bit unsigned integer specified by
  Value and returns the decremented value. The decrement operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  If Value is NULL, then ASSERT().

  @param  Value A pointer to the 32-bit value to decrement.

  @return The decremented value.

**/
UINT32
EFIAPI
InterlockedDecrement (
  IN      UINT32                    *Value
  );


/**
  Performs an atomic compare exchange operation on a 32-bit unsigned integer.

  Performs an atomic compare exchange operation on the 32-bit unsigned integer
  specified by Value.  If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned.  If Value is not equal to CompareValue,
  then Value is returned.  The compare exchange operation must be performed using
  MP safe mechanisms.

  If Value is NULL, then ASSERT().

  @param  Value         A pointer to the 32-bit value for the compare exchange
                        operation.
  @param  CompareValue  32-bit value used in compare operation.
  @param  ExchangeValue 32-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT32
EFIAPI
InterlockedCompareExchange32 (
  IN OUT  UINT32                    *Value,
  IN      UINT32                    CompareValue,
  IN      UINT32                    ExchangeValue
  );


/**
  Performs an atomic compare exchange operation on a 64-bit unsigned integer.

  Performs an atomic compare exchange operation on the 64-bit unsigned integer specified
  by Value.  If Value is equal to CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to CompareValue, then Value is returned.
  The compare exchange operation must be performed using MP safe mechanisms.

  If Value is NULL, then ASSERT().

  @param  Value         A pointer to the 64-bit value for the compare exchange
                        operation.
  @param  CompareValue  64-bit value used in compare operation.
  @param  ExchangeValue 64-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT64
EFIAPI
InterlockedCompareExchange64 (
  IN OUT  UINT64                    *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  );


/**
  Performs an atomic compare exchange operation on a pointer value.

  Performs an atomic compare exchange operation on the pointer value specified
  by Value. If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned. If Value is not equal to
  CompareValue, then Value is returned. The compare exchange operation must be
  performed using MP safe mechanisms.

  If Value is NULL, then ASSERT().

  @param  Value         A pointer to the pointer value for the compare exchange
                        operation.
  @param  CompareValue  Pointer value used in compare operation.
  @param  ExchangeValue Pointer value used in exchange operation.

**/
VOID *
EFIAPI
InterlockedCompareExchangePointer (
  IN OUT  VOID                      **Value,
  IN      VOID                      *CompareValue,
  IN      VOID                      *ExchangeValue
  );


//
// Base Library Checksum Functions
//

/**
  Calculate the sum of all elements in a buffer in unit of UINT8.
  During calculation, the carry bits are dropped.

  This function calculates the sum of all elements in a buffer
  in unit of UINT8. The carry bits in result of addition are dropped.
  The result is returned as UINT8. If Length is Zero, then Zero is
  returned.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the sum operation.
  @param  Length	    The size, in bytes, of Buffer .

  @return Sum         The sum of Buffer with carry bits dropped during additions.

**/
UINT8
EFIAPI
CalculateSum8 (
  IN      CONST UINT8     	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the two's complement checksum of all elements in a buffer
  of 8-bit values.

  This function first calculates the sum of the 8-bit values in the
  buffer specified by Buffer and Length.  The carry bits in the result
  of addition are dropped. Then, the two's complement of the sum is
  returned.  If Length is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().


  @param  Buffer			Pointer to the buffer to carry out the checksum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Checksum	  The 2's complement checksum of Buffer.

**/
UINT8
EFIAPI
CalculateCheckSum8 (
  IN      CONST UINT8     	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the sum of all elements in a buffer of 16-bit values.  During
  calculation, the carry bits are dropped.

  This function calculates the sum of the 16-bit values in the buffer
  specified by Buffer and Length. The carry bits in result of addition are dropped.
  The 16-bit result is returned.  If Length is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If Length is not aligned on a 16-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the sum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Sum         The sum of Buffer with carry bits dropped during additions.

**/
UINT16
EFIAPI
CalculateSum16 (
  IN      CONST UINT16    	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the two's complement checksum of all elements in a buffer of
  16-bit values.

  This function first calculates the sum of the 16-bit values in the buffer
  specified by Buffer and Length.  The carry bits in the result of addition
  are dropped. Then, the two's complement of the sum is returned.  If Length
  is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If Length is not aligned on a 16-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the checksum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Checksum	  The 2's complement checksum of Buffer.

**/
UINT16
EFIAPI
CalculateCheckSum16 (
  IN      CONST UINT16    	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the sum of all elements in a buffer of 32-bit values.  During
  calculation, the carry bits are dropped.

  This function calculates the sum of the 32-bit values in the buffer
  specified by Buffer and Length. The carry bits in result of addition are dropped.
  The 32-bit result is returned.  If Length is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 32-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the sum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Sum         The sum of Buffer with carry bits dropped during additions.

**/
UINT32
EFIAPI
CalculateSum32 (
  IN      CONST UINT32    	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the two's complement checksum of all elements in a buffer of
  32-bit values.

  This function first calculates the sum of the 32-bit values in the buffer
  specified by Buffer and Length.  The carry bits in the result of addition
  are dropped. Then, the two's complement of the sum is returned.  If Length
  is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 32-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the checksum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Checksum	  The 2's complement checksum of Buffer.

**/
UINT32
EFIAPI
CalculateCheckSum32 (
  IN      CONST UINT32    	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the sum of all elements in a buffer of 64-bit values.  During
  calculation, the carry bits are dropped.

  This function calculates the sum of the 64-bit values in the buffer
  specified by Buffer and Length. The carry bits in result of addition are dropped.
  The 64-bit result is returned.  If Length is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is not aligned on a 64-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the sum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Sum         The sum of Buffer with carry bits dropped during additions.

**/
UINT64
EFIAPI
CalculateSum64 (
  IN      CONST UINT64    	        *Buffer,
  IN      UINTN		  	              Length
  );


/**
  Returns the two's complement checksum of all elements in a buffer of
  64-bit values.

  This function first calculates the sum of the 64-bit values in the buffer
  specified by Buffer and Length.  The carry bits in the result of addition
  are dropped. Then, the two's complement of the sum is returned.  If Length
  is 0, then 0 is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is not aligned on a 64-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer			Pointer to the buffer to carry out the checksum operation.
  @param  Length	    The size, in bytes, of Buffer.

  @return Checksum	  The 2's complement checksum of Buffer.

**/
UINT64
EFIAPI
CalculateCheckSum64 (
  IN      CONST UINT64    	        *Buffer,
  IN      UINTN		  	              Length
  );


//
// Base Library CPU Functions
//
typedef
VOID
(EFIAPI *SWITCH_STACK_ENTRY_POINT) (
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2   OPTIONAL
  );


/**
  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

**/
VOID
EFIAPI
MemoryFence (
  VOID
  );


/**
  Saves the current CPU context that can be restored with a call to LongJump()
  and returns 0.

  Saves the current CPU context in the buffer specified by JumpBuffer and
  returns 0. The initial call to SetJump() must always return 0. Subsequent
  calls to LongJump() cause a non-zero value to be returned by SetJump().

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  JumpBuffer  A pointer to CPU context buffer.

  @retval 0 Indicates a return from SetJump().

**/
UINTN
EFIAPI
SetJump (
  OUT     BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
  );


/**
  Restores the CPU context that was saved with SetJump().

  Restores the CPU context from the buffer specified by JumpBuffer. This
  function never returns to the caller. Instead is resumes execution based on
  the state of JumpBuffer.

  If JumpBuffer is NULL, then ASSERT().
  For IPF CPUs, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().
  If Value is 0, then ASSERT().

  @param  JumpBuffer  A pointer to CPU context buffer.
  @param  Value       The value to return when the SetJump() context is
                      restored and must be non-zero.

**/
VOID
EFIAPI
LongJump (
  IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
  IN      UINTN                     Value
  );


/**
  Enables CPU interrupts.

  Enables CPU interrupts.

**/
VOID
EFIAPI
EnableInterrupts (
  VOID
  );


/**
  Disables CPU interrupts.

  Disables CPU interrupts.

**/
VOID
EFIAPI
DisableInterrupts (
  VOID
  );


/**
  Disables CPU interrupts and returns the interrupt state prior to the disable
  operation.

  Disables CPU interrupts and returns the interrupt state prior to the disable
  operation.

  @retval TRUE  CPU interrupts were enabled on entry to this call.
  @retval FALSE CPU interrupts were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndDisableInterrupts (
  VOID
  );


/**
  Enables CPU interrupts for the smallest window required to capture any
  pending interrupts.

  Enables CPU interrupts for the smallest window required to capture any
  pending interrupts.

**/
VOID
EFIAPI
EnableDisableInterrupts (
  VOID
  );


/**
  Retrieves the current CPU interrupt state.

  Retrieves the current CPU interrupt state. Returns TRUE is interrupts are
  currently enabled. Otherwise returns FALSE.

  @retval TRUE  CPU interrupts are enabled.
  @retval FALSE CPU interrupts are disabled.

**/
BOOLEAN
EFIAPI
GetInterruptState (
  VOID
  );


/**
  Set the current CPU interrupt state.

  Sets the current CPU interrupt state to the state specified by
  InterruptState. If InterruptState is TRUE, then interrupts are enabled. If
  InterruptState is FALSE, then interrupts are disabled. InterruptState is
  returned.

  @param  InterruptState  TRUE if interrupts should enabled. FALSE if
                          interrupts should be disabled.

  @return InterruptState

**/
BOOLEAN
EFIAPI
SetInterruptState (
  IN      BOOLEAN                   InterruptState
  );


/**
  Places the CPU in a sleep state until an interrupt is received.

  Places the CPU in a sleep state until an interrupt is received. If interrupts
  are disabled prior to calling this function, then the CPU will be placed in a
  sleep state indefinitely.

**/
VOID
EFIAPI
CpuSleep (
  VOID
  );


/**
  Requests CPU to pause for a short period of time.

  Requests CPU to pause for a short period of time. Typically used in MP
  systems to prevent memory starvation while waiting for a spin lock.

**/
VOID
EFIAPI
CpuPause (
  VOID
  );


/**
  Flushes all the Translation Lookaside Buffers(TLB) entries in a CPU.

  Flushes all the Translation Lookaside Buffers(TLB) entries in a CPU.

**/
VOID
EFIAPI
CpuFlushTlb (
  VOID
  );


/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the
  new stack specified by NewStack and passing in the parameters specified
  by Context1 and Context2.  Context1 and Context2 are optional and may
  be NULL.  The function EntryPoint must never return.  This function
  supports a variable number of arguments following the NewStack parameter.
  These additional arguments are ignored on IA-32, x64, and EBC.
  IPF CPUs expect one additional parameter of type VOID * that specifies
  the new backing store pointer.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.

**/
VOID
EFIAPI
SwitchStack (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack,
  ...
  );


/**
  Generates a breakpoint on the CPU.

  Generates a breakpoint on the CPU. The breakpoint must be implemented such
  that code can resume normal execution after the breakpoint.

**/
VOID
EFIAPI
CpuBreakpoint (
  VOID
  );


/**
  Executes an infinite loop.

  Forces the CPU to execute an infinite loop. A debugger may be used to skip
  past the loop and the code that follows the loop must execute properly. This
  implies that the infinite loop must not cause the code that follow it to be
  optimized away.

**/
VOID
EFIAPI
CpuDeadLoop (
  VOID
  );


#if defined (MDE_CPU_IPF)

/**
  Flush a range of  cache lines in the cache coherency domain of the calling
  CPU.

  Invalidates the  cache lines specified by Address and Length. If Address is
  not aligned on a cache line boundary, then entire cache line containing
  Address is invalidated. If Address + Length is not aligned on a cache line
  boundary, then the entire instruction cache line containing Address + Length
  -1 is invalidated. This function may choose to invalidate the entire
  instruction cache if that is more efficient than invalidating the specified
  range. If Length is 0, the no instruction cache lines are invalidated.
  Address is returned.

  If Length is greater than (MAX_ADDRESS - Address + 1), then ASSERT().

  @param  Address The base address of the instruction lines to invalidate. If
                  the CPU is in a physical addressing mode, then Address is a
                  physical address. If the CPU is in a virtual addressing mode,
                  then Address is a virtual address.

  @param  Length  The number of bytes to invalidate from the instruction cache.

  @return Address

**/
VOID *
EFIAPI
IpfFlushCacheRange (
  IN      VOID                      *Address,
  IN      UINTN                     Length
  );


/**
  Executes a FC instruction
  Executes a FC instruction on the cache line specified by Address.
  The cache line size affected is at least 32-bytes (aligned on a 32-byte boundary).
  An implementation may flush a larger region.  This function is only available on IPF.

  @param Address  	The Address of cache line to be flushed.

  @return The address of FC instruction executed.

**/
UINT64
EFIAPI
AsmFc (
  IN  UINT64  Address
  );


/**
  Executes a FC.I instruction.
  Executes a FC.I instruction on the cache line specified by Address.
  The cache line size affected is at least 32-bytes (aligned on a 32-byte boundary).
  An implementation may flush a larger region.  This function is only available on IPF.

  @param Address  	The Address of cache line to be flushed.

  @return The address of FC.I instruction executed.

**/
UINT64
EFIAPI
AsmFci (
  IN  UINT64  Address
  );


/**
  Reads the current value of a Processor Identifier Register (CPUID).
  The Index of largest implemented CPUID (One less than the number of implemented CPUID
  registers) is determined by CPUID [3] bits {7:0}.
  No parameter checking is performed on Index.  If the Index value is beyond the
  implemented CPUID register range, a Reserved Register/Field fault may occur.  The caller
  must either guarantee that Index is valid, or the caller must set up fault handlers to
  catch the faults.  This function is only available on IPF.

  @param Index		The 8-bit Processor Identifier Register index to read.

  @return The current value of Processor Identifier Register specified by Index.

**/
UINT64
EFIAPI
AsmReadCpuid (
  IN  UINT8   Index
  );


/**
  Reads the current value of 64-bit Processor Status Register (PSR).
  This function is only available on IPF.

  @return The current value of PSR.

**/
UINT64
EFIAPI
AsmReadPsr (
  VOID
  );


/**
  Writes the current value of 64-bit Processor Status Register (PSR).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of PSR must be 0 or a Reserved Register/Field fault may occur.  The caller must either guarantee that Value is valid, or the caller must set up fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to PSR.

  @return The 64-bit value written to the PSR.

**/
UINT64
EFIAPI
AsmWritePsr (
  IN UINT64  Value
  );


/**
  Reads the current value of 64-bit Kernel Register #0 (KR0).
  This function is only available on IPF.

  @return The current value of KR0.

**/
UINT64
EFIAPI
AsmReadKr0 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #1 (KR1).
  This function is only available on IPF.

  @return The current value of KR1.

**/
UINT64
EFIAPI
AsmReadKr1 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #2 (KR2).
  This function is only available on IPF.

  @return The current value of KR2.

**/
UINT64
EFIAPI
AsmReadKr2 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #3 (KR3).
  This function is only available on IPF.

  @return The current value of KR3.

**/
UINT64
EFIAPI
AsmReadKr3 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #4 (KR4).
  This function is only available on IPF.

  @return The current value of KR4.

**/
UINT64
EFIAPI
AsmReadKr4 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #5 (KR5).
  This function is only available on IPF.

  @return The current value of KR5.

**/
UINT64
EFIAPI
AsmReadKr5 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #6 (KR6).
  This function is only available on IPF.

  @return The current value of KR6.

**/
UINT64
EFIAPI
AsmReadKr6 (
  VOID
  );


/**
  Reads the current value of 64-bit Kernel Register #7 (KR7).
  This function is only available on IPF.

  @return The current value of KR7.

**/
UINT64
EFIAPI
AsmReadKr7 (
  VOID
  );


/**
  Write the current value of 64-bit Kernel Register #0 (KR0).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR0.

  @return The 64-bit value written to the KR0.

**/
UINT64
EFIAPI
AsmWriteKr0 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #1 (KR1).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR1.

  @return The 64-bit value written to the KR1.

**/
UINT64
EFIAPI
AsmWriteKr1 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #2 (KR2).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR2.

  @return The 64-bit value written to the KR2.

**/
UINT64
EFIAPI
AsmWriteKr2 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #3 (KR3).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR3.

  @return The 64-bit value written to the KR3.

**/
UINT64
EFIAPI
AsmWriteKr3 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #4 (KR4).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR4.

  @return The 64-bit value written to the KR4.

**/
UINT64
EFIAPI
AsmWriteKr4 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #5 (KR5).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR5.

  @return The 64-bit value written to the KR5.

**/
UINT64
EFIAPI
AsmWriteKr5 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #6 (KR6).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR6.

  @return The 64-bit value written to the KR6.

**/
UINT64
EFIAPI
AsmWriteKr6 (
  IN UINT64  Value
  );


/**
  Write the current value of 64-bit Kernel Register #7 (KR7).
  This function is only available on IPF.

  @param  Value 	The 64-bit value to write to KR7.

  @return The 64-bit value written to the KR7.

**/
UINT64
EFIAPI
AsmWriteKr7 (
  IN UINT64  Value
  );


/**
  Reads the current value of Interval Timer Counter Register (ITC).
  This function is only available on IPF.

  @return The current value of ITC.

**/
UINT64
EFIAPI
AsmReadItc (
  VOID
  );


/**
  Reads the current value of Interval Timer Vector Register (ITV).
  This function is only available on IPF.

  @return The current value of ITV.

**/
UINT64
EFIAPI
AsmReadItv (
  VOID
  );


/**
  Reads the current value of Interval Timer Match Register (ITM).
  This function is only available on IPF.

  @return The current value of ITM.
**/
UINT64
EFIAPI
AsmReadItm (
  VOID
  );


/**
  Writes the current value of 64-bit Interval Timer Counter Register (ITC).
  This function is only available on IPF.

  @param Value		The 64-bit value to write to ITC.

  @return The 64-bit value written to the ITC.

**/
UINT64
EFIAPI
AsmWriteItc (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Interval Timer Match Register (ITM).
  This function is only available on IPF.

  @param Value		The 64-bit value to write to ITM.

  @return The 64-bit value written to the ITM.

**/
UINT64
EFIAPI
AsmWriteItm (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Interval Timer Vector Register (ITV).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of ITV must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to ITV.

  @return The 64-bit value written to the ITV.

**/
UINT64
EFIAPI
AsmWriteItv (
  IN UINT64  Value
  );


/**
  Reads the current value of Default Control Register (DCR).
  This function is only available on IPF.

  @return The current value of DCR.

**/
UINT64
EFIAPI
AsmReadDcr (
  VOID
  );


/**
  Reads the current value of Interruption Vector Address Register (IVA).
  This function is only available on IPF.

  @return The current value of IVA.
**/
UINT64
EFIAPI
AsmReadIva (
  VOID
  );


/**
  Reads the current value of Page Table Address Register (PTA).
  This function is only available on IPF.

  @return The current value of PTA.

**/
UINT64
EFIAPI
AsmReadPta (
  VOID
  );


/**
  Writes the current value of 64-bit Default Control Register (DCR).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of DCR must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to DCR.

  @return The 64-bit value written to the DCR.

**/
UINT64
EFIAPI
AsmWriteDcr (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Interruption Vector Address Register (IVA).
  The size of vector table is 32 K bytes and is 32 K bytes aligned
  the low 15 bits of Value is ignored when written.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to IVA.

  @return The 64-bit value written to the IVA.

**/
UINT64
EFIAPI
AsmWriteIva (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Page Table Address Register (PTA).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of DCR must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to PTA.

  @return The 64-bit value written to the PTA.
**/
UINT64
EFIAPI
AsmWritePta (
  IN UINT64  Value
  );


/**
  Reads the current value of Local Interrupt ID Register (LID).
  This function is only available on IPF.

  @return The current value of LID.

**/
UINT64
EFIAPI
AsmReadLid (
  VOID
  );


/**
  Reads the current value of External Interrupt Vector Register (IVR).
  This function is only available on IPF.

  @return The current value of IVR.

**/
UINT64
EFIAPI
AsmReadIvr (
  VOID
  );


/**
  Reads the current value of Task Priority Register (TPR).
  This function is only available on IPF.

  @return The current value of TPR.

**/
UINT64
EFIAPI
AsmReadTpr (
  VOID
  );


/**
  Reads the current value of External Interrupt Request Register #0 (IRR0).
  This function is only available on IPF.

  @return The current value of IRR0.

**/
UINT64
EFIAPI
AsmReadIrr0 (
  VOID
  );


/**
  Reads the current value of External Interrupt Request Register #1 (IRR1).
  This function is only available on IPF.

  @return The current value of IRR1.

**/
UINT64
EFIAPI
AsmReadIrr1 (
  VOID
  );


/**
  Reads the current value of External Interrupt Request Register #2 (IRR2).
  This function is only available on IPF.

  @return The current value of IRR2.

**/
UINT64
EFIAPI
AsmReadIrr2 (
  VOID
  );


/**
  Reads the current value of External Interrupt Request Register #3 (IRR3).
  This function is only available on IPF.

  @return The current value of IRR3.

**/
UINT64
EFIAPI
AsmReadIrr3 (
  VOID
  );


/**
  Reads the current value of Performance Monitor Vector Register (PMV).
  This function is only available on IPF.

  @return The current value of PMV.

**/
UINT64
EFIAPI
AsmReadPmv (
  VOID
  );


/**
  Reads the current value of Corrected Machine Check Vector Register (CMCV).
  This function is only available on IPF.

  @return The current value of CMCV.

**/
UINT64
EFIAPI
AsmReadCmcv (
  VOID
  );


/**
  Reads the current value of Local Redirection Register #0 (LRR0).
  This function is only available on IPF.

  @return The current value of LRR0.

**/
UINT64
EFIAPI
AsmReadLrr0 (
  VOID
  );


/**
  Reads the current value of Local Redirection Register #1 (LRR1).
  This function is only available on IPF.

  @return The current value of LRR1.

**/
UINT64
EFIAPI
AsmReadLrr1 (
  VOID
  );


/**
  Writes the current value of 64-bit Page Local Interrupt ID Register (LID).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of LID must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to LID.

  @return The 64-bit value written to the LID.

**/
UINT64
EFIAPI
AsmWriteLid (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Task Priority Register (TPR).
  No parameter checking is performed on Value.  All bits of Value corresponding to
  reserved fields of TPR must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to TPR.

  @return The 64-bit value written to the TPR.

**/
UINT64
EFIAPI
AsmWriteTpr (
  IN UINT64  Value
  );


/**
  Performs a write operation on End OF External Interrupt Register (EOI).
  Writes a value of 0 to the EOI Register.  This function is only available on IPF.

**/
VOID
EFIAPI
AsmWriteEoi (
  VOID
  );


/**
  Writes the current value of 64-bit Performance Monitor Vector Register (PMV).
  No parameter checking is performed on Value.  All bits of Value corresponding
  to reserved fields of PMV must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to PMV.

  @return The 64-bit value written to the PMV.

**/
UINT64
EFIAPI
AsmWritePmv (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Corrected Machine Check Vector Register (CMCV).
  No parameter checking is performed on Value.  All bits of Value corresponding
  to reserved fields of CMCV must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to CMCV.

  @return The 64-bit value written to the CMCV.

**/
UINT64
EFIAPI
AsmWriteCmcv (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Local Redirection Register #0 (LRR0).
  No parameter checking is performed on Value.  All bits of Value corresponding
  to reserved fields of LRR0 must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to LRR0.

  @return The 64-bit value written to the LRR0.

**/
UINT64
EFIAPI
AsmWriteLrr0 (
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Local Redirection Register #1 (LRR1).
  No parameter checking is performed on Value.  All bits of Value corresponding
  to reserved fields of LRR1 must be 0 or a Reserved Register/Field fault may occur.
  The caller must either guarantee that Value is valid, or the caller must
  set up fault handlers to catch the faults.
  This function is only available on IPF.

  @param Value		The 64-bit value to write to LRR1.

  @return The 64-bit value written to the LRR1.

**/
UINT64
EFIAPI
AsmWriteLrr1 (
  IN UINT64  Value
  );


/**
  Reads the current value of Instruction Breakpoint Register (IBR).
  
  The Instruction Breakpoint Registers are used in pairs.  The even numbered
  registers contain breakpoint addresses, and the odd numbered registers contain
  breakpoint mask conditions.  At least 4 instruction registers pairs are implemented
  on all processor models.   Implemented registers are contiguous starting with
  register 0.  No parameter checking is performed on Index, and if the Index value
  is beyond the implemented IBR register range, a Reserved Register/Field fault may
  occur.  The caller must either guarantee that Index is valid, or the caller must
  set up fault handlers to catch the faults.
  This function is only available on IPF.

  @param Index		The 8-bit Instruction Breakpoint Register index to read.

  @return The current value of Instruction Breakpoint Register specified by Index.

**/
UINT64
EFIAPI
AsmReadIbr (
  IN  UINT8   Index
  );


/**
  Reads the current value of Data Breakpoint Register (DBR).

  The Data Breakpoint Registers are used in pairs.  The even numbered registers
  contain breakpoint addresses, and odd numbered registers contain breakpoint
  mask conditions.  At least 4 data registers pairs are implemented on all processor
  models.  Implemented registers are contiguous starting with register 0.
  No parameter checking is performed on Index.  If the Index value is beyond
  the implemented DBR register range, a Reserved Register/Field fault may occur.
  The caller must either guarantee that Index is valid, or the caller must set up
  fault handlers to catch the faults.
  This function is only available on IPF.

  @param Index		The 8-bit Data Breakpoint Register index to read.

  @return The current value of Data Breakpoint Register specified by Index.

**/
UINT64
EFIAPI
AsmReadDbr (
  IN  UINT8   Index
  );


/**
  Reads the current value of Performance Monitor Configuration Register (PMC).

  All processor implementations provide at least 4 performance counters
  (PMC/PMD [4]...PMC/PMD [7] pairs), and 4 performance monitor counter overflow
  status registers (PMC [0]¡­ PMC [3]).  Processor implementations may provide
  additional implementation-dependent PMC and PMD to increase the number of
  ¡®generic¡¯ performance counters (PMC/PMD pairs).  The remainder of PMC and PMD
  register set is implementation dependent.  No parameter checking is performed
  on Index.  If the Index value is beyond the implemented PMC register range,
  zero value will be returned.
  This function is only available on IPF.

  @param Index		The 8-bit Performance Monitor Configuration Register index to read.

  @return The current value of Performance Monitor Configuration Register
  specified by Index.

**/
UINT64
EFIAPI
AsmReadPmc (
  IN  UINT8   Index
  );


/**
  Reads the current value of Performance Monitor Data Register (PMD).

  All processor implementations provide at least 4 performance counters
  (PMC/PMD [4]...PMC/PMD [7] pairs), and 4 performance monitor counter
  overflow status registers (PMC [0]¡­ PMC [3]).  Processor implementations may
  provide additional implementation-dependent PMC and PMD to increase the number
  of ¡®generic¡¯ performance counters (PMC/PMD pairs).  The remainder of PMC and PMD
  register set is implementation dependent.  No parameter checking is performed
  on Index.  If the Index value is beyond the implemented PMD register range,
  zero value will be returned.
  This function is only available on IPF.

  @param Index		The 8-bit Performance Monitor Data Register index to read.

  @return The current value of Performance Monitor Data Register specified by Index.

**/
UINT64
EFIAPI
AsmReadPmd (
  IN  UINT8   Index
  );


/**
  Writes the current value of 64-bit Instruction Breakpoint Register (IBR).

  Writes current value of Instruction Breakpoint Register specified by Index.
  The Instruction Breakpoint Registers are used in pairs.  The even numbered
  registers contain breakpoint addresses, and odd numbered registers contain
  breakpoint mask conditions.  At least 4 instruction registers pairs are implemented
  on all processor models.  Implemented registers are contiguous starting with
  register 0.  No parameter checking is performed on Index.  If the Index value
  is beyond the implemented IBR register range, a Reserved Register/Field fault may
  occur.  The caller must either guarantee that Index is valid, or the caller must
  set up fault handlers to catch the faults.
  This function is only available on IPF.

  @param Index		The 8-bit Instruction Breakpoint Register index to write.
  @param Value		The 64-bit value to write to IBR.

  @return The 64-bit value written to the IBR.

**/
UINT64
EFIAPI
AsmWriteIbr (
  IN UINT8   Index,
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Data Breakpoint Register (DBR).

  Writes current value of Data Breakpoint Register specified by Index.
  The Data Breakpoint Registers are used in pairs.  The even numbered registers
  contain breakpoint addresses, and odd numbered registers contain breakpoint
  mask conditions.  At least 4 data registers pairs are implemented on all processor
  models.  Implemented registers are contiguous starting with register 0.  No parameter
  checking is performed on Index.  If the Index value is beyond the implemented
  DBR register range, a Reserved Register/Field fault may occur.  The caller must
  either guarantee that Index is valid, or the caller must set up fault handlers to
  catch the faults.
  This function is only available on IPF.

  @param Index		The 8-bit Data Breakpoint Register index to write.
  @param Value		The 64-bit value to write to DBR.

  @return The 64-bit value written to the DBR.

**/
UINT64
EFIAPI
AsmWriteDbr (
  IN UINT8   Index,
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Performance Monitor Configuration Register (PMC).

  Writes current value of Performance Monitor Configuration Register specified by Index.
  All processor implementations provide at least 4 performance counters
  (PMC/PMD [4]...PMC/PMD [7] pairs), and 4 performance monitor counter overflow status
  registers (PMC [0]¡­ PMC [3]).  Processor implementations may provide additional
  implementation-dependent PMC and PMD to increase the number of ¡®generic¡¯ performance
  counters (PMC/PMD pairs).  The remainder of PMC and PMD register set is implementation
  dependent.  No parameter checking is performed on Index.  If the Index value is
  beyond the implemented PMC register range, the write is ignored.
  This function is only available on IPF.

  @param Index		The 8-bit Performance Monitor Configuration Register index to write.
  @param Value		The 64-bit value to write to PMC.

  @return The 64-bit value written to the PMC.

**/
UINT64
EFIAPI
AsmWritePmc (
  IN UINT8   Index,
  IN UINT64  Value
  );


/**
  Writes the current value of 64-bit Performance Monitor Data Register (PMD).

  Writes current value of Performance Monitor Data Register specified by Index.
  All processor implementations provide at least 4 performance counters
  (PMC/PMD [4]...PMC/PMD [7] pairs), and 4 performance monitor counter overflow
  status registers (PMC [0]¡­ PMC [3]).  Processor implementations may provide
  additional implementation-dependent PMC and PMD to increase the number of ¡®generic¡¯
  performance counters (PMC/PMD pairs).  The remainder of PMC and PMD register set
  is implementation dependent.  No parameter checking is performed on Index.  If the
  Index value is beyond the implemented PMD register range, the write is ignored.
  This function is only available on IPF.

  @param Index		The 8-bit Performance Monitor Data Register index to write.
  @param Value		The 64-bit value to write to PMD.

  @return The 64-bit value written to the PMD.

**/
UINT64
EFIAPI
AsmWritePmd (
  IN UINT8   Index,
  IN UINT64  Value
  );


/**
  Reads the current value of 64-bit Global Pointer (GP).

  Reads and returns the current value of GP.
  This function is only available on IPF.

  @return The current value of GP.

**/
UINT64
EFIAPI
AsmReadGp (
  VOID
  );


/**
  Write the current value of 64-bit Global Pointer (GP).

  Writes the current value of GP. The 64-bit value written to the GP is returned.
  No parameter checking is performed on Value.
  This function is only available on IPF.

  @param Value  The 64-bit value to write to GP.

  @return The 64-bit value written to the GP.

**/
UINT64
EFIAPI
AsmWriteGp (
  IN UINT64  Value
  );


/**
  Reads the current value of 64-bit Stack Pointer (SP).

  Reads and returns the current value of SP.
  This function is only available on IPF.

  @return The current value of SP.

**/
UINT64
EFIAPI
AsmReadSp (
  VOID
  );


/**
  Determines if the CPU is currently executing in virtual, physical, or mixed mode.

  Determines the current execution mode of the CPU.
  If the CPU is in virtual mode(PSR.RT=1, PSR.DT=1, PSR.IT=1), then 1 is returned.
  If the CPU is in physical mode(PSR.RT=0, PSR.DT=0, PSR.IT=0), then 0 is returned.
  If the CPU is not in physical mode or virtual mode, then it is in mixed mode,
  and -1 is returned.
  This function is only available on IPF.

  @return  1  The CPU is in virtual mode.
  @return  0  The CPU is in physical mode.
  @return -1  The CPU is in mixed mode.

**/
INT64
EFIAPI
AsmCpuVirtual (
  VOID
  );


/**
  Makes a PAL procedure call.

  This is a wrapper function to make a PAL procedure call.  Based on the Index
  value this API will make static or stacked PAL call.  The following table
  describes the usage of PAL Procedure Index Assignment. Architected procedures
  may be designated as required or optional.  If a PAL procedure is specified
  as optional, a unique return code of 0xFFFFFFFFFFFFFFFF is returned in the
  Status field of the PAL_CALL_RETURN structure.
  This indicates that the procedure is not present in this PAL implementation.
  It is the caller¡¯s responsibility to check for this return code after calling
  any optional PAL procedure.
  No parameter checking is performed on the 5 input parameters, but there are
  some common rules that the caller should follow when making a PAL call.  Any
  address passed to PAL as buffers for return parameters must be 8-byte aligned.
  Unaligned addresses may cause undefined results.  For those parameters defined
  as reserved or some fields defined as reserved must be zero filled or the invalid
  argument return value may be returned or undefined result may occur during the
  execution of the procedure.  If the PalEntryPoint  does not point to a valid
  PAL entry point then the system behavior is undefined.  This function is only
  available on IPF.

  @param PalEntryPoint	The PAL procedure calls entry point.
  @param Index			    The PAL procedure Index number.
  @param Arg2			      The 2nd parameter for PAL procedure calls.
  @param Arg3			      The 3rd parameter for PAL procedure calls.
  @param Arg4			      The 4th parameter for PAL procedure calls.

  @return structure returned from the PAL Call procedure, including the status and return value.

**/
PAL_CALL_RETURN
EFIAPI
AsmPalCall (
  IN UINT64  PalEntryPoint,
  IN UINT64  Index,
  IN UINT64  Arg2,
  IN UINT64  Arg3,
  IN UINT64  Arg4
  );


/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new memory location for RSE backing
                      store.

**/
VOID
EFIAPI
AsmSwitchStackAndBackingStore (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack,
  IN      VOID                      *NewBsp
  );


//
// Bugbug: This call should be removed after
//         the PalCall Instance issue has been fixed.
//
/**
  Performs a PAL call using static calling convention.

  An internal function to perform a PAL call using static calling convention.

  @param  PalEntryPoint The entry point address of PAL. The address in ar.kr5
                        would be used if this parameter were NULL on input.
  @param  Arg1          The first argument of a PAL call.
  @param  Arg1          The second argument of a PAL call.
  @param  Arg1          The third argument of a PAL call.
  @param  Arg1          The fourth argument of a PAL call.

  @return The values returned in r8, r9, r10 and r11.

**/
PAL_CALL_RETURN
PalCallStatic (
  IN      CONST VOID                *PalEntryPoint,
  IN      UINT64                    Arg1,
  IN      UINT64                    Arg2,
  IN      UINT64                    Arg3,
  IN      UINT64                    Arg4
  );


#elif defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
//
// IA32 and X64 Specific Functions
//
//
// Byte packed structure for 16-bit Real Mode EFLAGS
//
typedef union {
  struct {
    UINT32  CF:1;           // Carry Flag
    UINT32  Reserved_0:1;   // Reserved
    UINT32  PF:1;           // Parity Flag
    UINT32  Reserved_1:1;   // Reserved
    UINT32  AF:1;           // Auxiliary Carry Flag
    UINT32  Reserved_2:1;   // Reserved
    UINT32  ZF:1;           // Zero Flag
    UINT32  SF:1;           // Sign Flag
    UINT32  TF:1;           // Trap Flag
    UINT32  IF:1;           // Interrupt Enable Flag
    UINT32  DF:1;           // Direction Flag
    UINT32  OF:1;           // Overflow Flag
    UINT32  IOPL:2;         // I/O Privilege Level
    UINT32  NT:1;           // Nested Task
    UINT32  Reserved_3:1;   // Reserved
  } Bits;
  UINT16    Uint16;
} IA32_FLAGS16;

//
// Byte packed structure for EFLAGS/RFLAGS
// 32-bits on IA-32
// 64-bits on X64.  The upper 32-bits on X64 are reserved
//
typedef union {
  struct {
    UINT32  CF:1;           // Carry Flag
    UINT32  Reserved_0:1;   // Reserved
    UINT32  PF:1;           // Parity Flag
    UINT32  Reserved_1:1;   // Reserved
    UINT32  AF:1;           // Auxiliary Carry Flag
    UINT32  Reserved_2:1;   // Reserved
    UINT32  ZF:1;           // Zero Flag
    UINT32  SF:1;           // Sign Flag
    UINT32  TF:1;           // Trap Flag
    UINT32  IF:1;           // Interrupt Enable Flag
    UINT32  DF:1;           // Direction Flag
    UINT32  OF:1;           // Overflow Flag
    UINT32  IOPL:2;         // I/O Privilege Level
    UINT32  NT:1;           // Nested Task
    UINT32  Reserved_3:1;   // Reserved
    UINT32  RF:1;           // Resume Flag
    UINT32  VM:1;           // Virtual 8086 Mode
    UINT32  AC:1;           // Alignment Check
    UINT32  VIF:1;          // Virtual Interrupt Flag
    UINT32  VIP:1;          // Virtual Interrupt Pending
    UINT32  ID:1;           // ID Flag
    UINT32  Reserved_4:10;  // Reserved
  } Bits;
  UINTN     UintN;
} IA32_EFLAGS32;

//
// Byte packed structure for Control Register 0 (CR0)
// 32-bits on IA-32
// 64-bits on X64.  The upper 32-bits on X64 are reserved
//
typedef union {
  struct {
    UINT32  PE:1;           // Protection Enable
    UINT32  MP:1;           // Monitor Coprocessor
    UINT32  EM:1;           // Emulation
    UINT32  TS:1;           // Task Switched
    UINT32  ET:1;           // Extension Type
    UINT32  NE:1;           // Numeric Error
    UINT32  Reserved_0:10;  // Reserved
    UINT32  WP:1;           // Write Protect
    UINT32  Reserved_1:1;   // Reserved
    UINT32  AM:1;           // Alignment Mask
    UINT32  Reserved_2:10;  // Reserved
    UINT32  NW:1;           // Mot Write-through
    UINT32  CD:1;           // Cache Disable
    UINT32  PG:1;           // Paging
  } Bits;
  UINTN     UintN;
} IA32_CR0;

//
// Byte packed structure for Control Register 4 (CR4)
// 32-bits on IA-32
// 64-bits on X64.  The upper 32-bits on X64 are reserved
//
typedef union {
  struct {
    UINT32  VME:1;          // Virtual-8086 Mode Extensions
    UINT32  PVI:1;          // Protected-Mode Virtual Interrupts
    UINT32  TSD:1;          // Time Stamp Disable
    UINT32  DE:1;           // Debugging Extensions
    UINT32  PSE:1;          // Page Size Extensions
    UINT32  PAE:1;          // Physical Address Extension
    UINT32  MCE:1;          // Machine Check Enable
    UINT32  PGE:1;          // Page Global Enable
    UINT32  PCE:1;          // Performance Monitoring Counter
                            // Enable
    UINT32  OSFXSR:1;       // Operating System Support for
                            // FXSAVE and FXRSTOR instructions
    UINT32  OSXMMEXCPT:1;   // Operating System Support for
                            // Unmasked SIMD Floating Point
                            // Exceptions
    UINT32  Reserved_0:2;   // Reserved
    UINT32  VMXE:1;         // VMX Enable
    UINT32  Reserved_1:18;  // Reseved
  } Bits;
  UINTN     UintN;
} IA32_CR4;

//
// Byte packed structure for an IDTR, GDTR, LDTR descriptor
/// @bug  How to make this structure byte-packed in a compiler independent way?
//
#pragma pack (1)
typedef struct {
  UINT16  Limit;
  UINTN   Base;
} IA32_DESCRIPTOR;
#pragma pack ()

#define IA32_IDT_GATE_TYPE_TASK          0x85
#define IA32_IDT_GATE_TYPE_INTERRUPT_16  0x86
#define IA32_IDT_GATE_TYPE_TRAP_16       0x87
#define IA32_IDT_GATE_TYPE_INTERRUPT_32  0x8E
#define IA32_IDT_GATE_TYPE_TRAP_32       0x8F

//
// Byte packed structure for an Interrupt Gate Descriptor
//
typedef union {
  struct {
    UINT32  OffsetLow:16;   // Offset bits 15..0
    UINT32  Selector:16;    // Selector
    UINT32  Reserved_0:8;   // Reserved
    UINT32  GateType:8;     // Gate Type.  See #defines above
    UINT32  OffsetHigh:16;  // Offset bits 31..16
  } Bits;
  UINT64  Uint64;
} IA32_IDT_GATE_DESCRIPTOR;

//
// Byte packed structure for an FP/SSE/SSE2 context
//
typedef struct {
  UINT8  Buffer[512];
} IA32_FX_BUFFER;

//
// Structures for the 16-bit real mode thunks
//
typedef struct {
  UINT32                            Reserved1;
  UINT32                            Reserved2;
  UINT32                            Reserved3;
  UINT32                            Reserved4;
  UINT8                             BL;
  UINT8                             BH;
  UINT16                            Reserved5;
  UINT8                             DL;
  UINT8                             DH;
  UINT16                            Reserved6;
  UINT8                             CL;
  UINT8                             CH;
  UINT16                            Reserved7;
  UINT8                             AL;
  UINT8                             AH;
  UINT16                            Reserved8;
} IA32_BYTE_REGS;

typedef struct {
  UINT16                            DI;
  UINT16                            Reserved1;
  UINT16                            SI;
  UINT16                            Reserved2;
  UINT16                            BP;
  UINT16                            Reserved3;
  UINT16                            SP;
  UINT16                            Reserved4;
  UINT16                            BX;
  UINT16                            Reserved5;
  UINT16                            DX;
  UINT16                            Reserved6;
  UINT16                            CX;
  UINT16                            Reserved7;
  UINT16                            AX;
  UINT16                            Reserved8;
} IA32_WORD_REGS;

typedef struct {
  UINT32                            EDI;
  UINT32                            ESI;
  UINT32                            EBP;
  UINT32                            ESP;
  UINT32                            EBX;
  UINT32                            EDX;
  UINT32                            ECX;
  UINT32                            EAX;
  UINT16                            DS;
  UINT16                            ES;
  UINT16                            FS;
  UINT16                            GS;
  IA32_EFLAGS32                     EFLAGS;
  UINT32                            Eip;
  UINT16                            CS;
  UINT16                            SS;
} IA32_DWORD_REGS;

typedef union {
  IA32_DWORD_REGS                   E;
  IA32_WORD_REGS                    X;
  IA32_BYTE_REGS                    H;
} IA32_REGISTER_SET;

//
// Byte packed structure for an 16-bit real mode thunks
//
typedef struct {
  IA32_REGISTER_SET                 *RealModeState;
  VOID                              *RealModeBuffer;
  UINT32                            RealModeBufferSize;
  UINT32                            ThunkAttributes;
} THUNK_CONTEXT;

#define THUNK_ATTRIBUTE_BIG_REAL_MODE             0x00000001
#define THUNK_ATTRIBUTE_DISABLE_A20_MASK_INT_15   0x00000002
#define THUNK_ATTRIBUTE_DISABLE_A20_MASK_KBD_CTRL 0x00000004

/**
  Retrieves CPUID information.

  Executes the CPUID instruction with EAX set to the value specified by Index.
  This function always returns Index.
  If Eax is not NULL, then the value of EAX after CPUID is returned in Eax.
  If Ebx is not NULL, then the value of EBX after CPUID is returned in Ebx.
  If Ecx is not NULL, then the value of ECX after CPUID is returned in Ecx.
  If Edx is not NULL, then the value of EDX after CPUID is returned in Edx.
  This function is only available on IA-32 and X64.

  @param  Index The 32-bit value to load into EAX prior to invoking the CPUID
                instruction.
  @param  Eax   Pointer to the 32-bit EAX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Ebx   Pointer to the 32-bit EBX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Ecx   Pointer to the 32-bit ECX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.
  @param  Edx   Pointer to the 32-bit EDX value returned by the CPUID
                instruction. This is an optional parameter that may be NULL.

  @return Index

**/
UINT32
EFIAPI
AsmCpuid (
  IN      UINT32                    Index,
  OUT     UINT32                    *Eax,  OPTIONAL
  OUT     UINT32                    *Ebx,  OPTIONAL
  OUT     UINT32                    *Ecx,  OPTIONAL
  OUT     UINT32                    *Edx   OPTIONAL
  );


/**
  Retrieves CPUID information using an extended leaf identifier.

  Executes the CPUID instruction with EAX set to the value specified by Index
  and ECX set to the value specified by SubIndex. This function always returns
  Index. This function is only available on IA-32 and x64.

  If Eax is not NULL, then the value of EAX after CPUID is returned in Eax.
  If Ebx is not NULL, then the value of EBX after CPUID is returned in Ebx.
  If Ecx is not NULL, then the value of ECX after CPUID is returned in Ecx.
  If Edx is not NULL, then the value of EDX after CPUID is returned in Edx.

  @param  Index     The 32-bit value to load into EAX prior to invoking the
                    CPUID instruction.
  @param  SubIndex  The 32-bit value to load into ECX prior to invoking the
                    CPUID instruction.
  @param  Eax       Pointer to the 32-bit EAX value returned by the CPUID
                    instruction. This is an optional parameter that may be
                    NULL.
  @param  Ebx       Pointer to the 32-bit EBX value returned by the CPUID
                    instruction. This is an optional parameter that may be
                    NULL.
  @param  Ecx       Pointer to the 32-bit ECX value returned by the CPUID
                    instruction. This is an optional parameter that may be
                    NULL.
  @param  Edx       Pointer to the 32-bit EDX value returned by the CPUID
                    instruction. This is an optional parameter that may be
                    NULL.

  @return Index

**/
UINT32
EFIAPI
AsmCpuidEx (
  IN      UINT32                    Index,
  IN      UINT32                    SubIndex,
  OUT     UINT32                    *Eax,  OPTIONAL
  OUT     UINT32                    *Ebx,  OPTIONAL
  OUT     UINT32                    *Ecx,  OPTIONAL
  OUT     UINT32                    *Edx   OPTIONAL
  );


/**
  Returns the lower 32-bits of a Machine Specific Register(MSR).

  Reads and returns the lower 32-bits of the MSR specified by Index.
  No parameter checking is performed on Index, and some Index values may cause
  CPU exceptions. The caller must either guarantee that Index is valid, or the
  caller must set up exception handlers to catch the exceptions. This function
  is only available on IA-32 and X64.

  @param  Index The 32-bit MSR index to read.

  @return The lower 32 bits of the MSR identified by Index.

**/
UINT32
EFIAPI
AsmReadMsr32 (
  IN      UINT32                    Index
  );


/**
  Zero-extend a 32-bit value and writes it to a Machine Specific Register(MSR).

  Writes the 32-bit value specified by Value to the MSR specified by Index. The
  upper 32-bits of the MSR write are set to zero. The 32-bit value written to
  the MSR is returned. No parameter checking is performed on Index or Value,
  and some of these may cause CPU exceptions. The caller must either guarantee
  that Index and Value are valid, or the caller must establish proper exception
  handlers. This function is only available on IA-32 and X64.

  @param  Index The 32-bit MSR index to write.
  @param  Value The 32-bit value to write to the MSR.

  @return Value

**/
UINT32
EFIAPI
AsmWriteMsr32 (
  IN      UINT32                    Index,
  IN      UINT32                    Value
  );


/**
  Reads a 64-bit MSR, performs a bitwise inclusive OR on the lower 32-bits, and
  writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the lower 32-bits of the read result and the value specified by
  OrData, and writes the result to the 64-bit MSR specified by Index. The lower
  32-bits of the value written to the MSR is returned. No parameter checking is
  performed on Index or OrData, and some of these may cause CPU exceptions. The
  caller must either guarantee that Index and OrData are valid, or the caller
  must establish proper exception handlers. This function is only available on
  IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  OrData  The value to OR with the read value from the MSR.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrOr32 (
  IN      UINT32                    Index,
  IN      UINT32                    OrData
  );


/**
  Reads a 64-bit MSR, performs a bitwise AND on the lower 32-bits, and writes
  the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  lower 32-bits of the read result and the value specified by AndData, and
  writes the result to the 64-bit MSR specified by Index. The lower 32-bits of
  the value written to the MSR is returned. No parameter checking is performed
  on Index or AndData, and some of these may cause CPU exceptions. The caller
  must either guarantee that Index and AndData are valid, or the caller must
  establish proper exception handlers. This function is only available on IA-32
  and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrAnd32 (
  IN      UINT32                    Index,
  IN      UINT32                    AndData
  );


/**
  Reads a 64-bit MSR, performs a bitwise AND followed by a bitwise inclusive OR
  on the lower 32-bits, and writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  lower 32-bits of the read result and the value specified by AndData
  preserving the upper 32-bits, performs a bitwise inclusive OR between the
  result of the AND operation and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Address. The lower 32-bits of the value
  written to the MSR is returned. No parameter checking is performed on Index,
  AndData, or OrData, and some of these may cause CPU exceptions. The caller
  must either guarantee that Index, AndData, and OrData are valid, or the
  caller must establish proper exception handlers. This function is only
  available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The lower 32-bit value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrAndThenOr32 (
  IN      UINT32                    Index,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  );


/**
  Reads a bit field of an MSR.

  Reads the bit field in the lower 32-bits of a 64-bit MSR. The bit field is
  specified by the StartBit and the EndBit. The value of the bit field is
  returned. The caller must either guarantee that Index is valid, or the caller
  must set up exception handlers to catch the exceptions. This function is only
  available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.

  @return The bit field read from the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldRead32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to an MSR.

  Writes Value to a bit field in the lower 32-bits of a  64-bit MSR. The bit
  field is specified by the StartBit and the EndBit. All other bits in the
  destination MSR are preserved. The lower 32-bits of the MSR written is
  returned. Extra left bits in Value are stripped. The caller must either
  guarantee that Index and the data written is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  Value     New value of the bit field.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldWrite32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    Value
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise OR, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The lower 32-bits of the value
  written to the MSR are returned. Extra left bits in OrData are stripped. The
  caller must either guarantee that Index and the data written is valid, or
  the caller must set up exception handlers to catch the exceptions. This
  function is only available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  OrData    The value to OR with the read value from the MSR.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldOr32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    OrData
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by AndData, and writes the result to the
  64-bit MSR specified by Index. The lower 32-bits of the value written to the
  MSR are returned. Extra left bits in AndData are stripped. The caller must
  either guarantee that Index and the data written is valid, or the caller must
  set up exception handlers to catch the exceptions. This function is only
  available on IA-32 and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the MSR.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldAnd32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND followed by a
  bitwise inclusive OR between the read result and the value specified by
  AndData, and writes the result to the 64-bit MSR specified by Index. The
  lower 32-bits of the value written to the MSR are returned. Extra left bits
  in both AndData and OrData are stripped. The caller must either guarantee
  that Index and the data written is valid, or the caller must set up exception
  handlers to catch the exceptions. This function is only available on IA-32
  and X64.

  If StartBit is greater than 31, then ASSERT().
  If EndBit is greater than 31, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..31.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..31.
  @param  AndData   The value to AND with the read value from the MSR.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The lower 32-bit of the value written to the MSR.

**/
UINT32
EFIAPI
AsmMsrBitFieldAndThenOr32 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT32                    AndData,
  IN      UINT32                    OrData
  );


/**
  Returns a 64-bit Machine Specific Register(MSR).

  Reads and returns the 64-bit MSR specified by Index. No parameter checking is
  performed on Index, and some Index values may cause CPU exceptions. The
  caller must either guarantee that Index is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  @param  Index The 32-bit MSR index to read.

  @return The value of the MSR identified by Index.

**/
UINT64
EFIAPI
AsmReadMsr64 (
  IN      UINT32                    Index
  );


/**
  Writes a 64-bit value to a Machine Specific Register(MSR), and returns the
  value.

  Writes the 64-bit value specified by Value to the MSR specified by Index. The
  64-bit value written to the MSR is returned. No parameter checking is
  performed on Index or Value, and some of these may cause CPU exceptions. The
  caller must either guarantee that Index and Value are valid, or the caller
  must establish proper exception handlers. This function is only available on
  IA-32 and X64.

  @param  Index The 32-bit MSR index to write.
  @param  Value The 64-bit value to write to the MSR.

  @return Value

**/
UINT64
EFIAPI
AsmWriteMsr64 (
  IN      UINT32                    Index,
  IN      UINT64                    Value
  );


/**
  Reads a 64-bit MSR, performs a bitwise inclusive OR, and writes the result
  back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The value written to the MSR is
  returned. No parameter checking is performed on Index or OrData, and some of
  these may cause CPU exceptions. The caller must either guarantee that Index
  and OrData are valid, or the caller must establish proper exception handlers.
  This function is only available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  OrData  The value to OR with the read value from the MSR.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrOr64 (
  IN      UINT32                    Index,
  IN      UINT64                    OrData
  );


/**
  Reads a 64-bit MSR, performs a bitwise AND, and writes the result back to the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by OrData, and writes the result to the
  64-bit MSR specified by Index. The value written to the MSR is returned. No
  parameter checking is performed on Index or OrData, and some of these may
  cause CPU exceptions. The caller must either guarantee that Index and OrData
  are valid, or the caller must establish proper exception handlers. This
  function is only available on IA-32 and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrAnd64 (
  IN      UINT32                    Index,
  IN      UINT64                    AndData
  );


/**
  Reads a 64-bit MSR, performs a bitwise AND followed by a bitwise inclusive
  OR, and writes the result back to the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between read
  result and the value specified by AndData, performs a bitwise inclusive OR
  between the result of the AND operation and the value specified by OrData,
  and writes the result to the 64-bit MSR specified by Index. The value written
  to the MSR is returned. No parameter checking is performed on Index, AndData,
  or OrData, and some of these may cause CPU exceptions. The caller must either
  guarantee that Index, AndData, and OrData are valid, or the caller must
  establish proper exception handlers. This function is only available on IA-32
  and X64.

  @param  Index   The 32-bit MSR index to write.
  @param  AndData The value to AND with the read value from the MSR.
  @param  OrData  The value to OR with the result of the AND operation.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrAndThenOr64 (
  IN      UINT32                    Index,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  );


/**
  Reads a bit field of an MSR.

  Reads the bit field in the 64-bit MSR. The bit field is specified by the
  StartBit and the EndBit. The value of the bit field is returned. The caller
  must either guarantee that Index is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to read.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.

  @return The value read from the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldRead64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  );


/**
  Writes a bit field to an MSR.

  Writes Value to a bit field in a 64-bit MSR. The bit field is specified by
  the StartBit and the EndBit. All other bits in the destination MSR are
  preserved. The MSR written is returned. Extra left bits in Value are
  stripped. The caller must either guarantee that Index and the data written is
  valid, or the caller must set up exception handlers to catch the exceptions.
  This function is only available on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  Value     New value of the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldWrite64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    Value
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise inclusive OR, and
  writes the result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise inclusive OR
  between the read result and the value specified by OrData, and writes the
  result to the 64-bit MSR specified by Index. The value written to the MSR is
  returned. Extra left bits in OrData are stripped. The caller must either
  guarantee that Index and the data written is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  OrData    The value to OR with the read value from the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldOr64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    OrData
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND, and writes the
  result back to the bit field in the 64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND between the
  read result and the value specified by AndData, and writes the result to the
  64-bit MSR specified by Index. The value written to the MSR is returned.
  Extra left bits in AndData are stripped. The caller must either guarantee
  that Index and the data written is valid, or the caller must set up exception
  handlers to catch the exceptions. This function is only available on IA-32
  and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the bit field.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldAnd64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData
  );


/**
  Reads a bit field in a 64-bit MSR, performs a bitwise AND followed by a
  bitwise inclusive OR, and writes the result back to the bit field in the
  64-bit MSR.

  Reads the 64-bit MSR specified by Index, performs a bitwise AND followed by
  a bitwise inclusive OR between the read result and the value specified by
  AndData, and writes the result to the 64-bit MSR specified by Index. The
  value written to the MSR is returned. Extra left bits in both AndData and
  OrData are stripped. The caller must either guarantee that Index and the data
  written is valid, or the caller must set up exception handlers to catch the
  exceptions. This function is only available on IA-32 and X64.

  If StartBit is greater than 63, then ASSERT().
  If EndBit is greater than 63, then ASSERT().
  If EndBit is less than StartBit, then ASSERT().

  @param  Index     The 32-bit MSR index to write.
  @param  StartBit  The ordinal of the least significant bit in the bit field.
                    Range 0..63.
  @param  EndBit    The ordinal of the most significant bit in the bit field.
                    Range 0..63.
  @param  AndData   The value to AND with the read value from the bit field.
  @param  OrData    The value to OR with the result of the AND operation.

  @return The value written back to the MSR.

**/
UINT64
EFIAPI
AsmMsrBitFieldAndThenOr64 (
  IN      UINT32                    Index,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit,
  IN      UINT64                    AndData,
  IN      UINT64                    OrData
  );


/**
  Reads the current value of the EFLAGS register.

  Reads and returns the current value of the EFLAGS register. This function is
  only available on IA-32 and X64. This returns a 32-bit value on IA-32 and a
  64-bit value on X64.

  @return EFLAGS on IA-32 or RFLAGS on X64.

**/
UINTN
EFIAPI
AsmReadEflags (
  VOID
  );


/**
  Reads the current value of the Control Register 0 (CR0).

  Reads and returns the current value of CR0. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of the Control Register 0 (CR0).

**/
UINTN
EFIAPI
AsmReadCr0 (
  VOID
  );


/**
  Reads the current value of the Control Register 2 (CR2).

  Reads and returns the current value of CR2. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of the Control Register 2 (CR2).

**/
UINTN
EFIAPI
AsmReadCr2 (
  VOID
  );


/**
  Reads the current value of the Control Register 3 (CR3).

  Reads and returns the current value of CR3. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of the Control Register 3 (CR3).

**/
UINTN
EFIAPI
AsmReadCr3 (
  VOID
  );


/**
  Reads the current value of the Control Register 4 (CR4).

  Reads and returns the current value of CR4. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of the Control Register 4 (CR4).

**/
UINTN
EFIAPI
AsmReadCr4 (
  VOID
  );


/**
  Writes a value to Control Register 0 (CR0).

  Writes and returns a new value to CR0. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Cr0 The value to write to CR0.

  @return The value written to CR0.

**/
UINTN
EFIAPI
AsmWriteCr0 (
  UINTN  Cr0
  );


/**
  Writes a value to Control Register 2 (CR2).

  Writes and returns a new value to CR2. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Cr2 The value to write to CR2.

  @return The value written to CR2.

**/
UINTN
EFIAPI
AsmWriteCr2 (
  UINTN  Cr2
  );


/**
  Writes a value to Control Register 3 (CR3).

  Writes and returns a new value to CR3. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Cr3 The value to write to CR3.

  @return The value written to CR3.

**/
UINTN
EFIAPI
AsmWriteCr3 (
  UINTN  Cr3
  );


/**
  Writes a value to Control Register 4 (CR4).

  Writes and returns a new value to CR4. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Cr4 The value to write to CR4.

  @return The value written to CR4.

**/
UINTN
EFIAPI
AsmWriteCr4 (
  UINTN  Cr4
  );


/**
  Reads the current value of Debug Register 0 (DR0).

  Reads and returns the current value of DR0. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 0 (DR0).

**/
UINTN
EFIAPI
AsmReadDr0 (
  VOID
  );


/**
  Reads the current value of Debug Register 1 (DR1).

  Reads and returns the current value of DR1. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 1 (DR1).

**/
UINTN
EFIAPI
AsmReadDr1 (
  VOID
  );


/**
  Reads the current value of Debug Register 2 (DR2).

  Reads and returns the current value of DR2. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 2 (DR2).

**/
UINTN
EFIAPI
AsmReadDr2 (
  VOID
  );


/**
  Reads the current value of Debug Register 3 (DR3).

  Reads and returns the current value of DR3. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 3 (DR3).

**/
UINTN
EFIAPI
AsmReadDr3 (
  VOID
  );


/**
  Reads the current value of Debug Register 4 (DR4).

  Reads and returns the current value of DR4. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 4 (DR4).

**/
UINTN
EFIAPI
AsmReadDr4 (
  VOID
  );


/**
  Reads the current value of Debug Register 5 (DR5).

  Reads and returns the current value of DR5. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 5 (DR5).

**/
UINTN
EFIAPI
AsmReadDr5 (
  VOID
  );


/**
  Reads the current value of Debug Register 6 (DR6).

  Reads and returns the current value of DR6. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 6 (DR6).

**/
UINTN
EFIAPI
AsmReadDr6 (
  VOID
  );


/**
  Reads the current value of Debug Register 7 (DR7).

  Reads and returns the current value of DR7. This function is only available
  on IA-32 and X64. This returns a 32-bit value on IA-32 and a 64-bit value on
  X64.

  @return The value of Debug Register 7 (DR7).

**/
UINTN
EFIAPI
AsmReadDr7 (
  VOID
  );


/**
  Writes a value to Debug Register 0 (DR0).

  Writes and returns a new value to DR0. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr0 The value to write to Dr0.

  @return The value written to Debug Register 0 (DR0).

**/
UINTN
EFIAPI
AsmWriteDr0 (
  UINTN  Dr0
  );


/**
  Writes a value to Debug Register 1 (DR1).

  Writes and returns a new value to DR1. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr1 The value to write to Dr1.

  @return The value written to Debug Register 1 (DR1).

**/
UINTN
EFIAPI
AsmWriteDr1 (
  UINTN  Dr1
  );


/**
  Writes a value to Debug Register 2 (DR2).

  Writes and returns a new value to DR2. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr2 The value to write to Dr2.

  @return The value written to Debug Register 2 (DR2).

**/
UINTN
EFIAPI
AsmWriteDr2 (
  UINTN  Dr2
  );


/**
  Writes a value to Debug Register 3 (DR3).

  Writes and returns a new value to DR3. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr3 The value to write to Dr3.

  @return The value written to Debug Register 3 (DR3).

**/
UINTN
EFIAPI
AsmWriteDr3 (
  UINTN  Dr3
  );


/**
  Writes a value to Debug Register 4 (DR4).

  Writes and returns a new value to DR4. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr4 The value to write to Dr4.

  @return The value written to Debug Register 4 (DR4).

**/
UINTN
EFIAPI
AsmWriteDr4 (
  UINTN  Dr4
  );


/**
  Writes a value to Debug Register 5 (DR5).

  Writes and returns a new value to DR5. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr5 The value to write to Dr5.

  @return The value written to Debug Register 5 (DR5).

**/
UINTN
EFIAPI
AsmWriteDr5 (
  UINTN  Dr5
  );


/**
  Writes a value to Debug Register 6 (DR6).

  Writes and returns a new value to DR6. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr6 The value to write to Dr6.

  @return The value written to Debug Register 6 (DR6).

**/
UINTN
EFIAPI
AsmWriteDr6 (
  UINTN  Dr6
  );


/**
  Writes a value to Debug Register 7 (DR7).

  Writes and returns a new value to DR7. This function is only available on
  IA-32 and X64. This writes a 32-bit value on IA-32 and a 64-bit value on X64.

  @param  Dr7 The value to write to Dr7.

  @return The value written to Debug Register 7 (DR7).

**/
UINTN
EFIAPI
AsmWriteDr7 (
  UINTN  Dr7
  );


/**
  Reads the current value of Code Segment Register (CS).

  Reads and returns the current value of CS. This function is only available on
  IA-32 and X64.

  @return The current value of CS.

**/
UINT16
EFIAPI
AsmReadCs (
  VOID
  );


/**
  Reads the current value of Data Segment Register (DS).

  Reads and returns the current value of DS. This function is only available on
  IA-32 and X64.

  @return The current value of DS.

**/
UINT16
EFIAPI
AsmReadDs (
  VOID
  );


/**
  Reads the current value of Extra Segment Register (ES).

  Reads and returns the current value of ES. This function is only available on
  IA-32 and X64.

  @return The current value of ES.

**/
UINT16
EFIAPI
AsmReadEs (
  VOID
  );


/**
  Reads the current value of FS Data Segment Register (FS).

  Reads and returns the current value of FS. This function is only available on
  IA-32 and X64.

  @return The current value of FS.

**/
UINT16
EFIAPI
AsmReadFs (
  VOID
  );


/**
  Reads the current value of GS Data Segment Register (GS).

  Reads and returns the current value of GS. This function is only available on
  IA-32 and X64.

  @return The current value of GS.

**/
UINT16
EFIAPI
AsmReadGs (
  VOID
  );


/**
  Reads the current value of Stack Segment Register (SS).

  Reads and returns the current value of SS. This function is only available on
  IA-32 and X64.

  @return The current value of SS.

**/
UINT16
EFIAPI
AsmReadSs (
  VOID
  );


/**
  Reads the current value of Task Register (TR).

  Reads and returns the current value of TR. This function is only available on
  IA-32 and X64.

  @return The current value of TR.

**/
UINT16
EFIAPI
AsmReadTr (
  VOID
  );


/**
  Reads the current Global Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current GDTR descriptor and returns it in Gdtr. This
  function is only available on IA-32 and X64.

  If Gdtr is NULL, then ASSERT().

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
AsmReadGdtr (
  OUT     IA32_DESCRIPTOR           *Gdtr
  );


/**
  Writes the current Global Descriptor Table Register (GDTR) descriptor.

  Writes and the current GDTR descriptor specified by Gdtr. This function is
  only available on IA-32 and X64.

  If Gdtr is NULL, then ASSERT().

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
AsmWriteGdtr (
  IN      CONST IA32_DESCRIPTOR     *Gdtr
  );


/**
  Reads the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current IDTR descriptor and returns it in Idtr. This
  function is only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmReadIdtr (
  OUT     IA32_DESCRIPTOR           *Idtr
  );


/**
  Writes the current Interrupt Descriptor Table Register(GDTR) descriptor.

  Writes the current IDTR descriptor and returns it in Idtr. This function is
  only available on IA-32 and X64.

  If Idtr is NULL, then ASSERT().

  @param  Idtr  Pointer to a IDTR descriptor.

**/
VOID
EFIAPI
AsmWriteIdtr (
  IN      CONST IA32_DESCRIPTOR     *Idtr
  );


/**
  Reads the current Local Descriptor Table Register(LDTR) selector.

  Reads and returns the current 16-bit LDTR descriptor value. This function is
  only available on IA-32 and X64.

  @return The current selector of LDT.

**/
UINT16
EFIAPI
AsmReadLdtr (
  VOID
  );


/**
  Writes the current Local Descriptor Table Register (GDTR) selector.

  Writes and the current LDTR descriptor specified by Ldtr. This function is
  only available on IA-32 and X64.

  @param  Ldtr  16-bit LDTR selector value.

**/
VOID
EFIAPI
AsmWriteLdtr (
  IN      UINT16                    Ldtr
  );


/**
  Save the current floating point/SSE/SSE2 context to a buffer.

  Saves the current floating point/SSE/SSE2 state to the buffer specified by
  Buffer. Buffer must be aligned on a 16-byte boundary. This function is only
  available on IA-32 and X64.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-byte boundary, then ASSERT().

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
AsmFxSave (
  OUT     IA32_FX_BUFFER            *Buffer
  );


/**
  Restores the current floating point/SSE/SSE2 context from a buffer.

  Restores the current floating point/SSE/SSE2 state from the buffer specified
  by Buffer. Buffer must be aligned on a 16-byte boundary. This function is
  only available on IA-32 and X64.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-byte boundary, then ASSERT().
  If Buffer was not saved with AsmFxSave(), then ASSERT().

  @param  Buffer  Pointer to a buffer to save the floating point/SSE/SSE2 context.

**/
VOID
EFIAPI
AsmFxRestore (
  IN      CONST IA32_FX_BUFFER      *Buffer
  );


/**
  Reads the current value of 64-bit MMX Register #0 (MM0).

  Reads and returns the current value of MM0. This function is only available
  on IA-32 and X64.

  @return The current value of MM0.

**/
UINT64
EFIAPI
AsmReadMm0 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #1 (MM1).

  Reads and returns the current value of MM1. This function is only available
  on IA-32 and X64.

  @return The current value of MM1.

**/
UINT64
EFIAPI
AsmReadMm1 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #2 (MM2).

  Reads and returns the current value of MM2. This function is only available
  on IA-32 and X64.

  @return The current value of MM2.

**/
UINT64
EFIAPI
AsmReadMm2 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #3 (MM3).

  Reads and returns the current value of MM3. This function is only available
  on IA-32 and X64.

  @return The current value of MM3.

**/
UINT64
EFIAPI
AsmReadMm3 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #4 (MM4).

  Reads and returns the current value of MM4. This function is only available
  on IA-32 and X64.

  @return The current value of MM4.

**/
UINT64
EFIAPI
AsmReadMm4 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #5 (MM5).

  Reads and returns the current value of MM5. This function is only available
  on IA-32 and X64.

  @return The current value of MM5.

**/
UINT64
EFIAPI
AsmReadMm5 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #6 (MM6).

  Reads and returns the current value of MM6. This function is only available
  on IA-32 and X64.

  @return The current value of MM6.

**/
UINT64
EFIAPI
AsmReadMm6 (
  VOID
  );


/**
  Reads the current value of 64-bit MMX Register #7 (MM7).

  Reads and returns the current value of MM7. This function is only available
  on IA-32 and X64.

  @return The current value of MM7.

**/
UINT64
EFIAPI
AsmReadMm7 (
  VOID
  );


/**
  Writes the current value of 64-bit MMX Register #0 (MM0).

  Writes the current value of MM0. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM0.

**/
VOID
EFIAPI
AsmWriteMm0 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #1 (MM1).

  Writes the current value of MM1. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM1.

**/
VOID
EFIAPI
AsmWriteMm1 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #2 (MM2).

  Writes the current value of MM2. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM2.

**/
VOID
EFIAPI
AsmWriteMm2 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #3 (MM3).

  Writes the current value of MM3. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM3.

**/
VOID
EFIAPI
AsmWriteMm3 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #4 (MM4).

  Writes the current value of MM4. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM4.

**/
VOID
EFIAPI
AsmWriteMm4 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #5 (MM5).

  Writes the current value of MM5. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM5.

**/
VOID
EFIAPI
AsmWriteMm5 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #6 (MM6).

  Writes the current value of MM6. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM6.

**/
VOID
EFIAPI
AsmWriteMm6 (
  IN      UINT64                    Value
  );


/**
  Writes the current value of 64-bit MMX Register #7 (MM7).

  Writes the current value of MM7. This function is only available on IA32 and
  X64.

  @param  Value The 64-bit value to write to MM7.

**/
VOID
EFIAPI
AsmWriteMm7 (
  IN      UINT64                    Value
  );


/**
  Reads the current value of Time Stamp Counter (TSC).

  Reads and returns the current value of TSC. This function is only available
  on IA-32 and X64.

  @return The current value of TSC

**/
UINT64
EFIAPI
AsmReadTsc (
  VOID
  );


/**
  Reads the current value of a Performance Counter (PMC).

  Reads and returns the current value of performance counter specified by
  Index. This function is only available on IA-32 and X64.

  @param  Index The 32-bit Performance Counter index to read.

  @return The value of the PMC specified by Index.

**/
UINT64
EFIAPI
AsmReadPmc (
  IN      UINT32                    Index
  );


/**
  Sets up a monitor buffer that is used by AsmMwait().

  Executes a MONITOR instruction with the register state specified by Eax, Ecx
  and Edx. Returns Eax. This function is only available on IA-32 and X64.

  @param  Eax The value to load into EAX or RAX before executing the MONITOR
              instruction.
  @param  Ecx The value to load into ECX or RCX before executing the MONITOR
              instruction.
  @param  Edx The value to load into EDX or RDX before executing the MONITOR
              instruction.

  @return Eax

**/
UINTN
EFIAPI
AsmMonitor (
  IN      UINTN                     Eax,
  IN      UINTN                     Ecx,
  IN      UINTN                     Edx
  );


/**
  Executes an MWAIT instruction.

  Executes an MWAIT instruction with the register state specified by Eax and
  Ecx. Returns Eax. This function is only available on IA-32 and X64.

  @param  Eax The value to load into EAX or RAX before executing the MONITOR
              instruction.
  @param  Ecx The value to load into ECX or RCX before executing the MONITOR
              instruction.

  @return Eax

**/
UINTN
EFIAPI
AsmMwait (
  IN      UINTN                     Eax,
  IN      UINTN                     Ecx
  );


/**
  Executes a WBINVD instruction.

  Executes a WBINVD instruction. This function is only available on IA-32 and
  X64.

**/
VOID
EFIAPI
AsmWbinvd (
  VOID
  );


/**
  Executes a INVD instruction.

  Executes a INVD instruction. This function is only available on IA-32 and
  X64.

**/
VOID
EFIAPI
AsmInvd (
  VOID
  );


/**
  Flushes a cache line from all the instruction and data caches within the
  coherency domain of the CPU.

  Flushed the cache line specified by LinearAddress, and returns LinearAddress.
  This function is only available on IA-32 and X64.

  @param  LinearAddress The address of the cache line to flush. If the CPU is
                        in a physical addressing mode, then LinearAddress is a
                        physical address. If the CPU is in a virtual
                        addressing mode, then LinearAddress is a virtual
                        address.

  @return LinearAddress
**/
VOID *
EFIAPI
AsmFlushCacheLine (
  IN      VOID                      *LinearAddress
  );


/**
  Enables the 32-bit paging mode on the CPU.

  Enables the 32-bit paging mode on the CPU. CR0, CR3, CR4, and the page tables
  must be properly initialized prior to calling this service. This function
  assumes the current execution mode is 32-bit protected mode. This function is
  only available on IA-32. After the 32-bit paging mode is enabled, control is
  transferred to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If the current execution mode is not 32-bit protected mode, then ASSERT().
  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  There are a number of constraints that must be followed before calling this
  function:
  1)  Interrupts must be disabled.
  2)  The caller must be in 32-bit protected mode with flat descriptors. This
      means all descriptors must have a base of 0 and a limit of 4GB.
  3)  CR0 and CR4 must be compatible with 32-bit protected mode with flat
      descriptors.
  4)  CR3 must point to valid page tables that will be used once the transition
      is complete, and those page tables must guarantee that the pages for this
      function and the stack are identity mapped.

  @param  EntryPoint  A pointer to function to call with the new stack after
                      paging is enabled.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function as the first parameter after paging is enabled.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function as the second parameter after paging is enabled.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function after paging is enabled.

**/
VOID
EFIAPI
AsmEnablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );


/**
  Disables the 32-bit paging mode on the CPU.

  Disables the 32-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 32-paged protected
  mode. This function is only available on IA-32. After the 32-bit paging mode
  is disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be NULL. The function EntryPoint must never return.

  If the current execution mode is not 32-bit paged mode, then ASSERT().
  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  There are a number of constraints that must be followed before calling this
  function:
  1)  Interrupts must be disabled.
  2)  The caller must be in 32-bit paged mode.
  3)  CR0, CR3, and CR4 must be compatible with 32-bit paged mode.
  4)  CR3 must point to valid page tables that guarantee that the pages for
      this function and the stack are identity mapped.

  @param  EntryPoint  A pointer to function to call with the new stack after
                      paging is disabled.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function as the first parameter after paging is disabled.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function as the second parameter after paging is
                      disabled.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function after paging is disabled.

**/
VOID
EFIAPI
AsmDisablePaging32 (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack
  );


/**
  Enables the 64-bit paging mode on the CPU.

  Enables the 64-bit paging mode on the CPU. CR0, CR3, CR4, and the page tables
  must be properly initialized prior to calling this service. This function
  assumes the current execution mode is 32-bit protected mode with flat
  descriptors. This function is only available on IA-32. After the 64-bit
  paging mode is enabled, control is transferred to the function specified by
  EntryPoint using the new stack specified by NewStack and passing in the
  parameters specified by Context1 and Context2. Context1 and Context2 are
  optional and may be 0. The function EntryPoint must never return.

  If the current execution mode is not 32-bit protected mode with flat
  descriptors, then ASSERT().
  If EntryPoint is 0, then ASSERT().
  If NewStack is 0, then ASSERT().

  @param  Cs          The 16-bit selector to load in the CS before EntryPoint
                      is called. The descriptor in the GDT that this selector
                      references must be setup for long mode.
  @param  EntryPoint  The 64-bit virtual address of the function to call with
                      the new stack after paging is enabled.
  @param  Context1    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the first parameter after
                      paging is enabled.
  @param  Context2    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the second parameter after
                      paging is enabled.
  @param  NewStack    The 64-bit virtual address of the new stack to use for
                      the EntryPoint function after paging is enabled.

**/
VOID
EFIAPI
AsmEnablePaging64 (
  IN      UINT16                    CodeSelector,
  IN      UINT64                    EntryPoint,
  IN      UINT64                    Context1,  OPTIONAL
  IN      UINT64                    Context2,  OPTIONAL
  IN      UINT64                    NewStack
  );


/**
  Disables the 64-bit paging mode on the CPU.

  Disables the 64-bit paging mode on the CPU and returns to 32-bit protected
  mode. This function assumes the current execution mode is 64-paging mode.
  This function is only available on X64. After the 64-bit paging mode is
  disabled, control is transferred to the function specified by EntryPoint
  using the new stack specified by NewStack and passing in the parameters
  specified by Context1 and Context2. Context1 and Context2 are optional and
  may be 0. The function EntryPoint must never return.

  If the current execution mode is not 64-bit paged mode, then ASSERT().
  If EntryPoint is 0, then ASSERT().
  If NewStack is 0, then ASSERT().

  @param  Cs          The 16-bit selector to load in the CS before EntryPoint
                      is called. The descriptor in the GDT that this selector
                      references must be setup for 32-bit protected mode.
  @param  EntryPoint  The 64-bit virtual address of the function to call with
                      the new stack after paging is disabled.
  @param  Context1    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the first parameter after
                      paging is disabled.
  @param  Context2    The 64-bit virtual address of the context to pass into
                      the EntryPoint function as the second parameter after
                      paging is disabled.
  @param  NewStack    The 64-bit virtual address of the new stack to use for
                      the EntryPoint function after paging is disabled.

**/
VOID
EFIAPI
AsmDisablePaging64 (
  IN      UINT16                    CodeSelector,
  IN      UINT32                    EntryPoint,
  IN      UINT32                    Context1,  OPTIONAL
  IN      UINT32                    Context2,  OPTIONAL
  IN      UINT32                    NewStack
  );


//
// 16-bit thunking services
//

/**
  Retrieves the properties for 16-bit thunk functions.

  Computes the size of the buffer and stack below 1MB required to use the
  AsmPrepareThunk16(), AsmThunk16() and AsmPrepareAndThunk16() functions. This
  buffer size is returned in RealModeBufferSize, and the stack size is returned
  in ExtraStackSize. If parameters are passed to the 16-bit real mode code,
  then the actual minimum stack size is ExtraStackSize plus the maximum number
  of bytes that need to be passed to the 16-bit real mode code.

  If RealModeBufferSize is NULL, then ASSERT().
  If ExtraStackSize is NULL, then ASSERT().

  @param  RealModeBufferSize  A pointer to the size of the buffer below 1MB
                              required to use the 16-bit thunk functions.
  @param  ExtraStackSize      A pointer to the extra size of stack below 1MB
                              that the 16-bit thunk functions require for
                              temporary storage in the transition to and from
                              16-bit real mode.

**/
VOID
EFIAPI
AsmGetThunk16Properties (
  OUT     UINT32                    *RealModeBufferSize,
  OUT     UINT32                    *ExtraStackSize
  );


/**
  Prepares all structures a code required to use AsmThunk16().

  Prepares all structures and code required to use AsmThunk16().

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareThunk16 (
  OUT     THUNK_CONTEXT             *ThunkContext
  );


/**
  Transfers control to a 16-bit real mode entry point and returns the results.

  Transfers control to a 16-bit real mode entry point and returns the results.
  AsmPrepareThunk16() must be called with ThunkContext before this function is
  used.

  If ThunkContext is NULL, then ASSERT().
  If AsmPrepareThunk16() was not previously called with ThunkContext, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  );


/**
  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results.

  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results. If the
  caller only need to perform a single 16-bit real mode thunk, then this
  service should be used. If the caller intends to make more than one 16-bit
  real mode thunk, then it is more efficient if AsmPrepareThunk16() is called
  once and AsmThunk16() can be called for each 16-bit real mode thunk.

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareAndThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  );

#else

#endif

#endif

