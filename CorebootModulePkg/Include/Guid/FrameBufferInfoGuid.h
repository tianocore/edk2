/** @file
  This file defines the hob structure for frame buffer device.
  
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FRAME_BUFFER_INFO_GUID_H__
#define __FRAME_BUFFER_INFO_GUID_H__

///
/// Frame Buffer Information GUID
///
extern EFI_GUID gUefiFrameBufferInfoGuid;

typedef struct {
  UINT8 Position; // Position of the color
  UINT8 Mask;     // The number of bits expressed as a mask
} COLOR_PLACEMENT;

typedef struct {  
  UINT64             LinearFrameBuffer;  
  UINT32             HorizontalResolution;
  UINT32             VerticalResolution;
  UINT32             BitsPerPixel;
  UINT16             BytesPerScanLine;
  COLOR_PLACEMENT    Red;
  COLOR_PLACEMENT    Green;
  COLOR_PLACEMENT    Blue;
  COLOR_PLACEMENT    Reserved;
} FRAME_BUFFER_INFO;  
  
#endif
