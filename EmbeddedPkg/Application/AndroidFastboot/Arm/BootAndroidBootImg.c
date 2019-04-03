/** @file

  Copyright (c) 2013-2015, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AndroidFastbootApp.h"

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>

#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// Device Path representing an image in memory
#pragma pack(1)
typedef struct {
  MEMMAP_DEVICE_PATH                      Node1;
  EFI_DEVICE_PATH_PROTOCOL                End;
} MEMORY_DEVICE_PATH;
#pragma pack()

STATIC CONST MEMORY_DEVICE_PATH MemoryDevicePathTemplate =
{
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_MEMMAP_DP,
      {
        (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
        (UINT8)((sizeof (MEMMAP_DEVICE_PATH)) >> 8),
      },
    }, // Header
    0, // StartingAddress (set at runtime)
    0  // EndingAddress   (set at runtime)
  }, // Node1
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  } // End
};


/**
  Start an EFI Application from a Device Path

  @param  ParentImageHandle     Handle of the calling image
  @param  DevicePath            Location of the EFI Application

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
STATIC
EFI_STATUS
StartEfiApplication (
  IN EFI_HANDLE                  ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINTN                       LoadOptionsSize,
  IN VOID*                       LoadOptions
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL*   LoadedImage;

  // Load the image from the device path with Boot Services function
  Status = gBS->LoadImage (TRUE, ParentImageHandle, DevicePath, NULL, 0,
                  &ImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Passed LoadOptions to the EFI Application
  if (LoadOptionsSize != 0) {
    Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid,
                    (VOID **) &LoadedImage);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    LoadedImage->LoadOptionsSize  = LoadOptionsSize;
    LoadedImage->LoadOptions      = LoadOptions;
  }

  // Before calling the image, enable the Watchdog Timer for  the 5 Minute period
  gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);
  // Start the image
  Status = gBS->StartImage (ImageHandle, NULL, NULL);
  // Clear the Watchdog Timer after the image returns
  gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);

  return Status;
}

EFI_STATUS
BootAndroidBootImg (
  IN UINTN    BufferSize,
  IN VOID    *Buffer
  )
{
  EFI_STATUS                          Status;
  CHAR8                               KernelArgs[ANDROID_BOOTIMG_KERNEL_ARGS_SIZE];
  VOID                               *Kernel;
  UINTN                               KernelSize;
  VOID                               *Ramdisk;
  UINTN                               RamdiskSize;
  MEMORY_DEVICE_PATH                  KernelDevicePath;
  CHAR16                              *LoadOptions, *NewLoadOptions;

  Status = ParseAndroidBootImg (
            Buffer,
            &Kernel,
            &KernelSize,
            &Ramdisk,
            &RamdiskSize,
            KernelArgs
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KernelDevicePath = MemoryDevicePathTemplate;

  // Have to cast to UINTN before casting to EFI_PHYSICAL_ADDRESS in order to
  // appease GCC.
  KernelDevicePath.Node1.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Kernel;
  KernelDevicePath.Node1.EndingAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN) Kernel + KernelSize;

  // Initialize Linux command line
  LoadOptions = CatSPrint (NULL, L"%a", KernelArgs);
  if (LoadOptions == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (RamdiskSize != 0) {
    NewLoadOptions = CatSPrint (LoadOptions, L" initrd=0x%x,0x%x",
                       (UINTN)Ramdisk, RamdiskSize);
    FreePool (LoadOptions);
    if (NewLoadOptions == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    LoadOptions = NewLoadOptions;
  }

  Status = StartEfiApplication (gImageHandle,
             (EFI_DEVICE_PATH_PROTOCOL *) &KernelDevicePath,
             StrSize (LoadOptions),
             LoadOptions);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Couldn't Boot Linux: %d\n", Status));
    Status = EFI_DEVICE_ERROR;
    goto FreeLoadOptions;
  }

  // If we got here we do a confused face because BootLinuxFdt returned,
  // reporting success.
  DEBUG ((EFI_D_ERROR, "WARNING: BdsBootLinuxFdt returned EFI_SUCCESS.\n"));
  return EFI_SUCCESS;

FreeLoadOptions:
  FreePool (LoadOptions);
  return Status;
}
