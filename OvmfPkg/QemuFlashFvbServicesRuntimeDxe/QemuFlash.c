/** @file
  OVMF support for QEMU system firmware flash device

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiDxe.h"
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Guid/EventGroup.h>

#include "QemuFlash.h"

#define WRITE_BYTE_CMD           0x10
#define BLOCK_ERASE_CMD          0x20
#define CLEAR_STATUS_CMD         0x50
#define READ_STATUS_CMD          0x70
#define READ_DEVID_CMD           0x90
#define BLOCK_ERASE_CONFIRM_CMD  0xd0
#define READ_ARRAY_CMD           0xff

#define CLEARED_ARRAY_STATUS  0x00


STATIC UINT8       *mFlashBase = NULL;
STATIC UINTN       mFdBlockSize = 0;
STATIC UINTN       mFdBlockCount = 0;


VOID
QemuFlashConvertPointers (
  VOID
  )
{
  EfiConvertPointer (0x0, (VOID **) &mFlashBase);
}


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
    DEBUG ((EFI_D_INFO, "QEMU Flash: Failed to find probe location\n"));
    return FALSE;
  }

  DEBUG ((EFI_D_INFO, "QEMU Flash: Attempting flash detection at %p\n", Ptr));

  OriginalUint8 = *Ptr;
  *Ptr = CLEAR_STATUS_CMD;
  ProbeUint8 = *Ptr;
  if (OriginalUint8 != CLEAR_STATUS_CMD &&
      ProbeUint8 == CLEAR_STATUS_CMD) {
    DEBUG ((EFI_D_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
    *Ptr = OriginalUint8;
  } else {
    *Ptr = READ_STATUS_CMD;
    ProbeUint8 = *Ptr;
    if (ProbeUint8 == OriginalUint8) {
      DEBUG ((EFI_D_INFO, "QemuFlashDetected => FD behaves as ROM\n"));
    } else if (ProbeUint8 == READ_STATUS_CMD) {
      DEBUG ((EFI_D_INFO, "QemuFlashDetected => FD behaves as RAM\n"));
      *Ptr = OriginalUint8;
    } else if (ProbeUint8 == CLEARED_ARRAY_STATUS) {
      DEBUG ((EFI_D_INFO, "QemuFlashDetected => FD behaves as FLASH\n"));
      FlashDetected = TRUE;
      *Ptr = READ_ARRAY_CMD;
    }
  }

  DEBUG ((EFI_D_INFO, "QemuFlashDetected => %a\n",
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
    *Ptr = WRITE_BYTE_CMD;
    *Ptr = Buffer[Loop];
    Ptr++;
  }

  //
  // Restore flash to read mode
  //
  if (*NumBytes > 0) {
    *(Ptr - 1) = READ_ARRAY_CMD;
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
  *Ptr = BLOCK_ERASE_CMD;
  *Ptr = BLOCK_ERASE_CONFIRM_CMD;
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

  if (!QemuFlashDetected ()) {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_SUCCESS;
}

