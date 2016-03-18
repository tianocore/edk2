/** @file

 Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

#ifndef _NORFLASHPLATFORMLIB_H_
#define _NORFLASHPLATFORMLIB_H_

typedef struct {
  UINTN       DeviceBaseAddress;    // Start address of the Device Base Address (DBA)
  UINTN       RegionBaseAddress;    // Start address of one single region
  UINTN       Size;
  UINTN       BlockSize;
  EFI_GUID    Guid;
} NOR_FLASH_DESCRIPTION;

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  );

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDescriptions,
  OUT UINT32                  *Count
  );

#endif /* _NORFLASHPLATFORMLIB_H_ */
