/** @file
  Mde UEFI library API implementation.
  Print to StdErr or ConOut defined in EFI_SYSTEM_TABLE

  Copyright (c) 2007 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiLibInternal.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiColors[16] = {
  { 0x00, 0x00, 0x00, 0x00 },
  { 0x98, 0x00, 0x00, 0x00 },
  { 0x00, 0x98, 0x00, 0x00 },
  { 0x98, 0x98, 0x00, 0x00 },
  { 0x00, 0x00, 0x98, 0x00 },
  { 0x98, 0x00, 0x98, 0x00 },
  { 0x00, 0x98, 0x98, 0x00 },
  { 0x98, 0x98, 0x98, 0x00 },
  { 0x10, 0x10, 0x10, 0x00 },
  { 0xff, 0x10, 0x10, 0x00 },
  { 0x10, 0xff, 0x10, 0x00 },
  { 0xff, 0xff, 0x10, 0x00 },
  { 0x10, 0x10, 0xff, 0x00 },
  { 0xf0, 0x10, 0xff, 0x00 },
  { 0x10, 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff, 0x00 }
};

/**
  Internal function which prints a formatted Unicode string to the console output device
  specified by Console

  This function prints a formatted Unicode string to the console output device
  specified by Console and returns the number of Unicode characters that printed
  to it.  If the length of the formatted Unicode string is greater than PcdUefiLibMaxPrintBufferSize,
  then only the first PcdUefiLibMaxPrintBufferSize characters are sent to Console.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

  @param Format   Null-terminated Unicode format string.
  @param Console  The output console.
  @param Marker   VA_LIST marker for the variable argument list.

  @return The number of Unicode characters in the produced
          output buffer not including the Null-terminator.
**/
UINTN
InternalPrint (
  IN  CONST CHAR16                     *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  UINTN   Return;
  CHAR16  *Buffer;
  UINTN   BufferSize;

  ASSERT (Format != NULL);
  ASSERT (((UINTN) Format & BIT0) == 0);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrint (Buffer, BufferSize, Format, Marker);

  if (Console != NULL && Return > 0) {
    //
    // To be extra safe make sure Console has been initialized
    //
    Console->OutputString (Console, Buffer);
  }

  FreePool (Buffer);

  return Return;
}

/** 
  Prints a formatted Unicode string to the console output device specified by 
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device 
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of Unicode 
  characters that printed to ConOut.  If the length of the formatted Unicode 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

  @param Format   Null-terminated Unicode format string.
  @param ...      Variable argument list whose contents are accessed based 
                  on the format string specified by Format.
  
  @return Number of Unicode characters printed to ConOut.

**/
UINTN
EFIAPI
Print (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = InternalPrint (Format, gST->ConOut, Marker);

  VA_END (Marker);

  return Return;
}

/** 
  Prints a formatted Unicode string to the console output device specified by 
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device 
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of Unicode 
  characters that printed to StdErr.  If the length of the formatted Unicode 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

  @param Format   Null-terminated Unicode format string.
  @param ...      Variable argument list whose contents are accessed based 
                  on the format string specified by Format.
  
  @return Number of Unicode characters printed to StdErr.

**/
UINTN
EFIAPI
ErrorPrint (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  VA_START (Marker, Format);

  Return = InternalPrint( Format, gST->StdErr, Marker);

  VA_END (Marker);

  return Return;
}


/**
  Internal function which prints a formatted ASCII string to the console output device
  specified by Console

  This function prints a formatted ASCII string to the console output device
  specified by Console and returns the number of ASCII characters that printed
  to it.  If the length of the formatted ASCII string is greater than PcdUefiLibMaxPrintBufferSize,
  then only the first PcdUefiLibMaxPrintBufferSize characters are sent to Console.

  If Format is NULL, then ASSERT().

  @param Format   Null-terminated ASCII format string.
  @param Console  The output console.
  @param Marker   VA_LIST marker for the variable argument list.

  @return The number of Unicode characters in the produced
          output buffer not including the Null-terminator.

**/
UINTN
AsciiInternalPrint (
  IN  CONST CHAR8                      *Format,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Console,
  IN  VA_LIST                          Marker
  )
{
  UINTN   Return;
  CHAR16  *Buffer;
  UINTN   BufferSize;

  ASSERT (Format != NULL);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool(BufferSize);
  ASSERT (Buffer != NULL);

  Return = UnicodeVSPrintAsciiFormat (Buffer, BufferSize, Format, Marker);

  if (Console != NULL) {
    //
    // To be extra safe make sure Console has been initialized
    //
    Console->OutputString (Console, Buffer);
  }

  FreePool (Buffer);

  return Return;
}

/** 
  Prints a formatted ASCII string to the console output device specified by 
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device 
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of ASCII 
  characters that printed to ConOut.  If the length of the formatted ASCII 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.
  If Format is NULL, then ASSERT().

  @param Format   Null-terminated ASCII format string.
  @param ...      Variable argument list whose contents are accessed based 
                  on the format string specified by Format.
  
  @return Number of ASCII characters printed to ConOut.

**/
UINTN
EFIAPI
AsciiPrint (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;
  ASSERT (Format != NULL);

  VA_START (Marker, Format);

  Return = AsciiInternalPrint( Format, gST->ConOut, Marker);

  VA_END (Marker);

  return Return;
}

/** 
  Prints a formatted ASCII string to the console output device specified by 
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device 
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of ASCII 
  characters that printed to StdErr.  If the length of the formatted ASCII 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.
  If Format is NULL, then ASSERT().

  @param Format   Null-terminated ASCII format string.
  @param ...      Variable argument list whose contents are accessed based 
                  on the format string specified by Format.
  
  @return Number of ASCII characters printed to ConErr.

**/
UINTN
EFIAPI
AsciiErrorPrint (
  IN CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST Marker;
  UINTN   Return;

  ASSERT (Format != NULL);

  VA_START (Marker, Format);

  Return = AsciiInternalPrint( Format, gST->StdErr, Marker);

  VA_END (Marker);

  return Return;
}

/**
  Internal function to print a formatted Unicode string to a graphics console device specified by
  ConsoleOutputHandle defined in the EFI_SYSTEM_TABLE at the given (X,Y) coordinates.

  This function prints a formatted Unicode string to the graphics console device
  specified by ConsoleOutputHandle in EFI_SYSTEM_TABLE and returns the number of
  Unicode characters printed. The EFI_HII_FONT_PROTOCOL is used to convert the
  string to a bitmap using the glyphs registered with the
  HII database.  No wrapping is performed, so any portions of the string the fall
  outside the active display region will not be displayed.

  If a graphics console device is not associated with the ConsoleOutputHandle
  defined in the EFI_SYSTEM_TABLE then no string is printed, and 0 is returned.
  If the EFI_HII_FONT_PROTOCOL is not present in the handle database, then no
  string is printed, and 0 is returned.

  @param  PointX       X coordinate to print the string.
  @param  PointY       Y coordinate to print the string.
  @param  Foreground   The foreground color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL,
                       then the foreground color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  Background   The background color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL,
                       then the background color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  Buffer       Null-terminated Unicode formatted string.
  @param  PrintNum     The number of Unicode formatted string to be printed.

  @return  Number of Unicode Characters printed. Zero means no any character
           displayed successfully.

**/
UINTN
InternalPrintGraphic (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN CHAR16                           *Buffer,
  IN UINTN                            PrintNum
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  CHAR16                              *UnicodeWeight;
  UINT32                              HorizontalResolution;
  UINT32                              VerticalResolution;
  UINT32                              ColorDepth;
  UINT32                              RefreshRate;
  UINTN                               LineBufferLen;
  EFI_HII_FONT_PROTOCOL               *HiiFont;
  EFI_IMAGE_OUTPUT                    *Blt;
  EFI_FONT_DISPLAY_INFO               FontInfo;
  EFI_HII_ROW_INFO                    *RowInfoArray;
  UINTN                               RowInfoArraySize;
  EFI_GRAPHICS_OUTPUT_PROTOCOL        *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL               *UgaDraw;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL     *Sto;
  EFI_HANDLE                          ConsoleHandle;

  HorizontalResolution  = 0;
  VerticalResolution    = 0;
  Blt                   = NULL;

  ConsoleHandle = gST->ConsoleOutHandle;

  Status = gBS->HandleProtocol (
                  ConsoleHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );

  UgaDraw = NULL;
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // If no GOP available, try to open UGA Draw protocol if supported.
    //
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    ConsoleHandle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );
  }
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->HandleProtocol (
                  ConsoleHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &Sto
                  );

  if (EFI_ERROR (Status)) {
    return 0;
  }

  if (GraphicsOutput != NULL) {
    HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (UgaDraw != NULL && FeaturePcdGet (PcdUgaConsumeSupport)) {
    UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
  } else {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  ASSERT ((HorizontalResolution != 0) && (VerticalResolution !=0));

  Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  UnicodeWeight = Buffer;

  for (Index = 0; UnicodeWeight[Index] != 0; Index++) {
    if (UnicodeWeight[Index] == CHAR_BACKSPACE ||
        UnicodeWeight[Index] == CHAR_LINEFEED  ||
        UnicodeWeight[Index] == CHAR_CARRIAGE_RETURN) {
      UnicodeWeight[Index] = 0;
    }
  }

  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * EFI_GLYPH_HEIGHT;
  if (EFI_GLYPH_WIDTH * EFI_GLYPH_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * PrintNum > LineBufferLen) {
     Status = EFI_INVALID_PARAMETER;
     goto Error;
  }

  Blt = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  ASSERT (Blt != NULL);

  Blt->Width        = (UINT16) (HorizontalResolution);
  Blt->Height       = (UINT16) (VerticalResolution);

  ZeroMem (&FontInfo, sizeof (EFI_FONT_DISPLAY_INFO));

  if (Foreground != NULL) {
    CopyMem (&FontInfo.ForegroundColor, Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    CopyMem (
      &FontInfo.ForegroundColor,
      &mEfiColors[Sto->Mode->Attribute & 0x0f],
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }
  if (Background != NULL) {
    CopyMem (&FontInfo.BackgroundColor, Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    CopyMem (
      &FontInfo.BackgroundColor,
      &mEfiColors[Sto->Mode->Attribute >> 4],
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }

  if (GraphicsOutput != NULL) {
    Blt->Image.Screen = GraphicsOutput;

    Status = HiiFont->StringToImage (
                         HiiFont,
                         EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_DIRECT_TO_SCREEN,
                         Buffer,
                         &FontInfo,
                         &Blt,
                         PointX,
                         PointY,
                         NULL,
                         NULL,
                         NULL
                         );

  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    ASSERT (UgaDraw!= NULL);

    Blt->Image.Bitmap = AllocateZeroPool (Blt->Width * Blt->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    ASSERT (Blt->Image.Bitmap != NULL);

    RowInfoArray = NULL;
    //
    //  StringToImage only support blt'ing image to device using GOP protocol. If GOP is not supported in this platform,
    //  we ask StringToImage to print the string to blt buffer, then blt to device using UgaDraw.
    //
    Status = HiiFont->StringToImage (
                         HiiFont,
                         EFI_HII_IGNORE_IF_NO_GLYPH,
                         Buffer,
                         &FontInfo,
                         &Blt,
                         PointX,
                         PointY,
                         &RowInfoArray,
                         &RowInfoArraySize,
                         NULL
                         );

    if (!EFI_ERROR (Status)) {
      ASSERT (RowInfoArray != NULL);
      //
      // Line breaks are handled by caller of DrawUnicodeWeightAtCursorN, so the updated parameter RowInfoArraySize by StringToImage will
      // always be 1 or 0 (if there is no valid Unicode Char can be printed). ASSERT here to make sure.
      //
      ASSERT (RowInfoArraySize <= 1);

      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) Blt->Image.Bitmap,
                          EfiUgaBltBufferToVideo,
                          PointX,
                          PointY,
                          PointX,
                          PointY,
                          RowInfoArray[0].LineWidth,
                          RowInfoArray[0].LineHeight,
                          Blt->Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    }

    FreePool (RowInfoArray);
    FreePool (Blt->Image.Bitmap);

  } else {
    Status = EFI_UNSUPPORTED;
  }

  FreePool (Blt);

Error:
  if (EFI_ERROR (Status)) {
    return 0;
  } else {
    return PrintNum;
  }
}

/**
  Prints a formatted Unicode string to a graphics console device specified by 
  ConsoleOutputHandle defined in the EFI_SYSTEM_TABLE at the given (X,Y) coordinates.

  This function prints a formatted Unicode string to the graphics console device 
  specified by ConsoleOutputHandle in EFI_SYSTEM_TABLE and returns the number of 
  Unicode characters printed.  If the length of the formatted Unicode string is
  greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are printed.  The EFI_HII_FONT_PROTOCOL
  is used to convert the string to a bitmap using the glyphs registered with the 
  HII database.  No wrapping is performed, so any portions of the string the fall
  outside the active display region will not be displayed.

  If a graphics console device is not associated with the ConsoleOutputHandle 
  defined in the EFI_SYSTEM_TABLE then no string is printed, and 0 is returned.
  If the EFI_HII_FONT_PROTOCOL is not present in the handle database, then no 
  string is printed, and 0 is returned.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().

  @param  PointX       X coordinate to print the string.
  @param  PointY       Y coordinate to print the string.
  @param  ForeGround   The foreground color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL,
                       then the foreground color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  BackGround   The background color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL, 
                       then the background color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  Format       Null-terminated Unicode format string.  See Print Library 
                       for the supported format string syntax.
  @param  ...          Variable argument list whose contents are accessed based on 
                       the format string specified by Format.         

  @return  The number of Unicode characters printed.

**/
UINTN
EFIAPI
PrintXY (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CONST CHAR16                     *Format,
  ...
  )
{
  VA_LIST                             Marker;
  CHAR16                              *Buffer;
  UINTN                               BufferSize;
  UINTN                               PrintNum;
  UINTN                               ReturnNum;

  ASSERT (Format != NULL);
  ASSERT (((UINTN) Format & BIT0) == 0);

  VA_START (Marker, Format);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool (BufferSize);
  ASSERT (Buffer != NULL);

  PrintNum = UnicodeVSPrint (Buffer, BufferSize, Format, Marker);

  ReturnNum = InternalPrintGraphic (PointX, PointY, ForeGround, BackGround, Buffer, PrintNum);

  FreePool (Buffer);

  return ReturnNum;
}

/**
  Prints a formatted ASCII string to a graphics console device specified by 
  ConsoleOutputHandle defined in the EFI_SYSTEM_TABLE at the given (X,Y) coordinates.

  This function prints a formatted ASCII string to the graphics console device 
  specified by ConsoleOutputHandle in EFI_SYSTEM_TABLE and returns the number of 
  ASCII characters printed.  If the length of the formatted ASCII string is
  greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are printed.  The EFI_HII_FONT_PROTOCOL
  is used to convert the string to a bitmap using the glyphs registered with the 
  HII database.  No wrapping is performed, so any portions of the string the fall
  outside the active display region will not be displayed.

  If a graphics console device is not associated with the ConsoleOutputHandle 
  defined in the EFI_SYSTEM_TABLE then no string is printed, and 0 is returned.
  If the EFI_HII_FONT_PROTOCOL is not present in the handle database, then no 
  string is printed, and 0 is returned.
  If Format is NULL, then ASSERT().

  @param  PointX       X coordinate to print the string.
  @param  PointY       Y coordinate to print the string.
  @param  ForeGround   The foreground color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL,
                       then the foreground color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  BackGround   The background color of the string being printed.  This is
                       an optional parameter that may be NULL.  If it is NULL, 
                       then the background color of the current ConOut device
                       in the EFI_SYSTEM_TABLE is used.
  @param  Format       Null-terminated ASCII format string.  See Print Library 
                       for the supported format string syntax.
  @param  ...          Variable argument list whose contents are accessed based on 
                       the format string specified by Format.         

  @return  The number of ASCII characters printed.

**/
UINTN
EFIAPI
AsciiPrintXY (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CONST CHAR8                      *Format,
  ...
  )
{
  VA_LIST                             Marker;
  CHAR16                              *Buffer;
  UINTN                               BufferSize;
  UINTN                               PrintNum;
  UINTN                               ReturnNum;

  ASSERT (Format != NULL);

  VA_START (Marker, Format);

  BufferSize = (PcdGet32 (PcdUefiLibMaxPrintBufferSize) + 1) * sizeof (CHAR16);

  Buffer = (CHAR16 *) AllocatePool (BufferSize);
  ASSERT (Buffer != NULL);

  PrintNum = UnicodeSPrintAsciiFormat (Buffer, BufferSize, Format, Marker);

  ReturnNum = InternalPrintGraphic (PointX, PointY, ForeGround, BackGround, Buffer, PrintNum);

  FreePool (Buffer);

  return ReturnNum;
}

