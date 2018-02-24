/** @file
  The Platform Logo Protocol defines the interface to get the Platform logo
  image with the display attribute.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_LOGO_H__
#define __PLATFORM_LOGO_H__

#include <Protocol/HiiImage.h>

//
// GUID for EDKII Platform Logo Protocol
//
#define EDKII_PLATFORM_LOGO_PROTOCOL_GUID \
  { 0x53cd299f, 0x2bc1, 0x40c0, { 0x8c, 0x07, 0x23, 0xf6, 0x4f, 0xdb, 0x30, 0xe0 } }

typedef struct _EDKII_PLATFORM_LOGO_PROTOCOL EDKII_PLATFORM_LOGO_PROTOCOL;

typedef enum {
  EdkiiPlatformLogoDisplayAttributeLeftTop,
  EdkiiPlatformLogoDisplayAttributeCenterTop,
  EdkiiPlatformLogoDisplayAttributeRightTop,
  EdkiiPlatformLogoDisplayAttributeCenterRight,
  EdkiiPlatformLogoDisplayAttributeRightBottom,
  EdkiiPlatformLogoDisplayAttributeCenterBottom,
  EdkiiPlatformLogoDisplayAttributeLeftBottom,
  EdkiiPlatformLogoDisplayAttributeCenterLeft,
  EdkiiPlatformLogoDisplayAttributeCenter
} EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE;

/**
  Load a platform logo image and return its data and attributes.

  @param This              The pointer to this protocol instance.
  @param Instance          The visible image instance is found.
  @param Image             Points to the image.
  @param Attribute         The display attributes of the image returned.
  @param OffsetX           The X offset of the image regarding the Attribute.
  @param OffsetY           The Y offset of the image regarding the Attribute.

  @retval EFI_SUCCESS      The image was fetched successfully.
  @retval EFI_NOT_FOUND    The specified image could not be found.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PLATFORM_LOGO_GET_IMAGE)(
  IN     EDKII_PLATFORM_LOGO_PROTOCOL          *This,
  IN OUT UINT32                                *Instance,
     OUT EFI_IMAGE_INPUT                       *Image,
     OUT EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE *Attribute,
     OUT INTN                                  *OffsetX,
     OUT INTN                                  *OffsetY
  );


struct _EDKII_PLATFORM_LOGO_PROTOCOL {
  EDKII_PLATFORM_LOGO_GET_IMAGE GetImage;
};


extern EFI_GUID gEdkiiPlatformLogoProtocolGuid;

#endif
