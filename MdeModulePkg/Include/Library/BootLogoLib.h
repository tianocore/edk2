/** @file
  This library is only intended to be used by PlatformBootManagerLib
  to show progress bar and LOGO.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_LOGO_LIB_H_
#define _BOOT_LOGO_LIB_H_

#include <Protocol/PlatformLogo.h>
#include <Protocol/GraphicsOutput.h>

/**
  Show LOGO returned from Edkii Platform Logo protocol on all consoles.
**/
EFI_STATUS
EFIAPI
BootLogoEnableLogo (
  VOID
  );

/**
  Use SystemTable ConOut to turn on video based Simple Text Out consoles. The
  Simple Text Out screens will now be synced up with all non-video output devices.

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
BootLogoDisableLogo (
  VOID
  );

/**

  Update progress bar with title above it. It only works in Graphics mode.

  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS    Successly update the progress bar

**/
EFI_STATUS
EFIAPI
BootLogoUpdateProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleBackground,
  IN CHAR16                         *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  ProgressColor,
  IN UINTN                          Progress,
  IN UINTN                          PreviousValue
  );

#endif
