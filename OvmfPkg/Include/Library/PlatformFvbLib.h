/** @file
  Library to define platform customization functions for a
  Firmare Volume Block driver.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_FVB_LIB__
#define __PLATFORM_FVB_LIB__

#include <Protocol/FirmwareVolumeBlock.h>

/**
  This function will be called following a call to the
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Read function.

  @param[in] This     The EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.
  @param[in] Lba      The starting logical block index
                      from which to read.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes The number of bytes read.
  @param[in] Buffer   Pointer to the buffer that was read, and will be
                      returned to the caller.

**/
VOID
EFIAPI
PlatformFvbDataRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                NumBytes,
  IN        UINT8                                *Buffer
  );

/**
  This function will be called following a call to the
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Write function.

  @param[in] This     EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL instance.
  @param[in] Lba      The starting logical block index to written to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes The number of bytes written.
  @param[in] Buffer   Pointer to the buffer that was written.

**/
VOID
EFIAPI
PlatformFvbDataWritten (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                NumBytes,
  IN        UINT8                                *Buffer
  );

/**
  This function will be called following a call to the
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Erase function.

  @param This   Indicates the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL
                instance.
  @param List   The variable argument list as documented for
                the EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL Erase
                function.

**/
VOID
EFIAPI
PlatformFvbBlocksErased (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN  VA_LIST                                    List
  );

#endif
