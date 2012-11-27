/** @file
  Graphics Output Protocol functions for the QEMU video controller.

  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
UINT8 GraphicsController[9] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF
};

//
// 640 x 480 x 256 color @ 60 Hertz
//
UINT8 Crtc_640_480_256_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0x83, 0xdf, 0x50, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x22
};

UINT8 Crtc_640_480_32bpp_60[28] = {
  0x5d, 0x4f, 0x50, 0x82, 0x53, 0x9f, 0x00, 0x3e,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0x83, 0xdf, 0x40, 0x00, 0xe7, 0x04, 0xe3,
  0xff, 0x00, 0x00, 0x32
};

UINT16 Seq_640_480_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

UINT16 Seq_640_480_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x7e0e, 0x2b1b, 0x2f1c, 0x301d, 0x331e
};

//
// 800 x 600 x 256 color @ 60 Hertz
//
UINT8 Crtc_800_600_256_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x58, 0x8C, 0x57, 0x64, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x22
};

UINT8 Crtc_800_600_32bpp_60[28] = {
  0x7F, 0x63, 0x64, 0x80, 0x6B, 0x1B, 0x72, 0xF0,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x58, 0x8C, 0x57, 0x90, 0x00, 0x5F, 0x91, 0xE3,
  0xFF, 0x00, 0x00, 0x32
};

UINT16 Seq_800_600_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

UINT16 Seq_800_600_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x510e, 0x2b1b, 0x2f1c, 0x301d, 0x3a1e
};

UINT8 Crtc_960_720_32bpp_60[28] = {
  0xA3, 0x77, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xCF, 0xe0, 0x00, 0x00, 0x64, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16 Seq_960_720_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

//
// 1024 x 768 x 256 color @ 60 Hertz
//
UINT8 Crtc_1024_768_256_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x22
};

UINT16 Seq_1024_768_256_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1107, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

//
// 1024 x 768 x 24-bit color @ 60 Hertz
//
UINT8 Crtc_1024_768_24bpp_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0x80, 0x00, 0x00, 0x24, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16 Seq_1024_768_24bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1507, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

UINT8 Crtc_1024_768_32bpp_60[28] = {
  0xA3, 0x7F, 0x80, 0x86, 0x85, 0x96, 0x24, 0xFD,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x88, 0xFF, 0xe0, 0x00, 0x00, 0x64, 0xE3,
  0xFF, 0x4A, 0x00, 0x32
};

UINT16 Seq_1024_768_32bpp_60[15] = {
  0x0100, 0x0101, 0x0f02, 0x0003, 0x0e04, 0x1907, 0x0008, 0x4a0b,
  0x5b0c, 0x450d, 0x760e, 0x2b1b, 0x2f1c, 0x301d, 0x341e
};

///
/// Table of supported video modes
///
QEMU_VIDEO_CIRRUS_MODES  QemuVideoCirrusModes[] = {
//  {  640, 480, 8, 60, Crtc_640_480_256_60,  Seq_640_480_256_60,  0xe3 },
//  {  800, 600, 8, 60, Crtc_800_600_256_60,  Seq_800_600_256_60,  0xef },
  {  640, 480, 32, 60, Crtc_640_480_32bpp_60,  Seq_640_480_32bpp_60,  0xef },
  {  800, 600, 32, 60, Crtc_800_600_32bpp_60,  Seq_800_600_32bpp_60,  0xef },
//  { 1024, 768, 8, 60, Crtc_1024_768_256_60, Seq_1024_768_256_60, 0xef }
  { 1024, 768, 24, 60, Crtc_1024_768_24bpp_60, Seq_1024_768_24bpp_60, 0xef }
//  { 1024, 768, 32, 60, Crtc_1024_768_32bpp_60, Seq_1024_768_32bpp_60, 0xef }
//  { 960, 720, 32, 60, Crtc_960_720_32bpp_60, Seq_1024_768_32bpp_60, 0xef }
};

#define QEMU_VIDEO_CIRRUS_MODE_COUNT \
  (sizeof (QemuVideoCirrusModes) / sizeof (QemuVideoCirrusModes[0]))

/**
  Construct the valid video modes for QemuVideo.

**/
EFI_STATUS
QemuVideoCirrusModeSetup (
  QEMU_VIDEO_PRIVATE_DATA  *Private
  )
{
  UINT32                                 Index;
  QEMU_VIDEO_MODE_DATA                   *ModeData;
  QEMU_VIDEO_CIRRUS_MODES                *VideoMode;

  //
  // Setup Video Modes
  //
  Private->ModeData = AllocatePool (
                        sizeof (Private->ModeData[0]) * QEMU_VIDEO_CIRRUS_MODE_COUNT
                        );
  ModeData = Private->ModeData;
  VideoMode = &QemuVideoCirrusModes[0];
  for (Index = 0; Index < QEMU_VIDEO_CIRRUS_MODE_COUNT; Index ++) {
    ModeData->ModeNumber = Index;
    ModeData->HorizontalResolution          = VideoMode->Width;
    ModeData->VerticalResolution            = VideoMode->Height;
    ModeData->ColorDepth                    = VideoMode->ColorDepth;
    ModeData->RefreshRate                   = VideoMode->RefreshRate;
    DEBUG ((EFI_D_INFO,
      "Adding Cirrus Video Mode %d: %dx%d, %d-bit, %d Hz\n",
      ModeData->ModeNumber,
      ModeData->HorizontalResolution,
      ModeData->VerticalResolution,
      ModeData->ColorDepth,
      ModeData->RefreshRate
      ));

    ModeData ++ ;
    VideoMode ++;
  }
  Private->MaxMode = QEMU_VIDEO_CIRRUS_MODE_COUNT;

  return EFI_SUCCESS;
}

///
/// Table of supported video modes
///
QEMU_VIDEO_BOCHS_MODES  QemuVideoBochsModes[] = {
  {  640, 480, 32 },
  {  800, 600, 32 },
  { 1024, 768, 24 },
};

#define QEMU_VIDEO_BOCHS_MODE_COUNT \
  (sizeof (QemuVideoBochsModes) / sizeof (QemuVideoBochsModes[0]))

EFI_STATUS
QemuVideoBochsModeSetup (
  QEMU_VIDEO_PRIVATE_DATA  *Private
  )
{
  UINT32                                 Index;
  QEMU_VIDEO_MODE_DATA                   *ModeData;
  QEMU_VIDEO_BOCHS_MODES                 *VideoMode;

  //
  // Setup Video Modes
  //
  Private->ModeData = AllocatePool (
                        sizeof (Private->ModeData[0]) * QEMU_VIDEO_BOCHS_MODE_COUNT
                        );
  ModeData = Private->ModeData;
  VideoMode = &QemuVideoBochsModes[0];
  for (Index = 0; Index < QEMU_VIDEO_BOCHS_MODE_COUNT; Index ++) {
    ModeData->ModeNumber = Index;
    ModeData->HorizontalResolution          = VideoMode->Width;
    ModeData->VerticalResolution            = VideoMode->Height;
    ModeData->ColorDepth                    = VideoMode->ColorDepth;
    ModeData->RefreshRate                   = 60;
    DEBUG ((EFI_D_INFO,
      "Adding Bochs Video Mode %d: %dx%d, %d-bit, %d Hz\n",
      ModeData->ModeNumber,
      ModeData->HorizontalResolution,
      ModeData->VerticalResolution,
      ModeData->ColorDepth,
      ModeData->RefreshRate
      ));

    ModeData ++ ;
    VideoMode ++;
  }
  Private->MaxMode = QEMU_VIDEO_BOCHS_MODE_COUNT;

  return EFI_SUCCESS;
}

