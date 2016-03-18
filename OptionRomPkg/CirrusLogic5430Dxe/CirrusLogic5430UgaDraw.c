/** @file
  This file produces the graphics abstration of UGA Draw. It is called by
  CirrusLogic5430.c file which deals with the EFI 1.1 driver model.
  This file just does graphics.

  Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CirrusLogic5430.h"

//
// UGA Draw Protocol Member Functions
//
EFI_STATUS
EFIAPI
CirrusLogic5430UgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  OUT UINT32                *HorizontalResolution,
  OUT UINT32                *VerticalResolution,
  OUT UINT32                *ColorDepth,
  OUT UINT32                *RefreshRate
  )
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_UGA_DRAW_THIS (This);

  if (Private->HardwareNeedsStarting) {
    return EFI_NOT_STARTED;
  }

  if ((HorizontalResolution == NULL) ||
      (VerticalResolution == NULL)   ||
      (ColorDepth == NULL)           ||
      (RefreshRate == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *HorizontalResolution = Private->ModeData[Private->CurrentMode].HorizontalResolution;
  *VerticalResolution   = Private->ModeData[Private->CurrentMode].VerticalResolution;
  *ColorDepth           = Private->ModeData[Private->CurrentMode].ColorDepth;
  *RefreshRate          = Private->ModeData[Private->CurrentMode].RefreshRate;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CirrusLogic5430UgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL *This,
  IN  UINT32                HorizontalResolution,
  IN  UINT32                VerticalResolution,
  IN  UINT32                ColorDepth,
  IN  UINT32                RefreshRate
  )
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;
  UINTN                           Index;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_UGA_DRAW_THIS (This);

  for (Index = 0; Index < Private->MaxMode; Index++) {

    if (HorizontalResolution != Private->ModeData[Index].HorizontalResolution) {
      continue;
    }

    if (VerticalResolution != Private->ModeData[Index].VerticalResolution) {
      continue;
    }

    if (ColorDepth != Private->ModeData[Index].ColorDepth) {
      continue;
    }

    if (RefreshRate != Private->ModeData[Index].RefreshRate) {
      continue;
    }

    if (Private->LineBuffer) {
      gBS->FreePool (Private->LineBuffer);
    }

    Private->LineBuffer = NULL;
    Private->LineBuffer = AllocatePool (HorizontalResolution);
    if (Private->LineBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    InitializeGraphicsMode (Private, &CirrusLogic5430VideoModes[Private->ModeData[Index].ModeNumber]);

    Private->CurrentMode            = Index;

    Private->HardwareNeedsStarting  = FALSE;

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
CirrusLogic5430UgaDrawBlt (
  IN  EFI_UGA_DRAW_PROTOCOL     *This,
  IN  EFI_UGA_PIXEL             *BltBuffer, OPTIONAL
  IN  EFI_UGA_BLT_OPERATION     BltOperation,
  IN  UINTN                     SourceX,
  IN  UINTN                     SourceY,
  IN  UINTN                     DestinationX,
  IN  UINTN                     DestinationY,
  IN  UINTN                     Width,
  IN  UINTN                     Height,
  IN  UINTN                     Delta
  )
{
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private;
  EFI_TPL                         OriginalTPL;
  UINTN                           DstY;
  UINTN                           SrcY;
  EFI_UGA_PIXEL                   *Blt;
  UINTN                           X;
  UINT8                           Pixel;
  UINT32                          WidePixel;
  UINTN                           ScreenWidth;
  UINTN                           Offset;
  UINTN                           SourceOffset;

  Private = CIRRUS_LOGIC_5430_PRIVATE_DATA_FROM_UGA_DRAW_THIS (This);

  if ((UINT32)BltOperation >= EfiUgaBltMax) {
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
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //

  //
  // Make sure the SourceX, SourceY, DestinationX, DestinationY, Width, and Height parameters
  // are valid for the operation and the current screen geometry.
  //
  if (BltOperation == EfiUgaVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > Private->ModeData[Private->CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Private->ModeData[Private->CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Private->ModeData[Private->CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Private->ModeData[Private->CurrentMode].HorizontalResolution) {
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
  case EfiUgaVideoToBltBuffer:
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {

      Offset = (SrcY * Private->ModeData[Private->CurrentMode].HorizontalResolution) + SourceX;
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
        Blt         = (EFI_UGA_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + (DestinationX + X) * sizeof (EFI_UGA_PIXEL));

        Blt->Red    = (UINT8) (Private->LineBuffer[X] & 0xe0);
        Blt->Green  = (UINT8) ((Private->LineBuffer[X] & 0x1c) << 3);
        Blt->Blue   = (UINT8) ((Private->LineBuffer[X] & 0x03) << 6);
      }
    }
    break;

  case EfiUgaVideoToVideo:
    //
    // Perform hardware acceleration for Video to Video operations
    //
    ScreenWidth   = Private->ModeData[Private->CurrentMode].HorizontalResolution;
    SourceOffset  = (SourceY * Private->ModeData[Private->CurrentMode].HorizontalResolution) + (SourceX);
    Offset        = (DestinationY * Private->ModeData[Private->CurrentMode].HorizontalResolution) + (DestinationX);

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

  case EfiUgaVideoFill:
    Blt       = BltBuffer;
    Pixel     = (UINT8) ((Blt->Red & 0xe0) | ((Blt->Green >> 3) & 0x1c) | ((Blt->Blue >> 6) & 0x03));
    WidePixel = (Pixel << 8) | Pixel;
    WidePixel = (WidePixel << 16) | WidePixel;

    if (DestinationX == 0 && Width == Private->ModeData[Private->CurrentMode].HorizontalResolution) {
      Offset = DestinationY * Private->ModeData[Private->CurrentMode].HorizontalResolution;
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
        Offset = (DstY * Private->ModeData[Private->CurrentMode].HorizontalResolution) + DestinationX;
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

  case EfiUgaBltBufferToVideo:
    for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {

      for (X = 0; X < Width; X++) {
        Blt                     = (EFI_UGA_PIXEL *) ((UINT8 *) BltBuffer + (SrcY * Delta) + (SourceX + X) * sizeof (EFI_UGA_PIXEL));
        Private->LineBuffer[X]  = (UINT8) ((Blt->Red & 0xe0) | ((Blt->Green >> 3) & 0x1c) | ((Blt->Blue >> 6) & 0x03));
      }

      Offset = (DstY * Private->ModeData[Private->CurrentMode].HorizontalResolution) + DestinationX;

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
    break;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

//
// Construction and Destruction functions
//
EFI_STATUS
CirrusLogic5430UgaDrawConstructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  EFI_UGA_DRAW_PROTOCOL *UgaDraw;

  //
  // Fill in Private->UgaDraw protocol
  //
  UgaDraw           = &Private->UgaDraw;

  UgaDraw->GetMode  = CirrusLogic5430UgaDrawGetMode;
  UgaDraw->SetMode  = CirrusLogic5430UgaDrawSetMode;
  UgaDraw->Blt      = CirrusLogic5430UgaDrawBlt;

  //
  // Initialize the private data
  //
  Private->CurrentMode            = 0;
  Private->HardwareNeedsStarting  = TRUE;
  Private->LineBuffer             = NULL;

  //
  // Initialize the hardware
  //
  UgaDraw->SetMode (
            UgaDraw,
            Private->ModeData[Private->CurrentMode].HorizontalResolution,
            Private->ModeData[Private->CurrentMode].VerticalResolution,
            Private->ModeData[Private->CurrentMode].ColorDepth,
            Private->ModeData[Private->CurrentMode].RefreshRate
            );
  DrawLogo (
    Private,
    Private->ModeData[Private->CurrentMode].HorizontalResolution,
    Private->ModeData[Private->CurrentMode].VerticalResolution
    );

  return EFI_SUCCESS;
}

