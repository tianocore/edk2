/** @file
  Graphics Library

  Copyright (c) 2006 - 2008, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_GRAPHICS_LIB_H__
#define __EFI_GRAPHICS_LIB_H__

#include <Protocol/GraphicsOutput.h>


/**
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param[in]  FileNameGuid  File Name of graphics file in the FV(s).
  @param[out] Image         Pointer to pointer to return graphics image.  If NULL, a 
                            buffer will be allocated.
  @param[out] ImageSize     Size of the graphics Image in bytes. Zero if no image found.

  @retval   EFI_INVALID_PARAMETER  invalid parameter
  @retval   EFI_UNSUPPORTED        Range can not be erased
  @retval   EFI_SUCCESS            Image and ImageSize are valid. 
  @retval   EFI_BUFFER_TOO_SMALL   Image not big enough. ImageSize has required size
  @retval   EFI_NOT_FOUND          FileNameGuid not found

**/
EFI_STATUS
EFIAPI
GetGraphicsBitMapFromFV (
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  );

/**
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param[in]  ImageHandle   The driver image handle of the caller. The parameter is used to
                            optimize the loading of the image file so that the FV from which
                            the driver image is loaded will be tried first. 
  @param[in]  FileNameGuid  File Name of graphics file in the FV(s).
  @param[out] Image         Pointer to pointer to return graphics image.  If NULL, a 
                            buffer will be allocated.
  @param[out] ImageSize     Size of the graphics Image in bytes. Zero if no image found.

  @retval   EFI_INVALID_PARAMETER  invalid parameter
  @retval   EFI_UNSUPPORTED        Range can not be erased
  @retval   EFI_SUCCESS            Image and ImageSize are valid. 
  @retval   EFI_BUFFER_TOO_SMALL   Image not big enough. ImageSize has required size
  @retval   EFI_NOT_FOUND          FileNameGuid not found

**/
EFI_STATUS
EFIAPI
GetGraphicsBitMapFromFVEx (
  IN  EFI_HANDLE    ImageHandle,
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  );

/**
  Convert a *.BMP graphics image to a UGA blt buffer. If a NULL UgaBlt buffer
  is passed in a UgaBlt buffer will be allocated by this routine. If a UgaBlt
  buffer is passed in it will be used if it is big enough.

  @param[in]      BmpImage      Pointer to BMP file
  @param[in]      BmpImageSize  Number of bytes in BmpImage
  @param[in out]  UgaBlt        Buffer containing UGA version of BmpImage.
  @param[in out]  UgaBltSize    Size of UgaBlt in bytes.
  @param[out]     PixelHeight   Height of UgaBlt/BmpImage in pixels
  @param[out]     PixelWidth    Width of UgaBlt/BmpImage in pixels

  @retval EFI_SUCCESS           UgaBlt and UgaBltSize are returned. 
  @retval EFI_UNSUPPORTED       BmpImage is not a valid *.BMP image
  @retval EFI_BUFFER_TOO_SMALL  The passed in UgaBlt buffer is not big enough.
                                UgaBltSize will contain the required size.
**/
EFI_STATUS
EFIAPI
ConvertBmpToUgaBlt (
  IN  VOID      *BmpImage,
  IN  UINTN     BmpImageSize,
  IN OUT VOID   **UgaBlt,
  IN OUT UINTN  *UgaBltSize,
  OUT UINTN     *PixelHeight,
  OUT UINTN     *PixelWidth
  );


/**
  Use Console Control to turn off UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

  @param[in]  LogoFile   File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found

**/
EFI_STATUS
EFIAPI
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  );

/**
  Use Console Control to turn off GOP/UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

  @param  LogoFile    File name of logo to display on the center of the screen.
  @param  ImageHandle The driver image handle of the caller. The parameter is used to
                      optimize the loading of the logo file so that the FV from which
                      the driver image is loaded will be tried first.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found

**/
EFI_STATUS
EFIAPI
EnableQuietBootEx (
  IN  EFI_GUID    *LogoFile,
  IN  EFI_HANDLE  ImageHandle
  );


/**
  Use Console Control to turn on UGA based Simple Text Out consoles. The UGA 
  Simple Text Out screens will now be synced up with all non UGA output devices

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
DisableQuietBoot (
  VOID
  );


/**
  Use Console Control Protocol to lock the Console In Spliter virtual handle. 
  This is the ConInHandle and ConIn handle in the EFI system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param[in]  Password   Password used to lock ConIn device.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo
                          displayed.
  @retval EFI_UNSUPPORTED Password not found

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  );


/**
  Print to graphics screen at the given X,Y coordinates of the graphics screen.
  see definition of Print to find rules for constructing Fmt.

  @param[in]  X            Row to start printing at
  @param[in]  Y            Column to start printing at
  @param[in]  ForeGround   Foreground color
  @param[in]  BackGround   background color
  @param[in]  Fmt          Print format sting. See definition of Print
  @param[in]  ...          Argumnet stream defined by Fmt string

  @return  Number of Characters printed.

**/
UINTN
EFIAPI
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CHAR16                           *Fmt,
  ...
  );


#endif
