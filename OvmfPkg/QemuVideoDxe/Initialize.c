/** @file
  Graphics Output Protocol functions for the QEMU video controller.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Qemu.h"

///
/// Generic Attribute Controller Register Settings
///
UINT8  AttributeController[21] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x41, 0x00, 0x0F, 0x00, 0x00
};

///
/// Generic Graphics Controller Register Settings
///
UINT8  GraphicsController[9] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
};

//
// 640 x 480 x 256 color @ 60 Hertz
//
UINT8  Crtc_640_480_256_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0x83, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x22
};

UINT8  Crtc_640_480_32bpp_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0x83, 0xdf, 0x40, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x32
};

UINT16  Seq_640_480_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

UINT16  Seq_640_480_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

//
// 800 x 600 x 256 color @ 60 Hertz
//
UINT8  Crtc_800_600_256_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x58, 0x8C, 0x57, 0x64, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x22
};

UINT8  Crtc_800_600_32bpp_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x58, 0x8C, 0x57, 0x90, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x32
};

UINT16  Seq_800_600_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

UINT16  Seq_800_600_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

UINT8  Crtc_960_720_32bpp_60[28] = {
  0xA3, 0x77, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xCF, 0xe0, 0x00, 0x00, 0x64, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16  Seq_960_720_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

//
// 1024 x 768 x 256 color @ 60 Hertz
//
UINT8  Crtc_1024_768_256_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x22
};

UINT16  Seq_1024_768_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

//
// 1024 x 768 x 24-bit color @ 60 Hertz
//
UINT8  Crtc_1024_768_24bpp_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16  Seq_1024_768_24bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1507, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

UINT8  Crtc_1024_768_32bpp_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0xe0, 0x00, 0x00, 0x64, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16  Seq_1024_768_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

///
/// Table of supported video modes
///
QEMU_VIDEO_CIRRUS_MODES  QemuVideoCirrusModes[] = {
  //  {  640, 480, 8, Crtc_640_480_256_60,  Seq_640_480_256_60,  0xe3 },
  //  {  800, 600, 8, Crtc_800_600_256_60,  Seq_800_600_256_60,  0xef },
  { 640,  480, 32, Crtc_640_480_32bpp_60,  Seq_640_480_32bpp_60,  0xef },
  { 800,  600, 32, Crtc_800_600_32bpp_60,  Seq_800_600_32bpp_60,  0xef },
  //  { 1024, 768, 8, Crtc_1024_768_256_60, Seq_1024_768_256_60, 0xef }
  { 1024, 768, 24, Crtc_1024_768_24bpp_60, Seq_1024_768_24bpp_60, 0xef }
  //  { 1024, 768, 32, Crtc_1024_768_32bpp_60, Seq_1024_768_32bpp_60, 0xef }
  //  { 960, 720, 32, Crtc_960_720_32bpp_60, Seq_1024_768_32bpp_60, 0xef }
};

#define QEMU_VIDEO_CIRRUS_MODE_COUNT \
  (ARRAY_SIZE (QemuVideoCirrusModes))

/**
  Construct the valid video modes for QemuVideo.

**/
EFI_STATUS
QemuVideoCirrusModeSetup (
  QEMU_VIDEO_PRIVATE_DATA  *Private
  )
{
  UINT32                   Index;
  QEMU_VIDEO_MODE_DATA     *ModeData;
  QEMU_VIDEO_CIRRUS_MODES  *VideoMode;

  //
  // Setup Video Modes
  //
  Private->ModeData = AllocatePool (
                        sizeof (Private->ModeData[0]) * QEMU_VIDEO_CIRRUS_MODE_COUNT
                        );
  if (Private->ModeData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ModeData  = Private->ModeData;
  VideoMode = &QemuVideoCirrusModes[0];
  for (Index = 0; Index < QEMU_VIDEO_CIRRUS_MODE_COUNT; Index++) {
    ModeData->InternalModeIndex    = Index;
    ModeData->HorizontalResolution = VideoMode->Width;
    ModeData->VerticalResolution   = VideoMode->Height;
    ModeData->ColorDepth           = VideoMode->ColorDepth;
    DEBUG ((
      DEBUG_INFO,
      "Adding Mode %d as Cirrus Internal Mode %d: %dx%d, %d-bit\n",
      (INT32)(ModeData - Private->ModeData),
      ModeData->InternalModeIndex,
      ModeData->HorizontalResolution,
      ModeData->VerticalResolution,
      ModeData->ColorDepth
      ));

    ModeData++;
    VideoMode++;
  }

  Private->MaxMode = ModeData - Private->ModeData;

  return EFI_SUCCESS;
}

///
/// Table of supported video modes
///
STATIC QEMU_VIDEO_BOCHS_MODES  QemuVideoBochsModes[] = {
  { 640,  480  },
  { 800,  480  },
  { 800,  600  },
  { 832,  624  },
  { 960,  640  },
  { 1024, 600  },
  { 1024, 768  },
  { 1152, 864  },
  { 1152, 870  },
  { 1280, 720  },
  { 1280, 760  },
  { 1280, 768  },
  { 1280, 800  },
  { 1280, 960  },
  { 1280, 1024 },
  { 1360, 768  },
  { 1366, 768  },
  { 1400, 1050 },
  { 1440, 900  },
  { 1600, 900  },
  { 1600, 1200 },
  { 1680, 1050 },
  { 1920, 1080 },
  { 1920, 1200 },
  { 1920, 1440 },
  { 2000, 2000 },
  { 2048, 1536 },
  { 2048, 2048 },
  { 2560, 1440 },
  { 2560, 1600 },
  { 2560, 2048 },
  { 2800, 2100 },
  { 3200, 2400 },
  { 3840, 2160 },
  { 4096, 2160 },
  { 7680, 4320 },
  { 8192, 4320 }
};

#define QEMU_VIDEO_BOCHS_MODE_COUNT \
  (ARRAY_SIZE (QemuVideoBochsModes))

STATIC
VOID
QemuVideoBochsAddMode (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT32                   AvailableFbSize,
  UINT32                   Width,
  UINT32                   Height
  )
{
  QEMU_VIDEO_MODE_DATA  *ModeData = Private->ModeData + Private->MaxMode;
  UINTN                 RequiredFbSize;

  RequiredFbSize = (UINTN)Width * Height * 4;
  if (RequiredFbSize > AvailableFbSize) {
    DEBUG ((
      DEBUG_INFO,
      "Skipping Bochs Mode %dx%d, 32-bit (not enough vram)\n",
      Width,
      Height
      ));
    return;
  }

  ModeData->InternalModeIndex    = (UINT32)Private->MaxMode;
  ModeData->HorizontalResolution = Width;
  ModeData->VerticalResolution   = Height;
  ModeData->ColorDepth           = 32;
  DEBUG ((
    DEBUG_INFO,
    "Adding Bochs Internal Mode %d: %dx%d, %d-bit\n",
    ModeData->InternalModeIndex,
    ModeData->HorizontalResolution,
    ModeData->VerticalResolution,
    ModeData->ColorDepth
    ));

  Private->MaxMode++;
}

STATIC
VOID
QemuVideoBochsEdid (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  UINT32                   *XRes,
  UINT32                   *YRes
  )
{
  EFI_STATUS  Status;
  UINT32      X;
  UINT32      Y;

  if (Private->Variant != QEMU_VIDEO_BOCHS_MMIO) {
    return;
  }

  Status = Private->PciIo->Mem.Read (
                                 Private->PciIo,
                                 EfiPciIoWidthUint8,
                                 PCI_BAR_IDX2,
                                 0,
                                 sizeof (Private->Edid),
                                 Private->Edid
                                 );
  if (Status != EFI_SUCCESS) {
    DEBUG ((
      DEBUG_INFO,
      "%a: mmio read failed\n",
      __func__
      ));
    return;
  }

  if ((Private->Edid[0] != 0x00) ||
      (Private->Edid[1] != 0xff))
  {
    DEBUG ((
      DEBUG_INFO,
      "%a: magic check failed\n",
      __func__
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: blob found (extensions: %d)\n",
    __func__,
    Private->Edid[126]
    ));

  if ((Private->Edid[54] == 0x00) &&
      (Private->Edid[55] == 0x00))
  {
    DEBUG ((
      DEBUG_INFO,
      "%a: no detailed timing descriptor\n",
      __func__
      ));
    return;
  }

  X = Private->Edid[56] | ((Private->Edid[58] & 0xf0) << 4);
  Y = Private->Edid[59] | ((Private->Edid[61] & 0xf0) << 4);
  DEBUG ((
    DEBUG_INFO,
    "%a: default resolution: %dx%d\n",
    __func__,
    X,
    Y
    ));

  if ((X < 640) || (Y < 480)) {
    /* ignore hint, GraphicsConsoleDxe needs 640x480 or larger */
    return;
  }

  *XRes = X;
  *YRes = Y;

  if (PcdGet8 (PcdVideoResolutionSource) == 0) {
    Status = PcdSet32S (PcdVideoHorizontalResolution, *XRes);
    ASSERT_RETURN_ERROR (Status);
    Status = PcdSet32S (PcdVideoVerticalResolution, *YRes);
    ASSERT_RETURN_ERROR (Status);
    Status = PcdSet8S (PcdVideoResolutionSource, 2);
    ASSERT_RETURN_ERROR (Status);
  }

  // TODO: register edid as gEfiEdidDiscoveredProtocolGuid ?
}

EFI_STATUS
QemuVideoBochsModeSetup (
  QEMU_VIDEO_PRIVATE_DATA  *Private,
  BOOLEAN                  IsQxl
  )
{
  UINT32  AvailableFbSize;
  UINT32  Index, XRes = 0, YRes = 0;

  //
  // Fetch the available framebuffer size.
  //
  // VBE_DISPI_INDEX_VIDEO_MEMORY_64K is expected to return the size of the
  // drawable framebuffer. Up to and including qemu-2.1 however it used to
  // return the size of PCI BAR 0 (ie. the full video RAM size).
  //
  // On stdvga the two concepts coincide with each other; the full memory size
  // is usable for drawing.
  //
  // On QXL however, only a leading segment, "surface 0", can be used for
  // drawing; the rest of the video memory is used for the QXL guest-host
  // protocol. VBE_DISPI_INDEX_VIDEO_MEMORY_64K should report the size of
  // "surface 0", but since it doesn't (up to and including qemu-2.1), we
  // retrieve the size of the drawable portion from a field in the QXL ROM BAR,
  // where it is also available.
  //
  if (IsQxl) {
    UINT32  Signature;
    UINT32  DrawStart;

    Signature       = 0;
    DrawStart       = 0xFFFFFFFF;
    AvailableFbSize = 0;
    if (EFI_ERROR (
          Private->PciIo->Mem.Read (
                                Private->PciIo,
                                EfiPciIoWidthUint32,
                                PCI_BAR_IDX2,
                                0,
                                1,
                                &Signature
                                )
          ) ||
        (Signature != SIGNATURE_32 ('Q', 'X', 'R', 'O')) ||
        EFI_ERROR (
          Private->PciIo->Mem.Read (
                                Private->PciIo,
                                EfiPciIoWidthUint32,
                                PCI_BAR_IDX2,
                                36,
                                1,
                                &DrawStart
                                )
          ) ||
        (DrawStart != 0) ||
        EFI_ERROR (
          Private->PciIo->Mem.Read (
                                Private->PciIo,
                                EfiPciIoWidthUint32,
                                PCI_BAR_IDX2,
                                40,
                                1,
                                &AvailableFbSize
                                )
          ))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: can't read size of drawable buffer from QXL "
        "ROM\n",
        __func__
        ));
      return EFI_NOT_FOUND;
    }
  } else {
    AvailableFbSize  = BochsRead (Private, VBE_DISPI_INDEX_VIDEO_MEMORY_64K);
    AvailableFbSize *= SIZE_64KB;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: AvailableFbSize=0x%x\n",
    __func__,
    AvailableFbSize
    ));

  //
  // Setup Video Modes
  //
  Private->ModeData = AllocatePool (
                        sizeof (Private->ModeData[0]) * (QEMU_VIDEO_BOCHS_MODE_COUNT+1)
                        );
  if (Private->ModeData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  QemuVideoBochsEdid (Private, &XRes, &YRes);
  if (XRes && YRes) {
    QemuVideoBochsAddMode (
      Private,
      AvailableFbSize,
      XRes,
      YRes
      );
  }

  for (Index = 0; Index < QEMU_VIDEO_BOCHS_MODE_COUNT; Index++) {
    if ((QemuVideoBochsModes[Index].Width == XRes) &&
        (QemuVideoBochsModes[Index].Height == YRes))
    {
      continue; // duplicate with edid resolution
    }

    QemuVideoBochsAddMode (
      Private,
      AvailableFbSize,
      QemuVideoBochsModes[Index].Width,
      QemuVideoBochsModes[Index].Height
      );
  }

  return EFI_SUCCESS;
}
