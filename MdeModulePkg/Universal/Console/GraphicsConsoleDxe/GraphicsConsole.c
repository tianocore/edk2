/**@file
  This is the main routine for initializing the Graphics Console support routines.
Remaining Tasks
  Add all standard Glyphs from UEFI 2.0 Specification
  Implement optimal automatic Mode creation algorithm
  Solve palette issues for mixed graphics and text
  When does this protocol reset the palette?

Copyright (c) 2006 - 2007 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "GraphicsConsole.h"

STATIC
EFI_STATUS
GetTextColors (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background
  );

STATIC
EFI_STATUS
DrawUnicodeWeightAtCursorN (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *UnicodeWeight,
  IN  UINTN                            Count
  );

STATIC
EFI_STATUS
EraseCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  );

EFI_STATUS
CheckModeSupported (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput,
  IN  UINT32  HorizontalResolution,
  IN  UINT32  VerticalResolution,
  OUT UINT32  *CurrentModeNumber
  );

//
// Globals
//
GRAPHICS_CONSOLE_DEV        mGraphicsConsoleDevTemplate = {
  GRAPHICS_CONSOLE_DEV_SIGNATURE,
  (EFI_GRAPHICS_OUTPUT_PROTOCOL *) NULL,
  (EFI_UGA_DRAW_PROTOCOL *) NULL,
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
    (EFI_SIMPLE_TEXT_OUTPUT_MODE *) NULL
  },
  {
    0,
    0,
    EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK),
    0,
    0,
    TRUE
  },
  {
    { 80, 25, 0, 0, 0, 0 },  // Mode 0
    { 80, 50, 0, 0, 0, 0 },  // Mode 1
    { 100,31, 0, 0, 0, 0 },  // Mode 2
    {  0,  0, 0, 0, 0, 0 }   // Mode 3
  },
  (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) NULL,
  (EFI_HII_HANDLE ) 0
};

EFI_HII_DATABASE_PROTOCOL   *mHiiDatabase;
EFI_HII_FONT_PROTOCOL       *mHiiFont;
BOOLEAN                     mFirstAccessFlag = TRUE;

STATIC EFI_GUID             mFontPackageListGuid = {0xf5f219d3, 0x7006, 0x4648, 0xac, 0x8d, 0xd6, 0x1d, 0xfb, 0x7b, 0xc6, 0xad};

static CHAR16               mCrLfString[3] = { CHAR_CARRIAGE_RETURN, CHAR_LINEFEED, CHAR_NULL };

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL        mEfiColors[16] = {
  //
  // B     G     R
  //
  {0x00, 0x00, 0x00, 0x00},  // BLACK
  {0x98, 0x00, 0x00, 0x00},  // BLUE
  {0x00, 0x98, 0x00, 0x00},  // GREEN
  {0x98, 0x98, 0x00, 0x00},  // CYAN
  {0x00, 0x00, 0x98, 0x00},  // RED
  {0x98, 0x00, 0x98, 0x00},  // MAGENTA
  {0x00, 0x98, 0x98, 0x00},  // BROWN
  {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
  {0x30, 0x30, 0x30, 0x00},  // DARKGRAY - BRIGHT BLACK
  {0xff, 0x00, 0x00, 0x00},  // LIGHTBLUE - ?
  {0x00, 0xff, 0x00, 0x00},  // LIGHTGREEN - ?
  {0xff, 0xff, 0x00, 0x00},  // LIGHTCYAN
  {0x00, 0x00, 0xff, 0x00},  // LIGHTRED
  {0xff, 0x00, 0xff, 0x00},  // LIGHTMAGENTA
  {0x00, 0xff, 0xff, 0x00},  // LIGHTBROWN
  {0xff, 0xff, 0xff, 0x00}  // WHITE
};

static EFI_NARROW_GLYPH     mCursorGlyph = {
  0x0000,
  0x00,
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF }
};

EFI_DRIVER_BINDING_PROTOCOL gGraphicsConsoleDriverBinding = {
  GraphicsConsoleControllerDriverSupported,
  GraphicsConsoleControllerDriverStart,
  GraphicsConsoleControllerDriverStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL        *UgaDraw;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;

  GraphicsOutput = NULL;
  UgaDraw        = NULL;
  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // Open Graphics Output Protocol failed, try to open UGA Draw Protocol
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
  }
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
                  (VOID **) &DevicePath,
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
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUgaDrawProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );
  }
  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
/*++

  Routine Description:

    Start the controller.

  Arguments:

    This                - A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
    Controller          - The handle of the controller to start.
    RemainingDevicePath - A pointer to the remaining portion of a devcie path.

  Returns:

    EFI_SUCCESS          - Return successfully.
    EFI_OUT_OF_RESOURCES - Out of resources.

--*/
{
  EFI_STATUS                           Status;
  GRAPHICS_CONSOLE_DEV                 *Private;
  UINTN                                NarrowFontSize;
  UINT32                               HorizontalResolution;
  UINT32                               VerticalResolution;
  UINT32                               ColorDepth;
  UINT32                               RefreshRate;
  UINTN                                MaxMode;
  UINTN                                Columns;
  UINTN                                Rows;
  UINT32                               ModeNumber;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR      *SimplifiedFont;
  UINTN                                PackageLength;
  EFI_HII_PACKAGE_LIST_HEADER          *PackageList;
  UINT8                                *Package;
  UINT8                                *Location;

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
                  (VOID **) &Private->GraphicsOutput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR(Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &Private->UgaDraw,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
  }

  if (EFI_ERROR (Status)) {
    goto Error;
  }

  NarrowFontSize  = ReturnNarrowFontSize ();

  if (mFirstAccessFlag) {
    //
    // Add 4 bytes to the header for entire length for HiiLibPreparePackageList use only.
    // Looks ugly. Might be updated when font tool is ready.
    //
    PackageLength   = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + NarrowFontSize + 4;
    Package = AllocateZeroPool (PackageLength);
    if (Package == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Package, &PackageLength, 4);
    SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR*) (Package + 4);
    SimplifiedFont->Header.Length        = (UINT32) (PackageLength - 4);
    SimplifiedFont->Header.Type          = EFI_HII_PACKAGE_SIMPLE_FONTS;
    SimplifiedFont->NumberOfNarrowGlyphs = (UINT16) (NarrowFontSize / sizeof (EFI_NARROW_GLYPH));

    Location = (UINT8 *) (&SimplifiedFont->NumberOfWideGlyphs + 1);
    CopyMem (Location, UsStdNarrowGlyphData, NarrowFontSize);

    //
    // Add this simplified font package to a package list then install it.
    //
    PackageList = HiiLibPreparePackageList (1, &mFontPackageListGuid, Package);
    Status = mHiiDatabase->NewPackageList (mHiiDatabase, PackageList, NULL, &(Private->HiiHandle));
    ASSERT_EFI_ERROR (Status);
    SafeFreePool (PackageList);
    SafeFreePool (Package);

    mFirstAccessFlag = FALSE;
  }
  //
  // If the current mode information can not be retrieved, then attemp to set the default mode
  // of 800x600, 32 bit colot, 60 Hz refresh.
  //
  HorizontalResolution  = 800;
  VerticalResolution    = 600;

  if (Private->GraphicsOutput != NULL) {
    //
    // The console is build on top of Graphics Output Protocol, find the mode number
    // for the user-defined mode; if there are multiple video devices,
    // graphic console driver will set all the video devices to the same mode.
    //
    Status = CheckModeSupported (
                 Private->GraphicsOutput,
                 CURRENT_HORIZONTAL_RESOLUTION,
                 CURRENT_VERTICAL_RESOLUTION,
                 &ModeNumber
                 );
    if (!EFI_ERROR(Status)) {
      //
      // Update default mode to current mode
      //
      HorizontalResolution = CURRENT_HORIZONTAL_RESOLUTION;
      VerticalResolution   = CURRENT_VERTICAL_RESOLUTION;
    } else {
      //
      // if not supporting current mode, try 800x600 which is required by UEFI/EFI spec
      //
      Status = CheckModeSupported (
                   Private->GraphicsOutput,
                   800,
                   600,
                   &ModeNumber
                   );
    }

    if (EFI_ERROR (Status) || (ModeNumber == Private->GraphicsOutput->Mode->MaxMode)) {
      //
      // Set default mode failed or device don't support default mode, then get the current mode information
      //
      HorizontalResolution = Private->GraphicsOutput->Mode->Info->HorizontalResolution;
      VerticalResolution = Private->GraphicsOutput->Mode->Info->VerticalResolution;
      ModeNumber = Private->GraphicsOutput->Mode->Mode;
    }
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // At first try to set user-defined resolution
    //
    ColorDepth            = 32;
    RefreshRate           = 60;
    Status = Private->UgaDraw->SetMode (
                                Private->UgaDraw,
                                CURRENT_HORIZONTAL_RESOLUTION,
                                CURRENT_VERTICAL_RESOLUTION,
                                ColorDepth,
                                RefreshRate
                                );
    if (!EFI_ERROR (Status)) {
      HorizontalResolution = CURRENT_HORIZONTAL_RESOLUTION;
      VerticalResolution   = CURRENT_VERTICAL_RESOLUTION;
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      //
      // Try to set 800*600 which is required by UEFI/EFI spec
      //
      Status = Private->UgaDraw->SetMode (
                                  Private->UgaDraw,
                                  HorizontalResolution,
                                  VerticalResolution,
                                  ColorDepth,
                                  RefreshRate
                                  );
      if (EFI_ERROR (Status)) {
        Status = Private->UgaDraw->GetMode (
                                    Private->UgaDraw,
                                    &HorizontalResolution,
                                    &VerticalResolution,
                                    &ColorDepth,
                                    &RefreshRate
                                    );
        if (EFI_ERROR (Status)) {
          goto Error;
        }
      }
    } else {
      Status = EFI_UNSUPPORTED;
      goto Error;
    }
  }

  //
  // Compute the maximum number of text Rows and Columns that this current graphics mode can support
  //
  Columns = HorizontalResolution / EFI_GLYPH_WIDTH;
  Rows    = VerticalResolution / EFI_GLYPH_HEIGHT;

  //
  // See if the mode is too small to support the required 80x25 text mode
  //
  if (Columns < 80 || Rows < 25) {
    goto Error;
  }
  //
  // Add Mode #0 that must be 80x25
  //
  MaxMode = 0;
  Private->ModeData[MaxMode].GopWidth   = HorizontalResolution;
  Private->ModeData[MaxMode].GopHeight  = VerticalResolution;
  Private->ModeData[MaxMode].GopModeNumber = ModeNumber;
  Private->ModeData[MaxMode].DeltaX     = (HorizontalResolution - (80 * EFI_GLYPH_WIDTH)) >> 1;
  Private->ModeData[MaxMode].DeltaY     = (VerticalResolution - (25 * EFI_GLYPH_HEIGHT)) >> 1;
  MaxMode++;

  //
  // If it is possible to support Mode #1 - 80x50, than add it as an active mode
  //
  if (Rows >= 50) {
    Private->ModeData[MaxMode].GopWidth   = HorizontalResolution;
    Private->ModeData[MaxMode].GopHeight  = VerticalResolution;
    Private->ModeData[MaxMode].GopModeNumber = ModeNumber;
    Private->ModeData[MaxMode].DeltaX     = (HorizontalResolution - (80 * EFI_GLYPH_WIDTH)) >> 1;
    Private->ModeData[MaxMode].DeltaY     = (VerticalResolution - (50 * EFI_GLYPH_HEIGHT)) >> 1;
    MaxMode++;
  }

  //
  // If it is not to support Mode #1 - 80x50, then skip it
  //
  if (MaxMode < 2) {
    Private->ModeData[MaxMode].Columns    = 0;
    Private->ModeData[MaxMode].Rows       = 0;
    Private->ModeData[MaxMode].GopWidth   = HorizontalResolution;
    Private->ModeData[MaxMode].GopHeight  = VerticalResolution;
    Private->ModeData[MaxMode].GopModeNumber = ModeNumber;
    Private->ModeData[MaxMode].DeltaX     = 0;
    Private->ModeData[MaxMode].DeltaY     = 0;
    MaxMode++;
  }

  //
  // Add Mode #2 that must be 100x31 (graphic mode >= 800x600)
  //
  if (Columns >= 100 && Rows >= 31) {
    Private->ModeData[MaxMode].GopWidth   = HorizontalResolution;
    Private->ModeData[MaxMode].GopHeight  = VerticalResolution;
    Private->ModeData[MaxMode].GopModeNumber = ModeNumber;
    Private->ModeData[MaxMode].DeltaX     = (HorizontalResolution - (100 * EFI_GLYPH_WIDTH)) >> 1;
    Private->ModeData[MaxMode].DeltaY     = (VerticalResolution - (31 * EFI_GLYPH_HEIGHT)) >> 1;
    MaxMode++;
  }

  //
  // Add Mode #3 that uses the entire display for user-defined mode
  //
  if (HorizontalResolution > 800 && VerticalResolution > 600) {
    Private->ModeData[MaxMode].Columns    = HorizontalResolution/EFI_GLYPH_WIDTH;
    Private->ModeData[MaxMode].Rows       = VerticalResolution/EFI_GLYPH_HEIGHT;
    Private->ModeData[MaxMode].GopWidth   = HorizontalResolution;
    Private->ModeData[MaxMode].GopHeight  = VerticalResolution;
    Private->ModeData[MaxMode].GopModeNumber = ModeNumber;
    Private->ModeData[MaxMode].DeltaX     = (HorizontalResolution % EFI_GLYPH_WIDTH) >> 1;
    Private->ModeData[MaxMode].DeltaY     = (VerticalResolution % EFI_GLYPH_HEIGHT) >> 1;
    MaxMode++;
  }

  //
  // Update the maximum number of modes
  //
  Private->SimpleTextOutputMode.MaxMode = (INT32) MaxMode;

  //
  // Determine the number of text modes that this protocol can support
  //
  Status = GraphicsConsoleConOutSetMode (&Private->SimpleTextOutput, 0);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  DEBUG_CODE_BEGIN ();
    GraphicsConsoleConOutOutputString (&Private->SimpleTextOutput, (CHAR16 *)L"Graphics Console Started\n\r");
  DEBUG_CODE_END ();

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
    // Close the GOP or UGA IO Protocol
    //
    if (Private->GraphicsOutput != NULL) {
      gBS->CloseProtocol (
            Controller,
            &gEfiGraphicsOutputProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      gBS->CloseProtocol (
            Controller,
            &gEfiUgaDrawProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
    }

    //
    // Free private data
    //
    if (Private != NULL) {
      if (Private->LineBuffer != NULL) {
        FreePool (Private->LineBuffer);
      }
      FreePool (Private);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOutput;
  GRAPHICS_CONSOLE_DEV             *Private;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &SimpleTextOutput,
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
    // Close the GOP or UGA IO Protocol
    //
    if (Private->GraphicsOutput != NULL) {
      gBS->CloseProtocol (
            Controller,
            &gEfiGraphicsOutputProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      gBS->CloseProtocol (
            Controller,
            &gEfiUgaDrawProtocolGuid,
            This->DriverBindingHandle,
            Controller
            );
    }

    //
    // Remove the font pack
    //
    if (Private->HiiHandle != NULL) {
      HiiLibRemovePackages (Private->HiiHandle);
      mFirstAccessFlag = TRUE;
    }

    //
    // Free our instance data
    //
    if (Private != NULL) {
      FreePool (Private->LineBuffer);
      FreePool (Private);
    }
  }

  return Status;
}

EFI_STATUS
CheckModeSupported (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput,
  IN  UINT32  HorizontalResolution,
  IN  UINT32  VerticalResolution,
  OUT UINT32  *CurrentModeNumber
  )
{
  UINT32     ModeNumber;
  EFI_STATUS Status;
  UINTN      SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;

  Status = EFI_SUCCESS;

  for (ModeNumber = 0; ModeNumber < GraphicsOutput->Mode->MaxMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                       GraphicsOutput,
                       ModeNumber,
                       &SizeOfInfo,
                       &Info
                       );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == HorizontalResolution) &&
          (Info->VerticalResolution == VerticalResolution)) {
        Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
        if (!EFI_ERROR (Status)) {
          gBS->FreePool (Info);
          break;
        }
      }
      gBS->FreePool (Info);
    }
  }

  if (ModeNumber == GraphicsOutput->Mode->MaxMode) {
    Status = EFI_UNSUPPORTED;
  }

  *CurrentModeNumber = ModeNumber;
  return Status;
}

EFI_STATUS
EfiLocateHiiProtocol (
  VOID
  )
/*++

  Routine Description:
    Locate HII protocols for future usage.

  Arguments:

  Returns:

--*/
{
  EFI_HANDLE  Handle;
  UINTN       Size;
  EFI_STATUS  Status;

  //
  // There should only be one - so buffer size is this
  //
  Size = sizeof (EFI_HANDLE);

  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  &Size,
                  (VOID **) &Handle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiHiiDatabaseProtocolGuid,
                  (VOID **) &mHiiDatabase
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiHiiFontProtocolGuid,
                  (VOID **) &mHiiFont
                  );
  return Status;
}

//
// Body of the STO functions
//
EFI_STATUS
EFIAPI
GraphicsConsoleConOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.Reset().
    If ExtendeVerification is TRUE, then perform dependent Graphics Console
    device reset, and set display mode to mode 0.
    If ExtendedVerification is FALSE, only set display mode to mode 0.

  Arguments:

    This - Indicates the calling context.

    ExtendedVerification - Indicates that the driver may perform a more exhaustive
                           verification operation of the device during reset.

  Returns:

    EFI_SUCCESS
       The reset operation succeeds.

    EFI_DEVICE_ERROR
      The Graphics Console is not functioning correctly

--*/
{
  This->SetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BACKGROUND_BLACK));
  return This->SetMode (This, 0);
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.OutputString().
    The Unicode string will be converted to Glyphs and will be
    sent to the Graphics Console.


  Arguments:

    This - Indicates the calling context.

    WString - The Null-terminated Unicode string to be displayed on
              the Graphics Console.

  Returns:

    EFI_SUCCESS
       The string is output successfully.

    EFI_DEVICE_ERROR
      The Graphics Console failed to send the string out.

    EFI_WARN_UNKNOWN_GLYPH
      Indicates that some of the characters in the Unicode string could not
      be rendered and are skipped.

--*/
{
  GRAPHICS_CONSOLE_DEV  *Private;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL *UgaDraw;
  INTN                  Mode;
  UINTN                 MaxColumn;
  UINTN                 MaxRow;
  UINTN                 Width;
  UINTN                 Height;
  UINTN                 Delta;
  EFI_STATUS            Status;
  BOOLEAN               Warning;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Background;
  UINTN                 DeltaX;
  UINTN                 DeltaY;
  UINTN                 Count;
  UINTN                 Index;
  INT32                 OriginAttribute;
  EFI_TPL               OldTpl;
  CHAR16                         SpaceStr[] = { NARROW_CHAR, ' ', 0 };

  Status = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  //
  // Current mode
  //
  Mode      = This->Mode->Mode;
  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  UgaDraw   = Private->UgaDraw;

  MaxColumn = Private->ModeData[Mode].Columns;
  MaxRow    = Private->ModeData[Mode].Rows;
  DeltaX    = Private->ModeData[Mode].DeltaX;
  DeltaY    = Private->ModeData[Mode].DeltaY;
  Width     = MaxColumn * EFI_GLYPH_WIDTH;
  Height    = (MaxRow - 1) * EFI_GLYPH_HEIGHT;
  Delta     = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  //
  // The Attributes won't change when during the time OutputString is called
  //
  GetTextColors (This, &Foreground, &Background);

  EraseCursor (This);

  Warning = FALSE;

  //
  // Backup attribute
  //
  OriginAttribute = This->Mode->Attribute;

  while (*WString) {

    if (*WString == CHAR_BACKSPACE) {
      //
      // If the cursor is at the left edge of the display, then move the cursor
      // one row up.
      //
      if (This->Mode->CursorColumn == 0 && This->Mode->CursorRow > 0) {
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32) (MaxColumn - 1);
        This->OutputString (This, SpaceStr);
        EraseCursor (This);
        This->Mode->CursorRow--;
        This->Mode->CursorColumn = (INT32) (MaxColumn - 1);
      } else if (This->Mode->CursorColumn > 0) {
        //
        // If the cursor is not at the left edge of the display, then move the cursor
        // left one column.
        //
        This->Mode->CursorColumn--;
        This->OutputString (This, SpaceStr);
        EraseCursor (This);
        This->Mode->CursorColumn--;
      }

      WString++;

    } else if (*WString == CHAR_LINEFEED) {
      //
      // If the cursor is at the bottom of the display, then scroll the display one
      // row, and do not update the cursor position. Otherwise, move the cursor
      // down one row.
      //
      if (This->Mode->CursorRow == (INT32) (MaxRow - 1)) {
        if (GraphicsOutput != NULL) {
          //
          // Scroll Screen Up One Row
          //
          GraphicsOutput->Blt (
                    GraphicsOutput,
                    NULL,
                    EfiBltVideoToVideo,
                    DeltaX,
                    DeltaY + EFI_GLYPH_HEIGHT,
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
                    EFI_GLYPH_HEIGHT,
                    Delta
                    );
        } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
          //
          // Scroll Screen Up One Row
          //
          UgaDraw->Blt (
                    UgaDraw,
                    NULL,
                    EfiUgaVideoToVideo,
                    DeltaX,
                    DeltaY + EFI_GLYPH_HEIGHT,
                    DeltaX,
                    DeltaY,
                    Width,
                    Height,
                    Delta
                    );

          //
          // Print Blank Line at last line
          //
          UgaDraw->Blt (
                    UgaDraw,
                    (EFI_UGA_PIXEL *) (UINTN) &Background,
                    EfiUgaVideoFill,
                    0,
                    0,
                    DeltaX,
                    DeltaY + Height,
                    Width,
                    EFI_GLYPH_HEIGHT,
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

      This->Mode->Attribute &= (~ (UINT32) EFI_WIDE_ATTRIBUTE);
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
        if (WString[Count] == CHAR_NULL) {
          break;
        }

        if (WString[Count] == CHAR_BACKSPACE) {
          break;
        }

        if (WString[Count] == CHAR_LINEFEED) {
          break;
        }

        if (WString[Count] == CHAR_CARRIAGE_RETURN) {
          break;
        }

        if (WString[Count] == WIDE_CHAR) {
          break;
        }

        if (WString[Count] == NARROW_CHAR) {
          break;
        }
        //
        // Is the wide attribute on?
        //
        if (This->Mode->Attribute & EFI_WIDE_ATTRIBUTE) {
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

      Status = DrawUnicodeWeightAtCursorN (This, WString, Count);
      if (EFI_ERROR (Status)) {
        Warning = TRUE;
      }
      //
      // At the end of line, output carriage return and line feed
      //
      WString += Count;
      This->Mode->CursorColumn += (INT32) Index;
      if (This->Mode->CursorColumn > (INT32) MaxColumn) {
        This->Mode->CursorColumn -= 2;
        This->OutputString (This, SpaceStr);
      }

      if (This->Mode->CursorColumn >= (INT32) MaxColumn) {
        EraseCursor (This);
        This->OutputString (This, mCrLfString);
        EraseCursor (This);
      }
    }
  }

  This->Mode->Attribute = OriginAttribute;

  EraseCursor (This);

  if (Warning) {
    Status = EFI_WARN_UNKNOWN_GLYPH;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;

}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.TestString().
    If one of the characters in the *Wstring is
    neither valid valid Unicode drawing characters,
    not ASCII code, then this function will return
    EFI_UNSUPPORTED.


  Arguments:

    This - Indicates the calling context.

    WString - The Null-terminated Unicode string to be tested.

  Returns:

    EFI_SUCCESS
       The Graphics Console is capable of rendering the output string.

    EFI_UNSUPPORTED
      Some of the characters in the Unicode string cannot be rendered.

--*/
{
  EFI_STATUS            Status;
  UINT16                Count;

  EFI_IMAGE_OUTPUT      *Blt = NULL;

  Count = 0;

  while (WString[Count] != 0) {
    Status = mHiiFont->GetGlyph (
                         mHiiFont,
                         WString[Count],
                         NULL,
                         &Blt,
                         NULL
                         );
    SafeFreePool (Blt);
    Blt = NULL;
    Count++;

    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.QueryMode().
    It returnes information for an available text mode
    that the Graphics Console supports.
    In this driver,we only support text mode 80x25, which is
    defined as mode 0.


  Arguments:

    This - Indicates the calling context.

    ModeNumber - The mode number to return information on.

    Columns - The returned columns of the requested mode.

    Rows - The returned rows of the requested mode.

  Returns:

    EFI_SUCCESS
      The requested mode information is returned.

    EFI_UNSUPPORTED
      The mode number is not valid.

--*/
{
  GRAPHICS_CONSOLE_DEV  *Private;
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;

  if (ModeNumber >= (UINTN) This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  Status = EFI_SUCCESS;

  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);

  *Columns  = Private->ModeData[ModeNumber].Columns;
  *Rows     = Private->ModeData[ModeNumber].Rows;

  if (*Columns <= 0 && *Rows <= 0) {
    Status = EFI_UNSUPPORTED;
    goto Done;

  }

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.SetMode().
    Set the Graphics Console to a specified mode.
    In this driver, we only support mode 0.

  Arguments:

    This - Indicates the calling context.

    ModeNumber - The text mode to set.

  Returns:

    EFI_SUCCESS
       The requested text mode is set.

    EFI_DEVICE_ERROR
      The requested text mode cannot be set because of Graphics Console device error.

    EFI_UNSUPPORTED
      The text mode number is not valid.

--*/
{
  EFI_STATUS                      Status;
  GRAPHICS_CONSOLE_DEV            *Private;
  GRAPHICS_CONSOLE_MODE_DATA      *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *NewLineBuffer;
  UINT32                          HorizontalResolution;
  UINT32                          VerticalResolution;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL           *UgaDraw;
  UINT32                          ColorDepth;
  UINT32                          RefreshRate;
  EFI_TPL                         OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  UgaDraw   = Private->UgaDraw;
  ModeData  = &(Private->ModeData[ModeNumber]);

  if (ModeData->Columns <= 0 && ModeData->Rows <= 0) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  //
  // Make sure the requested mode number is supported
  //
  if (ModeNumber >= (UINTN) This->Mode->MaxMode) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (ModeData->Columns <= 0 && ModeData->Rows <= 0) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }
  //
  // Attempt to allocate a line buffer for the requested mode number
  //
  NewLineBuffer = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * ModeData->Columns * EFI_GLYPH_WIDTH * EFI_GLYPH_HEIGHT);

  if (NewLineBuffer == NULL) {
    //
    // The new line buffer could not be allocated, so return an error.
    // No changes to the state of the current console have been made, so the current console is still valid
    //
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  //
  // If the mode has been set at least one other time, then LineBuffer will not be NULL
  //
  if (Private->LineBuffer != NULL) {
    //
    // Clear the current text window on the current graphics console
    //
    This->ClearScreen (This);

    //
    // If the new mode is the same as the old mode, then just return EFI_SUCCESS
    //
    if ((INT32) ModeNumber == This->Mode->Mode) {
      FreePool (NewLineBuffer);
      Status = EFI_SUCCESS;
      goto Done;
    }
    //
    // Otherwise, the size of the text console and/or the GOP/UGA mode will be changed,
    // so erase the cursor, and free the LineBuffer for the current mode
    //
    EraseCursor (This);

    FreePool (Private->LineBuffer);
  }
  //
  // Assign the current line buffer to the newly allocated line buffer
  //
  Private->LineBuffer = NewLineBuffer;

  if (GraphicsOutput != NULL) {
    if (ModeData->GopModeNumber != GraphicsOutput->Mode->Mode) {
      //
      // Either no graphics mode is currently set, or it is set to the wrong resolution, so set the new grapghics mode
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
                          &mEfiColors[0],
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
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // Get the current UGA Draw mode information
    //
    Status = UgaDraw->GetMode (
                        UgaDraw,
                        &HorizontalResolution,
                        &VerticalResolution,
                        &ColorDepth,
                        &RefreshRate
                        );
    if (EFI_ERROR (Status) || HorizontalResolution != ModeData->GopWidth || VerticalResolution != ModeData->GopHeight) {
      //
      // Either no graphics mode is currently set, or it is set to the wrong resolution, so set the new grapghics mode
      //
      Status = UgaDraw->SetMode (
                          UgaDraw,
                          ModeData->GopWidth,
                          ModeData->GopHeight,
                          32,
                          60
                          );
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
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) (UINTN) &mEfiColors[0],
                          EfiUgaVideoFill,
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
  This->Mode->Mode = (INT32) ModeNumber;

  //
  // Move the text cursor to the upper left hand corner of the displat and enable it
  //
  This->SetCursorPosition (This, 0, 0);

  Status = EFI_SUCCESS;

Done:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.SetAttribute().

  Arguments:

    This - Indicates the calling context.

    Attrubute - The attribute to set. Only bit0..6 are valid, all other bits
                are undefined and must be zero.

  Returns:

    EFI_SUCCESS
      The requested attribute is set.

    EFI_DEVICE_ERROR
      The requested attribute cannot be set due to Graphics Console port error.

    EFI_UNSUPPORTED
      The attribute requested is not defined by EFI spec.

--*/
{
  EFI_TPL               OldTpl;

  if ((Attribute | 0xFF) != 0xFF) {
    return EFI_UNSUPPORTED;
  }

  if ((INT32) Attribute == This->Mode->Attribute) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  EraseCursor (This);

  This->Mode->Attribute = (INT32) Attribute;

  EraseCursor (This);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.ClearScreen().
    It clears the Graphics Console's display to the
    currently selected background color.


  Arguments:

    This - Indicates the calling context.

  Returns:

    EFI_SUCCESS
      The operation completed successfully.

    EFI_DEVICE_ERROR
      The Graphics Console cannot be cleared due to Graphics Console device error.

    EFI_UNSUPPORTED
      The Graphics Console is not in a valid text mode.

--*/
{
  EFI_STATUS                    Status;
  GRAPHICS_CONSOLE_DEV          *Private;
  GRAPHICS_CONSOLE_MODE_DATA    *ModeData;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_TPL                       OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  UgaDraw   = Private->UgaDraw;
  ModeData  = &(Private->ModeData[This->Mode->Mode]);

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
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    Status = UgaDraw->Blt (
                        UgaDraw,
                        (EFI_UGA_PIXEL *) (UINTN) &Background,
                        EfiUgaVideoFill,
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

  This->Mode->CursorColumn  = 0;
  This->Mode->CursorRow     = 0;

  EraseCursor (This);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.SetCursorPosition().

  Arguments:

    This - Indicates the calling context.

    Column - The row to set cursor to.

    Row - The column to set cursor to.

  Returns:

    EFI_SUCCESS
      The operation completed successfully.

    EFI_DEVICE_ERROR
      The request fails due to Graphics Console device error.

    EFI_UNSUPPORTED
      The Graphics Console is not in a valid text mode, or the cursor position
      is invalid for current mode.

--*/
{
  GRAPHICS_CONSOLE_DEV        *Private;
  GRAPHICS_CONSOLE_MODE_DATA  *ModeData;
  EFI_STATUS                  Status;
  EFI_TPL                     OldTpl;

  Status = EFI_SUCCESS;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  Private   = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  ModeData  = &(Private->ModeData[This->Mode->Mode]);

  if ((Column >= ModeData->Columns) || (Row >= ModeData->Rows)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  if (((INT32) Column == This->Mode->CursorColumn) && ((INT32) Row == This->Mode->CursorRow)) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  EraseCursor (This);

  This->Mode->CursorColumn  = (INT32) Column;
  This->Mode->CursorRow     = (INT32) Row;

  EraseCursor (This);

Done:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
GraphicsConsoleConOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
/*++
  Routine Description:

    Implements SIMPLE_TEXT_OUTPUT.EnableCursor().
    In this driver, the cursor cannot be hidden.

  Arguments:

    This - Indicates the calling context.

    Visible - If TRUE, the cursor is set to be visible,
              If FALSE, the cursor is set to be invisible.

  Returns:

    EFI_SUCCESS
      The request is valid.

    EFI_UNSUPPORTED
      The Graphics Console does not support a hidden cursor.

--*/
{
  EFI_TPL               OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  EraseCursor (This);

  This->Mode->CursorVisible = Visible;

  EraseCursor (This);

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetTextColors (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background
  )
{
  INTN  Attribute;

  Attribute   = This->Mode->Attribute & 0x7F;

  *Foreground = mEfiColors[Attribute & 0x0f];
  *Background = mEfiColors[Attribute >> 4];

  return EFI_SUCCESS;
}

EFI_STATUS
DrawUnicodeWeightAtCursorN (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                        *UnicodeWeight,
  IN  UINTN                         Count
  )
{
  EFI_STATUS                        Status;
  GRAPHICS_CONSOLE_DEV              *Private;
  EFI_IMAGE_OUTPUT                  *Blt;
  EFI_STRING                        String;
  EFI_FONT_DISPLAY_INFO             *FontInfo;
  EFI_UGA_DRAW_PROTOCOL             *UgaDraw;
  EFI_HII_ROW_INFO                  *RowInfoArray;
  UINTN                             RowInfoArraySize;

  Private = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  Blt = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Blt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Blt->Width        = (UINT16) (Private->ModeData[This->Mode->Mode].GopWidth);
  Blt->Height       = (UINT16) (Private->ModeData[This->Mode->Mode].GopHeight);

  String = AllocateCopyPool ((Count + 1) * sizeof (CHAR16), UnicodeWeight);
  if (String == NULL) {
    SafeFreePool (Blt);
    return EFI_OUT_OF_RESOURCES;
  }
  *(String + Count) = 0;

  FontInfo = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (sizeof (EFI_FONT_DISPLAY_INFO));
  if (FontInfo == NULL) {
    SafeFreePool (Blt);
    SafeFreePool (String);
    return EFI_OUT_OF_RESOURCES;
  }
  GetTextColors (This, &FontInfo->ForegroundColor, &FontInfo->BackgroundColor);

  if (Private->GraphicsOutput != NULL) {
    Blt->Image.Screen = Private->GraphicsOutput;

    Status = mHiiFont->StringToImage (
                         mHiiFont,
                         EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_DIRECT_TO_SCREEN,
                         String,
                         FontInfo,
                         &Blt,
                         This->Mode->CursorColumn * EFI_GLYPH_WIDTH + Private->ModeData[This->Mode->Mode].DeltaX,
                         This->Mode->CursorRow * EFI_GLYPH_HEIGHT + Private->ModeData[This->Mode->Mode].DeltaY,
                         NULL,
                         NULL,
                         NULL
                         );

  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    ASSERT (Private->UgaDraw!= NULL);

    UgaDraw = Private->UgaDraw;

    Blt->Image.Bitmap = AllocateZeroPool (Blt->Width * Blt->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (Blt->Image.Bitmap == NULL) {
      SafeFreePool (Blt);
      SafeFreePool (String);
      return EFI_OUT_OF_RESOURCES;
    }

    RowInfoArray = NULL;
    //
    //  StringToImage only support blt'ing image to device using GOP protocol. If GOP is not supported in this platform,
    //  we ask StringToImage to print the string to blt buffer, then blt to device using UgaDraw.
    //
    Status = mHiiFont->StringToImage (
                          mHiiFont,
                          EFI_HII_IGNORE_IF_NO_GLYPH,
                          String,
                          FontInfo,
                          &Blt,
                          This->Mode->CursorColumn * EFI_GLYPH_WIDTH + Private->ModeData[This->Mode->Mode].DeltaX,
                          This->Mode->CursorRow * EFI_GLYPH_HEIGHT + Private->ModeData[This->Mode->Mode].DeltaY,
                          &RowInfoArray,
                          &RowInfoArraySize,
                          NULL
                          );

    if (!EFI_ERROR (Status)) {
      //
      // Line breaks are handled by caller of DrawUnicodeWeightAtCursorN, so the updated parameter RowInfoArraySize by StringToImage will
      // always be 1 or 0 (if there is no valid Unicode Char can be printed). ASSERT here to make sure.
      //
      ASSERT (RowInfoArraySize <= 1);

      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) Blt->Image.Bitmap,
                          EfiUgaBltBufferToVideo,
                          This->Mode->CursorColumn * EFI_GLYPH_WIDTH  + Private->ModeData[This->Mode->Mode].DeltaX,
                          (This->Mode->CursorRow) * EFI_GLYPH_HEIGHT + Private->ModeData[This->Mode->Mode].DeltaY,
                          This->Mode->CursorColumn * EFI_GLYPH_WIDTH  + Private->ModeData[This->Mode->Mode].DeltaX,
                          (This->Mode->CursorRow) * EFI_GLYPH_HEIGHT + Private->ModeData[This->Mode->Mode].DeltaY,
                          RowInfoArray[0].LineWidth,
                          RowInfoArray[0].LineHeight,
                          Blt->Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    }

    SafeFreePool (RowInfoArray);
    SafeFreePool (Blt->Image.Bitmap);
  } else {
    Status = EFI_UNSUPPORTED;
  }

  SafeFreePool (Blt);
  SafeFreePool (String);
  SafeFreePool (FontInfo);
  return Status;
}


STATIC
EFI_STATUS
EraseCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  GRAPHICS_CONSOLE_DEV        *Private;
  EFI_SIMPLE_TEXT_OUTPUT_MODE *CurrentMode;
  INTN                        GlyphX;
  INTN                        GlyphY;
  EFI_GRAPHICS_OUTPUT_PROTOCOL        *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL       *UgaDraw;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION BltChar[EFI_GLYPH_HEIGHT][EFI_GLYPH_WIDTH];
  UINTN                       X;
  UINTN                       Y;

  CurrentMode = This->Mode;

  if (!CurrentMode->CursorVisible) {
    return EFI_SUCCESS;
  }

  Private = GRAPHICS_CONSOLE_CON_OUT_DEV_FROM_THIS (This);
  GraphicsOutput = Private->GraphicsOutput;
  UgaDraw = Private->UgaDraw;

  //
  // BUGBUG - we need to think about what to do with wide and narrow character deletions.
  //
  //
  // Blt a character to the screen
  //
  GlyphX  = (CurrentMode->CursorColumn * EFI_GLYPH_WIDTH) + Private->ModeData[CurrentMode->Mode].DeltaX;
  GlyphY  = (CurrentMode->CursorRow * EFI_GLYPH_HEIGHT) + Private->ModeData[CurrentMode->Mode].DeltaY;
  if (GraphicsOutput != NULL) {
    GraphicsOutput->Blt (
              GraphicsOutput,
              (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) BltChar,
              EfiBltVideoToBltBuffer,
              GlyphX,
              GlyphY,
              0,
              0,
              EFI_GLYPH_WIDTH,
              EFI_GLYPH_HEIGHT,
              EFI_GLYPH_WIDTH * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
              );
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    UgaDraw->Blt (
              UgaDraw,
              (EFI_UGA_PIXEL *) (UINTN) BltChar,
              EfiUgaVideoToBltBuffer,
              GlyphX,
              GlyphY,
              0,
              0,
              EFI_GLYPH_WIDTH,
              EFI_GLYPH_HEIGHT,
              EFI_GLYPH_WIDTH * sizeof (EFI_UGA_PIXEL)
              );
  }

  GetTextColors (This, &Foreground.Pixel, &Background.Pixel);

  //
  // Convert Monochrome bitmap of the Glyph to BltBuffer structure
  //
  for (Y = 0; Y < EFI_GLYPH_HEIGHT; Y++) {
    for (X = 0; X < EFI_GLYPH_WIDTH; X++) {
      if ((mCursorGlyph.GlyphCol1[Y] & (1 << X)) != 0) {
        BltChar[Y][EFI_GLYPH_WIDTH - X - 1].Raw ^= Foreground.Raw;
      }
    }
  }

  if (GraphicsOutput != NULL) {
    GraphicsOutput->Blt (
              GraphicsOutput,
              (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) BltChar,
              EfiBltBufferToVideo,
              0,
              0,
              GlyphX,
              GlyphY,
              EFI_GLYPH_WIDTH,
              EFI_GLYPH_HEIGHT,
              EFI_GLYPH_WIDTH * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
              );
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    UgaDraw->Blt (
              UgaDraw,
              (EFI_UGA_PIXEL *) (UINTN) BltChar,
              EfiUgaBltBufferToVideo,
              0,
              0,
              GlyphX,
              GlyphY,
              EFI_GLYPH_WIDTH,
              EFI_GLYPH_HEIGHT,
              EFI_GLYPH_WIDTH * sizeof (EFI_UGA_PIXEL)
              );
  }

  return EFI_SUCCESS;
}

/**
  The user Entry Point for module GraphicsConsole. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeGraphicsConsole (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

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


