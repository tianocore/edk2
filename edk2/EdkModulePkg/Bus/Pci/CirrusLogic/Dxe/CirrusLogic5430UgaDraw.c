/** @file
  This file produces the graphics abstration of UGA Draw. It is called by 
  CirrusLogic5430.c file which deals with the EFI 1.1 driver model. 
  This file just does graphics.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "CirrusLogic5430.h"

STATIC
VOID
ClearScreen (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  );

///
/// Generic Attribute Controller Register Settings
///
static UINT8                          AttributeController[21] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
  0x41, 0x00, 0x0F, 0x00, 0x00
};

///
/// Generic Graphics Controller Register Settings
///
static UINT8 GraphicsController[9] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
};

//
// 640 x 480 x 256 color @ 60 Hertz
//
static UINT8                          Crtc_640_480_256_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xe1, 0x83, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x22
};

static UINT16                         Seq_640_480_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b, 
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

//
// 800 x 600 x 256 color @ 60 Hertz
//
static UINT8                          Crtc_800_600_256_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0, 
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x58, 0x8C, 0x57, 0x64, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x22
};

static UINT16                         Seq_800_600_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b, 
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

//
// 1024 x 768 x 256 color @ 60 Hertz
//
static UINT8                          Crtc_1024_768_256_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD, 
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x22
};

static UINT16                         Seq_1024_768_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b, 
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

///
/// Table of supported video modes
///
static CIRRUS_LOGIC_5430_VIDEO_MODES  CirrusLogic5430VideoModes[] = {
  {  640, 480, 8, 60, Crtc_640_480_256_60,  Seq_640_480_256_60,  0xe3 },
  {  800, 600, 8, 60, Crtc_800_600_256_60,  Seq_800_600_256_60,  0xef }, 
  { 1024, 768, 8, 60, Crtc_1024_768_256_60, Seq_1024_768_256_60, 0xef } 
};

//
// UGA Draw Protocol Member Functions
//
/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  HorizontalResolution TODO: add argument description
  @param  VerticalResolution TODO: add argument description
  @param  ColorDepth TODO: add argument description
  @param  RefreshRate TODO: add argument description

  @retval  EFI_NOT_STARTED TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_SUCCESS TODO: Add description for return value

**/
STATIC
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

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  HorizontalResolution TODO: add argument description
  @param  VerticalResolution TODO: add argument description
  @param  ColorDepth TODO: add argument description
  @param  RefreshRate TODO: add argument description

  @retval  EFI_OUT_OF_RESOURCES TODO: Add description for return value
  @retval  EFI_SUCCESS TODO: Add description for return value
  @retval  EFI_NOT_FOUND TODO: Add description for return value

**/
STATIC
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

    InitializeGraphicsMode (Private, &CirrusLogic5430VideoModes[Index]);

    Private->CurrentMode            = Index;

    Private->HardwareNeedsStarting  = FALSE;

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  BltBuffer TODO: add argument description
  @param  BltOperation TODO: add argument description
  @param  SourceX TODO: add argument description
  @param  SourceY TODO: add argument description
  @param  DestinationX TODO: add argument description
  @param  DestinationY TODO: add argument description
  @param  Width TODO: add argument description
  @param  Height TODO: add argument description
  @param  Delta TODO: add argument description

  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_INVALID_PARAMETER TODO: Add description for return value
  @retval  EFI_SUCCESS TODO: Add description for return value

**/
STATIC
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

  if ((BltOperation < 0) || (BltOperation >= EfiUgaBltMax)) {
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

/**
  CirrusLogic5430UgaDrawConstructor

  TODO:    Private - add argument and description to function comment
  TODO:    EFI_SUCCESS - add return value to function comment
**/
EFI_STATUS
CirrusLogic5430UgaDrawConstructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  EFI_UGA_DRAW_PROTOCOL *UgaDraw;
  UINTN                 Index;

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
  Private->MaxMode      = CIRRUS_LOGIC_5430_UGA_DRAW_MODE_COUNT;
  Private->CurrentMode  = 0;
  for (Index = 0; Index < Private->MaxMode; Index++) {
    Private->ModeData[Index].HorizontalResolution = CirrusLogic5430VideoModes[Index].Width;
    Private->ModeData[Index].VerticalResolution   = CirrusLogic5430VideoModes[Index].Height;
    Private->ModeData[Index].ColorDepth           = 32;
    Private->ModeData[Index].RefreshRate          = CirrusLogic5430VideoModes[Index].RefreshRate;
  }

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
  DrawLogo (Private);

  return EFI_SUCCESS;
}

/**
  CirrusLogic5430UgaDrawDestructor

  TODO:    Private - add argument and description to function comment
  TODO:    EFI_SUCCESS - add return value to function comment
**/
EFI_STATUS
CirrusLogic5430UgaDrawDestructor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  return EFI_SUCCESS;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
outb (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT8                           Data
  )
{
  Private->PciIo->Io.Write (
                      Private->PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      Address,
                      1,
                      &Data
                      );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
outw (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address,
  UINT16                          Data
  )
{
  Private->PciIo->Io.Write (
                      Private->PciIo,
                      EfiPciIoWidthUint16,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      Address,
                      1,
                      &Data
                      );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description

  TODO: add return values

**/
UINT8
inb (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
  )
{
  UINT8 Data;

  Private->PciIo->Io.Read (
                      Private->PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      Address,
                      1,
                      &Data
                      );
  return Data;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Address TODO: add argument description

  TODO: add return values

**/
UINT16
inw (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Address
  )
{
  UINT16  Data;

  Private->PciIo->Io.Read (
                      Private->PciIo,
                      EfiPciIoWidthUint16,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      Address,
                      1,
                      &Data
                      );
  return Data;
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  Index TODO: add argument description
  @param  Red TODO: add argument description
  @param  Green TODO: add argument description
  @param  Blue TODO: add argument description

  TODO: add return values

**/
VOID
SetPaletteColor (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  UINTN                           Index,
  UINT8                           Red,
  UINT8                           Green,
  UINT8                           Blue
  )
{
  outb (Private, PALETTE_INDEX_REGISTER, (UINT8) Index);
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Red >> 2));
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Green >> 2));
  outb (Private, PALETTE_DATA_REGISTER, (UINT8) (Blue >> 2));
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
SetDefaultPalette (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  UINTN Index;
  UINTN RedIndex;
  UINTN GreenIndex;
  UINTN BlueIndex;

  Index = 0;
  for (RedIndex = 0; RedIndex < 8; RedIndex++) {
    for (GreenIndex = 0; GreenIndex < 8; GreenIndex++) {
      for (BlueIndex = 0; BlueIndex < 4; BlueIndex++) {
        SetPaletteColor (Private, Index, (UINT8) (RedIndex << 5), (UINT8) (GreenIndex << 5), (UINT8) (BlueIndex << 6));
        Index++;
      }
    }
  }
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
STATIC
VOID
ClearScreen (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  UINT32  Color;

  Color = 0;
  Private->PciIo->Mem.Write (
                        Private->PciIo,
                        EfiPciIoWidthFillUint32,
                        0,
                        0,
                        0x100000 >> 2,
                        &Color
                        );
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description

  TODO: add return values

**/
VOID
DrawLogo (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private
  )
{
  UINTN Offset;
  UINTN X;
  UINTN Y;
  UINTN ScreenWidth;
  UINTN ScreenHeight;
  UINT8 Color;

  ScreenWidth   = Private->ModeData[Private->CurrentMode].HorizontalResolution;
  ScreenHeight  = Private->ModeData[Private->CurrentMode].VerticalResolution;

  Offset        = 0;
  for (Y = 0; Y < ScreenHeight; Y++) {
    for (X = 0; X < ScreenWidth; X++) {
      Color                   = (UINT8) (256 * (X + Y) / (ScreenWidth + ScreenHeight));
      Private->LineBuffer[X]  = Color;
    }

    Private->PciIo->Mem.Write (
                          Private->PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          Offset + (Y * ScreenWidth),
                          ScreenWidth >> 2,
                          Private->LineBuffer
                          );
  }
}

/**
  TODO: Add function description

  @param  Private TODO: add argument description
  @param  ModeData TODO: add argument description

  TODO: add return values

**/
VOID
InitializeGraphicsMode (
  CIRRUS_LOGIC_5430_PRIVATE_DATA  *Private,
  CIRRUS_LOGIC_5430_VIDEO_MODES   *ModeData
  )
{
  UINT8 Byte;
  UINTN Index;

  outw (Private, SEQ_ADDRESS_REGISTER, 0x1206);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0012);

  for (Index = 0; Index < 15; Index++) {
    outw (Private, SEQ_ADDRESS_REGISTER, ModeData->SeqSettings[Index]);
  }

  outb (Private, SEQ_ADDRESS_REGISTER, 0x0f);
  Byte = (UINT8) ((inb (Private, SEQ_DATA_REGISTER) & 0xc7) ^ 0x30);
  outb (Private, SEQ_DATA_REGISTER, Byte);

  outb (Private, MISC_OUTPUT_REGISTER, ModeData->MiscSetting);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x0506);
  outw (Private, SEQ_ADDRESS_REGISTER, 0x0300);
  outw (Private, CRTC_ADDRESS_REGISTER, 0x2011);

  for (Index = 0; Index < 28; Index++) {
    outw (Private, CRTC_ADDRESS_REGISTER, (UINT16) ((ModeData->CrtcSettings[Index] << 8) | Index));
  }

  for (Index = 0; Index < 9; Index++) {
    outw (Private, GRAPH_ADDRESS_REGISTER, (UINT16) ((GraphicsController[Index] << 8) | Index));
  }

  inb (Private, INPUT_STATUS_1_REGISTER);

  for (Index = 0; Index < 21; Index++) {
    outb (Private, ATT_ADDRESS_REGISTER, (UINT8) Index);
    outb (Private, ATT_ADDRESS_REGISTER, AttributeController[Index]);
  }

  outb (Private, ATT_ADDRESS_REGISTER, 0x20);

  outw (Private, GRAPH_ADDRESS_REGISTER, 0x0009);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x000a);
  outw (Private, GRAPH_ADDRESS_REGISTER, 0x000b);
  outb (Private, DAC_PIXEL_MASK_REGISTER, 0xff);

  SetDefaultPalette (Private);
  ClearScreen (Private);
}
