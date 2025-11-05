/** @file

 Copyright (c) 2019, Linaro Ltd. All rights reserved

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <Base.h>
#include <PiDxe.h>
#include <Library/VirtNorFlashPlatformLib.h>

#define QEMU_NOR_BLOCK_SIZE  SIZE_256KB

EFI_STATUS
VirtNorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

VIRT_NOR_FLASH_DESCRIPTION  mNorFlashDevice =
{
  FixedPcdGet32 (PcdOvmfFdBaseAddress),
  FixedPcdGet64 (PcdFlashNvStorageVariableBase),
  FixedPcdGet32 (PcdOvmfFirmwareFdSize) -
  (FixedPcdGet64 (PcdFlashNvStorageVariableBase) - FixedPcdGet32 (PcdOvmfFdBaseAddress)),
  QEMU_NOR_BLOCK_SIZE
};

EFI_STATUS
VirtNorFlashPlatformGetDevices (
  OUT VIRT_NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                      *Count
  )
{
  *NorFlashDescriptions = &mNorFlashDevice;
  *Count                = 1;
  return EFI_SUCCESS;
}
