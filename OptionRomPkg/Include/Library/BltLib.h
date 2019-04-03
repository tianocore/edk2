/** @file
  Library for performing video blt operations

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BLT_LIB__
#define __BLT_LIB__

#include <Protocol/GraphicsOutput.h>


/**
  Configure the BltLib for a frame-buffer

  @param[in] FrameBuffer      Pointer to the start of the frame buffer
  @param[in] FrameBufferInfo  Describes the frame buffer characteristics

  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibConfigure (
  IN  VOID                                 *FrameBuffer,
  IN  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *FrameBufferInfo
  );


/**
  Performs a UEFI Graphics Output Protocol Blt operation.

  @param[in,out] BltBuffer     - The data to transfer to screen
  @param[in]     BltOperation  - The operation to perform
  @param[in]     SourceX       - The X coordinate of the source for BltOperation
  @param[in]     SourceY       - The Y coordinate of the source for BltOperation
  @param[in]     DestinationX  - The X coordinate of the destination for BltOperation
  @param[in]     DestinationY  - The Y coordinate of the destination for BltOperation
  @param[in]     Width         - The width of a rectangle in the blt rectangle in pixels
  @param[in]     Height        - The height of a rectangle in the blt rectangle in pixels
  @param[in]     Delta         - Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
                                 If a Delta of 0 is used, the entire BltBuffer will be operated on.
                                 If a subrectangle of the BltBuffer is used, then Delta represents
                                 the number of bytes in a row of the BltBuffer.

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibGopBlt (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
  IN     EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN     UINTN                              SourceX,
  IN     UINTN                              SourceY,
  IN     UINTN                              DestinationX,
  IN     UINTN                              DestinationY,
  IN     UINTN                              Width,
  IN     UINTN                              Height,
  IN     UINTN                              Delta
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Video Fill.

  @param[in]  Color         Color to fill the region with
  @param[in]  DestinationX  X location to start fill operation
  @param[in]  DestinationY  Y location to start fill operation
  @param[in]  Width         Width (in pixels) to fill
  @param[in]  Height        Height to fill

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibVideoFill (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *Color,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation.

  @param[out] BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibVideoToBltBuffer (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters.

  @param[out] BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  DestinationX  X location within BltBuffer
  @param[in]  DestinationY  Y location within BltBuffer
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height
  @param[in]  Delta         Number of bytes in a row of BltBuffer

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibVideoToBltBufferEx (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation.

  @param[in]  BltBuffer     Output buffer for pixel color data
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibBufferToVideo (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters.

  @param[in]  BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within BltBuffer
  @param[in]  SourceY       Y location within BltBuffer
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height
  @param[in]  Delta         Number of bytes in a row of BltBuffer

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibBufferToVideoEx (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  );


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Video operation

  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibVideoToVideo (
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  );


/**
  Returns the sizes related to the video device

  @param[out]  Width   Width (in pixels)
  @param[out]  Height  Height (in pixels)

  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibGetSizes (
  OUT UINTN                                 *Width,  OPTIONAL
  OUT UINTN                                 *Height  OPTIONAL
  );

#endif

