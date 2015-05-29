/** @file

 Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

#include <ArmPlatform.h>
#include <Library/NorFlashPlatformLib.h>

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  return EFI_SUCCESS;
}

NOR_FLASH_DESCRIPTION mNorFlashDevices[] = {
  {
    QEMU_NOR0_BASE,
    QEMU_NOR0_BASE,
    QEMU_NOR0_SIZE,
    QEMU_NOR_BLOCK_SIZE,
    {0xF9B94AE2, 0x8BA6, 0x409B, {0x9D, 0x56, 0xB9, 0xB4, 0x17, 0xF5, 0x3C, 0xB3}}
  }, {
    QEMU_NOR1_BASE,
    QEMU_NOR1_BASE,
    QEMU_NOR1_SIZE,
    QEMU_NOR_BLOCK_SIZE,
    {0x8047DB4B, 0x7E9C, 0x4C0C, {0x8E, 0xBC, 0xDF, 0xBB, 0xAA, 0xCA, 0xCE, 0x8F}}
  }
};

EFI_STATUS
NorFlashPlatformGetDevices (
  OUT NOR_FLASH_DESCRIPTION   **NorFlashDescriptions,
  OUT UINT32                  *Count
  )
{
  *NorFlashDescriptions = mNorFlashDevices;
  *Count = sizeof (mNorFlashDevices) / sizeof (mNorFlashDevices[0]);
  return EFI_SUCCESS;
}
