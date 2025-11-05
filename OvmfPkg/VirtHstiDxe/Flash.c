/** @file

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "VirtHstiDxe.h"

#define WRITE_BYTE_CMD           0x10
#define BLOCK_ERASE_CMD          0x20
#define CLEAR_STATUS_CMD         0x50
#define READ_STATUS_CMD          0x70
#define READ_DEVID_CMD           0x90
#define BLOCK_ERASE_CONFIRM_CMD  0xd0
#define READ_ARRAY_CMD           0xff
#define CLEARED_ARRAY_STATUS     0x00

/* based on QemuFlashDetected (QemuFlashFvbServicesRuntimeDxe) */
UINT32
VirtHstiQemuFirmwareFlashCheck (
  UINT32  Address
  )
{
  volatile UINT8  *Ptr;

  UINTN  Offset;
  UINT8  OriginalUint8;
  UINT8  ProbeUint8;

  for (Offset = 0; Offset < EFI_PAGE_SIZE; Offset++) {
    Ptr        = (UINT8 *)(UINTN)(Address + Offset);
    ProbeUint8 = *Ptr;
    if ((ProbeUint8 != CLEAR_STATUS_CMD) &&
        (ProbeUint8 != READ_STATUS_CMD) &&
        (ProbeUint8 != CLEARED_ARRAY_STATUS))
    {
      break;
    }
  }

  if (Offset >= EFI_PAGE_SIZE) {
    DEBUG ((DEBUG_INFO, "%a: check failed\n", __func__));
    return QEMU_FIRMWARE_FLASH_UNKNOWN;
  }

  OriginalUint8 = *Ptr;
  *Ptr          = CLEAR_STATUS_CMD;
  ProbeUint8    = *Ptr;
  if ((OriginalUint8 != CLEAR_STATUS_CMD) &&
      (ProbeUint8 == CLEAR_STATUS_CMD))
  {
    *Ptr = OriginalUint8;
    DEBUG ((DEBUG_INFO, "%a: %p behaves as RAM\n", __func__, Ptr));
    return QEMU_FIRMWARE_FLASH_IS_RAM;
  }

  *Ptr       = READ_STATUS_CMD;
  ProbeUint8 = *Ptr;
  if (ProbeUint8 == OriginalUint8) {
    DEBUG ((DEBUG_INFO, "%a: %p behaves as ROM\n", __func__, Ptr));
    return QEMU_FIRMWARE_FLASH_IS_ROM;
  }

  if (ProbeUint8 == READ_STATUS_CMD) {
    *Ptr = OriginalUint8;
    DEBUG ((DEBUG_INFO, "%a: %p behaves as RAM\n", __func__, Ptr));
    return QEMU_FIRMWARE_FLASH_IS_RAM;
  }

  if (ProbeUint8 == CLEARED_ARRAY_STATUS) {
    *Ptr       = WRITE_BYTE_CMD;
    *Ptr       = OriginalUint8;
    *Ptr       = READ_STATUS_CMD;
    ProbeUint8 = *Ptr;
    *Ptr       = READ_ARRAY_CMD;
    if (ProbeUint8 & 0x10 /* programming error */) {
      DEBUG ((DEBUG_INFO, "%a: %p behaves as FLASH, write-protected\n", __func__, Ptr));
      return QEMU_FIRMWARE_FLASH_READ_ONLY;
    } else {
      DEBUG ((DEBUG_INFO, "%a: %p behaves as FLASH, writable\n", __func__, Ptr));
      return QEMU_FIRMWARE_FLASH_WRITABLE;
    }
  }

  DEBUG ((DEBUG_INFO, "%a: check failed\n", __func__));
  return QEMU_FIRMWARE_FLASH_UNKNOWN;
}
