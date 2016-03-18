/** @file
  This library is only intended to be used by PlatformBootManagerLib
  to show progress bar and LOGO.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOT_LOGO_LIB_H_
#define _BOOT_LOGO_LIB_H_

#include <Protocol/PlatformLogo.h>

/**
  Show LOGO on all consoles.

  @param[in]  ImageFormat Format of the image file.
  @param[in]  LogoFile    The file name of logo to display.
  @param[in]  Attribute   The display attributes of the image returned.
  @param[in]  OffsetX     The X offset of the image regarding the Attribute.
  @param[in]  OffsetY     The Y offset of the image regarding the Attribute.

  @retval EFI_SUCCESS     Logo was displayed.
  @retval EFI_UNSUPPORTED Logo was not found or cannot be displayed.
**/
EFI_STATUS
EFIAPI
BootLogoEnableLogo (
  IN  IMAGE_FORMAT                          ImageFormat,
  IN  EFI_GUID                              *Logo,
  IN  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE Attribute,
  IN  INTN                                  OffsetX,
  IN  INTN                                  OffsetY
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
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
  );

#endif
