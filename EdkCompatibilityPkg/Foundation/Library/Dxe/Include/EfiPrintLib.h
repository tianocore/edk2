/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiPrintLib.h

Abstract:

  Light weight lib to support EFI drivers.

--*/

#ifndef _EFI_PRINT_LIB_H_
#define _EFI_PRINT_LIB_H_

#include EFI_PROTOCOL_DEFINITION(GraphicsOutput)
#include EFI_PROTOCOL_DEFINITION(UgaDraw)
#include EFI_PROTOCOL_DEFINITION(Print)

UINTN
ErrorPrint (
  IN CONST CHAR16 *ErrorString,
  IN CONST CHAR8  *Format,
  ...
  )
/*++

Routine Description:

  Print function for a maximum of EFI_DRIVER_LIB_MAX_PRINT_BUFFER ascii 
  characters.

Arguments:

  ErrorString   - Error message printed first

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
;

VOID
ErrorDumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  )
/*++

Routine Description:

  Dump error info by hex.

Arguments:

  Indent    - Indent number
  Offset    - Offset number
  DataSize  - Size of user data
  UserData  - User data to dump

Returns:

  None

--*/
;

UINTN
Print (
  IN CONST CHAR16  *Format,
  ...
  )
/*++

Routine Description:

    Prints a formatted unicode string to the default console

Arguments:

    fmt         - Format string

Returns:

    Length of string printed to the console

--*/
;

UINTN
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background, OPTIONAL
  IN CHAR16                           *Fmt,
  ...
  )
/*++

Routine Description:

    Prints a formatted unicode string to the default console

Arguments:

    X           - X coordinate to start printing
    
    Y           - Y coordinate to start printing
    
    ForeGround  - Foreground color
    
    BackGround  - Background color

    Fmt         - Format string
    
    ...         - Print arguments

Returns:

    Length of string printed to the console

--*/
;

UINTN
Aprint (
  IN CONST CHAR8  *Format,
  ...
  )
/*++

Routine Description:

  Print function for a maximum of EFI_DRIVER_LIB_MAX_PRINT_BUFFER ascii 
  characters.

Arguments:

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
;

UINTN
UPrint (
  IN CONST CHAR16  *Format,
  ...
  )
/*++

Routine Description:

  Print function for a maximum of EFI_DRIVER_LIB_MAX_PRINT_BUFFER ascii 
  characters.

Arguments:

  Format - Ascii format string see file header for more details.

  ...    - Vararg list consumed by processing Format.

Returns: 

  Number of characters printed.

--*/
;

UINTN
EFIAPI
VSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         StrLen,
  IN  CONST CHAR16  *Format,
  IN  VA_LIST       Marker
  )
/*++

Routine Description:

    Prints a formatted unicode string to a buffer

Arguments:

    StartOfBuffer   - Output buffer to print the formatted string into
    StrLen          - Size of Str.  String is truncated to this size.
                      A size of 0 means there is no limit
    Format          - The format string
    Marker          - Vararg list consumed by processing Format.

Returns:

    String length returned in buffer

--*/
;

UINTN
SPrint (
  OUT CHAR16      *Buffer,
  IN UINTN        BufferSize,
  IN CONST CHAR16 *Format,
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
;

//
// BoxDraw support
//
BOOLEAN
IsValidEfiCntlChar (
  IN  CHAR16  CharC
  )
/*++

Routine Description:

  Test whether a wide char is a valid control char.

Arguments:

  CharC - A char

Returns:

  TRUE    - A control char
  
  FALSE   - Not a control char

--*/
;

BOOLEAN
IsValidAscii (
  IN  CHAR16  Ascii
  )
/*++

Routine Description:

  Test whether a wide char is a normal printable char

Arguments:

  Ascii - A char

Returns:

  True      - A normal, printable char
  FALSE     - Not a normal, printable char

--*/
;

BOOLEAN
LibIsValidTextGraphics (
  IN  CHAR16  Graphic,
  OUT CHAR8   *PcAnsi,    OPTIONAL
  OUT CHAR8   *Ascii      OPTIONAL
  )
/*++

Routine Description:

    Detects if a Unicode char is for Box Drawing text graphics.

Arguments:

    Graphic - Unicode char to test.

    PcAnsi  - Optional pointer to return PCANSI equivalent of Graphic.

    Ascii   - Optional pointer to return Ascii equivalent of Graphic.

Returns:

    TRUE if Gpaphic is a supported Unicode Box Drawing character.

--*/
;

#endif
