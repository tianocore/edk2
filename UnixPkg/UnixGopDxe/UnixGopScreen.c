/*++

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    UnixGopScreen.c

Abstract:

  This file produces the graphics abstration of UGA. It is called by
  UnixGopDriver.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

--*/

#include "UnixGop.h"

EFI_UNIX_THUNK_PROTOCOL *mUnix;
EFI_EVENT               mGopScreenExitBootServicesEvent;

GOP_MODE_DATA mGopModeData[] = {
    { 800,  600, 0, 0 },
    { 640,  480, 0, 0 },
    { 720,  400, 0, 0 },
    {1024,  768, 0, 0 },
    {1280, 1024, 0, 0 }
    };


EFI_STATUS
UnixGopStartWindow (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  );

VOID
EFIAPI
KillUgaGopThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// UGA Protocol Member Functions
//

EFI_STATUS
EFIAPI
UnixGopQuerytMode (
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



EFI_STATUS
EFIAPI
UnixGopSetMode (
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
    Status = UnixGopStartWindow (
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
  
  
  Status = Private->UgaIo->UgaSize(
                            Private->UgaIo,
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



EFI_STATUS
EFIAPI
UnixGopBlt (
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
  UGA_BLT_ARGS      GopBltArgs;

  Private = GOP_PRIVATE_DATA_FROM_THIS (This);

  if ((BltOperation < 0) || (BltOperation >= EfiGraphicsOutputBltOperationMax)) {
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
  // Pack UGA Draw protocol parameters to UGA_BLT_ARGS structure to adapt to
  // GopBlt() API of Unix UGA IO protocol.
  //
  GopBltArgs.DestinationX = DestinationX;
  GopBltArgs.DestinationY = DestinationY;
  GopBltArgs.Height       = Height;
  GopBltArgs.Width        = Width;
  GopBltArgs.SourceX      = SourceX;
  GopBltArgs.SourceY      = SourceY;
  GopBltArgs.Delta        = Delta;
  Status = Private->UgaIo->UgaBlt (
                            Private->UgaIo,
                            (EFI_UGA_PIXEL *)BltBuffer,
                            BltOperation,
                            &GopBltArgs
                            );

  gBS->RestoreTPL (OriginalTPL);

  return Status;
}


//
// Construction and Destruction functions
//

EFI_STATUS
UnixGopSupported (
  IN  EFI_UNIX_IO_PROTOCOL  *UnixIo
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    UnixIo - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  //
  // Check to see if the IO abstraction represents a device type we support.
  //
  // This would be replaced a check of PCI subsystem ID, etc.
  //
  if (!CompareGuid (UnixIo->TypeGuid, &gEfiUnixGopGuid)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


VOID
EFIAPI
GopPrivateInvokeRegisteredFunction (
  IN VOID           *Context,
  IN EFI_KEY_DATA   *KeyData
  );


EFI_STATUS
UnixGopStartWindow (
  IN  GOP_PRIVATE_DATA    *Private,
  IN  UINT32              HorizontalResolution,
  IN  UINT32              VerticalResolution,
  IN  UINT32              ColorDepth,
  IN  UINT32              RefreshRate
  )
{
  EFI_STATUS          Status;

  mUnix  = Private->UnixThunk;

  //
  // Register to be notified on exit boot services so we can destroy the window.
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  KillUgaGopThread,
                  Private,
                  &mGopScreenExitBootServicesEvent
                  );

  Status = Private->UnixThunk->UgaCreate (&Private->UgaIo, Private->WindowName);
  if (!EFI_ERROR (Status)) {
    // Register callback to support RegisterKeyNotify()
    Status  = Private->UgaIo->UgaRegisterKeyNotify (Private->UgaIo, GopPrivateInvokeRegisteredFunction, Private);
    ASSERT_EFI_ERROR (Status);
  }
  return Status;
}

EFI_STATUS
UnixGopConstructor (
  GOP_PRIVATE_DATA    *Private
  )
{
  Private->ModeData = mGopModeData;

  Private->GraphicsOutput.QueryMode      = UnixGopQuerytMode;
  Private->GraphicsOutput.SetMode        = UnixGopSetMode;
  Private->GraphicsOutput.Blt            = UnixGopBlt;

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
  Private->UgaIo                  = NULL;

  UnixGopInitializeSimpleTextInForWindow (Private);

  UnixGopInitializeSimplePointerForWindow (Private);

  return EFI_SUCCESS;
}



EFI_STATUS
UnixGopDestructor (
  GOP_PRIVATE_DATA     *Private
  )
{
  if (!Private->HardwareNeedsStarting) {
    Private->UgaIo->UgaClose (Private->UgaIo);
    Private->UgaIo = NULL;
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
KillUgaGopThread (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
/*++

Routine Description:

  This is the UGA screen's callback notification function for exit-boot-services.
  All we do here is call UnixGopDestructor().

Arguments:

  Event   - not used
  Context - pointer to the Private structure.

Returns:

  None.

--*/
{
  EFI_STATUS  Status;
  Status = UnixGopDestructor (Context);
}
