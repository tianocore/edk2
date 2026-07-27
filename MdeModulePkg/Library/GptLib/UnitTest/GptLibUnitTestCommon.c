/** @file
  Shared mock disk and GPT construction helpers used by the GptLib
  host-based unit tests.

  Copyright (c) 2026, SUSE LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include "GptLibUnitTestCommon.h"

UINT8  mDiskImage[DISK_IMAGE_SIZE];

EFI_BLOCK_IO_MEDIA     mBlockIoMedia;
EFI_BLOCK_IO_PROTOCOL  mBlockIo;
EFI_DISK_IO_PROTOCOL   mDiskIo;
BOOLEAN                mWriteProtected;

CONST EFI_GUID  mEspTypeGuid = {
  0xC12A7328, 0xF81F, 0x11D2, { 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }
};

CONST EFI_GUID  mPartitionGuid1 = {
  0x11111111, 0x2222, 0x3333, { 0x44, 0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77 }
};

CONST EFI_GUID  mPartitionGuid2 = {
  0x88888888, 0x9999, 0xAAAA, { 0xBB, 0xBB, 0xCC, 0xCC, 0xDD, 0xDD, 0xEE, 0xEE }
};

/**
  Mock implementation of EFI_DISK_IO_PROTOCOL.ReadDisk backed by the in-memory
  disk image.

  @param[in]   This        Pointer to the EFI_DISK_IO_PROTOCOL instance.
  @param[in]   MediaId     ID of the medium to read from.
  @param[in]   Offset      Starting byte offset on the logical disk.
  @param[in]   BufferSize  Number of bytes to read.
  @param[out]  Buffer      Buffer into which the data is read.

  @retval  EFI_SUCCESS       The data was read successfully.
  @retval  EFI_DEVICE_ERROR  The request was invalid or out of range.
**/
STATIC
EFI_STATUS
EFIAPI
MockReadDisk (
  IN  EFI_DISK_IO_PROTOCOL  *This,
  IN  UINT32                MediaId,
  IN  UINT64                Offset,
  IN  UINTN                 BufferSize,
  OUT VOID                  *Buffer
  )
{
  if ((Buffer == NULL) ||
      (MediaId != mBlockIoMedia.MediaId) ||
      (Offset > DISK_IMAGE_SIZE) ||
      (BufferSize > (DISK_IMAGE_SIZE - Offset)))
  {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Buffer, mDiskImage + Offset, BufferSize);
  return EFI_SUCCESS;
}

/**
  Mock implementation of EFI_DISK_IO_PROTOCOL.WriteDisk backed by the in-memory
  disk image, honoring the write-protected flag.

  @param[in]  This        Pointer to the EFI_DISK_IO_PROTOCOL instance.
  @param[in]  MediaId     ID of the medium to write to.
  @param[in]  Offset      Starting byte offset on the logical disk.
  @param[in]  BufferSize  Number of bytes to write.
  @param[in]  Buffer      Buffer holding the data to write.

  @retval  EFI_SUCCESS         The data was written successfully.
  @retval  EFI_WRITE_PROTECTED  The mock medium is write-protected.
  @retval  EFI_DEVICE_ERROR    The request was invalid or out of range.
**/
STATIC
EFI_STATUS
EFIAPI
MockWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  )
{
  if (mWriteProtected) {
    return EFI_WRITE_PROTECTED;
  }

  if ((Buffer == NULL) ||
      (MediaId != mBlockIoMedia.MediaId) ||
      (Offset > DISK_IMAGE_SIZE) ||
      (BufferSize > (DISK_IMAGE_SIZE - Offset)))
  {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (mDiskImage + Offset, Buffer, BufferSize);
  return EFI_SUCCESS;
}

/**
  Return a pointer into the in-memory disk image at the given LBA.

  @param[in]  Lba  Logical block address to locate.

  @return  Pointer to the start of the requested block within the disk image.
**/
UINT8 *
GetDiskLba (
  IN EFI_LBA  Lba
  )
{
  return mDiskImage + (UINTN)(Lba * SECTOR_SIZE);
}

/**
  Return a pointer to the primary GPT header within the in-memory disk image.

  @return  Pointer to the primary EFI_PARTITION_TABLE_HEADER.
**/
EFI_PARTITION_TABLE_HEADER *
GetPrimaryHeader (
  VOID
  )
{
  return (EFI_PARTITION_TABLE_HEADER *)(VOID *)GetDiskLba (PRIMARY_PART_HEADER_LBA);
}

/**
  Recompute and store the header CRC32 for the given GPT header.

  @param[in,out]  Header  The GPT header whose CRC32 field is updated.
**/
VOID
UpdateGptHeaderCrc (
  IN OUT EFI_PARTITION_TABLE_HEADER  *Header
  )
{
  Header->Header.CRC32 = 0;
  Header->Header.CRC32 = CalculateCrc32 (Header, Header->Header.HeaderSize);
}

/**
  Populate a partition entry with the ESP type GUID and the given attributes.

  @param[in,out]  Entry                The partition entry to fill.
  @param[in]      UniquePartitionGuid  Unique partition GUID to assign.
  @param[in]      StartingLba          Starting LBA of the partition.
  @param[in]      EndingLba            Ending LBA of the partition.
**/
VOID
FillPartitionEntry (
  IN OUT EFI_PARTITION_ENTRY  *Entry,
  IN CONST EFI_GUID           *UniquePartitionGuid,
  IN EFI_LBA                  StartingLba,
  IN EFI_LBA                  EndingLba
  )
{
  CopyGuid (&Entry->PartitionTypeGUID, &mEspTypeGuid);
  CopyGuid (&Entry->UniquePartitionGUID, UniquePartitionGuid);
  Entry->StartingLBA = StartingLba;
  Entry->EndingLBA   = EndingLba;
}

/**
  Construct a complete GPT header and partition entry array at the given LBA in
  the in-memory disk image, recalculating all CRCs.

  @param[in]  HeaderLba      LBA at which to write the GPT header.
  @param[in]  AlternateLba   Value stored in the header AlternateLBA field.
  @param[in]  EntryArrayLba  LBA at which to write the partition entry array.
  @param[in]  NumEntries     Number of partition entries in the array.
  @param[in]  EntrySize      Size in bytes of each partition entry.
  @param[in]  NumPartitions  Number of populated partition entries.
**/
VOID
WriteGptTableAt (
  IN EFI_LBA  HeaderLba,
  IN EFI_LBA  AlternateLba,
  IN EFI_LBA  EntryArrayLba,
  IN UINT32   NumEntries,
  IN UINT32   EntrySize,
  IN UINT32   NumPartitions
  )
{
  EFI_PARTITION_TABLE_HEADER  *Header;
  UINT8                       *Array;
  UINT32                      ArraySize;

  ArraySize = NumEntries * EntrySize;
  Array     = GetDiskLba (EntryArrayLba);
  ZeroMem (Array, ArraySize);
  if (NumPartitions >= 1) {
    FillPartitionEntry ((EFI_PARTITION_ENTRY *)(VOID *)Array, &mPartitionGuid1, 34, 133);
  }

  if (NumPartitions >= 2) {
    FillPartitionEntry ((EFI_PARTITION_ENTRY *)(VOID *)(Array + EntrySize), &mPartitionGuid2, 134, 233);
  }

  Header = (EFI_PARTITION_TABLE_HEADER *)(VOID *)GetDiskLba (HeaderLba);
  ZeroMem (Header, SECTOR_SIZE);
  Header->Header.Signature         = EFI_PTAB_HEADER_ID;
  Header->Header.Revision          = TEST_GPT_REVISION_V1;
  Header->Header.HeaderSize        = sizeof (EFI_PARTITION_TABLE_HEADER);
  Header->MyLBA                    = HeaderLba;
  Header->AlternateLBA             = AlternateLba;
  Header->FirstUsableLBA           = FIRST_USABLE_LBA;
  Header->LastUsableLBA            = LAST_USABLE_LBA;
  Header->PartitionEntryLBA        = EntryArrayLba;
  Header->NumberOfPartitionEntries = NumEntries;
  Header->SizeOfPartitionEntry     = EntrySize;
  Header->PartitionEntryArrayCRC32 = CalculateCrc32 (Array, ArraySize);
  UpdateGptHeaderCrc (Header);
}

/**
  Reset the mock disk and protocol state and lay down a valid primary GPT.
**/
VOID
SetupDiskWithValidPrimary (
  VOID
  )
{
  ZeroMem (mDiskImage, sizeof (mDiskImage));

  ZeroMem (&mBlockIoMedia, sizeof (mBlockIoMedia));
  mBlockIoMedia.MediaId   = 1;
  mBlockIoMedia.BlockSize = SECTOR_SIZE;
  mBlockIoMedia.LastBlock = LAST_LBA;

  ZeroMem (&mBlockIo, sizeof (mBlockIo));
  mBlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION;
  mBlockIo.Media    = &mBlockIoMedia;

  ZeroMem (&mDiskIo, sizeof (mDiskIo));
  mDiskIo.Revision  = EFI_DISK_IO_PROTOCOL_REVISION;
  mDiskIo.ReadDisk  = MockReadDisk;
  mDiskIo.WriteDisk = MockWriteDisk;

  mWriteProtected = FALSE;

  WriteGptTableAt (
    PRIMARY_PART_HEADER_LBA,
    LAST_LBA,
    PRIMARY_PART_HEADER_LBA + 1,
    NUM_PARTITION_ENTRIES,
    PARTITION_ENTRY_SIZE,
    2
    );
}

/**
  Invoke PartitionValidGptTable against the mock disk at the given LBA.

  @param[in]   Lba     LBA of the GPT header to validate.
  @param[out]  Header  Buffer to receive the validated GPT header.

  @retval  TRUE   The GPT table at Lba is valid.
  @retval  FALSE  The GPT table at Lba is invalid.
**/
BOOLEAN
ValidateAt (
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *Header
  )
{
  return PartitionValidGptTable (&mBlockIo, &mDiskIo, Lba, Header);
}

/**
  Initialize a GPT header in memory for PartitionCheckGptEntry tests.

  @param[out]  Header      The GPT header to initialize.
  @param[in]   NumEntries  Number of partition entries to record.
  @param[in]   EntrySize   Size in bytes of each partition entry.
**/
VOID
InitCheckEntryHeader (
  OUT EFI_PARTITION_TABLE_HEADER  *Header,
  IN  UINT32                      NumEntries,
  IN  UINT32                      EntrySize
  )
{
  ZeroMem (Header, sizeof (*Header));
  Header->FirstUsableLBA           = FIRST_USABLE_LBA;
  Header->LastUsableLBA            = 200;
  Header->NumberOfPartitionEntries = NumEntries;
  Header->SizeOfPartitionEntry     = EntrySize;
}
