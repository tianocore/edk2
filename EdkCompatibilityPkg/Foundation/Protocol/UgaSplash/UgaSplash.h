/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    UgaSplash.h

Abstract:

  UGA Splash screen protocol.

  Abstraction of a very simple graphics device.

--*/

#ifndef __UGA_SPLASH_H__
#define __UGA_SPLASH_H__

#include EFI_PROTOCOL_DEFINITION (GraphicsOutput)
#include EFI_PROTOCOL_DEFINITION (UgaDraw)


#define EFI_UGA_SPLASH_PROTOCOL_GUID \
  { 0xa45b3a0d, 0x2e55, 0x4c03, {0xad, 0x9c, 0x27, 0xd4, 0x82, 0xb, 0x50, 0x7e} }

typedef struct _EFI_UGA_SPLASH_PROTOCOL   EFI_UGA_SPLASH_PROTOCOL;


struct _EFI_UGA_SPLASH_PROTOCOL {
  UINT32                        PixelWidth;
  UINT32                        PixelHeight;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Image;
};

extern EFI_GUID gEfiUgaSplashProtocolGuid;

#endif
