/** @file
Boot Logo 2 Protocol is used to convey information of Logo dispayed during boot.

The Boot Logo 2 Protocol is a replacement for the Boot Logo Protocol.  If a
platform produces both the Boot Logo 2 Protocol and the Boot Logo Protocol
then the Boot Logo 2 Protocol must be used instead of the Boot Logo Protocol.

Copyright (c) 2016, Microsoft Corporation
Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_LOGO2_H_
#define _BOOT_LOGO2_H_

#include <Protocol/GraphicsOutput.h>

#define EDKII_BOOT_LOGO2_PROTOCOL_GUID \
  { \
    0x4b5dc1df, 0x1eaa, 0x48b2, { 0xa7, 0xe9, 0xea, 0xc4, 0x89, 0xa0, 0xb, 0x5c } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EDKII_BOOT_LOGO2_PROTOCOL EDKII_BOOT_LOGO2_PROTOCOL;

/**
  Update information of logo image drawn on screen.

  @param[in] This          The pointer to the Boot Logo protocol 2 instance.
  @param[in] BltBuffer     The BLT buffer for logo drawn on screen. If BltBuffer
                           is set to NULL, it indicates that logo image is no
                           longer on the screen.
  @param[in] DestinationX  X coordinate of destination for the BltBuffer.
  @param[in] DestinationY  Y coordinate of destination for the BltBuffer.
  @param[in] Width         Width of rectangle in BltBuffer in pixels.
  @param[in] Height        Hight of rectangle in BltBuffer in pixels.

  @retval EFI_SUCCESS            The boot logo information was updated.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.
  @retval EFI_OUT_OF_RESOURCES   The logo information was not updated due to
                                 insufficient memory resources.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_SET_BOOT_LOGO2)(
  IN EDKII_BOOT_LOGO2_PROTOCOL      *This,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer       OPTIONAL,
  IN UINTN                          DestinationX,
  IN UINTN                          DestinationY,
  IN UINTN                          Width,
  IN UINTN                          Height
  );

/**
  Get the location of the boot logo on the screen.

  @param[in]  This          The pointer to the Boot Logo Protocol 2 instance
  @param[out] BltBuffer     Returns pointer to the GOP BLT buffer that was
                            previously registered with SetBootLogo2(). The
                            buffer returned must not be modified or freed.
  @param[out] DestinationX  Returns the X start position of the GOP BLT buffer
                            that was previously registered with SetBootLogo2().
  @param[out] DestinationY  Returns the Y start position of the GOP BLT buffer
                            that was previously registered with SetBootLogo2().
  @param[out] Width         Returns the width of the GOP BLT buffer
                            that was previously registered with SetBootLogo2().
  @param[out] Height        Returns the height of the GOP BLT buffer
                            that was previously registered with SetBootLogo2().

  @retval EFI_SUCCESS            The location of the boot logo was returned.
  @retval EFI_NOT_READY          The boot logo has not been set.
  @retval EFI_INVALID_PARAMETER  BltBuffer is NULL.
  @retval EFI_INVALID_PARAMETER  DestinationX is NULL.
  @retval EFI_INVALID_PARAMETER  DestinationY is NULL.
  @retval EFI_INVALID_PARAMETER  Width is NULL.
  @retval EFI_INVALID_PARAMETER  Height is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_GET_BOOT_LOGO2)(
  IN  EDKII_BOOT_LOGO2_PROTOCOL      *This,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **BltBuffer,
  OUT UINTN                          *DestinationX,
  OUT UINTN                          *DestinationY,
  OUT UINTN                          *Width,
  OUT UINTN                          *Height
  );

struct _EDKII_BOOT_LOGO2_PROTOCOL {
  EDKII_SET_BOOT_LOGO2    SetBootLogo;
  EDKII_GET_BOOT_LOGO2    GetBootLogo;
};

extern EFI_GUID  gEdkiiBootLogo2ProtocolGuid;

#endif
