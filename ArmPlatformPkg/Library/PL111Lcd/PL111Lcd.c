/** @file
  This file contains the platform independent parts of PL111Lcd

  Copyright (c) 2011-2018, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/LcdPlatformLib.h>
#include <Library/MemoryAllocationLib.h>

#include "PL111Lcd.h"

/** Check for presence of PL111.

  @retval EFI_SUCCESS          Returns success if platform implements a
                               PL111 controller.

  @retval EFI_NOT_FOUND        PL111 display controller not found the platform.
**/
EFI_STATUS
LcdIdentify (
  VOID
  )
{
  DEBUG ((
    DEBUG_WARN,
    "Probing ID registers at 0x%lx for a PL111\n",
    PL111_REG_CLCD_PERIPH_ID_0
    ));

  // Check if this is a PL111
  if ((MmioRead8 (PL111_REG_CLCD_PERIPH_ID_0) == PL111_CLCD_PERIPH_ID_0) &&
      (MmioRead8 (PL111_REG_CLCD_PERIPH_ID_1) == PL111_CLCD_PERIPH_ID_1) &&
      ((MmioRead8 (PL111_REG_CLCD_PERIPH_ID_2) & 0xf) == PL111_CLCD_PERIPH_ID_2) &&
      (MmioRead8 (PL111_REG_CLCD_PERIPH_ID_3) == PL111_CLCD_PERIPH_ID_3) &&
      (MmioRead8 (PL111_REG_CLCD_P_CELL_ID_0) == PL111_CLCD_P_CELL_ID_0) &&
      (MmioRead8 (PL111_REG_CLCD_P_CELL_ID_1) == PL111_CLCD_P_CELL_ID_1) &&
      (MmioRead8 (PL111_REG_CLCD_P_CELL_ID_2) == PL111_CLCD_P_CELL_ID_2) &&
      (MmioRead8 (PL111_REG_CLCD_P_CELL_ID_3) == PL111_CLCD_P_CELL_ID_3))
  {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/** Initialize display.

  @param[in]  VramBaseAddress    Address of the framebuffer.

  @retval EFI_SUCCESS            Initialization of display successful.
**/
EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS  VramBaseAddress
  )
{
  // Define start of the VRAM. This never changes for any graphics mode
  MmioWrite32 (PL111_REG_LCD_UP_BASE, (UINT32)VramBaseAddress);
  MmioWrite32 (PL111_REG_LCD_LP_BASE, 0); // We are not using a double buffer

  // Disable all interrupts from the PL111
  MmioWrite32 (PL111_REG_LCD_IMSC, 0);

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
  EFI_STATUS    Status;
  SCAN_TIMINGS  *Horizontal;
  SCAN_TIMINGS  *Vertical;
  UINT32        LcdControl;
  LCD_BPP       LcdBpp;

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

  Status = LcdPlatformGetBpp (ModeNumber, &LcdBpp);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Get the pixel format information
  Status = LcdPlatformQueryMode (ModeNumber, &ModeInfo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Disable the CLCD_LcdEn bit
  MmioAnd32 (PL111_REG_LCD_CONTROL, ~PL111_CTRL_LCD_EN);

  // Set Timings
  MmioWrite32 (
    PL111_REG_LCD_TIMING_0,
    HOR_AXIS_PANEL (
      Horizontal->BackPorch,
      Horizontal->FrontPorch,
      Horizontal->Sync,
      Horizontal->Resolution
      )
    );

  MmioWrite32 (
    PL111_REG_LCD_TIMING_1,
    VER_AXIS_PANEL (
      Vertical->BackPorch,
      Vertical->FrontPorch,
      Vertical->Sync,
      Vertical->Resolution
      )
    );

  MmioWrite32 (
    PL111_REG_LCD_TIMING_2,
    CLK_SIG_POLARITY (Horizontal->Resolution)
    );

  MmioWrite32 (PL111_REG_LCD_TIMING_3, 0);

  // PL111_REG_LCD_CONTROL
  LcdControl = PL111_CTRL_LCD_EN | PL111_CTRL_LCD_BPP (LcdBpp) |
               PL111_CTRL_LCD_TFT | PL111_CTRL_LCD_PWR;
  if (ModeInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    LcdControl |= PL111_CTRL_BGR;
  }

  MmioWrite32 (PL111_REG_LCD_CONTROL, LcdControl);

  return EFI_SUCCESS;
}

/** De-initializes the display.
*/
VOID
LcdShutdown (
  VOID
  )
{
  // Disable the controller
  MmioAnd32 (PL111_REG_LCD_CONTROL, ~PL111_CTRL_LCD_EN);
}
