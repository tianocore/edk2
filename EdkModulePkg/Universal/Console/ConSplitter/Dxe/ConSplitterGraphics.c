/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ConSplitterGraphics.c

Abstract:

  Support for ConsoleControl protocol. Support for UGA Draw spliter.
  Support for DevNull Console Out. This console uses memory buffers
  to represnt the console. It allows a console to start very early and
  when a new console is added it is synced up with the current console

--*/


#include "ConSplitter.h"

static CHAR16 mCrLfString[3] = { CHAR_CARRIAGE_RETURN, CHAR_LINEFEED, CHAR_NULL };

EFI_STATUS
EFIAPI
ConSpliterConsoleControlGetMode (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL    *This,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE *Mode,
  OUT BOOLEAN                         *GopExists,
  OUT BOOLEAN                         *StdInLocked
  )
/*++

  Routine Description:
    Return the current video mode information. Also returns info about existence
    of UGA Draw devices in system, and if the Std In device is locked. All the
    arguments are optional and only returned if a non NULL pointer is passed in.

  Arguments:
    This - Protocol instance pointer.
    Mode        - Are we in text of grahics mode.
    UgaExists   - TRUE if UGA Spliter has found a UGA device
    StdInLocked - TRUE if StdIn device is keyboard locked

  Returns:
    EFI_SUCCESS - Mode information returned.
    EFI_INVALID_PARAMETER - Invalid parameters.

--*/
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;

  Private = CONSOLE_CONTROL_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  if (Mode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Mode = Private->ConsoleOutputMode;

  if (GopExists != NULL) {
    *GopExists = FALSE;
    for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
      if ((Private->TextOutList[Index].GraphicsOutput != NULL) || (Private->TextOutList[Index].UgaDraw != NULL)) {
        *GopExists = TRUE;
        break;
      }
    }
  }

  if (StdInLocked != NULL) {
    *StdInLocked = ConSpliterConssoleControlStdInLocked ();
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ConSpliterConsoleControlSetMode (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL    *This,
  IN  EFI_CONSOLE_CONTROL_SCREEN_MODE Mode
  )
/*++

  Routine Description:
    Set the current mode to either text or graphics. Graphics is
    for Quiet Boot.

  Arguments:
    This  - Protocol instance pointer.
    Mode  - Mode to set the

  Returns:
    EFI_SUCCESS     - Mode information returned.
    EFI_INVALID_PARAMETER - Invalid parameter.
    EFI_UNSUPPORTED - Operation unsupported.

--*/
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  TEXT_OUT_AND_GOP_DATA           *TextAndGop;
  BOOLEAN                         Supported;

  Private = CONSOLE_CONTROL_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  if (Mode >= EfiConsoleControlScreenMaxValue) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Judge current mode with wanted mode at first.
  //
  if (Private->ConsoleOutputMode == Mode) {
    return EFI_SUCCESS;
  }

  Supported   = FALSE;
  TextAndGop  = &Private->TextOutList[0];
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++, TextAndGop++) {
    if ((TextAndGop->GraphicsOutput != NULL) || (TextAndGop->UgaDraw != NULL)) {
      Supported = TRUE;
      break;
    }
  }

  if ((!Supported) && (Mode == EfiConsoleControlScreenGraphics)) {
    return EFI_UNSUPPORTED;
  }

  Private->ConsoleOutputMode  = Mode;

  TextAndGop = &Private->TextOutList[0];
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++, TextAndGop++) {

    TextAndGop->TextOutEnabled = TRUE;
    //
    // If we are going into Graphics mode disable ConOut to any UGA device
    //
    if ((Mode == EfiConsoleControlScreenGraphics) &&((TextAndGop->GraphicsOutput != NULL) || (TextAndGop->UgaDraw != NULL))) {
      TextAndGop->TextOutEnabled = FALSE;
      DevNullGopSync (Private, TextAndGop->GraphicsOutput, TextAndGop->UgaDraw);
    }
  }

  if (Mode == EfiConsoleControlScreenText) {
    DevNullSyncGopStdOut (Private);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ConSpliterGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This                  - Protocol instance pointer.
    ModeNumber            - The mode number to return information on.
    Info                  - Caller allocated buffer that returns information about ModeNumber.
    SizeOfInfo            - A pointer to the size, in bytes, of the Info buffer.

  Returns:
    EFI_SUCCESS           - Mode information returned.
    EFI_BUFFER_TOO_SMALL  - The Info buffer was too small.
    EFI_DEVICE_ERROR      - A hardware error occurred trying to retrieve the video mode.
    EFI_NOT_STARTED       - Video display is not initialized. Call SetMode ()
    EFI_INVALID_PARAMETER - One of the input args was NULL.

--*/
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  TEXT_OUT_GOP_MODE               *Mode;

  if (This == NULL || Info == NULL || SizeOfInfo == NULL || ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // retrieve private data
  //
  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  if (Private->HardwareNeedsStarting) {
    return EFI_NOT_STARTED;
  }

  *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  CopyMem (*Info, Private->GraphicsOutput.Mode->Info, *SizeOfInfo);
  Mode = &Private->GraphicsOutputModeBuffer[ModeNumber];
  (*Info)->HorizontalResolution = Mode->HorizontalResolution;
  (*Info)->VerticalResolution = Mode->VerticalResolution;
  (*Info)->PixelsPerScanLine = Mode->HorizontalResolution;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ConSpliterGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
  )
/*++

Routine Description:

  Graphics output protocol interface to set video mode

  Arguments:
    This             - Protocol instance pointer.
    ModeNumber       - The mode number to be set.

  Returns:
    EFI_SUCCESS      - Graphics mode was changed.
    EFI_DEVICE_ERROR - The device had an error and could not complete the request.
    EFI_UNSUPPORTED  - ModeNumber is not supported by this device.

--*/
{
  EFI_STATUS                             Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA         *Private;
  UINTN                                  Index;
  EFI_STATUS                             ReturnStatus;
  TEXT_OUT_GOP_MODE                      *Mode;
  UINTN                                  Size;
  EFI_GRAPHICS_OUTPUT_PROTOCOL           *GraphicsOutput;
  UINTN                                  NumberIndex;
  UINTN                                  SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
  EFI_UGA_DRAW_PROTOCOL                  *UgaDraw;

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  if (ModeNumber == This->Mode->Mode) {
    return EFI_SUCCESS;
  }

  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // GopDevNullSetMode ()
  //
  ReturnStatus = EFI_SUCCESS;

  //
  // Free the old version
  //
  if (Private->GraphicsOutputBlt != NULL) {
    FreePool (Private->GraphicsOutputBlt);
  }

  //
  // Allocate the virtual Blt buffer
  //
  Mode = &Private->GraphicsOutputModeBuffer[ModeNumber];
  Size = Mode->HorizontalResolution * Mode->VerticalResolution * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  Private->GraphicsOutputBlt = AllocateZeroPool (Size);

  if (Private->GraphicsOutputBlt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (!Private->HardwareNeedsStarting) {
    if (Private->ConsoleOutputMode != EfiConsoleControlScreenGraphics) {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      //
      // Find corresponding ModeNumber of this GraphicsOutput instance
      //
      for (NumberIndex = 0; NumberIndex < GraphicsOutput->Mode->MaxMode; NumberIndex ++) {
        Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32) NumberIndex, &SizeOfInfo, &Info);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        if ((Info->HorizontalResolution == Mode->HorizontalResolution) && (Info->VerticalResolution == Mode->VerticalResolution)) {
          FreePool (Info);
          break;
        }
        FreePool (Info);
      }

      Status = GraphicsOutput->SetMode (GraphicsOutput, (UINT32) NumberIndex);
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    }

    UgaDraw = Private->TextOutList[Index].UgaDraw;
    if (UgaDraw != NULL) {
      Status = UgaDraw->SetMode (
                          UgaDraw,
                          Mode->HorizontalResolution,
                          Mode->VerticalResolution,
                          32,
                          60
                          );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    }
  }

  This->Mode->Mode = ModeNumber;

  Info = This->Mode->Info;
  Info->HorizontalResolution = Mode->HorizontalResolution;
  Info->VerticalResolution   = Mode->VerticalResolution;
  Info->PixelsPerScanLine    = Mode->HorizontalResolution;

  //
  // Information is not enough here, so the following items remain unchanged:
  //  GraphicsOutputMode->Info->Version, GraphicsOutputMode->Info->PixelFormat
  //  GraphicsOutputMode->SizeOfInfo, GraphicsOutputMode->FrameBufferBase, GraphicsOutputMode->FrameBufferSize
  // These items will be initialized/updated when a new GOP device is added into ConsoleSplitter.
  //

  Private->HardwareNeedsStarting = FALSE;

  return ReturnStatus;
}

STATIC
EFI_STATUS
DevNullGraphicsOutputBlt (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA                *Private,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                 *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION             BltOperation,
  IN  UINTN                                         SourceX,
  IN  UINTN                                         SourceY,
  IN  UINTN                                         DestinationX,
  IN  UINTN                                         DestinationY,
  IN  UINTN                                         Width,
  IN  UINTN                                         Height,
  IN  UINTN                                         Delta         OPTIONAL
  )
{
  UINTN                         SrcY;
  BOOLEAN                       Forward;
  UINTN                         Index;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *ScreenPtr;
  UINTN                         HorizontalResolution;
  UINTN                         VerticalResolution;

  if ((BltOperation < EfiBltVideoFill) || (BltOperation >= EfiGraphicsOutputBltOperationMax)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  HorizontalResolution  = Private->GraphicsOutput.Mode->Info->HorizontalResolution;
  VerticalResolution    = Private->GraphicsOutput.Mode->Info->VerticalResolution;

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if ((SourceY + Height) > VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if ((SourceX + Width) > HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    BltPtr    = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + DestinationY * Delta + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    ScreenPtr = &Private->GraphicsOutputBlt[SourceY * HorizontalResolution + SourceX];
    while (Height) {
      CopyMem (BltPtr, ScreenPtr, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      BltPtr = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltPtr + Delta);
      ScreenPtr += HorizontalResolution;
      Height--;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if ((BltOperation == EfiBltVideoToVideo) && (DestinationY > SourceY)) {
      //
      // Copy backwards, only care the Video to Video Blt
      //
      ScreenPtr = &Private->GraphicsOutputBlt[(DestinationY + Height - 1) * HorizontalResolution + DestinationX];
      SrcY      = SourceY + Height - 1;
      Forward   = FALSE;
    } else {
      //
      // Copy forwards, for other cases
      //
      ScreenPtr = &Private->GraphicsOutputBlt[DestinationY * HorizontalResolution + DestinationX];
      SrcY      = SourceY;
      Forward   = TRUE;
    }

    while (Height != 0) {
      if (BltOperation == EfiBltVideoFill) {
        for (Index = 0; Index < Width; Index++) {
          ScreenPtr[Index] = *BltBuffer;
        }
      } else {
        if (BltOperation == EfiBltBufferToVideo) {
          BltPtr = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + SrcY * Delta + SourceX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        } else {
          BltPtr = &Private->GraphicsOutputBlt[SrcY * HorizontalResolution + SourceX];
        }

        CopyMem (ScreenPtr, BltPtr, Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      }

      if (Forward) {
        ScreenPtr += HorizontalResolution;
        SrcY ++;
      } else {
        ScreenPtr -= HorizontalResolution;
        SrcY --;
      }
      Height--;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ConSpliterGraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL                  *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL                 *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION             BltOperation,
  IN  UINTN                                         SourceX,
  IN  UINTN                                         SourceY,
  IN  UINTN                                         DestinationX,
  IN  UINTN                                         DestinationY,
  IN  UINTN                                         Width,
  IN  UINTN                                         Height,
  IN  UINTN                                         Delta         OPTIONAL
  )
/*++

  Routine Description:
    The following table defines actions for BltOperations:
    EfiBltVideoFill - Write data from the  BltBuffer pixel (SourceX, SourceY)
      directly to every pixel of the video display rectangle
      (DestinationX, DestinationY)
      (DestinationX + Width, DestinationY + Height).
      Only one pixel will be used from the BltBuffer. Delta is NOT used.
    EfiBltVideoToBltBuffer - Read data from the video display rectangle
      (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
      the BltBuffer rectangle (DestinationX, DestinationY )
      (DestinationX + Width, DestinationY + Height). If DestinationX or
      DestinationY is not zero then Delta must be set to the length in bytes
      of a row in the BltBuffer.
    EfiBltBufferToVideo - Write data from the  BltBuffer rectangle
      (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
      video display rectangle (DestinationX, DestinationY)
      (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
      not zero then Delta must be set to the length in bytes of a row in the
      BltBuffer.
    EfiBltVideoToVideo - Copy from the video display rectangle
      (SourceX, SourceY) (SourceX + Width, SourceY + Height) .
      to the video display rectangle (DestinationX, DestinationY)
      (DestinationX + Width, DestinationY + Height).
     The BltBuffer and Delta  are not used in this mode.

  Arguments:
    This          - Protocol instance pointer.
    BltBuffer     - Buffer containing data to blit into video buffer. This
                    buffer has a size of Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    BltOperation  - Operation to perform on BlitBuffer and video memory
    SourceX       - X coordinate of source for the BltBuffer.
    SourceY       - Y coordinate of source for the BltBuffer.
    DestinationX  - X coordinate of destination for the BltBuffer.
    DestinationY  - Y coordinate of destination for the BltBuffer.
    Width         - Width of rectangle in BltBuffer in pixels.
    Height        - Hight of rectangle in BltBuffer in pixels.
    Delta         -

  Returns:
    EFI_SUCCESS           - The Blt operation completed.
    EFI_INVALID_PARAMETER - BltOperation is not valid.
    EFI_DEVICE_ERROR      - A hardware error occured writting to the video
                             buffer.

--*/
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL           *UgaDraw;

  Private = GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Sync up DevNull GOP device
  //
  ReturnStatus = DevNullGraphicsOutputBlt (
                  Private,
                  BltBuffer,
                  BltOperation,
                  SourceX,
                  SourceY,
                  DestinationX,
                  DestinationY,
                  Width,
                  Height,
                  Delta
                  );

  if (Private->ConsoleOutputMode != EfiConsoleControlScreenGraphics) {
    return ReturnStatus;
  }
  //
  // return the worst status met
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    GraphicsOutput = Private->TextOutList[Index].GraphicsOutput;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                              GraphicsOutput,
                              BltBuffer,
                              BltOperation,
                              SourceX,
                              SourceY,
                              DestinationX,
                              DestinationY,
                              Width,
                              Height,
                              Delta
                              );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiBltVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }

    UgaDraw = Private->TextOutList[Index].UgaDraw;
    if (UgaDraw != NULL) {
      Status = UgaDraw->Blt (
                              UgaDraw,
                              (EFI_UGA_PIXEL *) BltBuffer,
                              (EFI_UGA_BLT_OPERATION) BltOperation,
                              SourceX,
                              SourceY,
                              DestinationX,
                              DestinationY,
                              Width,
                              Height,
                              Delta
                              );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      } else if (BltOperation == EfiBltVideoToBltBuffer) {
        //
        // Only need to read the data into buffer one time
        //
        return EFI_SUCCESS;
      }
    }
  }

  return ReturnStatus;
}

EFI_STATUS
DevNullGopSync (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput,
  IN  EFI_UGA_DRAW_PROTOCOL           *UgaDraw
  )
{
  if (GraphicsOutput != NULL) {
    return GraphicsOutput->Blt (
                      GraphicsOutput,
                      Private->GraphicsOutputBlt,
                      EfiBltBufferToVideo,
                      0,
                      0,
                      0,
                      0,
                      Private->GraphicsOutput.Mode->Info->HorizontalResolution,
                      Private->GraphicsOutput.Mode->Info->VerticalResolution,
                      0
                      );
  } else {
    return UgaDraw->Blt (
                      UgaDraw,
                      (EFI_UGA_PIXEL *) Private->GraphicsOutputBlt,
                      EfiUgaBltBufferToVideo,
                      0,
                      0,
                      0,
                      0,
                      Private->GraphicsOutput.Mode->Info->HorizontalResolution,
                      Private->GraphicsOutput.Mode->Info->VerticalResolution,
                      0
                      );
  }
}


EFI_STATUS
DevNullTextOutOutputString (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  CHAR16                          *WString
  )
/*++

  Routine Description:
    Write a Unicode string to the output device.

  Arguments:
    Private - Pointer to the console output splitter's private data. It
              indicates the calling context.
    WString - The NULL-terminated Unicode string to be displayed on the output
              device(s). All output devices must also support the Unicode
              drawing defined in this file.

  Returns:
    EFI_SUCCESS            - The string was output to the device.
    EFI_DEVICE_ERROR       - The device reported an error while attempting to
                              output the text.
    EFI_UNSUPPORTED        - The output device's mode is not currently in a
                              defined text mode.
    EFI_WARN_UNKNOWN_GLYPH - This warning code indicates that some of the
                              characters in the Unicode string could not be
                              rendered and were skipped.

--*/
{
  UINTN                       SizeScreen;
  UINTN                       SizeAttribute;
  UINTN                       Index;
  EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
  CHAR16                      *Screen;
  CHAR16                      *NullScreen;
  CHAR16                      InsertChar;
  CHAR16                      TempChar;
  CHAR16                      *PStr;
  INT32                       *Attribute;
  INT32                       *NullAttributes;
  INT32                       CurrentWidth;
  UINTN                       LastRow;
  UINTN                       MaxColumn;

  Mode            = &Private->TextOutMode;
  NullScreen      = Private->DevNullScreen;
  NullAttributes  = Private->DevNullAttributes;
  LastRow         = Private->DevNullRows - 1;
  MaxColumn       = Private->DevNullColumns;

  if (Mode->Attribute & EFI_WIDE_ATTRIBUTE) {
    CurrentWidth = 2;
  } else {
    CurrentWidth = 1;
  }

  while (*WString) {

    if (*WString == CHAR_BACKSPACE) {
      //
      // If the cursor is at the left edge of the display, then move the cursor
      // one row up.
      //
      if (Mode->CursorColumn == 0 && Mode->CursorRow > 0) {
        Mode->CursorRow--;
        Mode->CursorColumn = (INT32) MaxColumn;
      }

      //
      // If the cursor is not at the left edge of the display,
      // then move the cursor left one column.
      //
      if (Mode->CursorColumn > 0) {
        Mode->CursorColumn--;
        if (Mode->CursorColumn > 0 &&
            NullAttributes[Mode->CursorRow * MaxColumn + Mode->CursorColumn - 1] & EFI_WIDE_ATTRIBUTE
            ) {
          Mode->CursorColumn--;

          //
          // Insert an extra backspace
          //
          InsertChar  = CHAR_BACKSPACE;
          PStr        = WString + 1;
          while (*PStr) {
            TempChar    = *PStr;
            *PStr       = InsertChar;
            InsertChar  = TempChar;
            PStr++;
          }

          *PStr     = InsertChar;
          *(++PStr) = 0;

          WString++;
        }
      }

      WString++;

    } else if (*WString == CHAR_LINEFEED) {
      //
      // If the cursor is at the bottom of the display,
      // then scroll the display one row, and do not update
      // the cursor position. Otherwise, move the cursor down one row.
      //
      if (Mode->CursorRow == (INT32) (LastRow)) {
        //
        // Scroll Screen Up One Row
        //
        SizeAttribute = LastRow * MaxColumn;
        CopyMem (
          NullAttributes,
          NullAttributes + MaxColumn,
          SizeAttribute * sizeof (INT32)
          );

        //
        // Each row has an ending CHAR_NULL. So one more character each line
        // for DevNullScreen than DevNullAttributes
        //
        SizeScreen = SizeAttribute + LastRow;
        CopyMem (
          NullScreen,
          NullScreen + (MaxColumn + 1),
          SizeScreen * sizeof (CHAR16)
          );

        //
        // Print Blank Line at last line
        //
        Screen    = NullScreen + SizeScreen;
        Attribute = NullAttributes + SizeAttribute;

        for (Index = 0; Index < MaxColumn; Index++, Screen++, Attribute++) {
          *Screen     = ' ';
          *Attribute  = Mode->Attribute;
        }
      } else {
        Mode->CursorRow++;
      }

      WString++;
    } else if (*WString == CHAR_CARRIAGE_RETURN) {
      //
      // Move the cursor to the beginning of the current row.
      //
      Mode->CursorColumn = 0;
      WString++;
    } else {
      //
      // Print the character at the current cursor position and
      // move the cursor right one column. If this moves the cursor
      // past the right edge of the display, then the line should wrap to
      // the beginning of the next line. This is equivalent to inserting
      // a CR and an LF. Note that if the cursor is at the bottom of the
      // display, and the line wraps, then the display will be scrolled
      // one line.
      //
      Index = Mode->CursorRow * MaxColumn + Mode->CursorColumn;

      while (Mode->CursorColumn < (INT32) MaxColumn) {
        if (*WString == CHAR_NULL) {
          break;
        }

        if (*WString == CHAR_BACKSPACE) {
          break;
        }

        if (*WString == CHAR_LINEFEED) {
          break;
        }

        if (*WString == CHAR_CARRIAGE_RETURN) {
          break;
        }

        if (*WString == WIDE_CHAR || *WString == NARROW_CHAR) {
          CurrentWidth = (*WString == WIDE_CHAR) ? 2 : 1;
          WString++;
          continue;
        }

        if (Mode->CursorColumn + CurrentWidth > (INT32) MaxColumn) {
          //
          // If a wide char is at the rightmost column, then move the char
          // to the beginning of the next row
          //
          NullScreen[Index + Mode->CursorRow] = L' ';
          NullAttributes[Index]               = Mode->Attribute | (UINT32) EFI_WIDE_ATTRIBUTE;
          Index++;
          Mode->CursorColumn++;
        } else {
          NullScreen[Index + Mode->CursorRow] = *WString;
          NullAttributes[Index]               = Mode->Attribute;
          if (CurrentWidth == 1) {
            NullAttributes[Index] &= (~ (UINT32) EFI_WIDE_ATTRIBUTE);
          } else {
            NullAttributes[Index] |= (UINT32) EFI_WIDE_ATTRIBUTE;
            NullAttributes[Index + 1] &= (~ (UINT32) EFI_WIDE_ATTRIBUTE);
          }

          Index += CurrentWidth;
          WString++;
          Mode->CursorColumn += CurrentWidth;
        }
      }
      //
      // At the end of line, output carriage return and line feed
      //
      if (Mode->CursorColumn >= (INT32) MaxColumn) {
        DevNullTextOutOutputString (Private, mCrLfString);
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DevNullTextOutSetMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  UINTN                           ModeNumber
  )
/*++

  Routine Description:
    Sets the output device(s) to a specified mode.

  Arguments:
    Private    - Private data structure pointer.
    ModeNumber - The mode number to set.

  Returns:
    EFI_SUCCESS      - The requested text mode was set.
    EFI_DEVICE_ERROR - The device had an error and
                       could not complete the request.
    EFI_UNSUPPORTED - The mode number was not valid.
    EFI_OUT_OF_RESOURCES - Out of resources.

--*/
{
  UINTN                         Size;
  UINTN                         Row;
  UINTN                         Column;
  TEXT_OUT_SPLITTER_QUERY_DATA  *Mode;

  //
  // No extra check for ModeNumber here, as it has been checked in
  // ConSplitterTextOutSetMode. And mode 0 should always be supported.
  //
  Mode    = &(Private->TextOutQueryData[ModeNumber]);
  Row     = Mode->Rows;
  Column  = Mode->Columns;

  if (Row <= 0 && Column <= 0) {
    return EFI_UNSUPPORTED;
  }

  if (Private->DevNullColumns != Column || Private->DevNullRows != Row) {

    Private->TextOutMode.Mode = (INT32) ModeNumber;
    Private->DevNullColumns   = Column;
    Private->DevNullRows      = Row;

    if (Private->DevNullScreen != NULL) {
      FreePool (Private->DevNullScreen);
    }

    Size                    = (Row * (Column + 1)) * sizeof (CHAR16);
    Private->DevNullScreen  = AllocateZeroPool (Size);
    if (Private->DevNullScreen == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (Private->DevNullAttributes != NULL) {
      FreePool (Private->DevNullAttributes);
    }

    Size                        = Row * Column * sizeof (INT32);
    Private->DevNullAttributes  = AllocateZeroPool (Size);
    if (Private->DevNullAttributes == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DevNullTextOutClearScreen (Private);

  return EFI_SUCCESS;
}

EFI_STATUS
DevNullTextOutClearScreen (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private
  )
/*++

  Routine Description:
    Clears the output device(s) display to the currently selected background
    color.

  Arguments:
    Private     - Protocol instance pointer.

  Returns:
    EFI_SUCCESS      - The operation completed successfully.
    EFI_DEVICE_ERROR - The device had an error and
                       could not complete the request.
    EFI_UNSUPPORTED - The output device is not in a valid text mode.

--*/
{
  UINTN   Row;
  UINTN   Column;
  CHAR16  *Screen;
  INT32   *Attributes;
  INT32   CurrentAttribute;

  //
  // Clear the DevNull Text Out Buffers.
  // The screen is filled with spaces.
  // The attributes are all synced with the current Simple Text Out Attribute
  //
  Screen            = Private->DevNullScreen;
  Attributes        = Private->DevNullAttributes;
  CurrentAttribute  = Private->TextOutMode.Attribute;

  for (Row = 0; Row < Private->DevNullRows; Row++) {
    for (Column = 0; Column < Private->DevNullColumns; Column++, Screen++, Attributes++) {
      *Screen     = ' ';
      *Attributes = CurrentAttribute;
    }
    //
    // Each line of the screen has a NULL on the end so we must skip over it
    //
    Screen++;
  }

  DevNullTextOutSetCursorPosition (Private, 0, 0);

  return DevNullTextOutEnableCursor (Private, TRUE);
}

EFI_STATUS
DevNullTextOutSetCursorPosition (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  UINTN                           Column,
  IN  UINTN                           Row
  )
/*++

  Routine Description:
    Sets the current coordinates of the cursor position

  Arguments:
    Private       - Protocol instance pointer.
    Column, Row - the position to set the cursor to. Must be greater than or
                  equal to zero and less than the number of columns and rows
                  by QueryMode ().

  Returns:
    EFI_SUCCESS      - The operation completed successfully.
    EFI_DEVICE_ERROR - The device had an error and
                       could not complete the request.
    EFI_UNSUPPORTED - The output device is not in a valid text mode, or the
                       cursor position is invalid for the current mode.

--*/
{
  //
  // No need to do extra check here as whether (Column, Row) is valid has
  // been checked in ConSplitterTextOutSetCursorPosition. And (0, 0) should
  // always be supported.
  //
  Private->TextOutMode.CursorColumn = (INT32) Column;
  Private->TextOutMode.CursorRow    = (INT32) Row;

  return EFI_SUCCESS;
}

EFI_STATUS
DevNullTextOutEnableCursor (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  BOOLEAN                         Visible
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.EnableCursor().
    In this driver, the cursor cannot be hidden.

  Arguments:

    Private - Indicates the calling context.

    Visible - If TRUE, the cursor is set to be visible, If FALSE, the cursor
              is set to be invisible.

  Returns:

    EFI_SUCCESS - The request is valid.


--*/
{
  Private->TextOutMode.CursorVisible = Visible;

  return EFI_SUCCESS;
}

EFI_STATUS
DevNullSyncGopStdOut (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private
  )
/*++
  Routine Description:
    Take the DevNull TextOut device and update the Simple Text Out on every
    UGA device.

  Arguments:
    Private - Indicates the calling context.

  Returns:
    EFI_SUCCESS - The request is valid.
    other       - Return status of TextOut->OutputString ()

--*/
{
  EFI_STATUS                    Status;
  EFI_STATUS                    ReturnStatus;
  UINTN                         Row;
  UINTN                         Column;
  UINTN                         List;
  UINTN                         MaxColumn;
  UINTN                         CurrentColumn;
  UINTN                         StartRow;
  UINTN                         StartColumn;
  INT32                         StartAttribute;
  BOOLEAN                       StartCursorState;
  CHAR16                        *Screen;
  CHAR16                        *Str;
  CHAR16                        *Buffer;
  CHAR16                        *BufferTail;
  CHAR16                        *ScreenStart;
  INT32                         CurrentAttribute;
  INT32                         *Attributes;
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *Sto;

  //
  // Save the devices Attributes, Cursor enable state and location
  //
  StartColumn       = Private->TextOutMode.CursorColumn;
  StartRow          = Private->TextOutMode.CursorRow;
  StartAttribute    = Private->TextOutMode.Attribute;
  StartCursorState  = Private->TextOutMode.CursorVisible;

  for (List = 0; List < Private->CurrentNumberOfConsoles; List++) {

    Sto = Private->TextOutList[List].TextOut;

    //
    // Skip non GOP/UGA devices
    //
    if ((Private->TextOutList[List].GraphicsOutput != NULL) || (Private->TextOutList[List].UgaDraw != NULL)) {
      Sto->EnableCursor (Sto, FALSE);
      Sto->ClearScreen (Sto);
    }
  }

  ReturnStatus  = EFI_SUCCESS;
  Screen        = Private->DevNullScreen;
  Attributes    = Private->DevNullAttributes;
  MaxColumn     = Private->DevNullColumns;

  Buffer        = AllocateZeroPool ((MaxColumn + 1) * sizeof (CHAR16));

  for (Row = 0; Row < Private->DevNullRows; Row++, Screen += (MaxColumn + 1), Attributes += MaxColumn) {

    if (Row == (Private->DevNullRows - 1)) {
      //
      // Don't ever sync the last character as it will scroll the screen
      //
      Screen[MaxColumn - 1] = 0x00;
    }

    Column = 0;
    while (Column < MaxColumn) {
      if (Screen[Column]) {
        CurrentAttribute  = Attributes[Column];
        CurrentColumn     = Column;
        ScreenStart       = &Screen[Column];

        //
        // the line end is alway 0x0. So Column should be less than MaxColumn
        // It should be still in the same row
        //
        for (Str = ScreenStart, BufferTail = Buffer; *Str != 0; Str++, Column++) {

          if (Attributes[Column] != CurrentAttribute) {
            Column--;
            break;
          }

          *BufferTail = *Str;
          BufferTail++;
          if (Attributes[Column] & EFI_WIDE_ATTRIBUTE) {
            Str++;
            Column++;
          }
        }

        *BufferTail = 0;

        for (List = 0; List < Private->CurrentNumberOfConsoles; List++) {

          Sto = Private->TextOutList[List].TextOut;

          //
          // Skip non GOP/UGA devices
          //
          if ((Private->TextOutList[List].GraphicsOutput != NULL) || (Private->TextOutList[List].UgaDraw != NULL)) {
            Sto->SetAttribute (Sto, CurrentAttribute);
            Sto->SetCursorPosition (Sto, CurrentColumn, Row);
            Status = Sto->OutputString (Sto, Buffer);
            if (EFI_ERROR (Status)) {
              ReturnStatus = Status;
            }
          }
        }

      }

      Column++;
    }
  }
  //
  // Restore the devices Attributes, Cursor enable state and location
  //
  for (List = 0; List < Private->CurrentNumberOfConsoles; List++) {
    Sto = Private->TextOutList[List].TextOut;

    //
    // Skip non GOP/UGA devices
    //
    if ((Private->TextOutList[List].GraphicsOutput != NULL) || (Private->TextOutList[List].UgaDraw != NULL)) {
      Sto->SetAttribute (Sto, StartAttribute);
      Sto->SetCursorPosition (Sto, StartColumn, StartRow);
      Status = Sto->EnableCursor (Sto, StartCursorState);
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    }
  }

  FreePool (Buffer);

  return ReturnStatus;
}
