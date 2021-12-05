/** @file  NorFlashBlockIoDxe.c

  Copyright (c) 2011-2013, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "NorFlash.h"

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.Reset
//
EFI_STATUS
EFIAPI
NorFlashBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  NOR_FLASH_INSTANCE  *Instance;

  Instance = INSTANCE_FROM_BLKIO_THIS (This);

  DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoReset(MediaId=0x%x)\n", This->Media->MediaId));

  return NorFlashReset (Instance);
}

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.ReadBlocks
//
EFI_STATUS
EFIAPI
NorFlashBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSizeInBytes,
  OUT VOID                   *Buffer
  )
{
  NOR_FLASH_INSTANCE  *Instance;
  EFI_STATUS          Status;
  EFI_BLOCK_IO_MEDIA  *Media;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = INSTANCE_FROM_BLKIO_THIS (This);
  Media    = This->Media;

  DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoReadBlocks(MediaId=0x%x, Lba=%ld, BufferSize=0x%x bytes (%d kB), BufferPtr @ 0x%08x)\n", MediaId, Lba, BufferSizeInBytes, Buffer));

  if (!Media) {
    Status = EFI_INVALID_PARAMETER;
  } else if (!Media->MediaPresent) {
    Status = EFI_NO_MEDIA;
  } else if (Media->MediaId != MediaId) {
    Status = EFI_MEDIA_CHANGED;
  } else if ((Media->IoAlign > 2) && (((UINTN)Buffer & (Media->IoAlign - 1)) != 0)) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = NorFlashReadBlocks (Instance, Lba, BufferSizeInBytes, Buffer);
  }

  return Status;
}

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.WriteBlocks
//
EFI_STATUS
EFIAPI
NorFlashBlockIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSizeInBytes,
  IN  VOID                   *Buffer
  )
{
  NOR_FLASH_INSTANCE  *Instance;
  EFI_STATUS          Status;

  Instance = INSTANCE_FROM_BLKIO_THIS (This);

  DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoWriteBlocks(MediaId=0x%x, Lba=%ld, BufferSize=0x%x bytes (%d kB), BufferPtr @ 0x%08x)\n", MediaId, Lba, BufferSizeInBytes, Buffer));

  if ( !This->Media->MediaPresent ) {
    Status = EFI_NO_MEDIA;
  } else if ( This->Media->MediaId != MediaId ) {
    Status = EFI_MEDIA_CHANGED;
  } else if ( This->Media->ReadOnly ) {
    Status = EFI_WRITE_PROTECTED;
  } else {
    Status = NorFlashWriteBlocks (Instance, Lba, BufferSizeInBytes, Buffer);
  }

  return Status;
}

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.FlushBlocks
//
EFI_STATUS
EFIAPI
NorFlashBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  // No Flush required for the NOR Flash driver
  // because cache operations are not permitted.

  DEBUG ((DEBUG_BLKIO, "NorFlashBlockIoFlushBlocks: Function NOT IMPLEMENTED (not required).\n"));

  // Nothing to do so just return without error
  return EFI_SUCCESS;
}
