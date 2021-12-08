/** @file
  Library for performing UEFI GOP Blt operations on a framebuffer

  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FRAMEBUFFER_BLT_LIB__
#define __FRAMEBUFFER_BLT_LIB__

#include <Protocol/GraphicsOutput.h>

//
// Opaque structure for the frame buffer configure.
//
typedef struct FRAME_BUFFER_CONFIGURE FRAME_BUFFER_CONFIGURE;

/**
  Create the configuration for a video frame buffer.

  The configuration is returned in the caller provided buffer.

  @param[in] FrameBuffer       Pointer to the start of the frame buffer.
  @param[in] FrameBufferInfo   Describes the frame buffer characteristics.
  @param[in,out] Configure     The created configuration information.
  @param[in,out] ConfigureSize Size of the configuration information.

  @retval RETURN_SUCCESS            The configuration was successful created.
  @retval RETURN_BUFFER_TOO_SMALL   The Configure is to too small. The required
                                    size is returned in ConfigureSize.
  @retval RETURN_UNSUPPORTED        The requested mode is not supported by
                                    this implementaion.
**/
RETURN_STATUS
EFIAPI
FrameBufferBltConfigure (
  IN      VOID                                  *FrameBuffer,
  IN      EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *FrameBufferInfo,
  IN OUT  FRAME_BUFFER_CONFIGURE                *Configure,
  IN OUT  UINTN                                 *ConfigureSize
  );

/**
  Performs a UEFI Graphics Output Protocol Blt operation.

  @param[in]     Configure    Pointer to a configuration which was successfully
                              created by FrameBufferBltConfigure ().
  @param[in,out] BltBuffer    The data to transfer to screen.
  @param[in]     BltOperation The operation to perform.
  @param[in]     SourceX      The X coordinate of the source for BltOperation.
  @param[in]     SourceY      The Y coordinate of the source for BltOperation.
  @param[in]     DestinationX The X coordinate of the destination for
                              BltOperation.
  @param[in]     DestinationY The Y coordinate of the destination for
                              BltOperation.
  @param[in]     Width        The width of a rectangle in the blt rectangle
                              in pixels.
  @param[in]     Height       The height of a rectangle in the blt rectangle
                              in pixels.
  @param[in]     Delta        Not used for EfiBltVideoFill and
                              EfiBltVideoToVideo operation. If a Delta of 0
                              is used, the entire BltBuffer will be operated
                              on. If a subrectangle of the BltBuffer is
                              used, then Delta represents the number of
                              bytes in a row of the BltBuffer.

  @retval RETURN_INVALID_PARAMETER Invalid parameter were passed in.
  @retval RETURN_SUCCESS           The Blt operation was performed successfully.
**/
RETURN_STATUS
EFIAPI
FrameBufferBlt (
  IN     FRAME_BUFFER_CONFIGURE             *Configure,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN     UINTN                              SourceX,
  IN     UINTN                              SourceY,
  IN     UINTN                              DestinationX,
  IN     UINTN                              DestinationY,
  IN     UINTN                              Width,
  IN     UINTN                              Height,
  IN     UINTN                              Delta
  );

#endif
