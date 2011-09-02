/** @file
  Load the deferred images after user is identified.
    
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserIdentifyManager.h"

EFI_HANDLE        mDeferredImageHandle;

/**
  The function will load all the deferred images again. If the deferred image is loaded
  successfully, try to start it.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context

**/
VOID
EFIAPI
LoadDeferredImage (
  IN EFI_EVENT                       Event,
  IN VOID                            *Context
  )
{
  EFI_STATUS                         Status;
  EFI_DEFERRED_IMAGE_LOAD_PROTOCOL   *DeferredImage;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuf;
  UINTN                              Index;
  UINTN                              DriverIndex;
  EFI_DEVICE_PATH_PROTOCOL           *ImageDevicePath;
  VOID                               *DriverImage;
  UINTN                              ImageSize; 
  BOOLEAN                            BootOption;
  EFI_HANDLE                         ImageHandle;
  UINTN                              ExitDataSize;
  CHAR16                             *ExitData;

  //
  // Find all the deferred image load protocols.
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDeferredImageLoadProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuf[Index],
                    &gEfiDeferredImageLoadProtocolGuid,
                    (VOID **) &DeferredImage
                    );
    if (EFI_ERROR (Status)) {
      continue ;
    }

    DriverIndex = 0;
    do {
      //
      // Load all the deferred images in this protocol instance.
      //
      Status = DeferredImage->GetImageInfo(
                                DeferredImage, 
                                DriverIndex, 
                                &ImageDevicePath, 
                                (VOID **) &DriverImage,
                                &ImageSize, 
                                &BootOption
                                );
      if (EFI_ERROR (Status)) {
        break;
      } 

      //
      // Load and start the image.
      //
      Status = gBS->LoadImage (
                      BootOption,
                      mDeferredImageHandle,
                      ImageDevicePath,
                      NULL,
                      0,
                      &ImageHandle
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Before calling the image, enable the Watchdog Timer for
        // a 5 Minute period
        //
        gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);
        Status = gBS->StartImage (ImageHandle, &ExitDataSize, &ExitData);
    
        //
        // Clear the Watchdog Timer after the image returns.
        //
        gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
      }
      DriverIndex++;
    } while (TRUE);
  }
  FreePool (HandleBuf); 
}


/**
  Register an event notification function for user profile changed.

  @param[in]  ImageHandle     Image handle this driver.

**/
VOID
LoadDeferredImageInit (
  IN EFI_HANDLE        ImageHandle
  )
{
  EFI_STATUS    Status;
  EFI_EVENT     Event;

  mDeferredImageHandle = ImageHandle;
  
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  LoadDeferredImage,
                  NULL,
                  &gEfiEventUserProfileChangedGuid,
                  &Event
                  );

  ASSERT (Status == EFI_SUCCESS);
}
