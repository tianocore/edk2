/** @file LcdHwLib.h

  This file contains interface functions for LcdHwLib of ArmPlatformPkg

  Copyright (c) 2017, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LCD_HW_LIB_H_
#define LCD_HW_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Check for presence of display

  @retval EFI_SUCCESS            Platform implements display.
  @retval EFI_NOT_FOUND          Display not found on the platform.

**/
EFI_STATUS
LcdIdentify (
  VOID
  );

/**
  Initialize display.

  @param  FrameBaseAddress       Address of the frame buffer.
  @retval EFI_SUCCESS            Display initialization success.
  @retval !(EFI_SUCCESS)         Display initialization failure.

**/
EFI_STATUS
LcdInitialize (
  EFI_PHYSICAL_ADDRESS  FrameBaseAddress
  );

/**
  Set requested mode of the display.

  @param  ModeNumber             Display mode number.
  @retval EFI_SUCCESS            Display set mode success.
  @retval EFI_DEVICE_ERROR       If mode not found/supported.

**/
EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  );

/**
  De-initializes the display.
**/
VOID
LcdShutdown (
  VOID
  );

#endif /* LCD_HW_LIB_H_ */
