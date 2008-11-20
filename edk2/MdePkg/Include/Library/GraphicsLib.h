/** @file
  Library supports displaying graphical splash screen,
  locking of keyboard input and printing character on
  screen.

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

/**
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param[in]  FileNameGuid  File Name of graphics file in the FV(s).
  @param[out] Image         Pointer to pointer to return graphics image.  If NULL, a 
                            buffer will be allocated.
  @param[out] ImageSize     Size of the graphics Image in bytes. Zero if no image found.

  @retval  EFI_SUCCESS          The image is found and data and size is returned.
  @retval  EFI_UNSUPPORTED      FvHandle does not support EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @retval  EFI_NOT_FOUND        The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR     A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED    The firmware volume containing the searched Firmware File is configured to disallow reads.

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

  @retval  EFI_SUCCESS          The image is found and data and size is returned.
  @retval  EFI_UNSUPPORTED      FvHandle does not support EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @retval  EFI_NOT_FOUND        The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR     A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED    The firmware volume containing the searched Firmware File is configured to disallow reads.

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

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  );

#endif
