/** @file
  EFI OEM Badging Protocol definition header file

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_OEM_BADGING_H__
#define __EFI_OEM_BADGING_H__

//
// GUID for EFI OEM Badging Protocol
//
#define EFI_OEM_BADGING_PROTOCOL_GUID \
  { 0x170e13c0, 0xbf1b, 0x4218, {0x87, 0x1d, 0x2a, 0xbd, 0xc6, 0xf8, 0x87, 0xbc } }


typedef struct _EFI_OEM_BADGING_PROTOCOL EFI_OEM_BADGING_PROTOCOL;

typedef enum {
  EfiBadgingFormatBMP,
  EfiBadgingFormatJPEG,
  EfiBadgingFormatTIFF,
  EfiBadgingFormatGIF,
  EfiBadgingFormatUnknown
} EFI_BADGING_FORMAT;

typedef enum {
  EfiBadgingDisplayAttributeLeftTop,
  EfiBadgingDisplayAttributeCenterTop,
  EfiBadgingDisplayAttributeRightTop,
  EfiBadgingDisplayAttributeCenterRight,
  EfiBadgingDisplayAttributeRightBottom,
  EfiBadgingDisplayAttributeCenterBottom,
  EfiBadgingDisplayAttributeLeftBottom,
  EfiBadgingDisplayAttributeCenterLeft,
  EfiBadgingDisplayAttributeCenter,
  EfiBadgingDisplayAttributeCustomized
} EFI_BADGING_DISPLAY_ATTRIBUTE;


typedef
EFI_STATUS
(EFIAPI *EFI_BADGING_GET_IMAGE) (
  IN     EFI_OEM_BADGING_PROTOCOL          *This,
  IN OUT UINT32                            *Instance,
     OUT EFI_BADGING_FORMAT                *Format,
     OUT UINT8                             **ImageData,
     OUT UINTN                             *ImageSize,
     OUT EFI_BADGING_DISPLAY_ATTRIBUTE     *Attribute,
     OUT UINTN                             *CoordinateX,
     OUT UINTN                             *CoordinateY
);


struct _EFI_OEM_BADGING_PROTOCOL {
  EFI_BADGING_GET_IMAGE       GetImage;
};


extern EFI_GUID gEfiOEMBadgingProtocolGuid;

#endif
