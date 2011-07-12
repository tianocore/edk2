/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Print.c

Abstract:

  Basic Ascii AvSPrintf() function named VSPrint(). VSPrint() enables very
  simple implemenation of SPrint() and Print() to support debug. 

  You can not Print more than EFI_DRIVER_LIB_MAX_PRINT_BUFFER characters at a 
  time. This makes the implementation very simple.

  VSPrint, Print, SPrint format specification has the follwoing form

  %[flags][width]type

  flags:
    '-' - Left justify
    '+' - Prefix a sign
    ' ' - Prefix a blank
    ',' - Place commas in numberss
    '0' - Prefix for width with zeros
    'l' - UINT64
    'L' - UINT64

  width:
    '*' - Get width from a UINTN argumnet from the argument list
    Decimal number that represents width of print

  type:
    'p' - arugment is VOID *; printed as hex number
    'X' - argument is a UINTN hex number, prefix '0'
    'x' - argument is a hex number
    'd' - argument is a decimal number
    'a' - argument is an ascii string 
    'S','s' - argument is an Unicode string
    'g' - argument is a pointer to an EFI_GUID
    't' - argument is a pointer to an EFI_TIME structure
    'c' - argument is an ascii character
    'r' - argument is EFI_STATUS
    '%' - Print a %

--*/

#include "TianoCommon.h"
#include "EfiCommonLib.h"
#include "PrintWidth.h"
#include "EfiPrintLib.h"
#include "Print.h"


STATIC
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN OUT CHAR_W *Buffer,
  IN  UINTN     BufferSize
  );

STATIC
UINTN
TimeToString (
  IN  EFI_TIME  *Time,
  IN OUT CHAR_W *Buffer,
  IN  UINTN     BufferSize
  );

STATIC
UINTN
EfiStatusToString (
  IN EFI_STATUS   Status,
  OUT CHAR_W      *Buffer,
  IN  UINTN       BufferSize
  );

UINTN
SPrint (
  OUT CHAR_W        *Buffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR_W  *Format,
  ...
  )
/*++

Routine Description:

  SPrint function to process format and place the results in Buffer.

Arguments:

  Buffer     - Wide char buffer to print the results of the parsing of Format into.

  BufferSize - Maximum number of characters to put into buffer. Zero means no 
               limit.

  Format - Format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
{
  UINTN   Return;
  VA_LIST Marker;

  VA_START (Marker, Format);
  Return = VSPrint (Buffer, BufferSize, Format, Marker);
  VA_END (Marker);
  
  return Return;
}


UINTN
EFIAPI
VSPrint (
  OUT CHAR_W        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR_W  *FormatString,
  IN  VA_LIST       Marker
  )
/*++

Routine Description:

  VSPrint function to process format and place the results in Buffer. Since a 
  VA_LIST is used this rountine allows the nesting of Vararg routines. Thus 
  this is the main print working routine

Arguments:

  StartOfBuffer - Unicode buffer to print the results of the parsing of Format into.

  BufferSize    - Maximum number of characters to put into buffer. Zero means 
                  no limit.

  FormatString  - Unicode format string see file header for more details.

  Marker        - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
{
  CHAR16    TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR_W    *Buffer;
  CHAR8     *AsciiStr;
  CHAR16    *UnicodeStr;
  CHAR_W    *Format;
  UINTN     Index;
  UINTN     Flags;
  UINTN     Width;
  UINTN     Count;
  UINTN     NumberOfCharacters;
  UINTN     BufferLeft;
  UINT64    Value;
  EFI_GUID  *TmpGUID;
  BOOLEAN   Done;

  //
  // Process the format string. Stop if Buffer is over run.
  //

  Buffer = StartOfBuffer;
  Format = (CHAR_W *)FormatString; 
  NumberOfCharacters = BufferSize/sizeof(CHAR_W);
  BufferLeft = BufferSize;
  for (Index = 0; (*Format != '\0') && (Index < NumberOfCharacters - 1); Format++) {
    if (*Format != '%') {
      if ((*Format == '\n') && (Index < NumberOfCharacters - 2)) {                  
        //
        // If carage return add line feed
        //
        Buffer[Index++] = '\r';
        BufferLeft -= sizeof(CHAR_W);
      }
      Buffer[Index++] = *Format;
      BufferLeft -= sizeof(CHAR_W);
    } else {
      
      //
      // Now it's time to parse what follows after %
      //    
      Flags = 0;
      Width = 0;
      for (Done = FALSE; !Done; ) {
        Format++;
    
        switch (*Format) {
    
        case '-': Flags |= LEFT_JUSTIFY; break;
        case '+': Flags |= PREFIX_SIGN;  break;
        case ' ': Flags |= PREFIX_BLANK; break;
        case ',': Flags |= COMMA_TYPE;   break;
        case 'L':
        case 'l': Flags |= LONG_TYPE;    break;
    
        case '*':
          Width = VA_ARG (Marker, UINTN);
          break;
    
        case '0':
          Flags |= PREFIX_ZERO;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          Count = 0;
          do {
            Count = (Count * 10) + *Format - '0';
            Format++;
          } while ((*Format >= '0')  &&  (*Format <= '9'));
          Format--;
          Width = Count;
          break;
    
        default:
          Done = TRUE;
        }
      }

      switch (*Format) {
      case 'p':
        //
        // Flag space, +, 0, L & l are invalid for type p.
        //
        Flags &= ~(PREFIX_BLANK| PREFIX_SIGN | LONG_TYPE);
        if (sizeof (VOID *) > 4) {
          Flags |= LONG_TYPE;
          Value = VA_ARG (Marker, UINT64);
        } else {
          Value = VA_ARG (Marker, UINTN);
        }
        Flags |= PREFIX_ZERO;

        EfiValueToHexStr (TempBuffer, Value, Flags, Width);
        UnicodeStr = TempBuffer;

        for ( ;(*UnicodeStr != '\0') && (Index < NumberOfCharacters - 1); UnicodeStr++) {
          Buffer[Index++] = *UnicodeStr;
        }
        break;
        
      case 'X':
        Flags |= PREFIX_ZERO;
        Width = sizeof (UINT64) * 2;
        //
        // break skiped on purpose
        //
      case 'x':
        if ((Flags & LONG_TYPE) == LONG_TYPE) {
          Value = VA_ARG (Marker, UINT64);
        } else {
          Value = VA_ARG (Marker, UINTN);
        }

        EfiValueToHexStr (TempBuffer, Value, Flags, Width);
        UnicodeStr = TempBuffer;

        for ( ;(*UnicodeStr != '\0') && (Index < NumberOfCharacters - 1); UnicodeStr++) {
          Buffer[Index++] = *UnicodeStr;
        }
        break;

      case 'd':
        if ((Flags & LONG_TYPE) == LONG_TYPE) {
          Value = VA_ARG (Marker, UINT64);
        } else {
          Value = (UINTN)VA_ARG (Marker, UINTN);
        }

        EfiValueToString (TempBuffer, Value, Flags, Width);
        UnicodeStr = TempBuffer;                              

        for ( ;(*UnicodeStr != '\0') && (Index < NumberOfCharacters - 1); UnicodeStr++) {
          Buffer[Index++] = *UnicodeStr;
        }
        break;

      case 's':
      case 'S':
        UnicodeStr = (CHAR16 *)VA_ARG (Marker, CHAR_W *);
        if (UnicodeStr == NULL) {
          UnicodeStr = L"<null string>";
        }
        for (Count = 0 ;(*UnicodeStr != '\0') && (Index < NumberOfCharacters - 1); UnicodeStr++, Count++) {
          Buffer[Index++] = *UnicodeStr;
        }
        //
        // Add padding if needed
        //
        for (; (Count < Width) && (Index < NumberOfCharacters - 1); Count++) {
          Buffer[Index++] = ' ';
        }

        break;

      case 'a':
        AsciiStr = (CHAR8 *)VA_ARG (Marker, CHAR8 *);
        if (AsciiStr == NULL) {
          AsciiStr = (CHAR8 *) "<null string>";
        }
        for (Count = 0 ;(*AsciiStr != '\0') && (Index < NumberOfCharacters - 1); AsciiStr++, Count++) {
          Buffer[Index++] = (CHAR_W)*AsciiStr;
        }
        //
        // Add padding if needed
        //
        for (;(Count < Width) && (Index < NumberOfCharacters - 1); Count++) {
          Buffer[Index++] = ' ';
        }
        break;

      case 'c':
        Buffer[Index++] = (CHAR_W)VA_ARG (Marker, UINTN);
        break;

      case 'g':
        TmpGUID = VA_ARG (Marker, EFI_GUID *); 
        if (TmpGUID != NULL) {
          Index += GuidToString (
                    TmpGUID, 
                    &Buffer[Index], 
                    BufferLeft
                    );
        }
        break;

      case 't':
        Index += TimeToString (
                  VA_ARG (Marker, EFI_TIME *), 
                  &Buffer[Index], 
                  BufferLeft
                  );
        break;

      case 'r':
        Index += EfiStatusToString (
                  VA_ARG (Marker, EFI_STATUS), 
                  &Buffer[Index], 
                  BufferLeft
                  );
        break;

      case '%':
        Buffer[Index++] = *Format;
        break;
    
      default:
        //
        // if the type is unknown print it to the screen
        //
        Buffer[Index++] = *Format;
      }
      BufferLeft = BufferSize - Index * sizeof(CHAR_W) ;
    } 
  }
  Buffer[Index++] = '\0'; 
   
  return &Buffer[Index] - StartOfBuffer;
}

STATIC
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN  CHAR_W    *Buffer,
  IN  UINTN     BufferSize
  )
/*++

Routine Description:

  VSPrint worker function that prints an EFI_GUID.

Arguments:

  Guid       - Pointer to GUID to print.

  Buffer     - Buffe to print Guid into.
  
  BufferSize - Size of Buffer.

Returns: 

  Number of characters printed.  

--*/
{
  UINTN Size;

  Size = SPrint (
            Buffer,
            BufferSize, 
            STRING_W ("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
            (UINTN)Guid->Data1,                    
            (UINTN)Guid->Data2,
            (UINTN)Guid->Data3,
            (UINTN)Guid->Data4[0],
            (UINTN)Guid->Data4[1],
            (UINTN)Guid->Data4[2],
            (UINTN)Guid->Data4[3],
            (UINTN)Guid->Data4[4],
            (UINTN)Guid->Data4[5],
            (UINTN)Guid->Data4[6],
            (UINTN)Guid->Data4[7]
            );

  //
  // SPrint will null terminate the string. The -1 skips the null
  //
  return Size - 1;
}


STATIC
UINTN
TimeToString (
  IN EFI_TIME   *Time,
  OUT CHAR_W    *Buffer,
  IN  UINTN     BufferSize
  )
/*++

Routine Description:

  VSPrint worker function that prints EFI_TIME.

Arguments:

  Time       - Pointer to EFI_TIME sturcture to print.

  Buffer     - Buffer to print Time into.
  
  BufferSize - Size of Buffer.

Returns: 

  Number of characters printed.  

--*/
{ 
  UINTN Size;

  Size = SPrint (
            Buffer,
            BufferSize, 
            STRING_W ("%02d/%02d/%04d  %02d:%02d"),
            (UINTN)Time->Month,
            (UINTN)Time->Day,
            (UINTN)Time->Year,
            (UINTN)Time->Hour,
            (UINTN)Time->Minute
            );

  //
  // SPrint will null terminate the string. The -1 skips the null
  //
  return Size - 1;
} 

STATIC
UINTN
EfiStatusToString (
  IN EFI_STATUS   Status,
  OUT CHAR_W      *Buffer,
  IN  UINTN       BufferSize
  )
/*++

Routine Description:

  VSPrint worker function that prints EFI_STATUS as a string. If string is
  not known a hex value will be printed.

Arguments:

  Status     -  EFI_STATUS sturcture to print.

  Buffer     - Buffer to print EFI_STATUS message string into.
  
  BufferSize - Size of Buffer.

Returns: 

  Number of characters printed.  

--*/
{
  UINTN   Size;
  CHAR8   *Desc;
  
  Desc = NULL;

  //
  // Can't use global Status String Array as UINTN is not constant for EBC
  //
  if (Status == EFI_SUCCESS) { Desc = (CHAR8 *) "Success"; } else 
  if (Status == EFI_LOAD_ERROR) { Desc = (CHAR8 *) "Load Error"; } else
  if (Status == EFI_INVALID_PARAMETER) { Desc = (CHAR8 *) "Invalid Parameter"; } else
  if (Status == EFI_UNSUPPORTED) { Desc = (CHAR8 *) "Unsupported"; } else
  if (Status == EFI_BAD_BUFFER_SIZE) { Desc = (CHAR8 *) "Bad Buffer Size"; } else
  if (Status == EFI_BUFFER_TOO_SMALL) { Desc = (CHAR8 *) "Buffer Too Small"; } else
  if (Status == EFI_NOT_READY) { Desc = (CHAR8 *) "Not Ready"; } else
  if (Status == EFI_DEVICE_ERROR) { Desc = (CHAR8 *) "Device Error"; } else
  if (Status == EFI_WRITE_PROTECTED) { Desc = (CHAR8 *) "Write Protected"; } else
  if (Status == EFI_OUT_OF_RESOURCES) { Desc = (CHAR8 *) "Out of Resources"; } else
  if (Status == EFI_VOLUME_CORRUPTED) { Desc = (CHAR8 *) "Volume Corrupt"; } else
  if (Status == EFI_VOLUME_FULL) { Desc = (CHAR8 *) "Volume Full"; } else
  if (Status == EFI_NO_MEDIA) { Desc = (CHAR8 *) "No Media"; } else
  if (Status == EFI_MEDIA_CHANGED) { Desc = (CHAR8 *) "Media changed"; } else
  if (Status == EFI_NOT_FOUND) { Desc = (CHAR8 *) "Not Found"; } else
  if (Status == EFI_ACCESS_DENIED) { Desc = (CHAR8 *) "Access Denied"; } else
  if (Status == EFI_NO_RESPONSE) { Desc = (CHAR8 *) "No Response"; } else
  if (Status == EFI_NO_MAPPING) { Desc = (CHAR8 *) "No mapping"; } else
  if (Status == EFI_TIMEOUT) { Desc = (CHAR8 *) "Time out"; } else
  if (Status == EFI_NOT_STARTED) { Desc = (CHAR8 *) "Not started"; } else
  if (Status == EFI_ALREADY_STARTED) { Desc = (CHAR8 *) "Already started"; } else
  if (Status == EFI_ABORTED) { Desc = (CHAR8 *) "Aborted"; } else
  if (Status == EFI_ICMP_ERROR) { Desc = (CHAR8 *) "ICMP Error"; } else
  if (Status == EFI_TFTP_ERROR) { Desc = (CHAR8 *) "TFTP Error"; } else
  if (Status == EFI_PROTOCOL_ERROR) { Desc = (CHAR8 *) "Protocol Error"; } else
  if (Status == EFI_WARN_UNKNOWN_GLYPH) { Desc = (CHAR8 *) "Warning Unknown Glyph"; } else
  if (Status == EFI_WARN_DELETE_FAILURE) { Desc = (CHAR8 *) "Warning Delete Failure"; } else
  if (Status == EFI_WARN_WRITE_FAILURE) { Desc = (CHAR8 *) "Warning Write Failure"; } else
  if (Status == EFI_WARN_BUFFER_TOO_SMALL) { Desc = (CHAR8 *) "Warning Buffer Too Small"; } 
  
  //
  // If we found a match, copy the message to the user's buffer. Otherwise
  // sprint the hex status code to their buffer.
  //
  if (Desc != NULL) {
    Size = SPrint (Buffer, BufferSize, STRING_W ("%a"), Desc);
  } else {
    Size = SPrint (Buffer, BufferSize, STRING_W ("%X"), Status);
  }
  return Size - 1;
}
