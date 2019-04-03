/** @file
  GopBltLib - Library to perform blt using the UEFI Graphics Output Protocol.

  Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiDxe.h"

#include <Protocol/GraphicsOutput.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BltLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL         *mGop = NULL;


/**
  Configure the FrameBufferLib instance

  @param[in] FrameBuffer      Pointer to the start of the frame buffer
  @param[in] FrameBufferInfo  Describes the frame buffer characteristics

  @retval  EFI_INVALID_PARAMETER - Invalid parameter
  @retval  EFI_UNSUPPORTED - The BltLib does not support this configuration
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibConfigure (
  IN  VOID                                 *FrameBuffer,
  IN  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *FrameBufferInfo
  )
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           HandleCount;
  UINTN                           Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiGraphicsOutputProtocolGuid,
                      (VOID*) &Gop
                      );
      if (!EFI_ERROR (Status) &&
          (FrameBuffer == (VOID*)(UINTN) Gop->Mode->FrameBufferBase)) {
        mGop = Gop;
        FreePool (HandleBuffer);
        return EFI_SUCCESS;
      }
    }

    FreePool (HandleBuffer);
  }

  return EFI_UNSUPPORTED;
}


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
InternalGopBltCommon (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION     BltOperation,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  if (mGop == NULL) {
    return EFI_DEVICE_ERROR;
  }

  return mGop->Blt (
                 mGop,
                 BltBuffer,
                 BltOperation,
                 SourceX,
                 SourceY,
                 DestinationX,
                 DestinationY,
                 Width,
                 Height,
                 Delta
                 );
}


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
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION     BltOperation,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  return InternalGopBltCommon (
           BltBuffer,
           BltOperation,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta
           );
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video Fill.

  @param[in]  Color         Color to fill the region with
  @param[in]  DestinationX  X location to start fill operation
  @param[in]  DestinationY  Y location to start fill operation
  @param[in]  Width         Width (in pixels) to fill
  @param[in]  Height        Height to fill

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoFill (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *Color,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  return InternalGopBltCommon (
           Color,
           EfiBltVideoFill,
           0,
           0,
           DestinationX,
           DestinationY,
           Width,
           Height,
           0
           );
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation.

  @param[out] BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoToBltBuffer (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  return InternalGopBltCommon (
           BltBuffer,
           EfiBltVideoToBltBuffer,
           SourceX,
           SourceY,
           0,
           0,
           Width,
           Height,
           0
           );
}


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
  @retval  EFI_SUCCESS - The sizes were returned

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
  )
{
  return InternalGopBltCommon (
           BltBuffer,
           EfiBltVideoToBltBuffer,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta
           );
}


/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation.

  @param[in]  BltBuffer     Output buffer for pixel color data
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibBufferToVideo (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  return InternalGopBltCommon (
           BltBuffer,
           EfiBltBufferToVideo,
           0,
           0,
           DestinationX,
           DestinationY,
           Width,
           Height,
           0
           );
}


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
  @retval  EFI_SUCCESS - The sizes were returned

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
  )
{
  return InternalGopBltCommon (
           BltBuffer,
           EfiBltBufferToVideo,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           Delta
           );
}


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
  @retval  EFI_SUCCESS - The sizes were returned

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
  )
{
  return InternalGopBltCommon (
           NULL,
           EfiBltVideoToVideo,
           SourceX,
           SourceY,
           DestinationX,
           DestinationY,
           Width,
           Height,
           0
           );
}

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
  )
{
  ASSERT (mGop != NULL);

  if (Width != NULL) {
    *Width = mGop->Mode->Info->HorizontalResolution;
  }
  if (Height != NULL) {
    *Height = mGop->Mode->Info->VerticalResolution;
  }

  return EFI_SUCCESS;
}

