/** @file
  Type definitions related to the VBE (VESA BIOS Extension, Int10h AH=4Fh)
  services GET INFORMATION (AL=00h) and GET MODE INFORMATION (AL=01h).

  For reference, see Ralf Brown's Interrupt List:
  <http://www.cs.cmu.edu/~ralf/files.html>
  <http://www.ctyme.com/rbrown.htm>

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __LEGACY_VGA_BIOS_H__
#define __LEGACY_VGA_BIOS_H__

#include <Base.h>

#pragma pack (1)
typedef struct {
  UINT8     Signature[4];
  UINT16    VesaVersion;
  UINT32    OemNameAddress;
  UINT32    Capabilities;
  UINT32    ModeListAddress;
  UINT16    VideoMem64K;
  UINT16    OemSoftwareVersion;
  UINT32    VendorNameAddress;
  UINT32    ProductNameAddress;
  UINT32    ProductRevAddress;
} VBE_INFO_BASE;

typedef struct {
  VBE_INFO_BASE    Base;
  UINT8            Buffer[256 - sizeof (VBE_INFO_BASE)];
} VBE_INFO;

typedef struct {
  UINT16    ModeAttr;
  UINT8     WindowAAttr;
  UINT8     WindowBAttr;
  UINT16    WindowGranularityKB;
  UINT16    WindowSizeKB;
  UINT16    WindowAStartSegment;
  UINT16    WindowBStartSegment;
  UINT32    WindowPositioningAddress;
  UINT16    BytesPerScanLine;

  UINT16    Width;
  UINT16    Height;
  UINT8     CharCellWidth;
  UINT8     CharCellHeight;
  UINT8     NumPlanes;
  UINT8     BitsPerPixel;
  UINT8     NumBanks;
  UINT8     MemoryModel;
  UINT8     BankSizeKB;
  UINT8     NumImagePagesLessOne;
  UINT8     Vbe3;

  UINT8     RedMaskSize;
  UINT8     RedMaskPos;
  UINT8     GreenMaskSize;
  UINT8     GreenMaskPos;
  UINT8     BlueMaskSize;
  UINT8     BlueMaskPos;
  UINT8     ReservedMaskSize;
  UINT8     ReservedMaskPos;
  UINT8     DirectColorModeInfo;

  UINT32    LfbAddress;
  UINT32    OffScreenAddress;
  UINT16    OffScreenSizeKB;

  UINT16    BytesPerScanLineLinear;
  UINT8     NumImagesLessOneBanked;
  UINT8     NumImagesLessOneLinear;
  UINT8     RedMaskSizeLinear;
  UINT8     RedMaskPosLinear;
  UINT8     GreenMaskSizeLinear;
  UINT8     GreenMaskPosLinear;
  UINT8     BlueMaskSizeLinear;
  UINT8     BlueMaskPosLinear;
  UINT8     ReservedMaskSizeLinear;
  UINT8     ReservedMaskPosLinear;
  UINT32    MaxPixelClockHz;
  UINT8     Reserved[190];
} VBE_MODE_INFO;

typedef struct {
  UINT16    ModeAttr;
  UINT8     WindowAAttr;
  UINT8     WindowBAttr;
  UINT16    WindowGranularityKB;
  UINT16    WindowSizeKB;
  UINT16    WindowAStartSegment;
  UINT16    WindowBStartSegment;
  UINT32    WindowPositioningAddress;
  UINT16    BytesPerScanLine;

  UINT16    Width;
  UINT16    Height;
  UINT8     CharCellWidth;
  UINT8     CharCellHeight;
  UINT8     NumPlanes;
  UINT8     BitsPerPixel;
  UINT8     NumBanks;
  UINT8     MemoryModel;
  UINT8     BankSizeKB;
  UINT8     NumImagePagesLessOne;
  UINT8     Vbe3;

  UINT8     RedMaskSize;
  UINT8     RedMaskPos;
  UINT8     GreenMaskSize;
  UINT8     GreenMaskPos;
  UINT8     BlueMaskSize;
  UINT8     BlueMaskPos;
  UINT8     ReservedMaskSize;
  UINT8     ReservedMaskPos;
  UINT8     DirectColorModeInfo;

  UINT32    LfbAddress;
  UINT32    OffScreenAddress;
  UINT16    OffScreenSizeKB;
} VBE2_MODE_INFO;
#pragma pack ()

#endif
