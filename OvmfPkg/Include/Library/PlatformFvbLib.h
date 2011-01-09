/** @file
  Library to define platform customization functions for a
  Firmare Volume Block driver.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN        UINTN                               NumBytes,
  IN        UINT8                               *Buffer
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
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL *This,
  IN  VA_LIST       List
  );


#endif

