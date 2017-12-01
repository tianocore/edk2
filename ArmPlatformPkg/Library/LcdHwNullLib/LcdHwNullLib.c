/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Library/DebugLib.h>
#include <Library/LcdPlatformLib.h>

/**
  Check for presence of display
  
  @retval EFI_SUCCESS            Platform implements display.
  @retval EFI_NOT_FOUND          Display not found on the platform.

**/
EFI_STATUS
LcdIdentify (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Initialize display.

  @param  FrameBaseAddress       Address of the frame buffer.
  @retval EFI_SUCCESS            Display initialization success.
  @retval !(EFI_SUCCESS)         Display initialization failure.

**/
EFI_STATUS
LcdInitialize (
  EFI_PHYSICAL_ADDRESS  FrameBaseAddress
  )
{
  return EFI_SUCCESS;
}

/**
  Set requested mode of the display.

  @param  ModeNumber             Display mode number.
  @retval EFI_SUCCESS            Display set mode success.
  @retval EFI_DEVICE_ERROR       If mode not found/supported.

**/
EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  )
{
  return EFI_SUCCESS;
}

/**
  De-initializes the display.
**/
VOID
LcdShutdown (
  VOID
  )
{
}
