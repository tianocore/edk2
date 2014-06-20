/** @file

 Copyright (c) 2011-2014, ARM Ltd. All rights reserved.<BR>

 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

#include <PiDxe.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

#define NOR_FLASH_DEVICE_COUNT                     1

// RTSM
NOR_FLASH_DESCRIPTION mNorFlashDevices[NOR_FLASH_DEVICE_COUNT] = {
  { // UEFI
    ARM_EB_SMB_NOR_BASE,
    ARM_EB_SMB_NOR_BASE,
    SIZE_128KB * 512,
    SIZE_128KB,
    {0xE7223039, 0x5836, 0x41E1, { 0xB5, 0x42, 0xD7, 0xEC, 0x73, 0x6C, 0x5E, 0x59 } }
  }
};

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDevices,
  OUT UINT32                  *Count
  )
{
  if ((NorFlashDevices == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *NorFlashDevices = mNorFlashDevices;
  *Count = NOR_FLASH_DEVICE_COUNT;

  return EFI_SUCCESS;
}
