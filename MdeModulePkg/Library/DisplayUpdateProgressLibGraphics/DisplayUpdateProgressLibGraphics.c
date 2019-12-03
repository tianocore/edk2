/** @file
  Provides services to display completion progress of a firmware update on a
  graphical console that supports the Graphics Output Protocol.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/BootLogo2.h>

//
// Values in percent of of logo height.
//
#define LOGO_BOTTOM_PADDING    20
#define PROGRESS_BLOCK_HEIGHT  10

//
// Graphics Output Protocol instance to display progress bar
//
EFI_GRAPHICS_OUTPUT_PROTOCOL  *mGop = NULL;

//
// Set to 100 percent so it is reset on first call.
//
UINTN mPreviousProgress = 100;

//
// Display coordinates for the progress bar.
//
UINTN  mStartX = 0;
UINTN  mStartY = 0;

//
// Width and height of the progress bar.
//
UINTN  mBlockWidth  = 0;
UINTN  mBlockHeight = 0;

//
// GOP bitmap of the progress bar. Initialized on every new progress of 100%
//
EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *mBlockBitmap;

//
// GOP bitmap of the progress bar backround.  Initialized once.
//
EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *mProgressBarBackground;

//
// Default mask used to detect the left, right , top, and bottom of logo.  Only
// green and blue pixels are used for logo detection.
//
const EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  mLogoDetectionColorMask = {
  {
    0xFF,  // Blue
    0xFF,  // Green
    0x00,  // Red
    0x00   // Reserved
  }
};

//
// Background color of progress bar.  Grey.
//
const EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  mProgressBarBackgroundColor = {
  {
    0x80,  // Blue
    0x80,  // Green
    0x80,  // Red
    0x00   // Reserved
  }
};

//
// Default color of progress completion.  White.
//
const EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  mProgressBarDefaultColor = {
  {
    0xFF,  // Blue
    0xFF,  // Green
    0xFF,  // Red
    0x00   // Reserved
  }
};

//
// Set to TRUE if a valid Graphics Output Protocol is found and the progress
// bar fits under the boot logo using the current graphics mode.
//
BOOLEAN mGraphicsGood = FALSE;

/**
  Internal function used to find the bounds of the white logo (on black or
  red background).

  These bounds are then computed to find the block size, 0%, 100%, etc.

**/
VOID
FindDim (
   VOID
  )
{
  EFI_STATUS                           Status;
  INTN                                 LogoX;
  INTN                                 LogoStartX;
  INTN                                 LogoEndX;
  INTN                                 LogoY;
  INTN                                 LogoStartY;
  INTN                                 LogoEndY;
  UINTN                                OffsetX;     // Logo screen coordinate
  UINTN                                OffsetY;     // Logo screen coordinate
  UINTN                                Width;       // Width of logo in pixels
  UINTN                                Height;      // Height of logo in pixels
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *Logo;
  EDKII_BOOT_LOGO2_PROTOCOL            *BootLogo;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  *Pixel;

  Logo     = NULL;
  BootLogo = NULL;

  //
  // Return if a Graphics Output Protocol ha snot been found.
  //
  if (mGop == NULL) {
    DEBUG ((DEBUG_ERROR, "No GOP found.  No progress bar support. \n"));
    return;
  }

  //
  // Get boot logo protocol so we know where on the screen to grab
  //
  Status = gBS->LocateProtocol (
                  &gEdkiiBootLogo2ProtocolGuid,
                  NULL,
                  (VOID **)&BootLogo
                  );
  if ((BootLogo == NULL) || (EFI_ERROR (Status))) {
    DEBUG ((DEBUG_ERROR, "Failed to locate gEdkiiBootLogo2ProtocolGuid.  No Progress bar support. \n", Status));
    return;
  }

  //
  // Get logo location and size
  //
  Status = BootLogo->GetBootLogo (
                       BootLogo,
                       &Logo,
                       &OffsetX,
                       &OffsetY,
                       &Width,
                       &Height
                       );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to Get Boot Logo Status = %r.  No Progress bar support. \n", Status));
    return;
  }

  //
  // Within logo buffer find where the actual logo starts/ends
  //
  LogoEndX = 0;
  LogoEndY = 0;

  //
  // Find left side of logo in logo coordinates
  //
  for (LogoX = 0, LogoStartX = Width; LogoX < LogoStartX; LogoX++) {
    Pixel = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *)(Logo + LogoX);
    for (LogoY = 0; LogoY < (INTN)Height; LogoY++) {
      if ((Pixel->Raw & mLogoDetectionColorMask.Raw) != 0x0) {
        LogoStartX = LogoX;
        //
        // For loop searches from right side back to this column.
        //
        LogoEndX = LogoX;
        DEBUG ((DEBUG_INFO, "StartX found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, Pixel->Raw));
        break;
      }
      Pixel = Pixel + Width;
    }
  }

  //
  // Find right side of logo
  //
  for (LogoX = Width - 1; LogoX >= LogoEndX; LogoX--) {
    Pixel = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *)(Logo + LogoX);
    for (LogoY = 0; LogoY < (INTN)Height; LogoY++) {
      if ((Pixel->Raw & mLogoDetectionColorMask.Raw) != 0x0) {
        LogoEndX = LogoX;
        DEBUG ((DEBUG_INFO, "EndX found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, Pixel->Raw));
        break;
      }
      Pixel = Pixel + Width;
    }
  }

  //
  // Compute mBlockWidth
  //
  mBlockWidth = ((LogoEndX - LogoStartX) + 99) / 100;

  //
  // Adjust mStartX based on block width so it is centered under logo
  //
  mStartX = LogoStartX + OffsetX - (((mBlockWidth * 100) - (LogoEndX - LogoStartX)) / 2);
  DEBUG ((DEBUG_INFO, "mBlockWidth set to 0x%X\n", mBlockWidth));
  DEBUG ((DEBUG_INFO, "mStartX set to 0x%X\n", mStartX));

  //
  // Find the top of the logo
  //
  for (LogoY = 0, LogoStartY = Height; LogoY < LogoStartY; LogoY++) {
    Pixel = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *)(Logo + (Width * LogoY));
    for (LogoX = 0; LogoX < (INTN)Width; LogoX++) {
      //not black or red
      if ((Pixel->Raw & mLogoDetectionColorMask.Raw) != 0x0) {
        LogoStartY = LogoY;
        LogoEndY = LogoY; //for next loop will search from bottom side back to this row.
        DEBUG ((DEBUG_INFO, "StartY found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, Pixel->Raw));
        break;
      }
      Pixel++;
    }
  }

  //
  // Find the bottom of the logo
  //
  for (LogoY = Height - 1; LogoY >= LogoEndY; LogoY--) {
    Pixel = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *)(Logo + (Width * LogoY));
    for (LogoX = 0; LogoX < (INTN)Width; LogoX++) {
      if ((Pixel->Raw & mLogoDetectionColorMask.Raw) != 0x0) {
        LogoEndY = LogoY;
        DEBUG ((DEBUG_INFO, "EndY found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, Pixel->Raw));
        break;
      }
      Pixel++;
    }
  }

  //
  // Compute bottom padding (distance between logo bottom and progress bar)
  //
  mStartY = (((LogoEndY - LogoStartY) * LOGO_BOTTOM_PADDING) / 100) + LogoEndY + OffsetY;

  //
  // Compute progress bar height
  //
  mBlockHeight = (((LogoEndY - LogoStartY) * PROGRESS_BLOCK_HEIGHT) / 100);

  DEBUG ((DEBUG_INFO, "mBlockHeight set to 0x%X\n", mBlockHeight));

  //
  // Create progress bar background (one time init).
  //
  mProgressBarBackground = AllocatePool (mBlockWidth * 100 * mBlockHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (mProgressBarBackground == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate progress bar background\n"));
    return;
  }

  //
  // Fill the progress bar with the background color
  //
  SetMem32 (
    mProgressBarBackground,
    (mBlockWidth * 100 * mBlockHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)),
    mProgressBarBackgroundColor.Raw
    );

  //
  // Allocate mBlockBitmap
  //
  mBlockBitmap = AllocatePool (mBlockWidth * mBlockHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (mBlockBitmap == NULL) {
    FreePool (mProgressBarBackground);
    DEBUG ((DEBUG_ERROR, "Failed to allocate block\n"));
    return;
  }

  //
  // Check screen width and height and make sure it fits.
  //
  if ((mBlockHeight > Height) || (mBlockWidth > Width) || (mBlockHeight < 1) || (mBlockWidth < 1)) {
    DEBUG ((DEBUG_ERROR, "DisplayUpdateProgressLib - Progress - Failed to get valid width and height.\n"));
    DEBUG ((DEBUG_ERROR, "DisplayUpdateProgressLib - Progress - mBlockHeight: 0x%X  mBlockWidth: 0x%X.\n", mBlockHeight, mBlockWidth));
    FreePool (mProgressBarBackground);
    FreePool (mBlockBitmap);
    return;
  }

  mGraphicsGood = TRUE;
}

/**
  Function indicates the current completion progress of a firmware update.
  Platform may override with its own specific function.

  @param[in] Completion  A value between 0 and 100 indicating the current
                         completion progress of a firmware update.  This
                         value must the the same or higher than previous
                         calls to this service.  The first call of 0 or a
                         value of 0 after reaching a value of 100 resets
                         the progress indicator to 0.
  @param[in] Color       Color of the progress indicator.  Only used when
                         Completion is 0 to set the color of the progress
                         indicator.  If Color is NULL, then the default color
                         is used.

  @retval EFI_SUCCESS            Progress displayed successfully.
  @retval EFI_INVALID_PARAMETER  Completion is not in range 0..100.
  @retval EFI_INVALID_PARAMETER  Completion is less than Completion value from
                                 a previous call to this service.
  @retval EFI_NOT_READY          The device used to indicate progress is not
                                 available.
**/
EFI_STATUS
EFIAPI
DisplayUpdateProgress (
  IN UINTN                                Completion,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION  *Color       OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       PreX;
  UINTN       Index;

  //
  // Check range
  //
  if (Completion > 100) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if this Completion percentage has already been displayed
  //
  if (Completion == mPreviousProgress) {
    return EFI_SUCCESS;
  }

  //
  // Find Graphics Output Protocol if not already set.  1 time.
  //
  if (mGop == NULL) {
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&mGop
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
      if (EFI_ERROR (Status)) {
        mGop = NULL;
        DEBUG ((DEBUG_ERROR, "Show Progress Function could not locate GOP.  Status = %r\n", Status));
        return EFI_NOT_READY;
      }
    }

    //
    // Run once
    //
    FindDim ();
  }

  //
  // Make sure a valid start, end, and size info are available (find the Logo)
  //
  if (!mGraphicsGood) {
    DEBUG ((DEBUG_INFO, "Graphics Not Good.  Not doing any onscreen visual display\n"));
    return EFI_NOT_READY;
  }

  //
  // Do special init on first call of each progress session
  //
  if (mPreviousProgress == 100) {
    //
    // Draw progress bar background
    //
    mGop->Blt (
            mGop,
            mProgressBarBackground,
            EfiBltBufferToVideo,
            0,
            0,
            mStartX,
            mStartY,
            (mBlockWidth * 100),
            mBlockHeight,
            0
            );

    DEBUG ((DEBUG_VERBOSE, "Color is 0x%X\n",
      (Color == NULL) ? mProgressBarDefaultColor.Raw : Color->Raw
      ));

    //
    // Update block bitmap with correct color
    //
    SetMem32 (
      mBlockBitmap,
      (mBlockWidth * mBlockHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)),
      (Color == NULL) ? mProgressBarDefaultColor.Raw : Color->Raw
      );

    //
    // Clear previous
    //
    mPreviousProgress = 0;
  }

  //
  // Can not update progress bar if Completion is less than previous
  //
  if (Completion < mPreviousProgress) {
    DEBUG ((DEBUG_WARN, "WARNING: Completion (%d) should not be lesss than Previous (%d)!!!\n", Completion, mPreviousProgress));
    return EFI_INVALID_PARAMETER;
  }

  PreX = ((mPreviousProgress * mBlockWidth) + mStartX);
  for (Index = 0; Index < (Completion - mPreviousProgress); Index++) {
    //
    // Show progress by coloring new area
    //
    mGop->Blt (
            mGop,
            mBlockBitmap,
            EfiBltBufferToVideo,
            0,
            0,
            PreX,
            mStartY,
            mBlockWidth,
            mBlockHeight,
            0
            );
    PreX += mBlockWidth;
  }

  mPreviousProgress = Completion;

  return EFI_SUCCESS;
}
