/** @file

  Copyright (c) 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiLib.h>
#include <Library/VirtioMmioDeviceLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define ARM_FVP_BASE_VIRTIO_BLOCK_BASE    0x1c130000

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} VIRTIO_BLK_DEVICE_PATH;
#pragma pack()

VIRTIO_BLK_DEVICE_PATH mVirtioBlockDevicePath =
{
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)( sizeof(VENDOR_DEVICE_PATH) ),
        (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_CALLER_ID_GUID,
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      sizeof (EFI_DEVICE_PATH_PROTOCOL),
      0
    }
  }
};

EFI_STATUS
EFIAPI
ArmFvpInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = gBS->InstallProtocolInterface (&ImageHandle,
                 &gEfiDevicePathProtocolGuid, EFI_NATIVE_INTERFACE,
                 &mVirtioBlockDevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Declare the Virtio BlockIo device
  Status = VirtioMmioInstallDevice (ARM_FVP_BASE_VIRTIO_BLOCK_BASE, ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmFvpDxe: Failed to install Virtio block device\n"));
  }

  return Status;
}
