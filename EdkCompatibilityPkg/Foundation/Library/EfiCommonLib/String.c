/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  String.c

Abstract:

  Unicode string primatives

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "EfiCommonLib.h"

VOID
EfiStrCpy (
  IN CHAR16   *Destination,
  IN CHAR16   *Source
  )
/*++

Routine Description:
  Copy the Unicode string Source to Destination.

Arguments:
  Destination - Location to copy string
  Source      - String to copy

Returns:
  NONE

--*/
{
  while (*Source) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
}

VOID
EfiStrnCpy (
  OUT CHAR16  *Dst,
  IN  CHAR16  *Src,
  IN  UINTN   Length
  )
/*++

Routine Description:
  Copy a string from source to destination

Arguments:
  Dst              Destination string
  Src              Source string
  Length           Length of destination string

Returns:

--*/
{
  UINTN Index;
  UINTN SrcLen;

  SrcLen = EfiStrLen (Src);

  Index = 0;
  while (Index < Length && Index < SrcLen) {
    Dst[Index] = Src[Index];
    Index++;
  }
  for (Index = SrcLen; Index < Length; Index++) {
    Dst[Index] = 0;
  }
}

UINTN
EfiStrLen (
  IN CHAR16   *String
  )
/*++

Routine Description:
  Return the number of Unicode characters in String. This is not the same as
  the length of the string in bytes.

Arguments:
  String - String to process

Returns:
  Number of Unicode characters in String

--*/
{
  UINTN Length;
  
  for (Length=0; *String; String++, Length++);
  return Length;
}


UINTN
EfiStrSize (
  IN CHAR16   *String
  )
/*++

Routine Description:
  Return the number bytes in the Unicode String. This is not the same as
  the length of the string in characters. The string size includes the NULL

Arguments:
  String - String to process

Returns:
  Number of bytes in String

--*/
{
  return ((EfiStrLen (String) + 1) * sizeof (CHAR16));
}


INTN
EfiStrCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2
  )
/*++

Routine Description:
  Compare the Unicode string pointed by String to the string pointed by String2.

Arguments:
  String - String to process

  String2 - The other string to process

Returns:
  Return a positive integer if String is lexicall greater than String2; Zero if 
  the two strings are identical; and a negative integer if String is lexically 
  less than String2.

--*/
{
  while (*String) {
    if (*String != *String2) {
      break;
    }

    String += 1;
    String2 += 1;
  }

  return *String - *String2;
}

INTN
EfiStrnCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2,
  IN UINTN    Length
  )
/*++

Routine Description:
  This function compares the Unicode string String to the Unicode
  string String2 for len characters.  If the first len characters
  of String is identical to the first len characters of String2,
  then 0 is returned.  If substring of String sorts lexicographically
  after String2, the function returns a number greater than 0. If
  substring of String sorts lexicographically before String2, the
  function returns a number less than 0.

Arguments:
  String  - Compare to String2
  String2 - Compare to String
  Length  - Number of Unicode characters to compare

Returns:
  0     - The substring of String and String2 is identical.
  > 0   - The substring of String sorts lexicographically after String2
  < 0   - The substring of String sorts lexicographically before String2

--*/
{
  while (*String && Length != 0) {
    if (*String != *String2) {
      break;
    }
    String  += 1;
    String2 += 1;
    Length  -= 1;
  }
  return Length > 0 ? *String - *String2 : 0;
}

VOID
EfiStrCat (
  IN CHAR16   *Destination,
  IN CHAR16   *Source
  )
/*++

Routine Description:
  Concatinate Source on the end of Destination

Arguments:
  Destination - String to added to the end of.
  Source      - String to concatinate.

Returns:
  NONE

--*/
{   
  EfiStrCpy (Destination + EfiStrLen (Destination), Source);
}

VOID
EfiStrnCat (
  IN CHAR16   *Dest,
  IN CHAR16   *Src,
  IN UINTN    Length
  )
/*++

Routine Description:
  Concatinate Source on the end of Destination

Arguments:
  Dst              Destination string
  Src              Source string
  Length           Length of destination string

Returns:

--*/
{
  EfiStrnCpy (Dest + EfiStrLen (Dest), Src, Length);
}

UINTN
EfiAsciiStrLen (
  IN CHAR8   *String
  )
/*++

Routine Description:
  Return the number of Ascii characters in String. This is not the same as
  the length of the string in bytes.

Arguments:
  String - String to process

Returns:
  Number of Ascii characters in String

--*/
{
  UINTN Length;
  
  for (Length=0; *String; String++, Length++);
  return Length;
}


CHAR8 *
EfiAsciiStrCpy (
  IN CHAR8    *Destination,
  IN CHAR8    *Source
  )
/*++

Routine Description:
  Copy the Ascii string Source to Destination.

Arguments:
  Destination - Location to copy string
  Source      - String to copy

Returns:
  Pointer just pass the end of Destination

--*/
{
  while (*Source) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
  return Destination + 1;
}

VOID
EfiAsciiStrnCpy (
  OUT CHAR8    *Dst,
  IN  CHAR8    *Src,
  IN  UINTN    Length
  )
/*++

Routine Description:
  Copy the Ascii string from source to destination

Arguments:
  Dst              Destination string
  Src              Source string
  Length           Length of destination string

Returns:

--*/
{
  UINTN Index;
  UINTN SrcLen;

  SrcLen = EfiAsciiStrLen (Src);

  Index = 0;
  while (Index < Length && Index < SrcLen) {
    Dst[Index] = Src[Index];
    Index++;
  }
  for (Index = SrcLen; Index < Length; Index++) {
    Dst[Index] = 0;
  }
}

UINTN
EfiAsciiStrSize (
  IN CHAR8   *String
  )
/*++

Routine Description:
  Return the number bytes in the Ascii String. This is not the same as
  the length of the string in characters. The string size includes the NULL

Arguments:
  String - String to process

Returns:
  Number of bytes in String

--*/
{
  return (EfiAsciiStrLen (String) + 1);
}


INTN
EfiAsciiStrCmp (
  IN CHAR8   *String,
  IN CHAR8   *String2
  )
/*++

Routine Description:
  Compare the Ascii string pointed by String to the string pointed by String2. 

Arguments:
  String - String to process

  String2 - The other string to process

Returns:
  Return a positive integer if String is lexicall greater than String2; Zero if 
  the two strings are identical; and a negative integer if String is lexically 
  less than String2.
--*/
{
  while (*String) {
    if (*String != *String2) {
      break;
    }

    String += 1;
    String2 += 1;
  }

  return *String - *String2;
}

INTN
EfiAsciiStrnCmp (
  IN CHAR8    *String,
  IN CHAR8    *String2,
  IN UINTN    Length
  )
{
  if (Length == 0) {
    return 0;
  }

  while ((*String != '\0') &&
         (*String == *String2) &&
         (Length > 1)) {
    String++;
    String2++;
    Length--;
  }
  return *String - *String2;
}

VOID
EfiAsciiStrCat (
  IN CHAR8   *Destination,
  IN CHAR8   *Source
  )
/*++

Routine Description:
  Concatinate Source on the end of Destination

Arguments:
  Destination - String to added to the end of.
  Source      - String to concatinate.

Returns:
  NONE

--*/
{   
  EfiAsciiStrCpy (Destination + EfiAsciiStrLen (Destination), Source);
}

VOID
EfiAsciiStrnCat (
  IN CHAR8   *Destination,
  IN CHAR8   *Source,
  IN UINTN   Length
  )
/*++

Routine Description:
  Concatinate Source on the end of Destination

Arguments:
  Destination - String to added to the end of.
  Source      - String to concatinate.

Returns:
  NONE

--*/
{
  EfiAsciiStrnCpy (Destination + EfiAsciiStrLen (Destination), Source, Length);
}

BOOLEAN
IsHexDigit (
  OUT UINT8      *Digit,
  IN  CHAR16      Char
  )
/*++

  Routine Description:
    Determines if a Unicode character is a hexadecimal digit.
    The test is case insensitive.

  Arguments:
    Digit - Pointer to byte that receives the value of the hex character.
    Char  - Unicode character to test.

  Returns:
    TRUE  - If the character is a hexadecimal digit.
    FALSE - Otherwise.

--*/
{
  if ((Char >= L'0') && (Char <= L'9')) {
    *Digit = (UINT8) (Char - L'0');
    return TRUE;
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    *Digit = (UINT8) (Char - L'A' + 0x0A);
    return TRUE;
  }

  if ((Char >= L'a') && (Char <= L'f')) {
    *Digit = (UINT8) (Char - L'a' + 0x0A);
    return TRUE;
  }

  return FALSE;
}

CHAR16
NibbleToHexChar (
  IN UINT8      Nibble
  )
/*++

  Routine Description:
    Converts the low nibble of a byte  to hex unicode character.

  Arguments:
    Nibble - lower nibble of a byte.

  Returns:
    Hex unicode character.

--*/
{
  Nibble &= 0x0F;
  if (Nibble <= 0x9) {
    return (CHAR16)(Nibble + L'0');
  }

  return (CHAR16)(Nibble - 0xA + L'A');
}

EFI_STATUS
HexStringToBuf (
  IN OUT UINT8                     *Buf,   
  IN OUT UINTN                    *Len,
  IN     CHAR16                    *Str,
  OUT    UINTN                     *ConvertedStrLen  OPTIONAL
  )
/*++

  Routine Description:
    Converts Unicode string to binary buffer.
    The conversion may be partial.
    The first character in the string that is not hex digit stops the conversion.
    At a minimum, any blob of data could be represented as a hex string.

  Arguments:
    Buf    - Pointer to buffer that receives the data.
    Len    - Length in bytes of the buffer to hold converted data.
                If routine return with EFI_SUCCESS, containing length of converted data.
                If routine return with EFI_BUFFER_TOO_SMALL, containg length of buffer desired.
    Str    - String to be converted from.
    ConvertedStrLen - Length of the Hex String consumed.

  Returns:
    EFI_SUCCESS: Routine Success.
    EFI_BUFFER_TOO_SMALL: The buffer is too small to hold converted data.
    EFI_

--*/
{
  UINTN       HexCnt;
  UINTN       Idx;
  UINTN       BufferLength;
  UINT8       Digit;
  UINT8       Byte;

  //
  // Find out how many hex characters the string has.
  //
  for (Idx = 0, HexCnt = 0; IsHexDigit (&Digit, Str[Idx]); Idx++, HexCnt++);

  if (HexCnt == 0) {
    *Len = 0;
    return EFI_SUCCESS;
  }
  //
  // Two Unicode characters make up 1 buffer byte. Round up.
  //
  BufferLength = (HexCnt + 1) / 2; 

  //
  // Test if  buffer is passed enough.
  //
  if (BufferLength > (*Len)) {
    *Len = BufferLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  *Len = BufferLength;

  for (Idx = 0; Idx < HexCnt; Idx++) {

    IsHexDigit (&Digit, Str[HexCnt - 1 - Idx]);

    //
    // For odd charaters, write the lower nibble for each buffer byte,
    // and for even characters, the upper nibble.
    //
    if ((Idx & 1) == 0) {
      Byte = Digit;
    } else {
      Byte = Buf[Idx / 2];
      Byte &= 0x0F;
      Byte = (UINT8)(Byte | (Digit << 4));
    }

    Buf[Idx / 2] = Byte;
  }

  if (ConvertedStrLen != NULL) {
    *ConvertedStrLen = HexCnt;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BufToHexString (
  IN OUT CHAR16                    *Str,
  IN OUT UINTN                     *HexStringBufferLength,
  IN     UINT8                     *Buf,
  IN     UINTN                      Len
  )
/*++

  Routine Description:
    Converts binary buffer to Unicode string.
    At a minimum, any blob of data could be represented as a hex string.

  Arguments:
    Str - Pointer to the string.
    HexStringBufferLength - Length in bytes of buffer to hold the hex string. Includes tailing '\0' character.
                                        If routine return with EFI_SUCCESS, containing length of hex string buffer.
                                        If routine return with EFI_BUFFER_TOO_SMALL, containg length of hex string buffer desired.
    Buf - Buffer to be converted from.
    Len - Length in bytes of the buffer to be converted.

  Returns:
    EFI_SUCCESS: Routine success.
    EFI_BUFFER_TOO_SMALL: The hex string buffer is too small.

--*/
{
  UINTN       Idx;
  UINT8       Byte;
  UINTN       StrLen;

  //
  // Make sure string is either passed or allocate enough.
  // It takes 2 Unicode characters (4 bytes) to represent 1 byte of the binary buffer.
  // Plus the Unicode termination character.
  //
  StrLen = Len * 2;
  if (StrLen > ((*HexStringBufferLength) - 1)) {
    *HexStringBufferLength = StrLen + 1;
    return EFI_BUFFER_TOO_SMALL;
  }

  *HexStringBufferLength = StrLen + 1;
  //
  // Ends the string.
  //
  Str[StrLen] = L'\0'; 

  for (Idx = 0; Idx < Len; Idx++) {

    Byte = Buf[Idx];
    Str[StrLen - 1 - Idx * 2] = NibbleToHexChar (Byte);
    Str[StrLen - 2 - Idx * 2] = NibbleToHexChar ((UINT8)(Byte >> 4));
  }

  return EFI_SUCCESS;
}

VOID
EfiStrTrim (
  IN OUT CHAR16   *str,
  IN     CHAR16   CharC
  )
/*++

Routine Description:
  
  Removes (trims) specified leading and trailing characters from a string.
  
Arguments: 
  
  str     - Pointer to the null-terminated string to be trimmed. On return, 
            str will hold the trimmed string. 
  CharC       - Character will be trimmed from str.
  
Returns:

--*/
{
  CHAR16  *p1;
  CHAR16  *p2;
  
  if (*str == 0) {
    return;
  }
  
  //
  // Trim off the leading and trailing characters c
  //
  for (p1 = str; *p1 && *p1 == CharC; p1++) {
    ;
  }
  
  p2 = str;
  if (p2 == p1) {
    while (*p1) {
      p2++;
      p1++;
    }
  } else {
    while (*p1) {    
    *p2 = *p1;    
    p1++;
    p2++;
    }
    *p2 = 0;
  }
  
  
  for (p1 = str + EfiStrLen(str) - 1; p1 >= str && *p1 == CharC; p1--) {
    ;
  }
  if  (p1 !=  str + EfiStrLen(str) - 1) { 
    *(p1 + 1) = 0;
  }
}
CHAR16*
EfiStrStr (
   IN  CHAR16  *String,
   IN  CHAR16  *StrCharSet
   )
/*++
 
Routine Description:
   
  Find a substring.
   
Arguments: 
   
  String      - Null-terminated string to search.
  StrCharSet  - Null-terminated string to search for.
   
Returns:
  The address of the first occurrence of the matching substring if successful, or NULL otherwise.
--*/
{
  CHAR16 *Src;
  CHAR16 *Sub;
  
  Src = String;
  Sub = StrCharSet;
  
  while ((*String != L'\0') && (*StrCharSet != L'\0')) {
    if (*String++ != *StrCharSet) {
      String = ++Src;
      StrCharSet = Sub;
    } else {
      StrCharSet++;
    }
  }
  if (*StrCharSet == L'\0') {
    return Src;
  } else {
    return NULL;
  }
}
 
CHAR8*
EfiAsciiStrStr (
  IN  CHAR8  *String,
  IN  CHAR8  *StrCharSet
  )
/*++

Routine Description:
  
  Find a Ascii substring.
  
Arguments: 
  
  String      - Null-terminated Ascii string to search.
  StrCharSet  - Null-terminated Ascii string to search for.
  
Returns:
  The address of the first occurrence of the matching Ascii substring if successful, or NULL otherwise.
--*/
{
  CHAR8 *Src;
  CHAR8 *Sub;
   
  Src = String;
  Sub = StrCharSet;
  
  while ((*String != '\0') && (*StrCharSet != '\0')) {
    if (*String++ != *StrCharSet) {
      String = ++Src;
      StrCharSet = Sub;
    } else {
      StrCharSet++;
    }
  }
  if (*StrCharSet == '\0') {
    return Src;
  } else {
    return NULL;
  }
}

