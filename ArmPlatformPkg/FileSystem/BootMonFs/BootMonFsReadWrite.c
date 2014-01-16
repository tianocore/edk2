/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include "BootMonFsInternal.h"

EFIAPI
EFI_STATUS
BootMonFsReadFile (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  )
{
  BOOTMON_FS_INSTANCE   *Instance;
  BOOTMON_FS_FILE       *File;
  EFI_DISK_IO_PROTOCOL  *DiskIo;
  EFI_BLOCK_IO_MEDIA    *Media;
  UINT64                FileStart;
  EFI_STATUS            Status;
  UINTN                 RemainingFileSize;

  // Ensure the file has been written in Flash before reading it.
  // This keeps the code simple and avoids having to manage a non-flushed file.
  BootMonFsFlushFile (This);

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = File->Instance;
  DiskIo = Instance->DiskIo;
  Media = Instance->Media;
  FileStart = (Media->LowestAlignedLba + File->HwDescription.BlockStart) * Media->BlockSize;

  if (File->Position >= File->HwDescription.Region[0].Size) {
    // The entire file has been read
    *BufferSize = 0;
    return EFI_DEVICE_ERROR;
  }

  // This driver assumes that the entire file is in region 0.
  RemainingFileSize = File->HwDescription.Region[0].Size - File->Position;

  // If read would go past end of file, truncate the read
  if (*BufferSize > RemainingFileSize) {
    *BufferSize = RemainingFileSize;
  }

  Status = DiskIo->ReadDisk (
                    DiskIo,
                    Media->MediaId,
                    FileStart + File->Position,
                    *BufferSize,
                    Buffer
                    );
  if (EFI_ERROR (Status)) {
    *BufferSize = 0;
  }

  File->Position += *BufferSize;

  return Status;
}

// Inserts an entry into the write chain
EFIAPI
EFI_STATUS
BootMonFsWriteFile (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  )
{
  BOOTMON_FS_FILE         *File;
  BOOTMON_FS_FILE_REGION  *Region;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!(File->OpenMode & EFI_FILE_MODE_WRITE)) {
    return EFI_ACCESS_DENIED;
  }

  // Allocate and initialize the memory region
  Region = (BOOTMON_FS_FILE_REGION*)AllocateZeroPool (sizeof (BOOTMON_FS_FILE_REGION));
  if (Region == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Region->Buffer  = AllocateCopyPool (*BufferSize, Buffer);
  if (Region->Buffer == NULL) {
    FreePool (Region);
    return EFI_OUT_OF_RESOURCES;
  }

  Region->Size    = *BufferSize;

  Region->Offset = File->Position;

  InsertTailList (&File->RegionToFlushLink, &Region->Link);

  File->Position += *BufferSize;

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
BootMonFsSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  )
{
  BOOTMON_FS_FILE         *File;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);

  // UEFI Spec section 12.5:
  // "Seeking to position 0xFFFFFFFFFFFFFFFF causes the current position to
  //  be set to the end of the file."
  if (Position == 0xFFFFFFFFFFFFFFFF) {
    File->Position = BootMonFsGetImageLength (File);
  } else {
    // NB: Seeking past the end of the file is valid.
    File->Position = Position;
  }

  return EFI_SUCCESS;
}

EFIAPI
EFI_STATUS
BootMonFsGetPosition (
  IN  EFI_FILE_PROTOCOL *This,
  OUT UINT64            *Position
  ) {
  BOOTMON_FS_FILE         *File;

  File = BOOTMON_FS_FILE_FROM_FILE_THIS (This);

  *Position = File->Position;
  return EFI_SUCCESS;
}
