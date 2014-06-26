/*++

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Print.c

Abstract:

  Basic Ascii AvSPrintf() function named AvSPrint(). AvSPrint() enables very
  simple implemenation of debug prints.

  You can not Print more than PEI_LIB_MAX_PRINT_BUFFER characters at a 
  time. This makes the implementation very simple.

  AvSPrint format specification has the follwoing form

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
    'S', 's' - argument is an Unicode string
    'g' - argument is a pointer to an EFI_GUID
    't' - argument is a pointer to an EFI_TIME structure
    'c' - argument is an ascii character
    'r' - argument is EFI_STATUS
    '%' - Print a %

--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "Print.h"


STATIC
UINTN
ValueToString (
  IN  OUT CHAR8   *Buffer, 
  IN  INT64       Value, 
  IN  UINTN       Flags, 
  IN  UINTN       Width
  );

STATIC
UINTN
ValueTomHexStr (
  IN  OUT CHAR8   *Buffer, 
  IN  UINT64      Value, 
  IN  UINTN       Flags, 
  IN  UINTN       Width
  );

STATIC
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN OUT CHAR8  *Buffer,
  IN  UINTN     BufferSize
  );

STATIC
UINTN
TimeToString (
  IN  EFI_TIME  *Time,
  IN OUT CHAR8  *Buffer,
  IN  UINTN     BufferSize
  );

STATIC
UINTN
EfiStatusToString (
  IN EFI_STATUS   Status,
  OUT CHAR8       *Buffer,
  IN  UINTN       BufferSize
  );


UINTN
ASPrint (
  OUT CHAR8         *Buffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *Format,
  ...
  )
/*++

Routine Description:

  ASPrint function to process format and place the results in Buffer.

Arguments:

  Buffer     - Ascii buffer to print the results of the parsing of Format into.

  BufferSize - Maximum number of characters to put into buffer. Zero means no 
               limit.

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
{
  UINTN   Return;
  VA_LIST Marker;

  VA_START(Marker, Format);
  Return = AvSPrint(Buffer, BufferSize, Format, Marker);
  VA_END (Marker);
  
  return Return;
}


UINTN
AvSPrint (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  VA_LIST       Marker
  )
/*++

Routine Description:

  AvSPrint function to process format and place the results in Buffer. Since a 
  VA_LIST is used this rountine allows the nesting of Vararg routines. Thus 
  this is the main print working routine

Arguments:

  StartOfBuffer - Ascii buffer to print the results of the parsing of Format into.

  BufferSize    - Maximum number of characters to put into buffer. Zero means 
                  no limit.

  FormatString  - Ascii format string see file header for more details.

  Marker        - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
{
  CHAR8     TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR8     *Buffer;
  CHAR8     *AsciiStr;
  CHAR16    *UnicodeStr;
  CHAR8     *Format;
  UINTN     Index;
  UINTN     Flags;
  UINTN     Width;
  UINTN     Count;
  UINTN     BufferLeft;
  UINT64    Value;
  EFI_GUID  *TmpGUID;
  BOOLEAN   Done;
 
  //
  // Process the format string. Stop if Buffer is over run.
  //
  Buffer              = StartOfBuffer;
  Format              = (CHAR8 *) FormatString;
  BufferLeft          = BufferSize;
  for (Index = 0; (*Format != '\0') && (Index < BufferSize - 1); Format++) {
    if (*Format != '%') {
      if ((*Format == '\n') && (Index < BufferSize - 2)) {
        //
        // If carage return add line feed
        //
        Buffer[Index++] = '\r';
        BufferLeft -= sizeof (CHAR8);
      }

      Buffer[Index++] = *Format;
      BufferLeft -= sizeof (CHAR8);
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
      
        ValueTomHexStr (TempBuffer, Value, Flags, Width);
        AsciiStr = TempBuffer;

        for (; (*AsciiStr != '\0') && (Index < BufferSize - 1); AsciiStr++) {
          Buffer[Index++] = *AsciiStr;
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

        ValueTomHexStr (TempBuffer, Value, Flags, Width);
        AsciiStr = TempBuffer;

        for (; (*AsciiStr != '\0') && (Index < BufferSize - 1); AsciiStr++) {
          Buffer[Index++] = *AsciiStr;
        }
        break;

      case 'd':
        if ((Flags & LONG_TYPE) == LONG_TYPE) {
          Value = VA_ARG (Marker, UINT64);
        } else {
          Value = (UINTN) VA_ARG (Marker, UINTN);
        }

        ValueToString (TempBuffer, Value, Flags, Width);
        AsciiStr = TempBuffer;

        for (; (*AsciiStr != '\0') && (Index < BufferSize - 1); AsciiStr++) {
          Buffer[Index++] = *AsciiStr;
        }
        break;

      case 's':
      case 'S':
        UnicodeStr = (CHAR16 *) VA_ARG (Marker, CHAR8 *);
        if (UnicodeStr == NULL) {
          UnicodeStr = L"<null string>";
        }

        for (Count = 0; (*UnicodeStr != '\0') && (Index < BufferSize - 1); UnicodeStr++, Count++) {
          Buffer[Index++] = (CHAR8) *UnicodeStr;
        }
        //
        // Add padding if needed
        //
        for (; (Count < Width) && (Index < BufferSize - 1); Count++) {
          Buffer[Index++] = ' ';
        }

        break;

      case 'a':
        AsciiStr = (CHAR8 *) VA_ARG (Marker, CHAR8 *);
        if (AsciiStr == NULL) {
          AsciiStr = "<null string>";
        }

        for (Count = 0; (*AsciiStr != '\0') && (Index < BufferSize - 1); AsciiStr++, Count++) {
          Buffer[Index++] = *AsciiStr;
        }
        //
        // Add padding if needed
        //
        for (; (Count < Width) && (Index < BufferSize - 1); Count++) {
          Buffer[Index++] = ' ';
        }
        break;

      case 'c':
        Buffer[Index++] = (CHAR8) VA_ARG (Marker, UINTN);
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

      BufferLeft = BufferSize - Index;
    }
  }

  Buffer[Index++] = '\0';

  return &Buffer[Index] - StartOfBuffer;
}



static CHAR8 mHexStr[] = { '0','1','2','3','4','5','6','7',
                          '8','9','A','B','C','D','E','F' };

STATIC
UINTN
ValueTomHexStr (
  IN  OUT CHAR8   *Buffer, 
  IN  UINT64      Value, 
  IN  UINTN       Flags, 
  IN  UINTN       Width
  )
/*++

Routine Description:

  AvSPrint worker function that prints a Value as a hex number in Buffer

Arguments:

  Buffer - Location to place ascii hex string of Value.

  Value  - Hex value to convert to a string in Buffer.

  Flags  - Flags to use in printing Hex string, see file header for details.

  Width  - Width of hex value.

Returns: 

  Number of characters printed.  

--*/
{
  CHAR8   TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR8   *TempStr;
  CHAR8   Prefix;
  CHAR8   *BufferPtr;
  UINTN   Count;
  UINTN   Index;

  TempStr = TempBuffer;
  BufferPtr = Buffer;
  
  //
  // Count starts at one since we will null terminate. Each iteration of the
  // loop picks off one nibble. Oh yea TempStr ends up backwards
  //
  Count = 0;
  do {
    *(TempStr++) = mHexStr[Value & 0x0f];
    Value = RShiftU64 (Value, 4);
    Count++;
  } while (Value != 0);

  if (Flags & PREFIX_ZERO) {
    Prefix = '0';
  } else if (!(Flags & LEFT_JUSTIFY)) { 
    Prefix = ' ';
  } else {
    Prefix = 0x00;
  }                    
  for (Index = Count; Index < Width; Index++) {
    *(TempStr++) = Prefix;
  }

  //
  // Reverse temp string into Buffer.
  //
  if (Width > 0 && (UINTN) (TempStr - TempBuffer) > Width) {
    TempStr = TempBuffer + Width;
  }
  Index = 0;
  while (TempStr != TempBuffer) {
    *(BufferPtr++) = *(--TempStr);
    Index++;
  }
    
  *BufferPtr = 0;
  return Index;
}

STATIC
UINTN
ValueToString (
  IN  OUT CHAR8   *Buffer, 
  IN  INT64       Value, 
  IN  UINTN       Flags, 
  IN  UINTN       Width
  )
/*++

Routine Description:

  AvSPrint worker function that prints a Value as a decimal number in Buffer

Arguments:

  Buffer - Location to place ascii decimal number string of Value.

  Value  - Decimal value to convert to a string in Buffer.

  Flags  - Flags to use in printing decimal string, see file header for details.

  Width  - Width of hex value.

Returns: 

  Number of characters printed.  

--*/
{
  CHAR8   TempBuffer[CHARACTER_NUMBER_FOR_VALUE];
  CHAR8   *TempStr;
  CHAR8   *BufferPtr;
  UINTN   Count;
  UINTN   NumberCount;
  UINTN   Remainder;
  BOOLEAN Negative;
  UINTN   Index;

  Negative    = FALSE;
  TempStr     = TempBuffer;
  BufferPtr   = Buffer;
  Count       = 0;
  NumberCount = 0;
  Remainder   = 0;

  if (Value < 0) {
    Negative = TRUE;
    Value    = -Value;
  }

  do {
    Value = (INT64)DivU64x32 ((UINT64)Value, 10, &Remainder);
    *(TempStr++) = (CHAR8)(Remainder + '0');
    Count++;
    NumberCount++;
    if ((Flags & COMMA_TYPE) == COMMA_TYPE) {
      if (NumberCount % 3 == 0 && Value != 0) {
        *(TempStr++) = ',';
        Count++;
      }
    }
  } while (Value != 0);

  if (Negative) {
    *(BufferPtr++) = '-';
    Count++;
  }

  //
  // Reverse temp string into Buffer.
  //
  if (Width > 0 && (UINTN) (TempStr - TempBuffer) > Width) {
    TempStr = TempBuffer + Width;
  }
  Index = 0;
  while (TempStr != TempBuffer) {
    *(BufferPtr++) = *(--TempStr);
  }  

  *BufferPtr = 0;
  return Index;
}

STATIC
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN  CHAR8     *Buffer,
  IN  UINTN     BufferSize
  )
/*++

Routine Description:

  AvSPrint worker function that prints an EFI_GUID.

Arguments:

  Guid       - Pointer to GUID to print.

  Buffer     - Buffe to print Guid into.
  
  BufferSize - Size of Buffer.

Returns: 

  Number of characters printed.  

--*/
{
  UINTN Size;

  Size = ASPrint (
            Buffer,
            BufferSize, 
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
  // ASPrint will null terminate the string. The -1 skips the null
  //
  return Size - 1;
}


STATIC
UINTN
TimeToString (
  IN EFI_TIME   *Time,
  OUT CHAR8     *Buffer,
  IN  UINTN     BufferSize
  )
/*++

Routine Description:

  AvSPrint worker function that prints EFI_TIME.

Arguments:

  Time       - Pointer to EFI_TIME sturcture to print.

  Buffer     - Buffer to print Time into.
  
  BufferSize - Size of Buffer.

Returns: 

  Number of characters printed.  

--*/
{ 
  UINTN Size;

  Size = ASPrint (
            Buffer,
            BufferSize, 
            "%02d/%02d/%04d  %02d:%02d",
            (UINTN)Time->Month,
            (UINTN)Time->Day,
            (UINTN)Time->Year,
            (UINTN)Time->Hour,
            (UINTN)Time->Minute
            );

  //
  // ASPrint will null terminate the string. The -1 skips the null
  //
  return Size - 1;
} 

STATIC
UINTN
EfiStatusToString (
  IN EFI_STATUS   Status,
  OUT CHAR8       *Buffer,
  IN  UINTN       BufferSize
  )
/*++

Routine Description:

  AvSPrint worker function that prints EFI_STATUS as a string. If string is
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
  
  if (Status == EFI_SUCCESS) {
    Desc = "Success";
  } else if (Status == EFI_LOAD_ERROR) {
    Desc = "Load Error";
  } else if (Status == EFI_INVALID_PARAMETER) {
    Desc = "Invalid Parameter";
  } else if (Status == EFI_UNSUPPORTED) {
    Desc = "Unsupported";
  } else if (Status == EFI_BAD_BUFFER_SIZE) {
    Desc = "Bad Buffer Size";
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    Desc = "Buffer Too Small";
  } else if (Status == EFI_NOT_READY) {
    Desc = "Not Ready";
  } else if (Status == EFI_DEVICE_ERROR) {
    Desc = "Device Error";
  } else if (Status == EFI_WRITE_PROTECTED) {
    Desc = "Write Protected";
  } else if (Status == EFI_OUT_OF_RESOURCES) {
    Desc = "Out of Resources";
  } else if (Status == EFI_VOLUME_CORRUPTED) {
    Desc = "Volume Corrupt";
  } else if (Status == EFI_VOLUME_FULL) {
    Desc = "Volume Full";
  } else if (Status == EFI_NO_MEDIA) {
    Desc = "No Media";
  } else if (Status == EFI_MEDIA_CHANGED) {
    Desc = "Media changed";
  } else if (Status == EFI_NOT_FOUND) {
    Desc = "Not Found";
  } else if (Status == EFI_ACCESS_DENIED) {
    Desc = "Access Denied";
  } else if (Status == EFI_NO_RESPONSE) {
    Desc = "No Response";
  } else if (Status == EFI_NO_MAPPING) {
    Desc = "No mapping";
  } else if (Status == EFI_TIMEOUT) {
    Desc = "Time out";
  } else if (Status == EFI_NOT_STARTED) {
    Desc = "Not started";
  } else if (Status == EFI_ALREADY_STARTED) {
    Desc = "Already started";
  } else if (Status == EFI_ABORTED) {
    Desc = "Aborted";
  } else if (Status == EFI_ICMP_ERROR) {
    Desc = "ICMP Error";
  } else if (Status == EFI_TFTP_ERROR) {
    Desc = "TFTP Error";
  } else if (Status == EFI_PROTOCOL_ERROR) {
    Desc = "Protocol Error";
  } else if (Status == EFI_WARN_UNKNOWN_GLYPH) {
    Desc = "Warning Unknown Glyph";
  } else if (Status == EFI_WARN_DELETE_FAILURE) {
    Desc = "Warning Delete Failure";
  } else if (Status == EFI_WARN_WRITE_FAILURE) {
    Desc = "Warning Write Failure";
  } else if (Status == EFI_WARN_BUFFER_TOO_SMALL) {
    Desc = "Warning Buffer Too Small";
  } else {
    Desc = NULL;
  }
  //
  // If we found a match, copy the message to the user's buffer. Otherwise
  // sprint the hex status code to their buffer.
  //
  if (Desc != NULL) {
    Size = ASPrint (Buffer, BufferSize, "%a", Desc);
  } else {
    Size = ASPrint (Buffer, BufferSize, "%X", Status);
  }
  return Size - 1;
}
