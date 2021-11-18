/** @file

 Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef _NORFLASHPLATFORMLIB_H_
#define _NORFLASHPLATFORMLIB_H_

typedef struct {
  UINTN    DeviceBaseAddress;       // Start address of the Device Base Address (DBA)
  UINTN    RegionBaseAddress;       // Start address of one single region
  UINTN    Size;
  UINTN    BlockSize;
} NOR_FLASH_DESCRIPTION;

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  );

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION  **NorFlashDescriptions,
  OUT UINT32                 *Count
  );

#endif /* _NORFLASHPLATFORMLIB_H_ */
