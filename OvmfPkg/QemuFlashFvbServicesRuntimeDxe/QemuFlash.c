/** @file
  OVMF support for QEMU system firmware flash device

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>

#include "QemuFlash.h"

#define WRITE_BYTE_CMD           0x10
#define BLOCK_ERASE_CMD          0x20
#define CLEAR_STATUS_CMD         0x50
#define READ_STATUS_CMD          0x70
#define READ_DEVID_CMD           0x90
#define BLOCK_ERASE_CONFIRM_CMD  0xd0
#define READ_ARRAY_CMD           0xff

#define CLEARED_ARRAY_STATUS  0x00


UINT8 *mFlashBase;

STATIC UINTN       mFdBlockSize = 0;
STATIC UINTN       mFdBlockCount = 0;

STATIC
volatile UINT8*
QemuFlashPtr (
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset
  )
{
  return mFlashBase + ((UINTN)Lba * mFdBlockSize) + Offset;
}


/**
  Determines if the QEMU flash memory device is present.

  @retval FALSE   The QEMU flash device is not present.
  @retval TRUE    The QEMU flash device is present.

**/
STATIC
BOOLEAN
QemuFlashDetected (
  VOID
  )
{
  BOOLEAN  FlashDetected;
  volatile UINT8  *Ptr;

  UINTN Offset;
  UINT8 OriginalUint8;
  UINT8 ProbeUint8;

  FlashDetected = FALSE;
  Ptr = QemuFlashPtr (0, 0);

  for (Offset = 0; Offset < mFdBlockSize; Offset++) {
    Ptr = QemuFlashPtr (0, Offset);
    ProbeUint8 = *Ptr;
    if (ProbeUint8 != CLEAR_STATUS_CMD &&
        ProbeUint8 != READ_STATUS_CMD &&
        ProbeUint8 != CLEARED_ARRAY_STATUS) {
      break;
    }
  }

  if (Offset >= mFdBlockSize) {
    DEBUG ((DEBUG_INFO, "QEMU Flash: Failed to find probe location\n"));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "QEMU Flash: Attempting flash detection at %p\n", Ptr));

  if (MemEncryptSevEsIsEnabled ()) {
    //
    // When SEV-ES is enabled, the check below can result in an infinite
    // loop with respect to a nested page fault. When the memslot is mapped
    // read-only, the nested page table entry is read-only. The check below
    // will cause a nested page fault that cannot be emulated, causing
    // the instruction to retried over and over. For SEV-ES, acknowledge that
    // the FD appears as ROM and not as FLASH, but report FLASH anyway because
    // FLASH behavior can be simulated using VMGEXIT.
    //
    DEBUG ((DEBUG_INFO,
      "QEMU Flash: SEV-ES enabled, assuming FD behaves as FLASH\n"));
    return TRUE;
  }

  OriginalUint8 = *Ptr;
  *Ptr = CLEAR_STATUS_CMD;
  ProbeUint8 = *Ptr;
  if (OriginalUint8 != CLEAR_STATUS_CMD &&
      ProbeUint8 == CLEAR_STATUS_CMD) {
    DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
    *Ptr = OriginalUint8;
  } else {
    *Ptr = READ_STATUS_CMD;
    ProbeUint8 = *Ptr;
    if (ProbeUint8 == OriginalUint8) {
      DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as ROM\n"));
    } else if (ProbeUint8 == READ_STATUS_CMD) {
      DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
      *Ptr = OriginalUint8;
    } else if (ProbeUint8 == CLEARED_ARRAY_STATUS) {
      DEBUG ((DEBUG_INFO, "QemuFlashDetected => FD behaves as FLASH\n"));
      FlashDetected = TRUE;
      *Ptr = READ_ARRAY_CMD;
    }
  }

  DEBUG ((DEBUG_INFO, "QemuFlashDetected => %a\n",
                      FlashDetected ? "Yes" : "No"));
  return FlashDetected;
}


/**
  Read from QEMU Flash

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
QemuFlashRead (
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  )
{
  UINT8  *Ptr;

  //
  // Only write to the first 64k. We don't bother saving the FTW Spare
  // block into the flash memory.
  //
  if (Lba >= mFdBlockCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get flash address
  //
  Ptr = (UINT8*) QemuFlashPtr (Lba, Offset);

  CopyMem (Buffer, Ptr, *NumBytes);

  return EFI_SUCCESS;
}


/**
  Write to QEMU Flash

  @param[in] Lba      The starting logical block index to write to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes On input, indicates the requested write size. On
                      output, indicates the actual number of bytes written
  @param[in] Buffer   Pointer to the data to write.

**/
EFI_STATUS
QemuFlashWrite (
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN        UINTN                               *NumBytes,
  IN        UINT8                               *Buffer
  )
{
  volatile UINT8  *Ptr;
  UINTN           Loop;

  //
  // Only write to the first 64k. We don't bother saving the FTW Spare
  // block into the flash memory.
  //
  if (Lba >= mFdBlockCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Program flash
  //
  Ptr = QemuFlashPtr (Lba, Offset);
  for (Loop = 0; Loop < *NumBytes; Loop++) {
    QemuFlashPtrWrite (Ptr, WRITE_BYTE_CMD);
    QemuFlashPtrWrite (Ptr, Buffer[Loop]);

    Ptr++;
  }

  //
  // Restore flash to read mode
  //
  if (*NumBytes > 0) {
    QemuFlashPtrWrite (Ptr - 1, READ_ARRAY_CMD);
  }

  return EFI_SUCCESS;
}


/**
  Erase a QEMU Flash block

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
QemuFlashEraseBlock (
  IN   EFI_LBA      Lba
  )
{
  volatile UINT8  *Ptr;

  if (Lba >= mFdBlockCount) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr = QemuFlashPtr (Lba, 0);
  QemuFlashPtrWrite (Ptr, BLOCK_ERASE_CMD);
  QemuFlashPtrWrite (Ptr, BLOCK_ERASE_CONFIRM_CMD);
  return EFI_SUCCESS;
}


/**
  Initializes QEMU flash memory support

  @retval EFI_WRITE_PROTECTED   The QEMU flash device is not present.
  @retval EFI_SUCCESS           The QEMU flash device is supported.

**/
EFI_STATUS
QemuFlashInitialize (
  VOID
  )
{
  mFlashBase = (UINT8*)(UINTN) PcdGet32 (PcdOvmfFdBaseAddress);
  mFdBlockSize = PcdGet32 (PcdOvmfFirmwareBlockSize);
  ASSERT(PcdGet32 (PcdOvmfFirmwareFdSize) % mFdBlockSize == 0);
  mFdBlockCount = PcdGet32 (PcdOvmfFirmwareFdSize) / mFdBlockSize;

  //
  // execute module specific hooks before probing the flash
  //
  QemuFlashBeforeProbe (
    (EFI_PHYSICAL_ADDRESS)(UINTN) mFlashBase,
    mFdBlockSize,
    mFdBlockCount
    );

  if (!QemuFlashDetected ()) {
    ASSERT (!FeaturePcdGet (PcdSmmSmramRequire));
    return EFI_WRITE_PROTECTED;
  }

  return EFI_SUCCESS;
}

