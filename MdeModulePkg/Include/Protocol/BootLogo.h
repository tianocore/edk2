/** @file
  Boot Logo protocol is used to convey information of Logo dispayed during boot.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_LOGO_H_
#define _BOOT_LOGO_H_

#include <Protocol/GraphicsOutput.h>

#define EFI_BOOT_LOGO_PROTOCOL_GUID \
  { \
    0xcdea2bd3, 0xfc25, 0x4c1c, { 0xb9, 0x7c, 0xb3, 0x11, 0x86, 0x6, 0x49, 0x90 } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_BOOT_LOGO_PROTOCOL EFI_BOOT_LOGO_PROTOCOL;

/**
  Update information of logo image drawn on screen.

  @param  This           The pointer to the Boot Logo protocol instance.
  @param  BltBuffer      The BLT buffer for logo drawn on screen. If BltBuffer
                         is set to NULL, it indicates that logo image is no
                         longer on the screen.
  @param  DestinationX   X coordinate of destination for the BltBuffer.
  @param  DestinationY   Y coordinate of destination for the BltBuffer.
  @param  Width          Width of rectangle in BltBuffer in pixels.
  @param  Height         Hight of rectangle in BltBuffer in pixels.

  @retval EFI_SUCCESS             The boot logo information was updated.
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_OUT_OF_RESOURCES    The logo information was not updated due to
                                  insufficient memory resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_SET_BOOT_LOGO)(
  IN EFI_BOOT_LOGO_PROTOCOL            *This,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     *BltBuffer       OPTIONAL,
  IN UINTN                             DestinationX,
  IN UINTN                             DestinationY,
  IN UINTN                             Width,
  IN UINTN                             Height
  );

struct _EFI_BOOT_LOGO_PROTOCOL {
  EFI_SET_BOOT_LOGO    SetBootLogo;
};

extern EFI_GUID  gEfiBootLogoProtocolGuid;

#endif
