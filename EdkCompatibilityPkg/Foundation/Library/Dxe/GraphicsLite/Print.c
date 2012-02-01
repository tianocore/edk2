/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
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
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include EFI_PROTOCOL_DEFINITION (HiiFont)
#else
#include EFI_PROTOCOL_DEFINITION (Hii)
#endif

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL  mEfiColors[16] = {
  {0x00, 0x00, 0x00, 0x00},
  {0x98, 0x00, 0x00, 0x00},
  {0x00, 0x98, 0x00, 0x00},
  {0x98, 0x98, 0x00, 0x00},
  {0x00, 0x00, 0x98, 0x00},
  {0x98, 0x00, 0x98, 0x00},
  {0x00, 0x98, 0x98, 0x00},
  {0x98, 0x98, 0x98, 0x00},
  {0x10, 0x10, 0x10, 0x00},
  {0xff, 0x10, 0x10, 0x00},
  {0x10, 0xff, 0x10, 0x00},
  {0xff, 0xff, 0x10, 0x00},
  {0x10, 0x10, 0xff, 0x00},
  {0xf0, 0x10, 0xff, 0x00},
  {0x10, 0xff, 0xff, 0x00},
  {0xff, 0xff, 0xff, 0x00},
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

  Length of string printed to the console

--*/
{
  VOID                           *Buffer;
  EFI_STATUS                     Status;
  UINTN                          Index;
  CHAR16                         *UnicodeWeight;
  UINT32                         HorizontalResolution;
  UINT32                         VerticalResolution;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  UINTN                          BufferLen;
  UINTN                          LineBufferLen;
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EFI_HII_FONT_PROTOCOL          *HiiFont;
  EFI_IMAGE_OUTPUT               *Blt;
  EFI_FONT_DISPLAY_INFO          *FontInfo;  
#else
  EFI_HII_PROTOCOL               *Hii;
  UINT16                         GlyphWidth;
  UINT32                         GlyphStatus;
  UINT16                         StringIndex;
  EFI_NARROW_GLYPH               *Glyph;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *LineBuffer;
#endif

  //
  // For now, allocate an arbitrarily long buffer
  //
  BufferLen = 0;
  Buffer = EfiLibAllocateZeroPool (0x10000);
  if (Buffer == NULL) {
    return 0;
  }

  if (GraphicsOutput != NULL) {
    HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
  } else {
    UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
  }
  ASSERT ((HorizontalResolution != 0) && (VerticalResolution !=0));
  
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  Blt      = NULL;
  FontInfo = NULL;
  ASSERT (GraphicsOutput != NULL);
  Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
  if (EFI_ERROR (Status)) {
    goto Error;
  }  
#else  
  LineBuffer = NULL;
  Status = gBS->LocateProtocol (&gEfiHiiProtocolGuid, NULL, (VOID**)&Hii);
  if (EFI_ERROR (Status)) {
    goto Error;
  }
  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * GLYPH_HEIGHT;
  LineBuffer = EfiLibAllocatePool (LineBufferLen);
  if (LineBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }  
#endif

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

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * EFI_GLYPH_HEIGHT;
  if (EFI_GLYPH_WIDTH * EFI_GLYPH_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * BufferLen > LineBufferLen) {
     Status = EFI_INVALID_PARAMETER;
     goto Error;
  }

  Blt = (EFI_IMAGE_OUTPUT *) EfiLibAllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Blt == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Blt->Width        = (UINT16) (HorizontalResolution);
  Blt->Height       = (UINT16) (VerticalResolution);
  Blt->Image.Screen = GraphicsOutput;
   
  FontInfo = (EFI_FONT_DISPLAY_INFO *) EfiLibAllocateZeroPool (sizeof (EFI_FONT_DISPLAY_INFO));
  if (FontInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }
  if (Foreground != NULL) {
    EfiCopyMem (&FontInfo->ForegroundColor, Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    EfiCopyMem (
      &FontInfo->ForegroundColor, 
      &mEfiColors[Sto->Mode->Attribute & 0x0f], 
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }
  if (Background != NULL) {
    EfiCopyMem (&FontInfo->BackgroundColor, Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    EfiCopyMem (
      &FontInfo->BackgroundColor, 
      &mEfiColors[Sto->Mode->Attribute >> 4], 
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }

  Status = HiiFont->StringToImage (
                       HiiFont,
                       EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_DIRECT_TO_SCREEN,
                       Buffer,
                       FontInfo,
                       &Blt,
                       X,
                       Y,
                       NULL,
                       NULL,
                       NULL
                       );
  
#else
  GlyphStatus = 0;

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

#endif

Error:
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EfiLibSafeFreePool (Blt);
  EfiLibSafeFreePool (FontInfo);
#else
  EfiLibSafeFreePool (LineBuffer);
#endif  
  gBS->FreePool (Buffer);
  
  if (EFI_ERROR (Status)) {
    return 0;
  }

  return BufferLen;
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
  UINTN                         LengthOfPrinted;

  Handle = gST->ConsoleOutHandle;

  GraphicsOutput = NULL;
  UgaDraw = NULL;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );

  if (EFI_ERROR (Status) || (GraphicsOutput == NULL)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );

    if (EFI_ERROR (Status) || (UgaDraw == NULL)) {
      return 0;
    }
  }

  Sto = NULL;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &Sto
                  );

  if (EFI_ERROR (Status) || (Sto == NULL)) {
    return 0;
  }

  VA_START (Args, Fmt);
  LengthOfPrinted = _IPrint (GraphicsOutput, UgaDraw, Sto, X, Y, ForeGround, BackGround, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
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
  EFI_STATUS          Status;
  EFI_PRINT_PROTOCOL  *PrintProtocol;

  Status = gBS->LocateProtocol (
                  &gEfiPrintProtocolGuid,
                  NULL,
                  (VOID **) &PrintProtocol
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
