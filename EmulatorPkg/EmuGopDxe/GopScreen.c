/*++ @file

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010 - 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    EmuGopScreen.c

Abstract:

  This file produces the graphics abstration of UGA. It is called by
  EmuGopDriver.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

**/

#include "Gop.h"


EFI_EVENT               mGopScreenExitBootServicesEvent;

GOP_MODE_DATA mGopModeData[] = {
    { 800,  600, 0, 0 },
    { 640,  480, 0, 0 },
    { 720,  400, 0, 0 },
    {1024,  768, 0, 0 },
    {1280, 1024, 0, 0 }
    };


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

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (Info == NULL || SizeOfInfo == NULL || (UINTN) ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = Private->ModeData[ModeNumber].HorizontalResolution;
  (*Info)->VerticalResolution   = Private->ModeData[ModeNumber].VerticalResolution;
  (*Info)->PixelFormat = PixelBltOnly;
  (*Info)->PixelsPerScanLine = (*Info)->HorizontalResolution;

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
EFI_STATUS
EFIAPI
EmuGopSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  EFI_STATUS                      Status;
  GOP_PRIVATE_DATA                *Private;
  GOP_MODE_DATA                   *ModeData;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   Fill;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  ModeData = &Private->ModeData[ModeNumber];
  This->Mode->Mode = ModeNumber;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  Private->GraphicsOutput.Mode->Info->PixelsPerScanLine = ModeData->HorizontalResolution;

  if (Private->HardwareNeedsStarting) {
    Status = EmuGopStartWindow (
              Private,
              ModeData->HorizontalResolution,
              ModeData->VerticalResolution,
              ModeData->ColorDepth,
              ModeData->RefreshRate
              );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Private->HardwareNeedsStarting = FALSE;
  }


  Status = Private->EmuGraphicsWindow->Size(
                            Private->EmuGraphicsWindow,
                            ModeData->HorizontalResolution,
                            ModeData->VerticalResolution
                            );


  Fill.Red                      = 0x7f;
  Fill.Green                    = 0x7F;
  Fill.Blue                     = 0x7f;
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
  @retval EFI_DEVICE_ERROR      A hardware error occured writting to the video buffer.

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
  GOP_PRIVATE_DATA  *Private;
  EFI_TPL           OriginalTPL;
  EFI_STATUS        Status;
  EMU_GRAPHICS_WINDOWS__BLT_ARGS      GopBltArgs;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if ((UINT32)BltOperation >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_UGA_PIXEL);
  }

  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Pack UGA Draw protocol parameters to EMU_GRAPHICS_WINDOWS__BLT_ARGS structure to adapt to
  // GopBlt() API of Unix UGA IO protocol.
  //
  GopBltArgs.DestinationX = DestinationX;
  GopBltArgs.DestinationY = DestinationY;
  GopBltArgs.Height       = Height;
  GopBltArgs.Width        = Width;
  GopBltArgs.SourceX      = SourceX;
  GopBltArgs.SourceY      = SourceY;
  GopBltArgs.Delta        = Delta;
  Status = Private->EmuGraphicsWindow->Blt (
                            Private->EmuGraphicsWindow,
                            (EFI_UGA_PIXEL *)BltBuffer,
                            (EFI_UGA_BLT_OPERATION)BltOperation,
                            &GopBltArgs
                            );

  gBS->RestoreTPL (OriginalTPL);

  return Status;
}


//
// Construction and Destruction functions
//

EFI_STATUS
EmuGopSupported (
  IN  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk
  )
{
  //
  // Check to see if the IO abstraction represents a device type we support.
  //
  // This would be replaced a check of PCI subsystem ID, etc.
  //
  if (!CompareGuid (EmuIoThunk->Protocol, &gEmuGraphicsWindowProtocolGuid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EmuGopStartWindow (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  )
{
  EFI_STATUS          Status;

  //
  // Register to be notified on exit boot services so we can destroy the window.
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  ShutdownGopEvent,
                  Private,
                  &mGopScreenExitBootServicesEvent
                  );

  Status = Private->EmuIoThunk->Open (Private->EmuIoThunk);
  if (!EFI_ERROR (Status)) {
    Private->EmuGraphicsWindow = Private->EmuIoThunk->Interface;

    // Register callback to support RegisterKeyNotify()
    Status  = Private->EmuGraphicsWindow->RegisterKeyNotify (
                                            Private->EmuGraphicsWindow,
                                            GopPrivateMakeCallbackFunction,
                                            GopPrivateBreakCallbackFunction,
                                            Private
                                            );
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}

EFI_STATUS
EmuGopConstructor (
  GOP_PRIVATE_DATA    *Private
  )
{
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

  Private->GraphicsOutput.Mode->MaxMode = sizeof(mGopModeData) / sizeof(GOP_MODE_DATA);
  //
  // Till now, we have no idea about the window size.
  //
  Private->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;
  Private->GraphicsOutput.Mode->Info->Version = 0;
  Private->GraphicsOutput.Mode->Info->HorizontalResolution = 0;
  Private->GraphicsOutput.Mode->Info->VerticalResolution = 0;
  Private->GraphicsOutput.Mode->Info->PixelFormat = PixelBltOnly;
  Private->GraphicsOutput.Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  Private->GraphicsOutput.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS) (UINTN) NULL;
  Private->GraphicsOutput.Mode->FrameBufferSize = 0;

  Private->HardwareNeedsStarting  = TRUE;
  Private->EmuGraphicsWindow                  = NULL;

  EmuGopInitializeSimpleTextInForWindow (Private);

  EmuGopInitializeSimplePointerForWindow (Private);

  return EFI_SUCCESS;
}



EFI_STATUS
EmuGopDestructor (
  GOP_PRIVATE_DATA     *Private
  )
{
  if (!Private->HardwareNeedsStarting) {
    Private->EmuIoThunk->Close (Private->EmuIoThunk);
    Private->EmuGraphicsWindow = NULL;
  }

  //
  // Free graphics output protocol occupied resource
  //
  if (Private->GraphicsOutput.Mode != NULL) {
    if (Private->GraphicsOutput.Mode->Info != NULL) {
      FreePool (Private->GraphicsOutput.Mode->Info);
    }
    FreePool (Private->GraphicsOutput.Mode);
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

