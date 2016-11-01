/** @file
  Implement defer image load services for user identification in UEFI2.2.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEFER_3RD_PARTY_IMAGE_LOAD_H_
#define _DEFER_3RD_PARTY_IMAGE_LOAD_H_

#include <PiDxe.h>
#include <Guid/EventGroup.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DxeSmmReadyToLock.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/ReportStatusCodeLib.h>

/**
  Returns information about a deferred image.

  This function returns information about a single deferred image. The deferred images are
  numbered consecutively, starting with 0.  If there is no image which corresponds to
  ImageIndex, then EFI_NOT_FOUND is returned. All deferred images may be returned by
  iteratively calling this function until EFI_NOT_FOUND is returned.
  Image may be NULL and ImageSize set to 0 if the decision to defer execution was made
  because of the location of the executable image, rather than its actual contents.

  @param[in]  This             Points to this instance of the EFI_DEFERRED_IMAGE_LOAD_PROTOCOL.
  @param[in]  ImageIndex       Zero-based index of the deferred index.
  @param[out] ImageDevicePath  On return, points to a pointer to the device path of the image.
                               The device path should not be freed by the caller.
  @param[out] Image            On return, points to the first byte of the image or NULL if the
                               image is not available. The image should not be freed by the caller
                               unless LoadImage() has been successfully called.
  @param[out] ImageSize        On return, the size of the image, or 0 if the image is not available.
  @param[out] BootOption       On return, points to TRUE if the image was intended as a boot option
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

/**
  Defer the 3rd party image load and installs Deferred Image Load Protocol.

  @param[in]  File                  This is a pointer to the device path of the file that
                                    is being dispatched. This will optionally be used for
                                    logging.
  @param[in]  BootPolicy            A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS               The file is not 3rd party image and can be loaded immediately.
  @retval EFI_ACCESS_DENIED         The file is 3rd party image and needs deferred.
**/
EFI_STATUS
Defer3rdPartyImageLoad (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  BOOLEAN                          BootPolicy
  );

/**
  Installs DeferredImageLoad Protocol and listens EndOfDxe event.
**/
VOID
Defer3rdPartyImageLoadInitialize (
  VOID
  );

#endif
