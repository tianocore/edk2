/** @file
Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiCirrusLogic5430GraphicsOutput.c

Abstract:

  This file produces the graphics abstration of Graphics Output Protocol. It is called by
  CirrusLogic5430.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

**/
#include "CirrusLogic5430.h"
#include <IndustryStandard/Acpi.h>


STATIC
VOID
CirrusLogic5430CompleteModeInfo (
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  Info->Version = 0;
  Info->PixelFormat = PixelBitMask;
  Info->PixelInformation.RedMask = PIXEL_RED_MASK;
  Info->PixelInformation.GreenMask = PIXEL_GREEN_MASK;
  Info->PixelInformation.BlueMask = PIXEL_BLUE_MASK;
  Info->PixelInformation.ReservedMask = 0;
  Info->PixelsPerScanLine = Info->HorizontalResolution;
}


STATIC
EFI_STATUS
CirrusLogic5430CompleteModeData (
  IN  CIRRUS_LOGIC_5430_PRIVATE_DATA    *Private,
  OUT EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     *FrameBufDesc;

  Info = Mode->Info;
  CirrusLogic5430CompleteModeInfo (Info);

  Private->PciIo->GetBarAttributes (
                        Private->PciIo,
                        0,
                        NULL,
                        (VOID**) &FrameBufDesc
                        );

  Mode->FrameBufferBase = FrameBufDesc->AddrRangeMin;
  Mode->FrameBufferSize = Info->HorizontalResolution * Info->VerticalResolution;

  return EFI_SUCCESS;
}


//
// Graphics Output Protocol Member Functions
//
EFI_STATUS
EFIAPI
CirrusLogic5430GraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
/*++

Routine Description:

  Graphics Output protocol interface to query video mode

  Arguments:
    This                  - Protocol instance pointer.
    ModeNumber            - The mode number to return information on.
    Info                  - Caller allocated buffer that returns information about ModeNumber.
    SizeOfInfo            - A pointer to the size, in bytes, of the Info buffer.

  Returns:
    EFI_SUCCESS           - Mode information returned.
    EFI_BUFFER_TOO_SMALL  - The Info buffer was too small.
    EFI_DEVICE_ERROR      - A hardware error occurred trying to retrieve the video mode.
    EFI_NOT_STARTED       - Video display is not initialized. Call SetMode ()
    EFI_INVALID_PARAMETER - One of the input args was NULL.

--*/
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS (This);

  if (Private->HardwareNeedsStarting) {
    return EFI_NOT_STARTED;
  }

  if (Info == NULL || SizeOfInfo == NULL || ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  *Info = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  if (*Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  (*Info)->HorizontalResolution = Private->ModeData[ModeNumber].HorizontalResolution;
  (*Info)->VerticalResolution   = Private->ModeData[ModeNumber].VerticalResolution;
  CirrusLogic5430CompleteModeInfo (*Info);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CirrusLogic5430GraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
  )
/*++

Routine Description:

  Graphics Output protocol interface to set video mode

  Arguments:
    This             - Protocol instance pointer.
    ModeNumber       - The mode number to be set.

  Returns:
    EFI_SUCCESS      - Graphics mode was changed.
    EFI_DEVICE_ERROR - The device had an error and could not complete the request.
    EFI_UNSUPPORTED  - ModeNumber is not supported by this device.

--*/
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA    *Private;
  CIRRUS_LOGIC_5430_MODE_DATA       *ModeData;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS (This);

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  ModeData = &Private->ModeData[ModeNumber];

  if (Private->LineBuffer) {
    gBS->FreePool (Private->LineBuffer);
  }

  Private->LineBuffer = NULL;
  Private->LineBuffer = AllocatePool (ModeData->HorizontalResolution);
  if (Private->LineBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeGraphicsMode (Private, &CirrusLogic5430VideoModes[ModeData->ModeNumber]);

  This->Mode->Mode = ModeNumber;
  This->Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  This->Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  This->Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  CirrusLogic5430CompleteModeData (Private, This->Mode);

  Private->HardwareNeedsStarting  = FALSE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CirrusLogic5430GraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
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
/*++

Routine Description:

  Graphics Output protocol instance to block transfer for CirrusLogic device

Arguments:

  This          - Pointer to Graphics Output protocol instance
  BltBuffer     - The data to transfer to screen
  BltOperation  - The operation to perform
  SourceX       - The X coordinate of the source for BltOperation
  SourceY       - The Y coordinate of the source for BltOperation
  DestinationX  - The X coordinate of the destination for BltOperation
  DestinationY  - The Y coordinate of the destination for BltOperation
  Width         - The width of a rectangle in the blt rectangle in pixels
  Height        - The height of a rectangle in the blt rectangle in pixels
  Delta         - Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
                  If a Delta of 0 is used, the entire BltBuffer will be operated on.
                  If a subrectangle of the BltBuffer is used, then Delta represents
                  the number of bytes in a row of the BltBuffer.

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in
  EFI_SUCCESS - Blt operation success

--*/
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;
  EFI_TPL                         OriginalTPL;
  UINTN                           DstY;
  UINTN                           SrcY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Blt;
  UINTN                           X;
  UINT8                           Pixel;
  UINT32                          WidePixel;
  UINTN                           ScreenWidth;
  UINTN                           Offset;
  UINTN                           SourceOffset;
  UINT32                          CurrentMode;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_GRAPHICS_OUTPUT_THIS (This);

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
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //

  CurrentMode = This->Mode->Mode;
  //
  // Make sure the SourceX, SourceY, DestinationX, DestinationY, Width, and Height parameters
  // are valid for the operation and the current screen geometry.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > Private->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Private->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Private->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Private->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {

      Offset = (SrcY * Private->ModeData[CurrentMode].HorizontalResolution) + SourceX;
      if (((Offset & 0x03) == 0) && ((Width & 0x03) == 0)) {
        Private->PciIo->Mem.Read (
                              Private->PciIo,
                              EfiPciIoWidthUint32,
                              0,
                              Offset,
                              Width >> 2,
                              Private->LineBuffer
                              );
      } else {
        Private->PciIo->Mem.Read (
                              Private->PciIo,
                              EfiPciIoWidthUint8,
                              0,
                              Offset,
                              Width,
                              Private->LineBuffer
                              );
      }

      for (X = 0; X < Width; X++) {
        Blt         = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + (DestinationX + X) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

        Blt->Red    = PIXEL_TO_RED_BYTE (Private->LineBuffer[X]);
        Blt->Green  = PIXEL_TO_GREEN_BYTE (Private->LineBuffer[X]);
        Blt->Blue   = PIXEL_TO_BLUE_BYTE (Private->LineBuffer[X]);
      }
    }
    break;

  case EfiBltVideoToVideo:
    //
    // Perform hardware acceleration for Video to Video operations
    //
    ScreenWidth   = Private->ModeData[CurrentMode].HorizontalResolution;
    SourceOffset  = (SourceY * Private->ModeData[CurrentMode].HorizontalResolution) + (SourceX);
    Offset        = (DestinationY * Private->ModeData[CurrentMode].HorizontalResolution) + (DestinationX);

    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0000);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0010);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0012);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0014);

    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0001);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0011);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0013);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0015);

    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) (((Width << 8) & 0xff00) | 0x20));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((Width & 0xff00) | 0x21));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) (((Height << 8) & 0xff00) | 0x22));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((Height & 0xff00) | 0x23));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) (((ScreenWidth << 8) & 0xff00) | 0x24));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((ScreenWidth & 0xff00) | 0x25));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) (((ScreenWidth << 8) & 0xff00) | 0x26));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((ScreenWidth & 0xff00) | 0x27));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((Offset) << 8) & 0xff00) | 0x28));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((Offset) >> 0) & 0xff00) | 0x29));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((Offset) >> 8) & 0xff00) | 0x2a));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((SourceOffset) << 8) & 0xff00) | 0x2c));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((SourceOffset) >> 0) & 0xff00) | 0x2d));
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((((SourceOffset) >> 8) & 0xff00) | 0x2e));
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x002f);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0030);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0d32);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0033);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0034);
    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0035);

    outw (Private, GRAPH_ADDRESS_REGISTER, 0x0231);

    outb (Private, GRAPH_ADDRESS_REGISTER, 0x31);
    while ((inb (Private, GRAPH_DATA_REGISTER) & 0x01) == 0x01)
      ;
    break;

  case EfiBltVideoFill:
    Blt       = BltBuffer;
    Pixel     = RGB_BYTES_TO_PIXEL (Blt->Red, Blt->Green, Blt->Blue);
    WidePixel = (Pixel << 8) | Pixel;
    WidePixel = (WidePixel << 16) | WidePixel;

    if (DestinationX == 0 && Width == Private->ModeData[CurrentMode].HorizontalResolution) {
      Offset = DestinationY * Private->ModeData[CurrentMode].HorizontalResolution;
      if (((Offset & 0x03) == 0) && (((Width * Height) & 0x03) == 0)) {
        Private->PciIo->Mem.Write (
                              Private->PciIo,
                              EfiPciIoWidthFillUint32,
                              0,
                              Offset,
                              (Width * Height) >> 2,
                              &WidePixel
                              );
      } else {
        Private->PciIo->Mem.Write (
                              Private->PciIo,
                              EfiPciIoWidthFillUint8,
                              0,
                              Offset,
                              Width * Height,
                              &Pixel
                              );
      }
    } else {
      for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {
        Offset = (DstY * Private->ModeData[CurrentMode].HorizontalResolution) + DestinationX;
        if (((Offset & 0x03) == 0) && ((Width & 0x03) == 0)) {
          Private->PciIo->Mem.Write (
                                Private->PciIo,
                                EfiPciIoWidthFillUint32,
                                0,
                                Offset,
                                Width >> 2,
                                &WidePixel
                                );
        } else {
          Private->PciIo->Mem.Write (
                                Private->PciIo,
                                EfiPciIoWidthFillUint8,
                                0,
                                Offset,
                                Width,
                                &Pixel
                                );
        }
      }
    }
    break;

  case EfiBltBufferToVideo:
    for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {

      for (X = 0; X < Width; X++) {
        Blt =
          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (
              (UINT8 *) BltBuffer +
              (SrcY * Delta) +
              ((SourceX + X) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))
            );
        Private->LineBuffer[X]  =
          RGB_BYTES_TO_PIXEL (Blt->Red, Blt->Green, Blt->Blue);
      }

      Offset = (DstY * Private->ModeData[CurrentMode].HorizontalResolution) + DestinationX;

      if (((Offset & 0x03) == 0) && ((Width & 0x03) == 0)) {
        Private->PciIo->Mem.Write (
                              Private->PciIo,
                              EfiPciIoWidthUint32,
                              0,
                              Offset,
                              Width >> 2,
                              Private->LineBuffer
                              );
      } else {
        Private->PciIo->Mem.Write (
                              Private->PciIo,
                              EfiPciIoWidthUint8,
                              0,
                              Offset,
                              Width,
                              Private->LineBuffer
                              );
      }
    }
    break;
  default:
    ASSERT (FALSE);
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

EFI_STATUS
CirrusLogic5430GraphicsOutputConstructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS                   Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;


  GraphicsOutput            = &Private->GraphicsOutput;
  GraphicsOutput->QueryMode = CirrusLogic5430GraphicsOutputQueryMode;
  GraphicsOutput->SetMode   = CirrusLogic5430GraphicsOutputSetMode;
  GraphicsOutput->Blt       = CirrusLogic5430GraphicsOutputBlt;

  //
  // Initialize the private data
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
                  (VOID **) &Private->GraphicsOutput.Mode
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                  (VOID **) &Private->GraphicsOutput.Mode->Info
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Private->GraphicsOutput.Mode->MaxMode = (UINT32) Private->MaxMode;
  Private->GraphicsOutput.Mode->Mode    = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;
  Private->HardwareNeedsStarting        = TRUE;
  Private->LineBuffer                   = NULL;

  //
  // Initialize the hardware
  //
  GraphicsOutput->SetMode (GraphicsOutput, 0);
  ASSERT (Private->GraphicsOutput.Mode->Mode < CIRRUS_LOGIC_5430_MODE_COUNT);
  DrawLogo (
    Private,
    Private->ModeData[Private->GraphicsOutput.Mode->Mode].HorizontalResolution,
    Private->ModeData[Private->GraphicsOutput.Mode->Mode].VerticalResolution
    );

  return EFI_SUCCESS;
}

EFI_STATUS
CirrusLogic5430GraphicsOutputDestructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
{
  if (Private->GraphicsOutput.Mode != NULL) {
    if (Private->GraphicsOutput.Mode->Info != NULL) {
      gBS->FreePool (Private->GraphicsOutput.Mode->Info);
    }
    gBS->FreePool (Private->GraphicsOutput.Mode);
  }

  return EFI_SUCCESS;
}


