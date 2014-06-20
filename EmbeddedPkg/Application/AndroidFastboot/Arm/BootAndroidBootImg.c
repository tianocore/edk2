/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

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

#include <Guid/ArmGlobalVariableHob.h>

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
  EFI_DEVICE_PATH_PROTOCOL           *FdtDevicePath;
  EFI_STATUS                          Status;
  CHAR8                               KernelArgs[BOOTIMG_KERNEL_ARGS_SIZE];
  VOID                               *Kernel;
  UINTN                               KernelSize;
  VOID                               *Ramdisk;
  UINTN                               RamdiskSize;
  MEMORY_DEVICE_PATH                  KernelDevicePath;
  MEMORY_DEVICE_PATH*                 RamdiskDevicePath;

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

  RamdiskDevicePath = NULL;
  if (RamdiskSize != 0) {
    RamdiskDevicePath = (MEMORY_DEVICE_PATH*)DuplicateDevicePath ((EFI_DEVICE_PATH_PROTOCOL*) &MemoryDevicePathTemplate);

    RamdiskDevicePath->Node1.StartingAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) Ramdisk;
    RamdiskDevicePath->Node1.EndingAddress   = ((EFI_PHYSICAL_ADDRESS)(UINTN) Ramdisk) + RamdiskSize;
  }

  // Get the default FDT device path
  Status = GetEnvironmentVariable ((CHAR16 *)L"Fdt", &gArmGlobalVariableGuid,
             NULL, 0, (VOID **)&FdtDevicePath);
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((EFI_D_ERROR, "Error: Please update FDT path in boot manager\n"));
    return EFI_DEVICE_ERROR;
  }
  ASSERT_EFI_ERROR (Status);

  Status = BdsBootLinuxFdt (
              (EFI_DEVICE_PATH_PROTOCOL *) &KernelDevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) RamdiskDevicePath,
              KernelArgs,
              FdtDevicePath
              );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Couldn't Boot Linux: %d\n", Status));
    return EFI_DEVICE_ERROR;
  }

  if (RamdiskDevicePath) {
    FreePool (RamdiskDevicePath);
  }

  FreePool (FdtDevicePath);

  // If we got here we do a confused face because BootLinuxFdt returned,
  // reporting success.
  DEBUG ((EFI_D_ERROR, "WARNING: BdsBootLinuxFdt returned EFI_SUCCESS.\n"));
  return EFI_SUCCESS;
}
