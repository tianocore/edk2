/** @file
  OVMF support for QEMU system firmware flash device

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QEMU_FLASH_H__
#define __QEMU_FLASH_H__

#include <Protocol/FirmwareVolumeBlock.h>

extern UINT8 *mFlashBase;

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
  );


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
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );


/**
  Erase a QEMU Flash block

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
QemuFlashEraseBlock (
  IN   EFI_LBA      Lba
  );


/**
  Initializes QEMU flash memory support

  @retval EFI_WRITE_PROTECTED   The QEMU flash device is not present.
  @retval EFI_SUCCESS           The QEMU flash device is supported.

**/
EFI_STATUS
QemuFlashInitialize (
  VOID
  );


VOID
QemuFlashConvertPointers (
  VOID
  );

VOID
QemuFlashBeforeProbe (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  IN  UINTN                   FdBlockSize,
  IN  UINTN                   FdBlockCount
  );

/**
  Write to QEMU Flash

  @param[in] Ptr    Pointer to the location to write.
  @param[in] Value  The value to write.

**/
VOID
QemuFlashPtrWrite (
  IN        volatile UINT8    *Ptr,
  IN        UINT8             Value
  );

#endif

