/** @file
  This is the main routine for initializing the Graphics Console support routines.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2025, Loongson Technology Corporation Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "GraphicsConsole.h"

//
// Graphics Console Device Private Data template
//
GRAPHICS_CONSOLE_DEV  mGraphicsConsoleDevTemplate = {
  GRAPHICS_CONSOLE_DEV_SIGNATURE,
  (EFI_GRAPHICS_OUTPUT_PROTOCOL *)NULL,
  {
    GraphicsConsoleConOutReset,
    GraphicsConsoleConOutOutputString,
    GraphicsConsoleConOutTestString,
    GraphicsConsoleConOutQueryMode,
    GraphicsConsoleConOutSetMode,
    GraphicsConsoleConOutSetAttribute,
    GraphicsConsoleConOutClearScreen,
    GraphicsConsoleConOutSetCursorPosition,
    GraphicsConsoleConOutEnableCursor,
    (EFI_SIMPLE_TEXT_OUTPUT_MODE *)NULL
  },
  {
    0,
    -1,
    EFI_TEXT_ATTR (EFI_LIGHTGRAY,          EFI_BLACK),
    0,
    0,
    FALSE
  },
  (GRAPHICS_CONSOLE_MODE_DATA *)NULL,
  (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)NULL
};

GRAPHICS_CONSOLE_MODE_DATA  mGraphicsConsoleModeData[] = {
  { 100, 31 },  //  800 x 600
  { 128, 40 },  // 1024 x 768
  { 160, 42 },  // 1280 x 800
  { 240, 56 },  // 1920 x 1080
  //
  // New modes can be added here.
  // The last entry is specific for full screen mode.
  //
  { 0,   0  }
};

EFI_HII_DATABASE_PROTOCOL  *mHiiDatabase;
EFI_HII_FONT_PROTOCOL      *mHiiFont;
EFI_HII_HANDLE             mHiiHandle;
VOID                       *mHiiRegistration;

EFI_GUID  mFontPackageListGuid = {
  0xf5f219d3, 0x7006, 0x4648, { 0xac, 0x8d, 0xd6, 0x1d, 0xfb, 0x7b, 0xc6, 0xad }
};

CHAR16  mCrLfString[3] = { CHAR_CARRIAGE_RETURN, CHAR_LINEFEED, CHAR_NULL };

EFI_GRAPHICS_OUTPUT_BLT_PIXEL  mGraphicsEfiColors[16] = {
  //
  // B    G    R   reserved
  //
  { 0x00, 0x00, 0x00, 0x00 },  // BLACK
  { 0x98, 0x00, 0x00, 0x00 },  // LIGHTBLUE
  { 0x00, 0x98, 0x00, 0x00 },  // LIGHGREEN
  { 0x98, 0x98, 0x00, 0x00 },  // LIGHCYAN
  { 0x00, 0x00, 0x98, 0x00 },  // LIGHRED
  { 0x98, 0x00, 0x98, 0x00 },  // MAGENTA
  { 0x00, 0x98, 0x98, 0x00 },  // BROWN
  { 0x98, 0x98, 0x98, 0x00 },  // LIGHTGRAY
  { 0x30, 0x30, 0x30, 0x00 },  // DARKGRAY - BRIGHT BLACK
  { 0xff, 0x00, 0x00, 0x00 },  // BLUE
  { 0x00, 0xff, 0x00, 0x00 },  // LIME
  { 0xff, 0xff, 0x00, 0x00 },  // CYAN
  { 0x00, 0x00, 0xff, 0x00 },  // RED
  { 0xff, 0x00, 0xff, 0x00 },  // FUCHSIA
  { 0x00, 0xff, 0xff, 0x00 },  // YELLOW
  { 0xff, 0xff, 0xff, 0x00 }   // WHITE
};

EFI_NARROW_GLYPH  mCursorGlyph = {
  0x0000,
  0x00,
  { 0x00,0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF }
};

CHAR16  SpaceStr[] = { NARROW_CHAR, ' ', 0 };

#define GRAPHICS_CONSOLE_MIN_TEXT_SCALE  1
#define GRAPHICS_CONSOLE_MAX_TEXT_SCALE  2
#define GRAPHICS_CONSOLE_HIDPI_HORIZONTAL_RESOLUTION  1920
#define GRAPHICS_CONSOLE_HIDPI_VERTICAL_RESOLUTION    1080
#define GRAPHICS_CONSOLE_MAX_GLYPH_WIDTH \
  (EFI_GLYPH_WIDTH * GRAPHICS_CONSOLE_MAX_TEXT_SCALE)
#define GRAPHICS_CONSOLE_MAX_GLYPH_HEIGHT \
  (EFI_GLYPH_HEIGHT * GRAPHICS_CONSOLE_MAX_TEXT_SCALE)

EFI_DRIVER_BINDING_PROTOCOL  gGraphicsConsoleDriverBinding = {
  GraphicsConsoleControllerDriverSupported,
  GraphicsConsoleControllerDriverStart,
  GraphicsConsoleControllerDriverStop,
  0xa,
  NULL,
  NULL
};

STATIC
UINTN
GetGraphicsConsoleTextScale (
  IN  UINT32  HorizontalResolution,
  IN  UINT32  VerticalResolution
  )
{
  UINTN  TextScale;
  UINTN  GlyphWidth;
  UINTN  GlyphHeight;

  TextScale = ((HorizontalResolution > GRAPHICS_CONSOLE_HIDPI_HORIZONTAL_RESOLUTION) &&
               (VerticalResolution > GRAPHICS_CONSOLE_HIDPI_VERTICAL_RESOLUTION)) ? 2 : 1;
  if ((TextScale < GRAPHICS_CONSOLE_MIN_TEXT_SCALE) || (TextScale > GRAPHICS_CONSOLE_MAX_TEXT_SCALE)) {
    DEBUG ((DEBUG_WARN, "%a: unsupported text scale %u, using 1x\n", __func__, (UINT32)TextScale));
    return 1;
  }

  GlyphWidth  = EFI_GLYPH_WIDTH * TextScale;
  GlyphHeight = EFI_GLYPH_HEIGHT * TextScale;
  if (((HorizontalResolution / GlyphWidth) < 80) || ((VerticalResolution / GlyphHeight) < 25)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: %ux%u cannot support 80x25 at %ux, using 1x\n",
      __func__,
      HorizontalResolution,
      VerticalResolution,
      (UINT32)TextScale
      ));
    return 1;
  }

  return TextScale;
}

/**
  Fill a graphics output BLT buffer with one color.

  @param[out] Buffer  The BLT buffer to fill.
  @param[in]  Width   The width of Buffer in pixels.
  @param[in]  Height  The height of Buffer in pixels.
  @param[in]  Color   The color to write into each pixel.

**/
STATIC
VOID
FillBltBuffer (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Buffer,
  IN  UINTN                          Width,
  IN  UINTN                          Height,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Color
  )
{
  UINTN  Index;
  UINTN  Size;

  Size = Width * Height;
  for (Index = 0; Index < Size; Index++) {
    Buffer[Index] = *Color;
  }
}

/**
  Scale a graphics output BLT buffer to twice its source width and height.

  @param[in]  Source       The source BLT buffer.
  @param[in]  SourceWidth  The source width in pixels.
  @param[in]  SourceHeight The source height in pixels.
  @param[in]  SourceStride The source row stride in pixels.
  @param[out] Destination  The allocated scaled BLT buffer.

  @retval EFI_SUCCESS            The destination buffer was allocated and filled.
  @retval EFI_INVALID_PARAMETER  A required parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   The destination buffer could not be allocated.

**/
STATIC
EFI_STATUS
ScaleBltBuffer2x (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Source,
  IN  UINTN                          SourceWidth,
  IN  UINTN                          SourceHeight,
  IN  UINTN                          SourceStride,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **Destination
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *ScaledBuffer;
  UINTN                          Row;
  UINTN                          Column;
  UINTN                          DestinationWidth;

  if ((Source == NULL) || (Destination == NULL) || (SourceWidth == 0) || (SourceHeight == 0) ||
      (SourceStride < SourceWidth))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((SourceWidth > (MAX_UINTN / 2)) || (SourceHeight > (MAX_UINTN / 2))) {
    return EFI_OUT_OF_RESOURCES;
  }

  DestinationWidth = SourceWidth * 2;
  if (DestinationWidth > (MAX_UINTN / (SourceHeight * 2) / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScaledBuffer = AllocateZeroPool (
                   DestinationWidth * (SourceHeight * 2) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                   );
  if (ScaledBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Row = 0; Row < SourceHeight; Row++) {
    UINTN  DestinationRow0;
    UINTN  DestinationRow1;

    DestinationRow0 = (Row * 2) * DestinationWidth;
    DestinationRow1 = DestinationRow0 + DestinationWidth;
    for (Column = 0; Column < SourceWidth; Column++) {
      EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Pixel;
      UINTN                          DestinationColumn;

      Pixel             = Source[Row * SourceStride + Column];
      DestinationColumn = Column * 2;

      ScaledBuffer[DestinationRow0 + DestinationColumn]     = Pixel;
      ScaledBuffer[DestinationRow0 + DestinationColumn + 1] = Pixel;
      ScaledBuffer[DestinationRow1 + DestinationColumn]     = Pixel;
      ScaledBuffer[DestinationRow1 + DestinationColumn + 1] = Pixel;
    }
  }

  *Destination = ScaledBuffer;
  return EFI_SUCCESS;
}

/**
  Test to see if Graphics Console could be supported on the Controller.

  Graphics Console could be supported if Graphics Output Protocol
  exists on the Controller.

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;

  GraphicsOutput = NULL;
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // We need to ensure that we do not layer on top of a virtual handle.
  // We need to ensure that the handles produced by the conspliter do not
  // get used.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  } else {
    goto Error;
  }

  //
  // Does Hii Exist?  If not, we aren't ready to run
  //
  Status = EfiLocateHiiProtocol ();

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
Error:
  if (GraphicsOutput != NULL) {
    gBS->CloseProtocol (
           Controller,
           &gEfiGraphicsOutputProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  return Status;
}

/**
  Initialize all the text modes which the graphics console supports.

  It returns information for available text modes that the graphics can support.

  @param[in]  HorizontalResolution     The size of video screen in pixels in the X dimension.
  @param[in]  VerticalResolution       The size of video screen in pixels in the Y dimension.
  @param[in]  GopModeNumber            The graphics mode number which graphics console is based on.
  @param[out] TextModeCount            The total number of text modes that graphics console supports.
  @param[out] TextModeData             The buffer to the text modes column and row information.
                                       Caller is responsible to free it when it's non-NULL.

  @retval EFI_SUCCESS                  The supporting mode information is returned.
  @retval EFI_INVALID_PARAMETER        The parameters are invalid.

**/
EFI_STATUS
InitializeGraphicsConsoleTextMode (
  IN UINT32                       HorizontalResolution,
  IN UINT32                       VerticalResolution,
  IN UINT32                       GopModeNumber,
  OUT UINTN                       *TextModeCount,
  OUT GRAPHICS_CONSOLE_MODE_DATA  **TextModeData
  )
{
  UINTN                       Index;
  UINTN                       Count;
  GRAPHICS_CONSOLE_MODE_DATA  *ModeBuffer;
  GRAPHICS_CONSOLE_MODE_DATA  *NewModeBuffer;
  UINTN                       ValidCount;
  UINTN                       ValidIndex;
  UINTN                       MaxColumns;
  UINTN                       MaxRows;
  UINTN                       TextScale;
  UINTN                       GlyphWidth;
  UINTN                       GlyphHeight;

  if ((TextModeCount == NULL) || (TextModeData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Count = sizeof (mGraphicsConsoleModeData) / sizeof (GRAPHICS_CONSOLE_MODE_DATA);

  //
  // Compute the maximum number of text Rows and Columns that this current graphics mode can support.
  // To make graphics console work well, MaxColumns and MaxRows should not be zero.
  //
  TextScale   = GetGraphicsConsoleTextScale (HorizontalResolution, VerticalResolution);
  GlyphWidth  = EFI_GLYPH_WIDTH * TextScale;
  GlyphHeight = EFI_GLYPH_HEIGHT * TextScale;
  MaxColumns  = HorizontalResolution / GlyphWidth;
  MaxRows     = VerticalResolution / GlyphHeight;

  //
  // According to UEFI spec, all output devices support at least 80x25 text mode.
  //
  ASSERT ((MaxColumns >= 80) && (MaxRows >= 25));

  //
  // Add full screen mode to the last entry.
  //
  mGraphicsConsoleModeData[Count - 1].Columns = MaxColumns;
  mGraphicsConsoleModeData[Count - 1].Rows    = MaxRows;

  //
  // Get defined mode buffer pointer.
  //
  ModeBuffer = mGraphicsConsoleModeData;

  //
  // Here we make sure that the final mode exposed does not include the duplicated modes,
  // and does not include the invalid modes which exceed the max column and row.
  // Reserve 2 modes for 80x25, 80x50 of graphics console.
  //
  NewModeBuffer = AllocateZeroPool (sizeof (GRAPHICS_CONSOLE_MODE_DATA) * (Count + 2));
  if (NewModeBuffer == NULL) {
    ASSERT (NewModeBuffer != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Mode 0 and mode 1 is for 80x25, 80x50 according to UEFI spec.
  //
  ValidCount = 0;

  NewModeBuffer[ValidCount].Columns       = 80;
  NewModeBuffer[ValidCount].Rows          = 25;
  NewModeBuffer[ValidCount].GopWidth      = HorizontalResolution;
  NewModeBuffer[ValidCount].GopHeight     = VerticalResolution;
  NewModeBuffer[ValidCount].GopModeNumber = GopModeNumber;
  NewModeBuffer[ValidCount].GlyphWidth    = GlyphWidth;
  NewModeBuffer[ValidCount].GlyphHeight   = GlyphHeight;
  NewModeBuffer[ValidCount].TextScale     = TextScale;
  NewModeBuffer[ValidCount].DeltaX =
    (HorizontalResolution - (NewModeBuffer[ValidCount].Columns * GlyphWidth)) >> 1;
  NewModeBuffer[ValidCount].DeltaY =
    (VerticalResolution - (NewModeBuffer[ValidCount].Rows * GlyphHeight)) >> 1;
  ValidCount++;

  if ((MaxColumns >= 80) && (MaxRows >= 50)) {
    NewModeBuffer[ValidCount].Columns = 80;
    NewModeBuffer[ValidCount].Rows    = 50;
    NewModeBuffer[ValidCount].DeltaX  = (HorizontalResolution - (80 * GlyphWidth)) >> 1;
    NewModeBuffer[ValidCount].DeltaY  = (VerticalResolution - (50 * GlyphHeight)) >> 1;
  }

  NewModeBuffer[ValidCount].GopWidth      = HorizontalResolution;
  NewModeBuffer[ValidCount].GopHeight     = VerticalResolution;
  NewModeBuffer[ValidCount].GopModeNumber = GopModeNumber;
  NewModeBuffer[ValidCount].GlyphWidth    = GlyphWidth;
  NewModeBuffer[ValidCount].GlyphHeight   = GlyphHeight;
  NewModeBuffer[ValidCount].TextScale     = TextScale;
  ValidCount++;

  //
  // Start from mode 2 to put the valid mode other than 80x25 and 80x50 in the output mode buffer.
  //
  for (Index = 0; Index < Count; Index++) {
    if ((ModeBuffer[Index].Columns == 0) || (ModeBuffer[Index].Rows == 0) ||
        (ModeBuffer[Index].Columns > MaxColumns) || (ModeBuffer[Index].Rows > MaxRows))
    {
      //
      // Skip the pre-defined mode which is invalid or exceeds the max column and row.
      //
      continue;
    }

    for (ValidIndex = 0; ValidIndex < ValidCount; ValidIndex++) {
      if ((ModeBuffer[Index].Columns == NewModeBuffer[ValidIndex].Columns) &&
          (ModeBuffer[Index].Rows == NewModeBuffer[ValidIndex].Rows))
      {
        //
        // Skip the duplicated mode.
        //
        break;
      }
    }

    if (ValidIndex == ValidCount) {
      NewModeBuffer[ValidCount].Columns       = ModeBuffer[Index].Columns;
      NewModeBuffer[ValidCount].Rows          = ModeBuffer[Index].Rows;
      NewModeBuffer[ValidCount].GopWidth      = HorizontalResolution;
      NewModeBuffer[ValidCount].GopHeight     = VerticalResolution;
      NewModeBuffer[ValidCount].GopModeNumber = GopModeNumber;
      NewModeBuffer[ValidCount].GlyphWidth    = GlyphWidth;
      NewModeBuffer[ValidCount].GlyphHeight   = GlyphHeight;
      NewModeBuffer[ValidCount].TextScale     = TextScale;
      NewModeBuffer[ValidCount].DeltaX =
        (HorizontalResolution - (NewModeBuffer[ValidCount].Columns * GlyphWidth)) >> 1;
      NewModeBuffer[ValidCount].DeltaY =
        (VerticalResolution - (NewModeBuffer[ValidCount].Rows * GlyphHeight)) >> 1;
      ValidCount++;
    }
  }

  DEBUG_CODE_BEGIN ();
  for (Index = 0; Index < ValidCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "Graphics - Mode %d, Column = %d, Row = %d, TextScale = %d\n",
      Index,
      NewModeBuffer[Index].Columns,
      NewModeBuffer[Index].Rows,
      (UINT32)NewModeBuffer[Index].TextScale
      ));
  }

  DEBUG_CODE_END ();

  //
  // Return valid mode count and mode information buffer.
  //
  *TextModeCount = ValidCount;
  *TextModeData  = NewModeBuffer;
  return EFI_SUCCESS;
}

/**
  Start this driver on Controller by opening Graphics Output protocol
  and installing Simple Text Out protocol on Controller.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                            Status;
  GRAPHICS_CONSOLE_DEV                  *Private;
  UINT32                                HorizontalResolution;
  UINT32                                VerticalResolution;
  UINT32                                ModeIndex;
  UINTN                                 MaxMode;
  UINT32                                ModeNumber;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE     *Mode;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  INT32                                 PreferMode;
  INT32                                 Index;
  UINTN                                 Column;
  UINTN                                 Row;
  UINTN                                 DefaultColumn;
  UINTN                                 DefaultRow;

  ModeNumber = 0;

  //
  // Initialize the Graphics Console device instance
  //
  Private = AllocateCopyPool (
              sizeof (GRAPHICS_CONSOLE_DEV),
              &mGraphicsConsoleDevTemplate
              );
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->SimpleTextOutput.Mode = &(Private->SimpleTextOutputMode);

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Private->GraphicsOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  HorizontalResolution = PcdGet32 (PcdVideoHorizontalResolution);
  VerticalResolution   = PcdGet32 (PcdVideoVerticalResolution);

  if (Private->GraphicsOutput != NULL) {
    //
    // The console is build on top of Graphics Output Protocol, find the mode number
    // for the user-defined mode; if there are multiple video devices,
    // graphic console driver will set all the video devices to the same mode.
    //
    if ((HorizontalResolution == 0x0) || (VerticalResolution == 0x0)) {
      //
      // Find the highest resolution which GOP supports.
      //
      MaxMode = Private->GraphicsOutput->Mode->MaxMode;

      for (ModeIndex = 0; (UINTN)ModeIndex < MaxMode; ModeIndex++) {
        Status = Private->GraphicsOutput->QueryMode (
                                            Private->GraphicsOutput,
                                            ModeIndex,
                                            &SizeOfInfo,
                                            &Info
                                            );
        if (!EFI_ERROR (Status)) {
          if ((Info->HorizontalResolution > HorizontalResolution) ||
              ((Info->HorizontalResolution == HorizontalResolution) && (Info->VerticalResolution > VerticalResolution)))
          {
            HorizontalResolution = Info->HorizontalResolution;
            VerticalResolution   = Info->VerticalResolution;
            ModeNumber           = ModeIndex;
          }

          FreePool (Info);
        }
      }

      if ((HorizontalResolution == 0x0) || (VerticalResolution == 0x0)) {
        Status = EFI_UNSUPPORTED;
        goto Error;
      }
    } else {
      //
      // Use user-defined resolution
      //
      Status = CheckModeSupported (
                 Private->GraphicsOutput,
                 HorizontalResolution,
                 VerticalResolution,
                 &ModeNumber
                 );
      if (EFI_ERROR (Status)) {
        //
        // if not supporting current mode, try 800x600 which is required by UEFI/EFI spec
        //
        HorizontalResolution = 800;
        VerticalResolution   = 600;
        Status               = CheckModeSupported (
                                 Private->GraphicsOutput,
                                 HorizontalResolution,
                                 VerticalResolution,
                                 &ModeNumber
                                 );
        Mode = Private->GraphicsOutput->Mode;
        if (EFI_ERROR (Status) && (Mode->MaxMode != 0)) {
          //
          // If set default mode failed or device doesn't support default mode, then get the current mode information
          //
          HorizontalResolution = Mode->Info->HorizontalResolution;
          VerticalResolution   = Mode->Info->VerticalResolution;
          ModeNumber           = Mode->Mode;
        }
      }
    }

    if (EFI_ERROR (Status) || (ModeNumber != Private->GraphicsOutput->Mode->Mode)) {
      //
      // Current graphics mode is not set or is not set to the mode which we have found,
      // set the new graphic mode.
      //
      Status = Private->GraphicsOutput->SetMode (Private->GraphicsOutput, ModeNumber);
      if (EFI_ERROR (Status)) {
        //
        // The mode set operation failed
        //
        goto Error;
      }
    }
  }

  DEBUG ((DEBUG_INFO, "GraphicsConsole video resolution %d x %d\n", HorizontalResolution, VerticalResolution));

  //
  // Initialize the mode which GraphicsConsole supports.
  //
  Status = InitializeGraphicsConsoleTextMode (
             HorizontalResolution,
             VerticalResolution,
             ModeNumber,
             &MaxMode,
             &Private->ModeData
             );

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Update the maximum number of modes
  //
  Private->SimpleTextOutputMode.MaxMode = (INT32)MaxMode;

  //
  // Initialize the Mode of graphics console devices
  //
  PreferMode    = -1;
  DefaultColumn = PcdGet32 (PcdConOutColumn);
  DefaultRow    = PcdGet32 (PcdConOutRow);
  Column        = 0;
  Row           = 0;
  for (Index = 0; Index < (INT32)MaxMode; Index++) {
    if ((DefaultColumn != 0) && (DefaultRow != 0)) {
      if ((Private->ModeData[Index].Columns == DefaultColumn) &&
          (Private->ModeData[Index].Rows == DefaultRow))
      {
        PreferMode = Index;
        break;
      }
    } else {
      if ((Private->ModeData[Index].Columns >= Column) &&
          (Private->ModeData[Index].Rows >= Row))
      {
        Column     = Private->ModeData[Index].Columns;
        Row        = Private->ModeData[Index].Rows;
        PreferMode = Index;
      }
    }
  }

  Private->SimpleTextOutput.Mode->Mode = (INT32)PreferMode;
  DEBUG ((DEBUG_INFO, "Graphics Console Started, Mode: %d\n", PreferMode));

  //
  // Install protocol interfaces for the Graphics Console device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  &Private->SimpleTextOutput,
                  NULL
                  );

Error:
  if (EFI_ERROR (Status)) {
    //
    // Close the GOP
    //
    if (Private->GraphicsOutput != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiGraphicsOutputProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    if (Private->LineBuffer != NULL) {
      FreePool (Private->LineBuffer);
    }

    if (Private->ModeData != NULL) {
      FreePool (Private->ModeData);
    }

    //
    // Free private data
    //
    FreePool (Private);
  }

  return Status;
}

/**
  Stop this driver on Controller by removing Simple Text Out protocol
  and closing the Graphics Output Protocol on Controller.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval EFI_NOT_STARTED   Simple Text Out protocol could not be found the
                            Controller.
  @retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOutput;
  GRAPHICS_CONSOLE_DEV             *Private;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&SimpleTextOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_STARTED;
  }

  Private = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (SimpleTextOutput);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  &Private->SimpleTextOutput
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Close the GOP Protocol
    //
    if (Private->GraphicsOutput != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiGraphicsOutputProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    if (Private->LineBuffer != NULL) {
      FreePool (Private->LineBuffer);
    }

    if (Private->ModeData != NULL) {
      FreePool (Private->ModeData);
    }

    //
    // Free our instance data
    //
    FreePool (Private);
  }

  return Status;
}

/**
  Check if the current specific mode supported the user defined resolution
  for the Graphics Console device based on Graphics Output Protocol.

  If yes, set the graphic device's current mode to this specific mode.

  @param  GraphicsOutput        Graphics Output Protocol instance pointer.
  @param  HorizontalResolution  User defined horizontal resolution
  @param  VerticalResolution    User defined vertical resolution.
  @param  CurrentModeNumber     Current specific mode to be check.

  @retval EFI_SUCCESS       The mode is supported.
  @retval EFI_UNSUPPORTED   The specific mode is out of range of graphics
                            device supported.
  @retval other             The specific mode does not support user defined
                            resolution or failed to set the current mode to the
                            specific mode on graphics device.

**/
EFI_STATUS
CheckModeSupported (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput,
  IN  UINT32                    HorizontalResolution,
  IN  UINT32                    VerticalResolution,
  OUT UINT32                    *CurrentModeNumber
  )
{
  UINT32                                ModeNumber;
  EFI_STATUS                            Status;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxMode;

  Status  = EFI_SUCCESS;
  MaxMode = GraphicsOutput->Mode->MaxMode;

  for (ModeNumber = 0; ModeNumber < MaxMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                               GraphicsOutput,
                               ModeNumber,
                               &SizeOfInfo,
                               &Info
                               );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == HorizontalResolution) &&
          (Info->VerticalResolution == VerticalResolution))
      {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == HorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == VerticalResolution))
        {
          //
          // If video device has been set to this mode, we do not need to SetMode again
          //
          FreePool (Info);
          break;
        } else {
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            FreePool (Info);
            break;
          }
        }
      }

      FreePool (Info);
    }
  }

  if (ModeNumber == GraphicsOutput->Mode->MaxMode) {
    Status = EFI_UNSUPPORTED;
  }

  *CurrentModeNumber = ModeNumber;
  return Status;
}

/**
  Locate HII Database protocol and HII Font protocol.

  @retval  EFI_SUCCESS     HII Database protocol and HII Font protocol
                           are located successfully.
  @return  other           Failed to locate HII Database protocol or
                           HII Font protocol.

**/
EFI_STATUS
EfiLocateHiiProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&mHiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **)&mHiiFont);
  return Status;
}

//
// Body of the STO functions
//

/**
  Reset the text output device hardware and optionally run diagnostics.

  Implements SIMPLE_TEXT_OUTPUT.Reset().
  If ExtendedVerification is TRUE, then perform dependent Graphics Console
  device reset, and set display mode to mode 0.
  If ExtendedVerification is FALSE, only set display mode to mode 0.

  @param  This                  Protocol instance pointer.
  @param  ExtendedVerification  Indicates that the driver may perform a more
                                exhaustive verification operation of the device
                                during reset.

  @retval EFI_SUCCESS          The text output device was reset.
  @retval EFI_DEVICE_ERROR     The text output device is not functioning correctly and
                               could not be reset.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS  Status;

  Status = This->SetMode (This, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = This->SetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK));
  return Status;
}

/**
  Write a Unicode string to the output device.

  Implements SIMPLE_TEXT_OUTPUT.OutputString().
  The Unicode string will be converted to Glyphs and will be
  sent to the Graphics Console.

  @param  This                    Protocol instance pointer.
  @param  WString                 The NULL-terminated Unicode string to be displayed
                                  on the output device(s). All output devices must
                                  also support the Unicode drawing defined in this file.

  @retval EFI_SUCCESS             The string was output to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  GRAPHICS_CONSOLE_DEV           *Private;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  INTN                           Mode;
  UINTN                          MaxColumn;
  UINTN                          MaxRow;
  UINTN                          Width;
  UINTN                          Height;
  UINTN                          Delta;
  EFI_STATUS                     Status;
  BOOLEAN                        Warning;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background;
  GRAPHICS_CONSOLE_MODE_DATA     *ModeData;
  UINTN                          DeltaX;
  UINTN                          DeltaY;
  UINTN                          GlyphWidth;
  UINTN                          GlyphHeight;
  UINTN                          Count;
  UINTN                          Index;
  INT32                          OriginAttribute;
  EFI_TPL                        OldTpl;

  if (This->Mode->Mode == -1) {
    //
    // If current mode is not valid, return error.
    //
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  //
  // Current mode
  //
  Mode           = This->Mode->Mode;
  Private        = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;

  ModeData    = &Private->ModeData[Mode];
  MaxColumn   = ModeData->Columns;
  MaxRow      = ModeData->Rows;
  DeltaX      = (UINTN)ModeData->DeltaX;
  DeltaY      = (UINTN)ModeData->DeltaY;
  GlyphWidth  = ModeData->GlyphWidth;
  GlyphHeight = ModeData->GlyphHeight;
  Width       = MaxColumn * GlyphWidth;
  Height      = (MaxRow - 1) * GlyphHeight;
  Delta       = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  //
  // The Attributes won't change when during the time OutputString is called
  //
  GetTextColors (This, &Foreground, &Background);

  FlushCursor (This);

  Warning = FALSE;

  //
  // Backup attribute
  //
  OriginAttribute = This->Mode->Attribute;

  while (*WString != L'\0') {
    if (*WString == CHAR_BACKSPACE) {
      //
      // If the cursor is at the left edge of the display, then move the cursor
      // one row up.
      //
      if ((This->Mode->CursorColumn == 0) && (This->Mode->CursorRow > 0)) {
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32)(MaxColumn - 1);
        This->OutputString (This, SpaceStr);
        FlushCursor (This);
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32)(MaxColumn - 1);
      } else if (This->Mode->CursorColumn > 0) {
        //
        // If the cursor is not at the left edge of the display, then move the cursor
        // left one column.
        //
        This->Mode->CursorColumn--;
        This->OutputString (This, SpaceStr);
        FlushCursor (This);
        This->Mode->CursorColumn--;
      }

      WString++;
    } else if (*WString == CHAR_LINEFEED) {
      //
      // If the cursor is at the bottom of the display, then scroll the display one
      // row, and do not update the cursor position. Otherwise, move the cursor
      // down one row.
      //
      if (This->Mode->CursorRow == (INT32)(MaxRow - 1)) {
        if (GraphicsOutput != NULL) {
          //
          // Scroll Screen Up One Row
          //
          GraphicsOutput->Blt (
                            GraphicsOutput,
                            NULL,
                            EfiBltVideoToVideo,
                            DeltaX,
                            DeltaY + GlyphHeight,
                            DeltaX,
                            DeltaY,
                            Width,
                            Height,
                            Delta
                            );

          //
          // Print Blank Line at last line
          //
          GraphicsOutput->Blt (
                            GraphicsOutput,
                            &Background,
                            EfiBltVideoFill,
                            0,
                            0,
                            DeltaX,
                            DeltaY + Height,
                            Width,
                            GlyphHeight,
                            Delta
                            );
        }
      } else {
        This->Mode->CursorRow++;
      }

      WString++;
    } else if (*WString == CHAR_CARRIAGE_RETURN) {
      //
      // Move the cursor to the beginning of the current row.
      //
      This->Mode->CursorColumn = 0;
      WString++;
    } else if (*WString == WIDE_CHAR) {
      This->Mode->Attribute |= EFI_WIDE_ATTRIBUTE;
      WString++;
    } else if (*WString == NARROW_CHAR) {
      This->Mode->Attribute &= (~(UINT32)EFI_WIDE_ATTRIBUTE);
      WString++;
    } else {
      //
      // Print the character at the current cursor position and move the cursor
      // right one column. If this moves the cursor past the right edge of the
      // display, then the line should wrap to the beginning of the next line. This
      // is equivalent to inserting a CR and an LF. Note that if the cursor is at the
      // bottom of the display, and the line wraps, then the display will be scrolled
      // one line.
      // If wide char is going to be displayed, need to display one character at a time
      // Or, need to know the display length of a certain string.
      //
      // Index is used to determine how many character width units (wide = 2, narrow = 1)
      // Count is used to determine how many characters are used regardless of their attributes
      //
      for (Count = 0, Index = 0; (This->Mode->CursorColumn + Index) < MaxColumn; Count++, Index++) {
        if ((WString[Count] == CHAR_NULL) ||
            (WString[Count] == CHAR_BACKSPACE) ||
            (WString[Count] == CHAR_LINEFEED) ||
            (WString[Count] == CHAR_CARRIAGE_RETURN) ||
            (WString[Count] == WIDE_CHAR) ||
            (WString[Count] == NARROW_CHAR))
        {
          break;
        }

        //
        // Is the wide attribute on?
        //
        if ((This->Mode->Attribute & EFI_WIDE_ATTRIBUTE) != 0) {
          //
          // If wide, add one more width unit than normal since we are going to increment at the end of the for loop
          //
          Index++;
          //
          // This is the end-case where if we are at column 79 and about to print a wide character
          // We should prevent this from happening because we will wrap inappropriately.  We should
          // not print this character until the next line.
          //
          if ((This->Mode->CursorColumn + Index + 1) > MaxColumn) {
            Index++;
            break;
          }
        }
      }

      Status = DrawUnicodeWeightAtCursorN (This, WString, Count, Index);
      if (EFI_ERROR (Status)) {
        Warning = TRUE;
      }

      //
      // At the end of line, output carriage return and line feed
      //
      WString                  += Count;
      This->Mode->CursorColumn += (INT32)Index;
      if (This->Mode->CursorColumn > (INT32)MaxColumn) {
        This->Mode->CursorColumn -= 2;
        This->OutputString (This, SpaceStr);
      }

      if (This->Mode->CursorColumn >= (INT32)MaxColumn) {
        FlushCursor (This);
        This->OutputString (This, mCrLfString);
        FlushCursor (This);
      }
    }
  }

  This->Mode->Attribute = OriginAttribute;

  FlushCursor (This);

  if (Warning) {
    Status = EFI_WARN_UNKNOWN_GLYPH;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Verifies that all characters in a Unicode string can be output to the
  target device.

  Implements SIMPLE_TEXT_OUTPUT.TestString().
  If one of the characters in the *Wstring is neither valid valid Unicode
  drawing characters, not ASCII code, then this function will return
  EFI_UNSUPPORTED

  @param  This    Protocol instance pointer.
  @param  WString The NULL-terminated Unicode string to be examined for the output
                  device(s).

  @retval EFI_SUCCESS      The device(s) are capable of rendering the output string.
  @retval EFI_UNSUPPORTED  Some of the characters in the Unicode string cannot be
                           rendered by one or more of the output devices mapped
                           by the EFI handle.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  EFI_STATUS  Status;
  UINT16      Count;

  EFI_IMAGE_OUTPUT  *Blt;

  Blt   = NULL;
  Count = 0;

  while (WString[Count] != 0) {
    //
    // In TerminalConOutOutputString(), WIDE_CHAR/NARROW_CHAR will be ignored.
    // So, WString contains WIDE_CHAR/NARROW_CHAR is also valid.
    //
    if ((WString[Count] == WIDE_CHAR) || (WString[Count] == NARROW_CHAR)) {
      continue;
    }

    Status = mHiiFont->GetGlyph (
                         mHiiFont,
                         WString[Count],
                         NULL,
                         &Blt,
                         NULL
                         );
    if (Blt != NULL) {
      FreePool (Blt);
      Blt = NULL;
    }

    Count++;

    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Returns information for an available text mode that the output device(s)
  supports

  Implements SIMPLE_TEXT_OUTPUT.QueryMode().
  It returnes information for an available text mode that the Graphics Console supports.
  In this driver,we only support text mode 80x25, which is defined as mode 0.

  @param  This                  Protocol instance pointer.
  @param  ModeNumber            The mode number to return information on.
  @param  Columns               The returned columns of the requested mode.
  @param  Rows                  The returned rows of the requested mode.

  @retval EFI_SUCCESS           The requested mode information is returned.
  @retval EFI_UNSUPPORTED       The mode number is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  GRAPHICS_CONSOLE_DEV  *Private;
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;

  if (ModeNumber >= (UINTN)This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  Status = EFI_SUCCESS;

  Private = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);

  *Columns = Private->ModeData[ModeNumber].Columns;
  *Rows    = Private->ModeData[ModeNumber].Rows;

  if ((*Columns <= 0) || (*Rows <= 0)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Sets the output device(s) to a specified mode.

  Implements SIMPLE_TEXT_OUTPUT.SetMode().
  Set the Graphics Console to a specified mode. In this driver, we only support mode 0.

  @param  This                  Protocol instance pointer.
  @param  ModeNumber            The text mode to set.

  @retval EFI_SUCCESS           The requested text mode is set.
  @retval EFI_DEVICE_ERROR      The requested text mode cannot be set because of
                                Graphics Console device error.
  @retval EFI_UNSUPPORTED       The text mode number is not valid.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  EFI_STATUS                     Status;
  GRAPHICS_CONSOLE_DEV           *Private;
  GRAPHICS_CONSOLE_MODE_DATA     *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *NewLineBuffer;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_TPL                        OldTpl;
  UINTN                          LineBufferSize;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private        = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;

  //
  // Make sure the requested mode number is supported
  //
  if (ModeNumber >= (UINTN)This->Mode->MaxMode) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  ModeData = &(Private->ModeData[ModeNumber]);

  if ((ModeData->Columns <= 0) && (ModeData->Rows <= 0)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  //
  // If the mode has been set at least one other time, then LineBuffer will not be NULL
  //
  if (Private->LineBuffer != NULL) {
    //
    // If the new mode is the same as the old mode, then just return EFI_SUCCESS
    //
    if ((INT32)ModeNumber == This->Mode->Mode) {
      //
      // Clear the current text window on the current graphics console
      //
      This->ClearScreen (This);
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // Otherwise, the size of the text console and/or the GOP mode will be changed,
    // so erase the cursor, and free the LineBuffer for the current mode
    //
    FlushCursor (This);

    FreePool (Private->LineBuffer);
  }

  //
  // Attempt to allocate a line buffer for the requested mode number
  //
  if ((ModeData->GlyphWidth == 0) ||
      (ModeData->GlyphHeight == 0) ||
      (ModeData->Columns >
       (MAX_UINTN / ModeData->GlyphWidth / ModeData->GlyphHeight / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))))
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  LineBufferSize = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) *
                   ModeData->Columns *
                   ModeData->GlyphWidth *
                   ModeData->GlyphHeight;
  NewLineBuffer = AllocatePool (LineBufferSize);

  if (NewLineBuffer == NULL) {
    //
    // The new line buffer could not be allocated, so return an error.
    // No changes to the state of the current console have been made, so the current console is still valid
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Assign the current line buffer to the newly allocated line buffer
  //
  Private->LineBuffer = NewLineBuffer;

  if (GraphicsOutput != NULL) {
    if (ModeData->GopModeNumber != GraphicsOutput->Mode->Mode) {
      //
      // Either no graphics mode is currently set, or it is set to the wrong resolution, so set the new graphics mode
      //
      Status = GraphicsOutput->SetMode (GraphicsOutput, ModeData->GopModeNumber);
      if (EFI_ERROR (Status)) {
        //
        // The mode set operation failed
        //
        goto Done;
      }
    } else {
      //
      // The current graphics mode is correct, so simply clear the entire display
      //
      Status = GraphicsOutput->Blt (
                                 GraphicsOutput,
                                 &mGraphicsEfiColors[0],
                                 EfiBltVideoFill,
                                 0,
                                 0,
                                 0,
                                 0,
                                 ModeData->GopWidth,
                                 ModeData->GopHeight,
                                 0
                                 );
    }
  }

  //
  // The new mode is valid, so commit the mode change
  //
  This->Mode->Mode = (INT32)ModeNumber;

  //
  // Move the text cursor to the upper left hand corner of the display and flush it
  //
  This->Mode->CursorColumn = 0;
  This->Mode->CursorRow    = 0;

  FlushCursor (This);

  Status = EFI_SUCCESS;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  Implements SIMPLE_TEXT_OUTPUT.SetAttribute().

  @param  This                  Protocol instance pointer.
  @param  Attribute             The attribute to set. Bits 0..3 are the foreground
                                color, and bits 4..6 are the background color.
                                All other bits are undefined and must be zero.

  @retval EFI_SUCCESS           The requested attribute is set.
  @retval EFI_DEVICE_ERROR      The requested attribute cannot be set due to Graphics Console port error.
  @retval EFI_UNSUPPORTED       The attribute requested is not defined.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  EFI_TPL  OldTpl;

  if ((Attribute | 0x7F) != 0x7F) {
    return EFI_UNSUPPORTED;
  }

  if ((INT32)Attribute == This->Mode->Attribute) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  FlushCursor (This);

  This->Mode->Attribute = (INT32)Attribute;

  FlushCursor (This);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

/**
  Clears the output device(s) display to the currently selected background
  color.

  Implements SIMPLE_TEXT_OUTPUT.ClearScreen().

  @param  This                  Protocol instance pointer.

  @retval  EFI_SUCCESS      The operation completed successfully.
  @retval  EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval  EFI_UNSUPPORTED  The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS                     Status;
  GRAPHICS_CONSOLE_DEV           *Private;
  GRAPHICS_CONSOLE_MODE_DATA     *ModeData;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background;
  EFI_TPL                        OldTpl;

  if (This->Mode->Mode == -1) {
    //
    // If current mode is not valid, return error.
    //
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private        = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  ModeData       = &(Private->ModeData[This->Mode->Mode]);

  GetTextColors (This, &Foreground, &Background);
  if (GraphicsOutput != NULL) {
    Status = GraphicsOutput->Blt (
                               GraphicsOutput,
                               &Background,
                               EfiBltVideoFill,
                               0,
                               0,
                               0,
                               0,
                               ModeData->GopWidth,
                               ModeData->GopHeight,
                               0
                               );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  This->Mode->CursorColumn = 0;
  This->Mode->CursorRow    = 0;

  FlushCursor (This);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Sets the current coordinates of the cursor position.

  Implements SIMPLE_TEXT_OUTPUT.SetCursorPosition().

  @param  This        Protocol instance pointer.
  @param  Column      The position to set the cursor to. Must be greater than or
                      equal to zero and less than the number of columns and rows
                      by QueryMode ().
  @param  Row         The position to set the cursor to. Must be greater than or
                      equal to zero and less than the number of columns and rows
                      by QueryMode ().

  @retval EFI_SUCCESS      The operation completed successfully.
  @retval EFI_DEVICE_ERROR The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED  The output device is not in a valid text mode, or the
                           cursor position is invalid for the current mode.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  GRAPHICS_CONSOLE_DEV        *Private;
  GRAPHICS_CONSOLE_MODE_DATA  *ModeData;
  EFI_STATUS                  Status;
  EFI_TPL                     OldTpl;

  if (This->Mode->Mode == -1) {
    //
    // If current mode is not valid, return error.
    //
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private  = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  ModeData = &(Private->ModeData[This->Mode->Mode]);

  if ((Column >= ModeData->Columns) || (Row >= ModeData->Rows)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if ((This->Mode->CursorColumn == (INT32)Column) && (This->Mode->CursorRow == (INT32)Row)) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  FlushCursor (This);

  This->Mode->CursorColumn = (INT32)Column;
  This->Mode->CursorRow    = (INT32)Row;

  FlushCursor (This);

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Makes the cursor visible or invisible.

  Implements SIMPLE_TEXT_OUTPUT.EnableCursor().

  @param  This                  Protocol instance pointer.
  @param  Visible               If TRUE, the cursor is set to be visible, If FALSE,
                                the cursor is set to be invisible.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_UNSUPPORTED       The output device's mode is not currently in a
                                defined text mode.

**/
EFI_STATUS
EFIAPI
GraphicsConsoleConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  EFI_TPL  OldTpl;

  if (This->Mode->Mode == -1) {
    //
    // If current mode is not valid, return error.
    //
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  FlushCursor (This);

  This->Mode->CursorVisible = Visible;

  FlushCursor (This);

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

/**
  Gets Graphics Console device's foreground color and background color.

  @param  This                  Protocol instance pointer.
  @param  Foreground            Returned text foreground color.
  @param  Background            Returned text background color.

  @retval EFI_SUCCESS           It returned always.

**/
EFI_STATUS
GetTextColors (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background
  )
{
  INTN  Attribute;

  Attribute = This->Mode->Attribute & 0x7F;

  *Foreground = mGraphicsEfiColors[Attribute & 0x0f];
  *Background = mGraphicsEfiColors[Attribute >> 4];

  return EFI_SUCCESS;
}

/**
  Draw Unicode string on the Graphics Console device's screen.

  @param  This                  Protocol instance pointer.
  @param  UnicodeWeight         One Unicode string to be displayed.
  @param  Count                 The count of Unicode string.
  @param  CellCount             The number of text cells to clear and display.

  @retval EFI_OUT_OF_RESOURCES  If no memory resource to use.
  @retval EFI_UNSUPPORTED       If no Graphics Output protocol
                                protocol exist.
  @retval EFI_DEVICE_ERROR      If the current text scale is invalid.
  @retval EFI_SUCCESS           Drawing Unicode string implemented successfully.

**/
EFI_STATUS
DrawUnicodeWeightAtCursorN (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *UnicodeWeight,
  IN  UINTN                            Count,
  IN  UINTN                            CellCount
  )
{
  EFI_STATUS                     Status;
  GRAPHICS_CONSOLE_DEV           *Private;
  GRAPHICS_CONSOLE_MODE_DATA     *ModeData;
  EFI_IMAGE_OUTPUT               *Blt;
  EFI_STRING                     String;
  EFI_FONT_DISPLAY_INFO          *FontInfo;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *ScaledBitmap;
  UINTN                          TextScale;
  UINTN                          SourceWidth;
  UINTN                          SourceHeight;
  UINTN                          PointX;
  UINTN                          PointY;
  UINTN                          BltWidth;
  UINTN                          BltHeight;
  UINTN                          BltDelta;

  if (Count == 0) {
    return EFI_SUCCESS;
  }

  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  ModeData  = &Private->ModeData[This->Mode->Mode];
  TextScale = ModeData->TextScale;
  if ((TextScale < GRAPHICS_CONSOLE_MIN_TEXT_SCALE) || (TextScale > GRAPHICS_CONSOLE_MAX_TEXT_SCALE)) {
    return EFI_DEVICE_ERROR;
  }

  ScaledBitmap = NULL;
  Blt          = (EFI_IMAGE_OUTPUT *)AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Blt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  String = AllocateCopyPool ((Count + 1) * sizeof (CHAR16), UnicodeWeight);
  if (String == NULL) {
    FreePool (Blt);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the end character
  //
  *(String + Count) = L'\0';

  FontInfo = (EFI_FONT_DISPLAY_INFO *)AllocateZeroPool (sizeof (EFI_FONT_DISPLAY_INFO));
  if (FontInfo == NULL) {
    FreePool (Blt);
    FreePool (String);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get current foreground and background colors.
  //
  GetTextColors (This, &FontInfo->ForegroundColor, &FontInfo->BackgroundColor);

  if (Private->GraphicsOutput != NULL) {
    PointX = (This->Mode->CursorColumn * ModeData->GlyphWidth) + ModeData->DeltaX;
    PointY = (This->Mode->CursorRow * ModeData->GlyphHeight) + ModeData->DeltaY;

    if (TextScale == 1) {
      //
      // If Graphics Output protocol exists, using HII Font protocol to draw.
      //
      Blt->Width        = (UINT16)(ModeData->GopWidth);
      Blt->Height       = (UINT16)(ModeData->GopHeight);
      Blt->Image.Screen = Private->GraphicsOutput;

      Status = mHiiFont->StringToImage (
                           mHiiFont,
                           EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_DIRECT_TO_SCREEN | EFI_HII_IGNORE_LINE_BREAK,
                           String,
                           FontInfo,
                           &Blt,
                           PointX,
                           PointY,
                           NULL,
                           NULL,
                           NULL
                           );
    } else {
      SourceHeight = EFI_GLYPH_HEIGHT;
      if ((CellCount > (MAX_UINTN / EFI_GLYPH_WIDTH)) ||
          (SourceHeight > MAX_UINT16))
      {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      SourceWidth = CellCount * EFI_GLYPH_WIDTH;
      if ((SourceWidth > MAX_UINT16) ||
          (SourceWidth > (MAX_UINTN / SourceHeight / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))))
      {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      Blt->Width        = (UINT16)SourceWidth;
      Blt->Height       = (UINT16)SourceHeight;
      Blt->Image.Bitmap = AllocateZeroPool (SourceWidth * SourceHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      if (Blt->Image.Bitmap == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      FillBltBuffer (Blt->Image.Bitmap, SourceWidth, SourceHeight, &FontInfo->BackgroundColor);

      Status = mHiiFont->StringToImage (
                           mHiiFont,
                           EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLIP_CLEAN_X |
                           EFI_HII_OUT_FLAG_CLIP_CLEAN_Y | EFI_HII_IGNORE_LINE_BREAK,
                           String,
                           FontInfo,
                           &Blt,
                           0,
                           0,
                           NULL,
                           NULL,
                           NULL
                           );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      Status = ScaleBltBuffer2x (
                 Blt->Image.Bitmap,
                 SourceWidth,
                 SourceHeight,
                 SourceWidth,
                 &ScaledBitmap
                 );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      BltWidth  = SourceWidth * 2;
      BltHeight = SourceHeight * 2;
      BltDelta  = BltWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
      Status    = Private->GraphicsOutput->Blt (
                                             Private->GraphicsOutput,
                                             ScaledBitmap,
                                             EfiBltBufferToVideo,
                                             0,
                                             0,
                                             PointX,
                                             PointY,
                                             BltWidth,
                                             BltHeight,
                                             BltDelta
                                             );
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

Done:
  if ((TextScale != 1) && (Blt != NULL) && (Blt->Image.Bitmap != NULL)) {
    FreePool (Blt->Image.Bitmap);
  }

  if (ScaledBitmap != NULL) {
    FreePool (ScaledBitmap);
  }

  if (Blt != NULL) {
    FreePool (Blt);
  }

  if (String != NULL) {
    FreePool (String);
  }

  if (FontInfo != NULL) {
    FreePool (FontInfo);
  }

  return Status;
}

/**
  Flush the cursor on the screen.

  If CursorVisible is FALSE, nothing to do and return directly.
  If CursorVisible is TRUE,
     i) If the cursor shows on screen, it will be erased.
    ii) If the cursor does not show on screen, it will be shown.

  @param  This                  Protocol instance pointer.

  @retval EFI_SUCCESS           The cursor is erased successfully.

**/
EFI_STATUS
FlushCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  GRAPHICS_CONSOLE_DEV                 *Private;
  EFI_SIMPLE_TEXT_OUTPUT_MODE          *CurrentMode;
  GRAPHICS_CONSOLE_MODE_DATA           *ModeData;
  INTN                                 GlyphX;
  INTN                                 GlyphY;
  EFI_GRAPHICS_OUTPUT_PROTOCOL         *GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  BltChar[GRAPHICS_CONSOLE_MAX_GLYPH_HEIGHT][GRAPHICS_CONSOLE_MAX_GLYPH_WIDTH];
  UINTN                                PosX;
  UINTN                                PosY;
  UINTN                                ScaleX;
  UINTN                                ScaleY;
  UINTN                                CursorColumn;
  UINTN                                CursorRow;
  UINTN                                GlyphWidth;
  UINTN                                GlyphHeight;
  UINTN                                TextScale;

  CurrentMode = This->Mode;

  if (!CurrentMode->CursorVisible) {
    return EFI_SUCCESS;
  }

  Private        = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  ModeData       = &Private->ModeData[CurrentMode->Mode];
  GlyphWidth     = ModeData->GlyphWidth;
  GlyphHeight    = ModeData->GlyphHeight;
  TextScale      = ModeData->TextScale;

  if ((TextScale < GRAPHICS_CONSOLE_MIN_TEXT_SCALE) || (TextScale > GRAPHICS_CONSOLE_MAX_TEXT_SCALE)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // In this driver, only narrow character was supported.
  //
  //
  // Blt a character to the screen
  //
  GlyphX = (CurrentMode->CursorColumn * GlyphWidth) + ModeData->DeltaX;
  GlyphY = (CurrentMode->CursorRow * GlyphHeight) + ModeData->DeltaY;
  if (GraphicsOutput != NULL) {
    GraphicsOutput->Blt (
                      GraphicsOutput,
                      (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BltChar,
                      EfiBltVideoToBltBuffer,
                      GlyphX,
                      GlyphY,
                      0,
                      0,
                      GlyphWidth,
                      GlyphHeight,
                      GlyphWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                      );
  }

  GetTextColors (This, &Foreground.Pixel, &Background.Pixel);

  //
  // Convert Monochrome bitmap of the Glyph to BltBuffer structure
  //
  for (PosY = 0; PosY < EFI_GLYPH_HEIGHT; PosY++) {
    for (PosX = 0; PosX < EFI_GLYPH_WIDTH; PosX++) {
      if ((mCursorGlyph.GlyphCol1[PosY] & (BIT0 << PosX)) != 0) {
        for (ScaleY = 0; ScaleY < TextScale; ScaleY++) {
          for (ScaleX = 0; ScaleX < TextScale; ScaleX++) {
            CursorRow    = (PosY * TextScale) + ScaleY;
            CursorColumn = ((EFI_GLYPH_WIDTH - PosX - 1) * TextScale) + ScaleX;
            BltChar[CursorRow][CursorColumn].Raw ^= Foreground.Raw;
          }
        }
      }
    }
  }

  if (GraphicsOutput != NULL) {
    GraphicsOutput->Blt (
                      GraphicsOutput,
                      (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BltChar,
                      EfiBltBufferToVideo,
                      0,
                      0,
                      GlyphX,
                      GlyphY,
                      GlyphWidth,
                      GlyphHeight,
                      GlyphWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                      );
  }

  return EFI_SUCCESS;
}

/**
  HII Database Protocol notification event handler.

  Register font package when HII Database Protocol has been installed.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
RegisterFontPackage (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR  *SimplifiedFont;
  UINT32                           PackageLength;
  UINT8                            *Package;
  UINT8                            *Location;
  EFI_HII_DATABASE_PROTOCOL        *HiiDatabase;

  //
  // Locate HII Database Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Add 4 bytes to the header for entire length for HiiAddPackages use only.
  //
  //    +--------------------------------+ <-- Package
  //    |                                |
  //    |    PackageLength(4 bytes)      |
  //    |                                |
  //    |--------------------------------| <-- SimplifiedFont
  //    |                                |
  //    |EFI_HII_SIMPLE_FONT_PACKAGE_HDR |
  //    |                                |
  //    |--------------------------------| <-- Location
  //    |                                |
  //    |     gUsStdNarrowGlyphData      |
  //    |                                |
  //    +--------------------------------+

  PackageLength = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + mNarrowFontSize + 4;
  Package       = AllocateZeroPool (PackageLength);
  if (Package == NULL) {
    ASSERT (Package != NULL);
    return;
  }

  WriteUnaligned32 ((UINT32 *)Package, PackageLength);
  SimplifiedFont                       = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *)(Package + 4);
  SimplifiedFont->Header.Length        = (UINT32)(PackageLength - 4);
  SimplifiedFont->Header.Type          = EFI_HII_PACKAGE_SIMPLE_FONTS;
  SimplifiedFont->NumberOfNarrowGlyphs = (UINT16)(mNarrowFontSize / sizeof (EFI_NARROW_GLYPH));

  Location = (UINT8 *)(&SimplifiedFont->NumberOfWideGlyphs + 1);
  CopyMem (Location, gUsStdNarrowGlyphData, mNarrowFontSize);

  //
  // Add this simplified font package to a package list then install it.
  //
  mHiiHandle = HiiAddPackages (
                 &mFontPackageListGuid,
                 NULL,
                 Package,
                 NULL
                 );
  ASSERT (mHiiHandle != NULL);
  FreePool (Package);
}

/**
  The user Entry Point for module GraphicsConsole. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval  EFI_SUCCESS       The entry point is executed successfully.
  @return  other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeGraphicsConsole (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Register notify function on HII Database Protocol to add font package.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiHiiDatabaseProtocolGuid,
    TPL_CALLBACK,
    RegisterFontPackage,
    NULL,
    &mHiiRegistration
    );

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gGraphicsConsoleDriverBinding,
             ImageHandle,
             &gGraphicsConsoleComponentName,
             &gGraphicsConsoleComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
