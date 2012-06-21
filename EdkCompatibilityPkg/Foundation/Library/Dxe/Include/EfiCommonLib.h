/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    EfiCommonLib.h

Abstract:

  Light weight lib to support EFI drivers.

--*/

#ifndef _EFI_COMMON_LIB_H_
#define _EFI_COMMON_LIB_H_

EFI_STATUS
EfiLibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
/*++

Routine Description:
  
  Return the EFI 1.0 System Tabl entry with TableGuid

Arguments:

  TableGuid - Name of entry to return in the system table
  Table     - Pointer in EFI system table associated with TableGuid

Returns: 

  EFI_SUCCESS - Table returned;
  EFI_NOT_FOUND - TableGuid not in EFI system table

--*/
;

//
// ASPrint and AvSPrint definitions you must include the specific library
// to get the expected behavior from the two functions
// PEI:  PeiLib
// Graphics:  Dxe\Graphics\Unicode  Dxe\Graphics\ASCII
// ASCII: Dxe\Print\ASCII
// Unicode: Dxe\Print\Unicode
//
UINTN
ASPrint (
  OUT CHAR8       *Buffer,
  IN UINTN        BufferSize,
  IN CONST CHAR8  *Format,
  ...
  )
/*++

Routine Description:

  Process format and place the results in Buffer for narrow chars.

Arguments:

  Buffer      - Narrow char buffer to print the results of the parsing of Format into.
  BufferSize  - Maximum number of characters to put into buffer.
  Format      - Format string
  ...         - Vararg list consumed by processing Format.

Returns:

  Number of characters printed.

--*/
;

UINTN
AvSPrint (
  OUT CHAR8       *StartOfBuffer,
  IN  UINTN       StrSize,
  IN  CONST CHAR8 *Format,
  IN  VA_LIST     Marker
  )
/*++

Routine Description:

  Internal implementation of ASPrint. 
  Process format and place the results in Buffer for narrow chars.

Arguments:

  StartOfBuffer - Narrow char buffer to print the results of the parsing of Format into.
  StrSize       - Maximum number of characters to put into buffer.
  FormatString  - Format string
  Marker        - Vararg list consumed by processing Format.

Returns:

  Number of characters printed.

--*/
;

//
// Lib functions which can be used in both PEI and DXE pahse
//
EFI_STATUS
EfiInitializeCommonDriverLib (
  IN EFI_HANDLE   ImageHandle,
  IN VOID         *SystemTable
  )
/*++

Routine Description:

  Initialize lib function calling phase: PEI or DXE
  
Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EfiCommonIoRead (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io read operation.

Arguments:

  Width   - Width of read operation
  Address - Start IO address to read
  Count   - Read count
  Buffer  - Buffer to store result

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonIoWrite (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io write operation.

Arguments:

  Width   - Width of write operation
  Address - Start IO address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonPciRead (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci read operation

Arguments:

  Width   - Width of PCI read
  Address - PCI address to read
  Count   - Read count
  Buffer  - Output buffer for the read

Returns:

  Status code

--*/
;

EFI_STATUS
EfiCommonPciWrite (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci write operation

Arguments:

  Width   - Width of PCI write
  Address - PCI address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns:

  Status code

--*/
;

BOOLEAN
EfiCompareGuid (
  IN EFI_GUID *Guid1,
  IN EFI_GUID *Guid2
  )
/*++

Routine Description:

  Compares two GUIDs

Arguments:

  Guid1 - guid to compare

  Guid2 - guid to compare

Returns:
  TRUE     if Guid1 == Guid2
  FALSE    if Guid1 != Guid2

--*/
;

VOID
EfiCommonLibSetMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN UINT8  Value
  )
/*++

Routine Description:

  Set Buffer to Value for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

  Value   - Value of the set operation.

Returns:

  None

--*/
;

VOID
EfiCommonLibCopyMem (
  IN VOID     *Destination,
  IN VOID     *Source,
  IN UINTN    Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
;

INTN
EfiCompareMem (
  IN VOID     *MemOne,
  IN VOID     *MemTwo,
  IN UINTN    Len
  )
/*++

Routine Description:

  Compares two memory buffers of a given length.

Arguments:

  MemOne - First memory buffer

  MemTwo - Second memory buffer

  Len    - Length of Mem1 and Mem2 memory regions to compare

Returns:

  = 0     if MemOne == MemTwo
  
  > 0     if MemOne > MemTwo
  
  < 0     if MemOne < MemTwo

--*/
;

VOID
EfiCommonLibZeroMem (
  IN VOID     *Buffer,
  IN UINTN    Size
  )
/*++

Routine Description:

  Set Buffer to 0 for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

Returns:

  None

--*/
;

//
// Min Max
//
#define EFI_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define EFI_MAX(a, b) (((a) > (b)) ? (a) : (b))

//
// Align a pointer. The pointer represented by ptr is aligned to the bound.
// The resulting pointer is always equal or greater (by no more than bound-1)
// than the ptr. I.e., if the ptr is already aligned, the result will be equal to ptr.
// Valid values for bound are powers of two: 2, 4, 8, 16, 32 etc.
// The returned pointer is VOID* this assignment-compatible with all pointer types.
//
#define EFI_ALIGN(ptr, bound) ((VOID *) (((UINTN) (ptr) + ((UINTN) (bound) - 1)) &~((UINTN) (bound) - 1)))

//
// Alignment tests.
//
#define EFI_UINTN_ALIGN_MASK    (sizeof (UINTN) - 1)
#define EFI_UINTN_ALIGNED(ptr)  (((UINTN) (ptr)) & EFI_UINTN_ALIGN_MASK)

//
// Integer division with rounding to the nearest rather than truncating.
// For example 8/3=2 but EFI_IDIV_ROUND(8,3)=3. 1/3=0 and EFI_IDIV_ROUND(1,3)=0.
// A half is rounded up e.g., EFI_IDIV_ROUND(1,2)=1 but 1/2=0.
//
#define EFI_IDIV_ROUND(r, s)  ((r) / (s) + (((2 * ((r) % (s))) < (s)) ? 0 : 1))

//
// ReportStatusCode.c init
//
VOID  *
EfiConstructStatusCodeData (
  IN  UINT16                    DataSize,
  IN  EFI_GUID                  *TypeGuid,
  IN OUT  EFI_STATUS_CODE_DATA  *Data
  )
/*++

Routine Description:

  Construct stanader header for optional data passed into ReportStatusCode

Arguments:

  DataSize - Size of optional data. Does not include EFI_STATUS_CODE_DATA header
  TypeGuid - GUID to place in EFI_STATUS_CODE_DATA
  Data     - Buffer to use.

Returns:

  Return pointer to Data buffer pointing past the end of EFI_STATUS_CODE_DATA

--*/
;

EFI_STATUS
EfiDebugVPrintWorker (
  IN  UINTN                   ErrorLevel,
  IN  CHAR8                   *Format,
  IN  VA_LIST                 Marker,
  IN  UINTN                   BufferSize,
  IN OUT VOID                 *Buffer
  )
/*++

Routine Description:

  Worker function for DEBUG(). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded do nothing.

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  ErrorLevel - If error level is set do the debug print.

  Format     - String to use for the print, followed by Print arguments.

  Marker     - VarArgs

  BufferSize - Size of Buffer.

  Buffer     - Caller allocated buffer, contains ReportStatusCode extended data
  
Returns:
  
  Status code

--*/
;

EFI_STATUS
EfiDebugAssertWorker (
  IN CHAR8                    *FileName,
  IN INTN                     LineNumber,
  IN CHAR8                    *Description,
  IN UINTN                    BufferSize,
  IN OUT VOID                 *Buffer
  )
/*++

Routine Description:

  Worker function for ASSERT (). If Error Logging hub is loaded log ASSERT
  information. If Error Logging hub is not loaded DEADLOOP ().

  We use UINT64 buffers due to IPF alignment concerns.

Arguments:

  FileName    - File name of failing routine.

  LineNumber  - Line number of failing ASSERT().

  Description - Description, usually the assertion,
  
  BufferSize - Size of Buffer.

  Buffer     - Caller allocated buffer, contains ReportStatusCode extendecd data

Returns:
  
  Status code

--*/
;

BOOLEAN
ReportStatusCodeExtractAssertInfo (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN EFI_STATUS_CODE_DATA     *Data,
  OUT CHAR8                   **Filename,
  OUT CHAR8                   **Description,
  OUT UINT32                  *LineNumber
  )
/*++

Routine Description:

  Extract assert information from status code data.

Arguments:

  CodeType    - Code type
  Value       - Code value
  Data        - Optional data associated with this status code.
  Filename    - Filename extracted from Data
  Description - Description extracted from Data
  LineNumber  - Line number extracted from Data

Returns:

  TRUE      - Successfully extracted
  
  FALSE     - Extraction failed

--*/
;

BOOLEAN
ReportStatusCodeExtractDebugInfo (
  IN EFI_STATUS_CODE_DATA     *Data,
  OUT UINT32                  *ErrorLevel,
  OUT VA_LIST                 *Marker,
  OUT CHAR8                   **Format
  )
/*++

Routine Description:

  Extract debug information from status code data.

Arguments:

  Data        - Optional data associated with status code.
  ErrorLevel  - Error level extracted from Data
  Marker      - VA_LIST extracted from Data
  Format      - Format string extracted from Data

Returns:

  TRUE      - Successfully extracted
  
  FALSE     - Extraction failed

--*/
;

BOOLEAN
CodeTypeToPostCode (
  IN  EFI_STATUS_CODE_TYPE    CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  OUT UINT8                   *PostCode
  )
/*++

Routine Description:

  Convert code value to an 8 bit post code

Arguments:

  CodeType  - Code type
  Value     - Code value
  PostCode  - Post code as output

Returns:

  TRUE    - Successfully converted

  FALSE   - Convertion failed

--*/
;

//
// math.c
//
UINT64
MultU64x32 (
  IN  UINT64  Multiplicand,
  IN  UINTN   Multiplier
  )
/*++  
  
Routine Description:

  This routine allows a 64 bit value to be multiplied with a 32 bit 
  value returns 64bit result.
  No checking if the result is greater than 64bits

Arguments:

  Multiplicand  - multiplicand
  Multiplier    - multiplier

Returns:

  Multiplicand * Multiplier
  
--*/
;

UINT64
DivU64x32 (
  IN  UINT64  Dividend,
  IN  UINTN   Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be divided with a 32 bit value returns 
  64bit result and the Remainder.

Arguments:

  Dividend  - dividend
  Divisor   - divisor
  Remainder - buffer for remainder
 
Returns:

  Dividend  / Divisor
  Remainder = Dividend mod Divisor

--*/
;

UINT64
RShiftU64 (
  IN  UINT64  Operand,
  IN  UINTN   Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be right shifted by 32 bits and returns the 
  shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift right.
 
Returns:

  Value shifted right identified by the Count.

--*/
;

UINT64
LShiftU64 (
  IN  UINT64  Operand,
  IN  UINTN   Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be left shifted by 32 bits and 
  returns the shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift left.
 
Returns:

  Value shifted left identified by the Count.

--*/
;

UINT64
Power10U64 (
  IN UINT64   Operand,
  IN UINTN    Power
  )
/*++

Routine Description:

  Raise 10 to the power of Power, and multiply the result with Operand

Arguments:

  Operand  - multiplicand
  Power    - power

Returns:

  Operand * 10 ^ Power

--*/
;

UINT8
Log2 (
  IN UINT64   Operand
  )
/*++

Routine Description:

  Calculates and floors logarithms based on 2

Arguments:

  Operand - value to calculate logarithm
 
Returns:

  The largest integer that is less than or equal
  to the logarithm of Operand based on 2 

--*/
;

UINT64
GetPowerOfTwo (
  IN  UINT64  Input
  )
/*++

Routine Description:

  Calculates the largest integer that is both 
  a power of two and less than Input

Arguments:

  Input  - value to calculate power of two

Returns:

  the largest integer that is both  a power of 
  two and less than Input

--*/
;

//
// Unicode String primatives
//
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
;

VOID
EfiStrnCpy (
  OUT CHAR16    *Dst,
  IN  CHAR16    *Src,
  IN  UINTN     Length
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
;

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
;

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
;

INTN
EfiStrCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2
  )
/*++

Routine Description:
  Return the alphabetic relationship between two stirngs. 

Arguments:
  String - Compare to String2
  
  String2 - Compare to String

Returns:
  0     - Identical
  
  > 0   - String is alphabeticly greater than String2
  
  < 0   - String is alphabeticly less than String2

--*/
;

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
;

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
;

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
;

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
  Number of Unicode characters in String

--*/
;

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
;

VOID
EfiAsciiStrnCpy (
  OUT CHAR8     *Dst,
  IN  CHAR8     *Src,
  IN  UINTN     Length
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
;

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
;


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
;

INTN
EfiAsciiStrnCmp (
  IN CHAR8    *String,
  IN CHAR8    *String2,
  IN UINTN    Length
  )
/*++

Routine Description:
  This function compares the ASCII string String to the ASCII
  string String2 for len characters.  If the first len characters
  of String is identical to the first len characters of String2,
  then 0 is returned.  If substring of String sorts lexicographically
  after String2, the function returns a number greater than 0. If
  substring of String sorts lexicographically before String2, the
  function returns a number less than 0.

Arguments:
  String  - Compare to String2
  String2 - Compare to String
  Length  - Number of ASCII characters to compare

Returns:
  0     - The substring of String and String2 is identical.
  > 0   - The substring of String sorts lexicographically after String2
  < 0   - The substring of String sorts lexicographically before String2

--*/
;

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
;

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
;

//
// Print primitives
//
#define LEFT_JUSTIFY  0x01
#define PREFIX_SIGN   0x02
#define PREFIX_BLANK  0x04
#define COMMA_TYPE    0x08
#define LONG_TYPE     0x10
#define PREFIX_ZERO   0x20

//
// Length of temp string buffer to store value string.
//
#define CHARACTER_NUMBER_FOR_VALUE  30

UINTN
EfiValueToHexStr (
  IN  OUT CHAR16  *Buffer,
  IN  UINT64      Value,
  IN  UINTN       Flags,
  IN  UINTN       Width
  )
/*++

Routine Description:

  VSPrint worker function that prints a Value as a hex number in Buffer

Arguments:

  Buffer - Location to place ascii hex string of Value.

  Value  - Hex value to convert to a string in Buffer.

  Flags  - Flags to use in printing Hex string, see file header for details.

  Width  - Width of hex value.

Returns: 

  Number of characters printed.  

--*/
;

UINTN
EfiValueToString (
  IN  OUT CHAR16  *Buffer,
  IN  INT64       Value,
  IN  UINTN       Flags,
  IN  UINTN       Width
  )
/*++

Routine Description:

  VSPrint worker function that prints a Value as a decimal number in Buffer

Arguments:

  Buffer - Location to place ascii decimal number string of Value.

  Value  - Decimal value to convert to a string in Buffer.

  Flags  - Flags to use in printing decimal string, see file header for details.

  Width  - Width of hex value.

Returns: 

  Number of characters printed.  

--*/
;

BOOLEAN
IsHexDigit (
  OUT UINT8       *Digit,
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
;

CHAR16
NibbleToHexChar (
  UINT8 Nibble
  )
/*++

  Routine Description:
    Converts the low nibble of a byte  to hex unicode character.

  Arguments:
    Nibble - lower nibble of a byte.

  Returns:
    Hex unicode character.

--*/
;

EFI_STATUS
HexStringToBuf (
  IN OUT UINT8                     *Buf,
  IN OUT UINTN                     *Len,
  IN     CHAR16                    *Str,
  OUT    UINTN                     *ConvertedStrLen OPTIONAL
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
;

EFI_STATUS
BufToHexString (
  IN OUT CHAR16                     *Str,
  IN OUT UINTN                      *HexStringBufferLength,
  IN     UINT8                      *Buf,
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
;

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

  None

--*/
;
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
;

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
;
#endif
