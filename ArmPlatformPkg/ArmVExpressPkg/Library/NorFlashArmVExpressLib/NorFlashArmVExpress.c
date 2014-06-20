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
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NorFlashPlatformLib.h>
#include <ArmPlatform.h>

#define NOR_FLASH_DEVICE_COUNT                     4

NOR_FLASH_DESCRIPTION mNorFlashDevices[NOR_FLASH_DEVICE_COUNT] = {
  { // BootMon
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE,
    SIZE_256KB * 255,
    SIZE_256KB,
    { 0xE7223039, 0x5836, 0x41E1, { 0xB5, 0x42, 0xD7, 0xEC, 0x73, 0x6C, 0x5E, 0x59 } }
  },
  { // BootMon non-volatile storage
    ARM_VE_SMB_NOR0_BASE,
    ARM_VE_SMB_NOR0_BASE + SIZE_256KB * 255,
    SIZE_64KB * 4,
    SIZE_64KB,
    { 0x02118005, 0x9DA7, 0x443A, { 0x92, 0xD5, 0x78, 0x1F, 0x02, 0x2A, 0xED, 0xBB } }
  },
  { // UEFI
    ARM_VE_SMB_NOR1_BASE,
    ARM_VE_SMB_NOR1_BASE,
    SIZE_256KB * 255,
    SIZE_256KB,
    { 0x1F15DA3C, 0x37FF, 0x4070, { 0xB4, 0x71, 0xBB, 0x4A, 0xF1, 0x2A, 0x72, 0x4A } }
  },
  { // UEFI Variable Services non-volatile storage
    ARM_VE_SMB_NOR1_BASE,
    ARM_VE_SMB_NOR1_BASE + SIZE_256KB * 255,
    SIZE_64KB * 3, //FIXME: Set 3 blocks because I did not succeed to copy 4 blocks into the ARM Versatile Express NOR Flash in the last NOR Flash. It should be 4 blocks
    SIZE_64KB,
    { 0xCC2CBF29, 0x1498, 0x4CDD, { 0x81, 0x71, 0xF8, 0xB6, 0xB4, 0x1D, 0x09, 0x09 } }
  }
};

EFI_STATUS
NorFlashPlatformInitialization (
  VOID
  )
{
  // Everything seems ok so far, so now we need to disable the platform-specific
  // flash write protection for Versatile Express
  if ((MmioRead32 (ARM_VE_SYS_FLASH) & 0x1) == 0) {
    // Writing to NOR FLASH is disabled, so enable it
    MmioWrite32 (ARM_VE_SYS_FLASH,1);
    DEBUG((DEBUG_BLKIO, "NorFlashWriteBlocks: informational - Had to enable HSYS_FLASH flag.\n" ));
  }

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
