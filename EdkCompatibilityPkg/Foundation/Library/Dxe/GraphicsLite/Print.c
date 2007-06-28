/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "TianoCommon.h"
#include "EfiCommonLib.h"
#include "PrintWidth.h"
#include "EfiPrintLib.h"
#include "Print.h"
#include EFI_PROTOCOL_DEFINITION (Hii)

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL  mEfiColors[16] = {
  0x00, 0x00, 0x00, 0x00,
  0x98, 0x00, 0x00, 0x00,
  0x00, 0x98, 0x00, 0x00,
  0x98, 0x98, 0x00, 0x00,
  0x00, 0x00, 0x98, 0x00,
  0x98, 0x00, 0x98, 0x00,
  0x00, 0x98, 0x98, 0x00,
  0x98, 0x98, 0x98, 0x00,
  0x10, 0x10, 0x10, 0x00,
  0xff, 0x10, 0x10, 0x00,
  0x10, 0xff, 0x10, 0x00,
  0xff, 0xff, 0x10, 0x00,
  0x10, 0x10, 0xff, 0x00,
  0xf0, 0x10, 0xff, 0x00,
  0x10, 0xff, 0xff, 0x00,
  0xff, 0xff, 0xff, 0x00,
};


UINTN
_IPrint (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput,
  IN EFI_UGA_DRAW_PROTOCOL            *UgaDraw,
  IN EFI_SIMPLE_TEXT_OUT_PROTOCOL     *Sto,
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN CHAR16                           *fmt,
  IN VA_LIST                          args
  )
/*++

Routine Description:

  Display string worker for: Print, PrintAt, IPrint, IPrintAt

Arguments:

  GraphicsOutput  - Graphics output protocol interface

  UgaDraw         - UGA draw protocol interface
  
  Sto             - Simple text out protocol interface
  
  X               - X coordinate to start printing
  
  Y               - Y coordinate to start printing
  
  Foreground      - Foreground color
  
  Background      - Background color
  
  fmt             - Format string
  
  args            - Print arguments

Returns: 

  EFI_SUCCESS             -  success
  EFI_OUT_OF_RESOURCES    -  out of resources

--*/
{
  VOID                           *Buffer;
  EFI_STATUS                     Status;
  UINT16                         GlyphWidth;
  UINT32                         GlyphStatus;
  UINT16                         StringIndex;
  UINTN                          Index;
  CHAR16                         *UnicodeWeight;
  EFI_NARROW_GLYPH               *Glyph;
  EFI_HII_PROTOCOL               *Hii;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *LineBuffer;
  UINT32                         HorizontalResolution;
  UINT32                         VerticalResolution;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  UINTN                          BufferLen;
  UINTN                          LineBufferLen;

  GlyphStatus = 0;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = EfiLibAllocateZeroPool (0x10000);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (GraphicsOutput != NULL) {
    HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
  } else {
    UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
  }
  ASSERT ((HorizontalResolution != 0) && (VerticalResolution !=0));
  
  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * GLYPH_HEIGHT;
  LineBuffer = EfiLibAllocatePool (LineBufferLen);
  if (LineBuffer == NULL) {
    gBS->FreePool (Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (&gEfiHiiProtocolGuid, NULL, &Hii);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  VSPrint (Buffer, 0x10000, fmt, args);
  
  UnicodeWeight = (CHAR16 *) Buffer;

  for (Index = 0; UnicodeWeight[Index] != 0; Index++) {
    if (UnicodeWeight[Index] == CHAR_BACKSPACE ||
        UnicodeWeight[Index] == CHAR_LINEFEED  ||
        UnicodeWeight[Index] == CHAR_CARRIAGE_RETURN) {
      UnicodeWeight[Index] = 0;
    }
  }

  BufferLen = EfiStrLen (Buffer);

  if (GLYPH_WIDTH * GLYPH_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * BufferLen > LineBufferLen) {
     Status = EFI_INVALID_PARAMETER;
     goto Error;
  }

  for (Index = 0; Index < BufferLen; Index++) {
    StringIndex = (UINT16) Index;
    Status      = Hii->GetGlyph (Hii, UnicodeWeight, &StringIndex, (UINT8 **) &Glyph, &GlyphWidth, &GlyphStatus);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if (Foreground == NULL || Background == NULL) {
      Status = Hii->GlyphToBlt (
                      Hii,
                      (UINT8 *) Glyph,
                      mEfiColors[Sto->Mode->Attribute & 0x0f],
                      mEfiColors[Sto->Mode->Attribute >> 4],
                      BufferLen,
                      GlyphWidth,
                      GLYPH_HEIGHT,
                      &LineBuffer[Index * GLYPH_WIDTH]
                      );
    } else {
      Status = Hii->GlyphToBlt (
                      Hii,
                      (UINT8 *) Glyph,
                      *Foreground,
                      *Background,
                      BufferLen,
                      GlyphWidth,
                      GLYPH_HEIGHT,
                      &LineBuffer[Index * GLYPH_WIDTH]
                      );
    }
  }

  //
  // Blt a character to the screen
  //
  if (GraphicsOutput != NULL) {
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        LineBuffer,
                        EfiBltBufferToVideo,
                        0,
                        0,
                        X,
                        Y,
                        GLYPH_WIDTH * BufferLen,
                        GLYPH_HEIGHT,
                        GLYPH_WIDTH * BufferLen * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                        );
  } else {
    Status = UgaDraw->Blt (
                        UgaDraw,
                        (EFI_UGA_PIXEL *) LineBuffer,
                        EfiUgaBltBufferToVideo,
                        0,
                        0,
                        X,
                        Y,
                        GLYPH_WIDTH * BufferLen,
                        GLYPH_HEIGHT,
                        GLYPH_WIDTH * BufferLen * sizeof (EFI_UGA_PIXEL)
                        );
  }

Error:
  gBS->FreePool (LineBuffer);
  gBS->FreePool (Buffer);
  return Status;
}


UINTN
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CHAR_W                           *Fmt,
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
{
  EFI_HANDLE                    Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *Sto;
  EFI_STATUS                    Status;
  VA_LIST                       Args;

  VA_START (Args, Fmt);

  Handle = gST->ConsoleOutHandle;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &GraphicsOutput
                  );

  UgaDraw = NULL;
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUgaDrawProtocolGuid,
                    &UgaDraw
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  &Sto
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return _IPrint (GraphicsOutput, UgaDraw, Sto, X, Y, ForeGround, BackGround, Fmt, Args);
}


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
  EFI_STATUS          Status;
  EFI_PRINT_PROTOCOL  *PrintProtocol;

  Status = gBS->LocateProtocol (
                  &gEfiPrintProtocolGuid,
                  NULL,
                  &PrintProtocol
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  } else {
    return PrintProtocol->VSPrint (
                            StartOfBuffer,
                            BufferSize,
                            FormatString,
                            Marker
                            );
  }
}
