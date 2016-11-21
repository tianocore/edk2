/** @file

  Copyright (c) 2013-2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AndroidFastbootApp.h"

#include <Protocol/DevicePath.h>

#include <Library/BdsLib.h>
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

EFI_STATUS
BootAndroidBootImg (
  IN UINTN    BufferSize,
  IN VOID    *Buffer
  )
{
  EFI_STATUS                          Status;
  CHAR8                               KernelArgs[BOOTIMG_KERNEL_ARGS_SIZE];
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

  Status = BdsStartEfiApplication (gImageHandle,
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
