/** @file
  Hob guid for Information about the graphics mode.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  This HOB is introduced in in PI Version 1.4.

**/

#ifndef _GRAPHICS_INFO_HOB_GUID_H_
#define _GRAPHICS_INFO_HOB_GUID_H_

#include <Protocol/GraphicsOutput.h>

#define EFI_PEI_GRAPHICS_INFO_HOB_GUID \
  { \
    0x39f62cce, 0x6825, 0x4669, { 0xbb, 0x56, 0x54, 0x1a, 0xba, 0x75, 0x3a, 0x07 } \
  }

typedef struct {
  EFI_PHYSICAL_ADDRESS                  FrameBufferBase;
  UINT32                                FrameBufferSize;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  GraphicsMode;
} EFI_PEI_GRAPHICS_INFO_HOB;

extern EFI_GUID gEfiGraphicsInfoHobGuid;

#endif
