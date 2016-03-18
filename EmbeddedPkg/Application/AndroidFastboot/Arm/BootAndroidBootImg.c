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

#define LINUX_LOADER_COMMAND_LINE       L"%s -f %s -c %s"

// This GUID is defined in the INGF file of ArmPkg/Application/LinuxLoader
CONST EFI_GUID mLinuxLoaderAppGuid = { 0x701f54f2, 0x0d70, 0x4b89, { 0xbc, 0x0a, 0xd9, 0xca, 0x25, 0x37, 0x90, 0x59 }};

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
  MEMORY_DEVICE_PATH*                 RamdiskDevicePath;
  CHAR16*                             KernelDevicePathTxt;
  CHAR16*                             RamdiskDevicePathTxt;
  EFI_DEVICE_PATH*                    LinuxLoaderDevicePath;
  CHAR16*                             LoadOptions;

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

  //
  // Boot Linux using the Legacy Linux Loader
  //

  Status = LocateEfiApplicationInFvByGuid (&mLinuxLoaderAppGuid, &LinuxLoaderDevicePath);
  if (EFI_ERROR (Status)) {
    Print (L"Couldn't Boot Linux: %d\n", Status);
    return EFI_DEVICE_ERROR;
  }

  KernelDevicePathTxt = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL *) &KernelDevicePath, FALSE, FALSE);
  if (KernelDevicePathTxt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RamdiskDevicePathTxt = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL *) RamdiskDevicePath, FALSE, FALSE);
  if (RamdiskDevicePathTxt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize Legacy Linux loader command line
  LoadOptions = CatSPrint (NULL, LINUX_LOADER_COMMAND_LINE, KernelDevicePathTxt, RamdiskDevicePathTxt, KernelArgs);
  if (LoadOptions == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BdsStartEfiApplication (gImageHandle, LinuxLoaderDevicePath, StrSize (LoadOptions), LoadOptions);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Couldn't Boot Linux: %d\n", Status));
    return EFI_DEVICE_ERROR;
  }

  if (RamdiskDevicePath) {
    FreePool (RamdiskDevicePathTxt);
    FreePool (RamdiskDevicePath);
  }

  FreePool (KernelDevicePathTxt);

  // If we got here we do a confused face because BootLinuxFdt returned,
  // reporting success.
  DEBUG ((EFI_D_ERROR, "WARNING: BdsBootLinuxFdt returned EFI_SUCCESS.\n"));
  return EFI_SUCCESS;
}
