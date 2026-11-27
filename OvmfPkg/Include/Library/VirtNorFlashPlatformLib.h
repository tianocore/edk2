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

/**
  Return the NOR flash devices exposed by the platform.

  @param[out] NorFlashDescriptions  Pointer to the platform NOR flash device
                                    description array.
  @param[out] Count                 Number of entries in NorFlashDescriptions.

  @retval EFI_SUCCESS            The NOR flash device descriptions were
                                 returned successfully.
  @retval EFI_INVALID_PARAMETER  NorFlashDescriptions or Count is NULL.
  @retval EFI_NOT_FOUND          No NOR flash devices are available.
  @retval Others                 The platform failed to retrieve the NOR flash
                                 device descriptions.
**/
EFI_STATUS
VirtNorFlashPlatformGetDevices (
  OUT VIRT_NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                      *Count
  );
