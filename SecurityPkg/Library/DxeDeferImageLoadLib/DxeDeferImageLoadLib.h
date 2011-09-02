/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by DeferImageLoadLib.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEFER_IMAGE_LOAD_LIB_H__
#define __DEFER_IMAGE_LOAD_LIB_H__

#include <PiDxe.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/UserCredential.h>
#include <Protocol/UserManager.h>
#include <Protocol/DevicePathToText.h>

#include <Guid/GlobalVariable.h>

//
// Image type definitions.
//
#define IMAGE_UNKNOWN                         0x00000001
#define IMAGE_FROM_FV                         0x00000002
#define IMAGE_FROM_OPTION_ROM                 0x00000004
#define IMAGE_FROM_REMOVABLE_MEDIA            0x00000008
#define IMAGE_FROM_FIXED_MEDIA                0x00000010

//
// The struct to save the deferred image information.
//
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL          *ImageDevicePath;
  VOID                              *Image;
  UINTN                             ImageSize;
  BOOLEAN                           BootOption;
} DEFERRED_IMAGE_INFO;

//
// The table to save the deferred image item.
//
typedef struct {
  UINTN                             Count;         ///< deferred image count
  DEFERRED_IMAGE_INFO               *ImageInfo;    ///< deferred image item
} DEFERRED_IMAGE_TABLE;

/**
  Returns information about a deferred image.

  This function returns information about a single deferred image. The deferred images are 
  numbered consecutively, starting with 0.  If there is no image which corresponds to 
  ImageIndex, then EFI_NOT_FOUND is returned. All deferred images may be returned by 
  iteratively calling this function until EFI_NOT_FOUND is returned.
  Image may be NULL and ImageSize set to 0 if the decision to defer execution was made 
  because of the location of the executable image, rather than its actual contents.  

  @param[in]  This              Points to this instance of the EFI_DEFERRED_IMAGE_LOAD_PROTOCOL.
  @param[in]  ImageIndex        Zero-based index of the deferred index.
  @param[out] ImageDevicePath   On return, points to a pointer to the device path of the image. 
                                The device path should not be freed by the caller. 
  @param[out] Image             On return, points to the first byte of the image or NULL if the 
                                image is not available. The image should not be freed by the caller
                                unless LoadImage() has been called successfully.  
  @param[out] ImageSize         On return, the size of the image, or 0 if the image is not available.
  @param[out] BootOption        On return, points to TRUE if the image was intended as a boot option 
                                or FALSE if it was not intended as a boot option. 
 
  @retval EFI_SUCCESS           Image information returned successfully.
  @retval EFI_NOT_FOUND         ImageIndex does not refer to a valid image.
  @retval EFI_INVALID_PARAMETER ImageDevicePath is NULL or Image is NULL or ImageSize is NULL or 
                                BootOption is NULL.
  
**/
EFI_STATUS
EFIAPI
GetDefferedImageInfo (
  IN     EFI_DEFERRED_IMAGE_LOAD_PROTOCOL  *This,
  IN     UINTN                             ImageIndex,
     OUT EFI_DEVICE_PATH_PROTOCOL          **ImageDevicePath,
     OUT VOID                              **Image,
     OUT UINTN                             *ImageSize,
     OUT BOOLEAN                           *BootOption
  );
  
#endif
