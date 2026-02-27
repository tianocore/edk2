/** @file

 Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#pragma once

typedef struct {
  UINTN    DeviceBaseAddress;       // Start address of the Device Base Address (DBA)
  UINTN    RegionBaseAddress;       // Start address of one single region
  UINTN    Size;
  UINTN    BlockSize;
} VIRT_NOR_FLASH_DESCRIPTION;

EFI_STATUS
VirtNorFlashPlatformInitialization (
  VOID
  );

EFI_STATUS
VirtNorFlashPlatformGetDevices (
  OUT VIRT_NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                      *Count
  );
