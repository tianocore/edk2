/** @file  Lcd.c

  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "HdLcd.h"

/**********************************************************************
 *
 *  This file contains all the bits of the Lcd that are
 *  platform independent.
 *
 **********************************************************************/

STATIC
UINTN
GetBytesPerPixel (
  IN  LCD_BPP       Bpp
  )
{
  switch(Bpp) {
  case LCD_BITS_PER_PIXEL_24:
    return 4;

  case LCD_BITS_PER_PIXEL_16_565:
  case LCD_BITS_PER_PIXEL_16_555:
  case LCD_BITS_PER_PIXEL_12_444:
    return 2;

  case LCD_BITS_PER_PIXEL_8:
  case LCD_BITS_PER_PIXEL_4:
  case LCD_BITS_PER_PIXEL_2:
  case LCD_BITS_PER_PIXEL_1:
    return 1;

  default:
    return 0;
  }
}

EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS   VramBaseAddress
  )
{
  // Disable the controller
  MmioWrite32(HDLCD_REG_COMMAND, HDLCD_DISABLE);

  // Disable all interrupts
  MmioWrite32(HDLCD_REG_INT_MASK, 0);

  // Define start of the VRAM. This never changes for any graphics mode
  MmioWrite32(HDLCD_REG_FB_BASE, (UINT32) VramBaseAddress);

  // Setup various registers that never change
  MmioWrite32(HDLCD_REG_BUS_OPTIONS,  (4 << 8) | HDLCD_BURST_8);
  MmioWrite32(HDLCD_REG_POLARITIES,   HDLCD_PXCLK_LOW | HDLCD_DATA_HIGH | HDLCD_DATEN_HIGH | HDLCD_HSYNC_LOW | HDLCD_VSYNC_HIGH);
  MmioWrite32(HDLCD_REG_PIXEL_FORMAT, HDLCD_LITTLE_ENDIAN | HDLCD_4BYTES_PER_PIXEL);
  MmioWrite32(HDLCD_REG_RED_SELECT,   (0 << 16 | 8 << 8 |  0));
  MmioWrite32(HDLCD_REG_GREEN_SELECT, (0 << 16 | 8 << 8 |  8));
  MmioWrite32(HDLCD_REG_BLUE_SELECT,  (0 << 16 | 8 << 8 | 16));

  return EFI_SUCCESS;
}

EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  )
{
  EFI_STATUS        Status;
  UINT32            HRes;
  UINT32            HSync;
  UINT32            HBackPorch;
  UINT32            HFrontPorch;
  UINT32            VRes;
  UINT32            VSync;
  UINT32            VBackPorch;
  UINT32            VFrontPorch;
  UINT32            BytesPerPixel;
  LCD_BPP           LcdBpp;


  // Set the video mode timings and other relevant information
  Status = LcdPlatformGetTimings (ModeNumber,
                                  &HRes,&HSync,&HBackPorch,&HFrontPorch,
                                  &VRes,&VSync,&VBackPorch,&VFrontPorch);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR( Status )) {
    return EFI_DEVICE_ERROR;
  }

  Status = LcdPlatformGetBpp (ModeNumber,&LcdBpp);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR( Status )) {
    return EFI_DEVICE_ERROR;
  }

  BytesPerPixel = GetBytesPerPixel(LcdBpp);

  // Disable the controller
  MmioWrite32(HDLCD_REG_COMMAND, HDLCD_DISABLE);

  // Update the frame buffer information with the new settings
  MmioWrite32(HDLCD_REG_FB_LINE_LENGTH, HRes * BytesPerPixel);
  MmioWrite32(HDLCD_REG_FB_LINE_PITCH,  HRes * BytesPerPixel);
  MmioWrite32(HDLCD_REG_FB_LINE_COUNT,  VRes - 1);

  // Set the vertical timing information
  MmioWrite32(HDLCD_REG_V_SYNC,         VSync);
  MmioWrite32(HDLCD_REG_V_BACK_PORCH,   VBackPorch);
  MmioWrite32(HDLCD_REG_V_DATA,         VRes - 1);
  MmioWrite32(HDLCD_REG_V_FRONT_PORCH,  VFrontPorch);

  // Set the horizontal timing information
  MmioWrite32(HDLCD_REG_H_SYNC,         HSync);
  MmioWrite32(HDLCD_REG_H_BACK_PORCH,   HBackPorch);
  MmioWrite32(HDLCD_REG_H_DATA,         HRes - 1);
  MmioWrite32(HDLCD_REG_H_FRONT_PORCH,  HFrontPorch);

  // Enable the controller
  MmioWrite32(HDLCD_REG_COMMAND, HDLCD_ENABLE);

  return EFI_SUCCESS;
}

VOID
LcdShutdown (
  VOID
  )
{
  // Disable the controller
  MmioWrite32 (HDLCD_REG_COMMAND, HDLCD_DISABLE);
}

EFI_STATUS
LcdIdentify (
  VOID
  )
{
  return EFI_SUCCESS;
}
