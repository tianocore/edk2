/** @file
  This file contains the platform independent parts of HdLcd

  Copyright (c) 2011-2018, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "HdLcd.h"

#define BYTES_PER_PIXEL 4

/** Initialize display.

  @param[in]  VramBaseAddress    Address of the framebuffer.

  @retval EFI_SUCCESS            Display initialization successful.
**/
EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS   VramBaseAddress
  )
{
  // Disable the controller
  MmioWrite32 (HDLCD_REG_COMMAND, HDLCD_DISABLE);

  // Disable all interrupts
  MmioWrite32 (HDLCD_REG_INT_MASK, 0);

  // Define start of the VRAM. This never changes for any graphics mode
  MmioWrite32 (HDLCD_REG_FB_BASE, (UINT32)VramBaseAddress);

  // Setup various registers that never change
  MmioWrite32 (HDLCD_REG_BUS_OPTIONS,  (4 << 8) | HDLCD_BURST_8);

  MmioWrite32 (HDLCD_REG_POLARITIES, HDLCD_DEFAULT_POLARITIES);

  MmioWrite32 (
    HDLCD_REG_PIXEL_FORMAT,
    HDLCD_LITTLE_ENDIAN | HDLCD_4BYTES_PER_PIXEL
    );

  return EFI_SUCCESS;
}

/** Set requested mode of the display.

  @param[in] ModeNumber          Display mode number.

  @retval EFI_SUCCESS            Display mode set successfully.
  @retval !(EFI_SUCCESS)         Other errors.
**/
EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  )
{
  EFI_STATUS        Status;
  SCAN_TIMINGS      *Horizontal;
  SCAN_TIMINGS      *Vertical;

  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  ModeInfo;

  // Set the video mode timings and other relevant information
  Status = LcdPlatformGetTimings (
             ModeNumber,
             &Horizontal,
             &Vertical
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ASSERT (Horizontal != NULL);
  ASSERT (Vertical != NULL);

  // Get the pixel format information.
  Status = LcdPlatformQueryMode (ModeNumber, &ModeInfo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // By default PcdArmHdLcdSwapBlueRedSelect is set to false
  // However on the Juno platform HW lines for BLUE and RED are swapped
  // Therefore PcdArmHdLcdSwapBlueRedSelect is set to TRUE for the Juno platform
  PixelFormat = FixedPcdGetBool (PcdArmHdLcdSwapBlueRedSelect)
                ? PixelRedGreenBlueReserved8BitPerColor
                : PixelBlueGreenRedReserved8BitPerColor;

  if (ModeInfo.PixelFormat == PixelFormat) {
    MmioWrite32 (HDLCD_REG_RED_SELECT,  (8 << 8) | 16);
    MmioWrite32 (HDLCD_REG_BLUE_SELECT, (8 << 8) | 0);
  } else {
    MmioWrite32 (HDLCD_REG_BLUE_SELECT, (8 << 8) | 16);
    MmioWrite32 (HDLCD_REG_RED_SELECT,  (8 << 8) | 0);
  }

  MmioWrite32 (HDLCD_REG_GREEN_SELECT, (8 << 8) | 8);

  // Disable the controller
  MmioWrite32 (HDLCD_REG_COMMAND, HDLCD_DISABLE);

  // Update the frame buffer information with the new settings
  MmioWrite32 (
    HDLCD_REG_FB_LINE_LENGTH,
    Horizontal->Resolution * BYTES_PER_PIXEL
    );

  MmioWrite32 (
    HDLCD_REG_FB_LINE_PITCH,
    Horizontal->Resolution * BYTES_PER_PIXEL
    );

  MmioWrite32 (HDLCD_REG_FB_LINE_COUNT, Vertical->Resolution - 1);

  // Set the vertical timing information
  MmioWrite32 (HDLCD_REG_V_SYNC,        Vertical->Sync);
  MmioWrite32 (HDLCD_REG_V_BACK_PORCH,  Vertical->BackPorch);
  MmioWrite32 (HDLCD_REG_V_DATA,        Vertical->Resolution - 1);
  MmioWrite32 (HDLCD_REG_V_FRONT_PORCH, Vertical->FrontPorch);

  // Set the horizontal timing information
  MmioWrite32 (HDLCD_REG_H_SYNC,        Horizontal->Sync);
  MmioWrite32 (HDLCD_REG_H_BACK_PORCH,  Horizontal->BackPorch);
  MmioWrite32 (HDLCD_REG_H_DATA,        Horizontal->Resolution - 1);
  MmioWrite32 (HDLCD_REG_H_FRONT_PORCH, Horizontal->FrontPorch);

  // Enable the controller
  MmioWrite32 (HDLCD_REG_COMMAND, HDLCD_ENABLE);

  return EFI_SUCCESS;
}

/** De-initializes the display.
**/
VOID
LcdShutdown (
  VOID
  )
{
  // Disable the controller
  MmioWrite32 (HDLCD_REG_COMMAND, HDLCD_DISABLE);
}

/** Check for presence of HDLCD.

  @retval EFI_SUCCESS            Returns success if platform implements a HDLCD
                                 controller.
  @retval EFI_NOT_FOUND          HDLCD display controller not found on the
                                 platform.
**/
EFI_STATUS
LcdIdentify (
  VOID
  )
{
  if ((MmioRead32 (HDLCD_REG_VERSION) >> 16) == HDLCD_PRODUCT_ID) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}
