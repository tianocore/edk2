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
#include "Defer3rdPartyImageLoad.h"

//
// The structure to save the deferred 3rd party image information.
//
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL          *ImageDevicePath;
  BOOLEAN                           BootOption;
  BOOLEAN                           Loaded;
} DEFERRED_3RD_PARTY_IMAGE_INFO;

//
// The table to save the deferred 3rd party image item.
//
typedef struct {
  UINTN                             Count;         ///< deferred 3rd party image count
  DEFERRED_3RD_PARTY_IMAGE_INFO     *ImageInfo;    ///< deferred 3rd party image item
} DEFERRED_3RD_PARTY_IMAGE_TABLE;

BOOLEAN                          mImageLoadedAfterEndOfDxe   = FALSE;
BOOLEAN                          mEndOfDxe                   = FALSE;
DEFERRED_3RD_PARTY_IMAGE_TABLE   mDeferred3rdPartyImage = {
  0,       // Deferred image count
  NULL     // The deferred image info
};

EFI_DEFERRED_IMAGE_LOAD_PROTOCOL mDeferredImageLoad   = {
  GetDefferedImageInfo
};

/**
  Return whether the file comes from FV.

  @param[in]    File    This is a pointer to the device path of the file
                        that is being dispatched.

  @retval TRUE  File comes from FV.
  @retval FALSE File doesn't come from FV.
**/
BOOLEAN
FileFromFv (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Find the deferred image which matches the device path.

  @param[in]  ImageDevicePath  A pointer to the device path of a image.
  @param[in]  BootOption       Whether the image is a boot option.

  @return Pointer to the found deferred image or NULL if not found.
**/
DEFERRED_3RD_PARTY_IMAGE_INFO *
LookupImage (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL    *ImageDevicePath,
  IN        BOOLEAN                     BootOption
  )
{
  UINTN                                 Index;
  UINTN                                 DevicePathSize;

  DevicePathSize = GetDevicePathSize (ImageDevicePath);

  for (Index = 0; Index < mDeferred3rdPartyImage.Count; Index++) {
    if (CompareMem (ImageDevicePath, mDeferred3rdPartyImage.ImageInfo[Index].ImageDevicePath, DevicePathSize) == 0) {
      ASSERT (mDeferred3rdPartyImage.ImageInfo[Index].BootOption == BootOption);
      return &mDeferred3rdPartyImage.ImageInfo[Index];
    }
  }

  return NULL;
}

/**
  Add the image info to a deferred image list.

  @param[in]  ImageDevicePath  A pointer to the device path of a image.
  @param[in]  BootOption       Whether the image is a boot option.

**/
VOID
QueueImage (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL    *ImageDevicePath,
  IN        BOOLEAN                     BootOption
  )
{
  DEFERRED_3RD_PARTY_IMAGE_INFO         *ImageInfo;

  //
  // Expand memory for the new deferred image.
  //
  ImageInfo = ReallocatePool (
                mDeferred3rdPartyImage.Count * sizeof (DEFERRED_3RD_PARTY_IMAGE_INFO),
                (mDeferred3rdPartyImage.Count + 1) * sizeof (DEFERRED_3RD_PARTY_IMAGE_INFO),
                mDeferred3rdPartyImage.ImageInfo
  );
  if (ImageInfo == NULL) {
    return;
  }
  mDeferred3rdPartyImage.ImageInfo = ImageInfo;

  //
  // Save the deferred image information.
  //
  ImageInfo = &mDeferred3rdPartyImage.ImageInfo[mDeferred3rdPartyImage.Count];
  ImageInfo->ImageDevicePath = DuplicateDevicePath (ImageDevicePath);
  if (ImageInfo->ImageDevicePath == NULL) {
    return;
  }
  ImageInfo->BootOption = BootOption;
  ImageInfo->Loaded     = FALSE;
  mDeferred3rdPartyImage.Count++;
}


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
  )
{
  UINTN                                    Index;
  UINTN                                    NewCount;

  if ((This == NULL) || (ImageSize == NULL) || (Image == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ImageDevicePath == NULL) || (BootOption == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Remove the loaded images from the defer list in the first call.
  //
  if (ImageIndex == 0) {
    NewCount = 0;
    for (Index = 0; Index < mDeferred3rdPartyImage.Count; Index++) {
      if (!mDeferred3rdPartyImage.ImageInfo[Index].Loaded) {
        CopyMem (
          &mDeferred3rdPartyImage.ImageInfo[NewCount],
          &mDeferred3rdPartyImage.ImageInfo[Index],
          sizeof (DEFERRED_3RD_PARTY_IMAGE_INFO)
          );
        NewCount++;
      }
    }

    mDeferred3rdPartyImage.Count = NewCount;
  }

  if (ImageIndex >= mDeferred3rdPartyImage.Count) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the request deferred image.
  //
  *ImageDevicePath = mDeferred3rdPartyImage.ImageInfo[ImageIndex].ImageDevicePath;
  *BootOption      = mDeferred3rdPartyImage.ImageInfo[ImageIndex].BootOption;
  *Image           = NULL;
  *ImageSize       = 0;

  return EFI_SUCCESS;
}

/**
  Callback function executed when the EndOfDxe event group is signaled.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
EndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
}

/**
  Event notification for gEfiDxeSmmReadyToLockProtocolGuid event.

  This function reports failure if any deferred image is loaded before
  this callback.
  Platform should publish ReadyToLock protocol immediately after signaling
  of the End of DXE Event.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
DxeSmmReadyToLock (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  VOID                      *Interface;

  Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }

  gBS->CloseEvent (Event);

  if (mImageLoadedAfterEndOfDxe) {
    //
    // Platform should not dispatch the 3rd party images after signaling EndOfDxe event
    // but before publishing DxeSmmReadyToLock protocol.
    //
    DEBUG ((
      DEBUG_ERROR,
      "[Security] 3rd party images must be dispatched after DxeSmmReadyToLock Protocol installation!\n"
      ));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED,
      (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_EC_ILLEGAL_SOFTWARE_STATE)
      );
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

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
  )
{
  DEFERRED_3RD_PARTY_IMAGE_INFO        *ImageInfo;

  //
  // Ignore if File is NULL.
  //
  if (File == NULL) {
    return EFI_SUCCESS;
  }

  if (FileFromFv (File)) {
    return EFI_SUCCESS;
  }

  ImageInfo = LookupImage (File, BootPolicy);

  DEBUG_CODE (
    CHAR16 *DevicePathStr;
    DevicePathStr = ConvertDevicePathToText (File, FALSE, FALSE);
    DEBUG ((
      DEBUG_INFO,
      "[Security] 3rd party image[%p] %s EndOfDxe: %s.\n", ImageInfo,
      mEndOfDxe ? L"can be loaded after": L"is deferred to load before",
      DevicePathStr
      ));
    if (DevicePathStr != NULL) {
      FreePool (DevicePathStr);
    }
    );

  if (mEndOfDxe) {
    mImageLoadedAfterEndOfDxe = TRUE;
    //
    // The image might be first time loaded after EndOfDxe,
    // So ImageInfo can be NULL.
    //
    if (ImageInfo != NULL) {
      ImageInfo->Loaded = TRUE;
    }
    return EFI_SUCCESS;
  } else {
    //
    // The image might be second time loaded before EndOfDxe,
    // So ImageInfo can be non-NULL.
    //
    if (ImageInfo == NULL) {
      QueueImage (File, BootPolicy);
    }
    return EFI_ACCESS_DENIED;
  }
}

/**
  Installs DeferredImageLoad Protocol and listens EndOfDxe event.
**/
VOID
Defer3rdPartyImageLoadInitialize (
  VOID
  )
{
  EFI_STATUS                           Status;
  EFI_HANDLE                           Handle;
  EFI_EVENT                            Event;
  VOID                                 *Registration;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiDeferredImageLoadProtocolGuid,
                  &mDeferredImageLoad,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  EfiCreateProtocolNotifyEvent (
    &gEfiDxeSmmReadyToLockProtocolGuid,
    TPL_CALLBACK,
    DxeSmmReadyToLock,
    NULL,
    &Registration
    );
}
