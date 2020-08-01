/*++ @file

Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
Copyright (c) 2015, Nahanni Systems, Inc.
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

    EmuGopScreen.c

Abstract:

  This file produces the graphics abstration of UGA. It is called by
  EmuGopDriver.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

**/

#include "Gop.h"
#include <Library/FrameBufferBltLib.h>


EFI_EVENT               mGopScreenExitBootServicesEvent;

GOP_MODE_DATA mGopModeData[] = {
    { 0,     0,    32, 0 },    // Filled in with user-spec'd resolution
    { 1024,  768,  32, 0 },
    { 800,   600,  32, 0 },
    { 640,   480,  32, 0 }
    };

STATIC
VOID
BhyveGopCompleteModeInfo (
  IN  GOP_MODE_DATA  *ModeData,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  Info->Version = 0;
  if (ModeData->ColorDepth == 8) {
    Info->PixelFormat = PixelBitMask;
    Info->PixelInformation.RedMask = PIXEL_RED_MASK;
    Info->PixelInformation.GreenMask = PIXEL_GREEN_MASK;
    Info->PixelInformation.BlueMask = PIXEL_BLUE_MASK;
    Info->PixelInformation.ReservedMask = 0;
  } else if (ModeData->ColorDepth == 24) {
    Info->PixelFormat = PixelBitMask;
    Info->PixelInformation.RedMask = PIXEL24_RED_MASK;
    Info->PixelInformation.GreenMask = PIXEL24_GREEN_MASK;
    Info->PixelInformation.BlueMask = PIXEL24_BLUE_MASK;
    Info->PixelInformation.ReservedMask = 0;
  } else if (ModeData->ColorDepth == 32) {
    DEBUG ((DEBUG_INFO, "%dx%d PixelBlueGreenRedReserved8BitPerColor\n",
     ModeData->HorizontalResolution, ModeData->VerticalResolution));
    Info->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  }
  Info->PixelsPerScanLine = Info->HorizontalResolution;
}


/**
  Returns information for an available graphics mode that the graphics device
  and the set of active video output devices supports.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            The mode number to return information on.
  @param  SizeOfInfo            A pointer to the size, in bytes, of the Info buffer.
  @param  Info                  A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL  The Info buffer was too small.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_NOT_STARTED       Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
EmuGopQuerytMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  GOP_PRIVATE_DATA  *Private;
  GOP_MODE_DATA     *ModeData;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (Info == NULL || SizeOfInfo == NULL || (UINTN) ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  ModeData = &Private->ModeData[ModeNumber];
  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = ModeData->HorizontalResolution;
  (*Info)->VerticalResolution   = ModeData->VerticalResolution;
  (*Info)->PixelFormat = PixelBitMask;
  (*Info)->PixelsPerScanLine = (*Info)->HorizontalResolution;
  BhyveGopCompleteModeInfo(ModeData, *Info);
  return EFI_SUCCESS;
}



/**
  Set the video device into the specified mode and clears the visible portions of
  the output display to black.

  @param  This              The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber        Abstraction that defines the current video mode.

  @retval EFI_SUCCESS       The graphics mode specified by ModeNumber was selected.
  @retval EFI_DEVICE_ERROR  The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED   ModeNumber is not supported by this device.

**/

FRAME_BUFFER_CONFIGURE *fbconf;

EFI_STATUS
EFIAPI
EmuGopSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  GOP_PRIVATE_DATA                *Private;
  GOP_MODE_DATA                   *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   Fill;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  UINTN confsize = 0;
  fbconf = NULL;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber >= This->Mode->MaxMode) {
    // Tell bhyve that we are switching out of vesa
    BhyveSetGraphicsMode(Private, 0, 0, 0);
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "BHYVE GopSetMode %d\n", ModeNumber));

  ModeData = &Private->ModeData[ModeNumber];
  This->Mode->Mode = ModeNumber;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  Private->GraphicsOutput.Mode->Info->PixelsPerScanLine = ModeData->HorizontalResolution;

  Info = This->Mode->Info;
  BhyveGopCompleteModeInfo(ModeData, Info);

  This->Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  This->Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  This->Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  This->Mode->FrameBufferBase = Private->GraphicsOutput.Mode->FrameBufferBase;

  /*
  This->Mode->FrameBufferSize = Info->HorizontalResolution * Info->VerticalResolution
                                * ((ModeData->ColorDepth + 7) / 8);
  */
  This->Mode->FrameBufferSize = Private->FbSize;
  DEBUG ((DEBUG_INFO, "BHYVE GOP FrameBufferBase: 0x%x, FrameBufferSize: 0x%x\n", This->Mode->FrameBufferBase, This->Mode->FrameBufferSize));

  BhyveSetGraphicsMode(Private, (UINT16)ModeData->HorizontalResolution, (UINT16)ModeData->VerticalResolution, (UINT16)ModeData->ColorDepth);

  RETURN_STATUS ret = FrameBufferBltConfigure (
    (VOID*)(UINTN) This->Mode->FrameBufferBase,
    This->Mode->Info, fbconf, &confsize
    );
  if (ret == EFI_BUFFER_TOO_SMALL || ret == EFI_INVALID_PARAMETER) {
          fbconf = AllocatePool(confsize);
          ret = FrameBufferBltConfigure(
                         (VOID*)(UINTN)This->Mode->FrameBufferBase,
                         This->Mode->Info, fbconf, &confsize);
          ASSERT(ret == EFI_SUCCESS);
  }

  Fill.Red   = 0;
  Fill.Green = 0;
  Fill.Blue  = 0;
  This->Blt (
          This,
          &Fill,
          EfiBltVideoFill,
          0,
          0,
          0,
          0,
          ModeData->HorizontalResolution,
          ModeData->VerticalResolution,
          ModeData->HorizontalResolution * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
          );
  return EFI_SUCCESS;
}



/**
  Blt a rectangle of pixels on the graphics screen. Blt stands for BLock Transfer.

  @param  This         Protocol instance pointer.
  @param  BltBuffer    Buffer containing data to blit into video buffer. This
                       buffer has a size of Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
  @param  BltOperation Operation to perform on BlitBuffer and video memory
  @param  SourceX      X coordinate of source for the BltBuffer.
  @param  SourceY      Y coordinate of source for the BltBuffer.
  @param  DestinationX X coordinate of destination for the BltBuffer.
  @param  DestinationY Y coordinate of destination for the BltBuffer.
  @param  Width        Width of rectangle in BltBuffer in pixels.
  @param  Height       Hight of rectangle in BltBuffer in pixels.
  @param  Delta        OPTIONAL

  @retval EFI_SUCCESS           The Blt operation completed.
  @retval EFI_INVALID_PARAMETER BltOperation is not valid.
  @retval EFI_DEVICE_ERROR      A hardware error occurred writting to the video buffer.

**/
EFI_STATUS
EFIAPI
EmuGopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,   OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  )
{
  EFI_TPL           OriginalTPL;
  EFI_STATUS        Status;

  if ((UINT32)BltOperation >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
  case EfiBltBufferToVideo:
  case EfiBltVideoFill:
  case EfiBltVideoToVideo:
    Status = FrameBufferBlt (
     fbconf,
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
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    ASSERT (FALSE);
  }

  gBS->RestoreTPL (OriginalTPL);

  return Status;
}


//
// Construction and Destruction functions
//

EFI_STATUS
EmuGopConstructor (
  GOP_PRIVATE_DATA    *Private
  )
{
  // Set mode 0 to be the requested resolution
  mGopModeData[0].HorizontalResolution = PcdGet32 ( PcdVideoHorizontalResolution);
  mGopModeData[0].VerticalResolution = PcdGet32 ( PcdVideoVerticalResolution );

  Private->ModeData = mGopModeData;

  Private->GraphicsOutput.QueryMode      = EmuGopQuerytMode;
  Private->GraphicsOutput.SetMode        = EmuGopSetMode;
  Private->GraphicsOutput.Blt            = EmuGopBlt;

  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  Private->GraphicsOutput.Mode = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  if (Private->GraphicsOutput.Mode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Private->GraphicsOutput.Mode->Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (Private->GraphicsOutput.Mode->Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }


  DEBUG ((DEBUG_INFO, "BHYVE Gop Constructor\n"));

  Private->GraphicsOutput.Mode->MaxMode = sizeof(mGopModeData) / sizeof(GOP_MODE_DATA);
  //
  // Till now, we have no idea about the window size.
  //
  Private->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALID_MODE_NUMBER;
  Private->GraphicsOutput.Mode->Info->Version = 0;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = 0;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = 0;
  Private->GraphicsOutput.Mode->Info->PixelFormat = PixelBitMask;
  Private->GraphicsOutput.Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  Private->GraphicsOutput.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS) Private->FbAddr;
  Private->GraphicsOutput.Mode->FrameBufferSize = Private->FbSize;

  return EFI_SUCCESS;
}



EFI_STATUS
EmuGopDestructor (
  GOP_PRIVATE_DATA     *Private
  )
{
  //
  // Free graphics output protocol occupied resource
  //
  if (Private->GraphicsOutput.Mode != NULL) {
    if (Private->GraphicsOutput.Mode->Info != NULL) {
      FreePool (Private->GraphicsOutput.Mode->Info);
    }
    FreePool (Private->GraphicsOutput.Mode);
    Private->GraphicsOutput.Mode = NULL;
  }

  return EFI_SUCCESS;
}


VOID
EFIAPI
ShutdownGopEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
/*++

Routine Description:

  This is the UGA screen's callback notification function for exit-boot-services.
  All we do here is call EmuGopDestructor().

Arguments:

  Event   - not used
  Context - pointer to the Private structure.

Returns:

  None.

**/
{
  EmuGopDestructor (Context);
}

