/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GraphicsLib.h

Abstract:

 
--*/

#ifndef __EFI_GRAPHICS_LIB_H__
#define __EFI_GRAPHICS_LIB_H__

#include <Protocol/GraphicsOutput.h>

EFI_STATUS
GetGraphicsBitMapFromFV (
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  )
/*++

Routine Description:

  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

Arguments:

  FileNameGuid  - File Name of graphics file in the FV(s).

  Image         - Pointer to pointer to return graphics image.  If NULL, a 
                  buffer will be allocated.

  ImageSize     - Size of the graphics Image in bytes. Zero if no image found.


Returns: 

  EFI_SUCCESS          - Image and ImageSize are valid. 
  EFI_BUFFER_TOO_SMALL - Image not big enough. ImageSize has required size
  EFI_NOT_FOUND        - FileNameGuid not found

--*/
;

EFI_STATUS
ConvertBmpToUgaBlt (
  IN  VOID      *BmpImage,
  IN  UINTN     BmpImageSize,
  IN OUT VOID   **UgaBlt,
  IN OUT UINTN  *UgaBltSize,
  OUT UINTN     *PixelHeight,
  OUT UINTN     *PixelWidth
  )
/*++

Routine Description:

  Convert a *.BMP graphics image to a UGA blt buffer. If a NULL UgaBlt buffer
  is passed in a UgaBlt buffer will be allocated by this routine. If a UgaBlt
  buffer is passed in it will be used if it is big enough.

Arguments:

  BmpImage      - Pointer to BMP file

  BmpImageSize  - Number of bytes in BmpImage

  UgaBlt        - Buffer containing UGA version of BmpImage.

  UgaBltSize    - Size of UgaBlt in bytes.

  PixelHeight   - Height of UgaBlt/BmpImage in pixels

  PixelWidth    - Width of UgaBlt/BmpImage in pixels


Returns: 

  EFI_SUCCESS           - UgaBlt and UgaBltSize are returned. 
  EFI_UNSUPPORTED       - BmpImage is not a valid *.BMP image
  EFI_BUFFER_TOO_SMALL  - The passed in UgaBlt buffer is not big enough.
                          UgaBltSize will contain the required size.

--*/
;

EFI_STATUS
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  )
/*++

Routine Description:

  Use Console Control to turn off UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

Arguments:

  LogoFile - File name of logo to display on the center of the screen.


Returns: 

  EFI_SUCCESS           - ConsoleControl has been flipped to graphics and logo
                          displayed.
  EFI_UNSUPPORTED       - Logo not found

--*/
;

EFI_STATUS
DisableQuietBoot (
  VOID
  )
/*++

Routine Description:

  Use Console Control to turn on UGA based Simple Text Out consoles. The UGA 
  Simple Text Out screens will now be synced up with all non UGA output devices

Arguments:

  NONE

Returns: 

  EFI_SUCCESS           - UGA devices are back in text mode and synced up.
  EFI_UNSUPPORTED       - Logo not found

--*/
;

EFI_STATUS
LockKeyboards (
  IN  CHAR16    *Password
  )
/*++

Routine Description:
  Use Console Control Protocol to lock the Console In Spliter virtual handle. 
  This is the ConInHandle and ConIn handle in the EFI system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

Arguments:
  Password - Password used to lock ConIn device


Returns: 

  EFI_SUCCESS     - ConsoleControl has been flipped to graphics and logo
                    displayed.
  EFI_UNSUPPORTED - Logo not found

--*/
;

UINTN
EFIAPI
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background, OPTIONAL
  IN CHAR16                           *Fmt,
  ...
  )
;


#endif
