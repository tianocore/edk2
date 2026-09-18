/** @file
  Decode a hard disk partitioned with the GPT scheme in the UEFI 2.0
  specification.

  Caution: This file requires additional review when modified.
  This driver will have external input - disk partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  PartitionValidGptTable(), PartitionCheckGptEntry() routine will accept disk
  partition content and validate the GPT table and GPT entry.

Copyright (c) 2026, SUSE LLC. All rights reserved.<BR>
Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/GptLib.h>

//
// The only GPT header revision defined by the UEFI specification.
//
#define GPT_HEADER_REVISION_V1  0x00010000

//
// Minimum on-disk GPT header size: large enough to cover every defined field
// through PartitionEntryArrayCRC32 (92 bytes). sizeof (EFI_PARTITION_TABLE_HEADER)
// cannot be used because the C structure is padded to an 8-byte boundary.
//
#define GPT_HEADER_MIN_SIZE  (OFFSET_OF (EFI_PARTITION_TABLE_HEADER, PartitionEntryArrayCRC32) + \
                              sizeof (((EFI_PARTITION_TABLE_HEADER *)0)->PartitionEntryArrayCRC32))

/**
  Check if the CRC field in the Partition table header is valid
  for Partition entry array.

  @param[in]  BlockIo     Parent BlockIo interface
  @param[in]  DiskIo      Disk Io Protocol.
  @param[in]  PartHeader  Partition table header structure

  @retval TRUE      the CRC is valid
  @retval FALSE     the CRC is invalid

**/
STATIC
BOOLEAN
PartitionCheckGptEntryArrayCRC (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

/**
  Checks the CRC32 value in the table header.

  @param  MaxSize   Max Size limit
  @param  Size      The size of the table
  @param  Hdr       Table to check

  @return TRUE    CRC Valid
  @return FALSE   CRC Invalid

**/
STATIC
BOOLEAN
PartitionCheckCrcAltSize (
  IN UINTN                 MaxSize,
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

/**
  Checks the CRC32 value in the table header.

  @param  MaxSize   Max Size limit
  @param  Hdr       Table to check

  @return TRUE      CRC Valid
  @return FALSE     CRC Invalid

**/
STATIC
BOOLEAN
PartitionCheckCrc (
  IN UINTN                 MaxSize,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

/**
  Updates the CRC32 value in the table header.

  @param  Size   The size of the table
  @param  Hdr    Table to update

**/
STATIC
VOID
PartitionSetCrcAltSize (
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

/**
  Updates the CRC32 value in the table header.

  @param  Hdr    Table to update

**/
STATIC
VOID
PartitionSetCrc (
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

/**
  This routine will read GPT partition table header and return it.

  Caution: This function may receive untrusted input.
  The GPT partition table header is external input, so this routine
  will do basic validation for GPT partition table header before return.

  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  DiskIo      Disk Io protocol.
  @param[in]  Lba         The starting Lba of the Partition Table
  @param[out] PartHeader  Stores the partition table that is read

  @retval TRUE      The partition table is valid
  @retval FALSE     The partition table is not valid

**/
BOOLEAN
PartitionValidGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
{
  EFI_STATUS                  Status;
  UINT32                      BlockSize;
  EFI_PARTITION_TABLE_HEADER  *PartHdr;
  UINT32                      MediaId;

  BlockSize = BlockIo->Media->BlockSize;
  MediaId   = BlockIo->Media->MediaId;
  PartHdr   = AllocateZeroPool (BlockSize);

  if (PartHdr == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate pool error\n"));
    return FALSE;
  }

  //
  // Read the EFI Partition Table Header
  //
  Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (Lba, BlockSize),
                     BlockSize,
                     PartHdr
                     );
  if (EFI_ERROR (Status)) {
    FreePool (PartHdr);
    return FALSE;
  }

  //
  // Validate the header fields against the constraints the UEFI specification
  // places on a GPT header. SizeOfPartitionEntry must be 128 * 2^n. Note this
  // routine validates both the primary and the backup header, so it must not
  // assume the entry array sits before FirstUsableLBA: that holds for the
  // primary but not for the backup, whose array follows the usable region.
  //
  if ((PartHdr->Header.Signature != EFI_PTAB_HEADER_ID) ||
      (PartHdr->Header.Revision != GPT_HEADER_REVISION_V1) ||
      (PartHdr->Header.HeaderSize < GPT_HEADER_MIN_SIZE) ||
      !PartitionCheckCrc (BlockSize, &PartHdr->Header) ||
      (PartHdr->MyLBA != Lba) ||
      (PartHdr->NumberOfPartitionEntries == 0) ||
      (PartHdr->SizeOfPartitionEntry < sizeof (EFI_PARTITION_ENTRY)) ||
      ((PartHdr->SizeOfPartitionEntry & (PartHdr->SizeOfPartitionEntry - 1)) != 0)
      )
  {
    DEBUG ((DEBUG_INFO, "Invalid efi partition table header\n"));
    FreePool (PartHdr);
    return FALSE;
  }

  //
  // Ensure PartitionEntryLBA * BlockSize and
  // NumberOfPartitionEntries * SizeOfPartitionEntry don't overflow when they
  // are later used to read and size the partition entry array. Both entry
  // fields are UINT32, so the product is evaluated in 32-bit arithmetic and
  // must be bounded by MAX_UINT32 to avoid truncation before it is widened
  // to UINTN.
  //
  if ((PartHdr->PartitionEntryLBA > DivU64x32 (MAX_UINT64, BlockSize)) ||
      (PartHdr->NumberOfPartitionEntries > DivU64x32 (MAX_UINT32, PartHdr->SizeOfPartitionEntry)))
  {
    FreePool (PartHdr);
    return FALSE;
  }

  CopyMem (PartHeader, PartHdr, sizeof (EFI_PARTITION_TABLE_HEADER));
  if (!PartitionCheckGptEntryArrayCRC (BlockIo, DiskIo, PartHeader)) {
    FreePool (PartHdr);
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, " Valid efi partition table header\n"));
  FreePool (PartHdr);
  return TRUE;
}

/**
  Check if the CRC field in the Partition table header is valid
  for Partition entry array.

  @param[in]  BlockIo     Parent BlockIo interface
  @param[in]  DiskIo      Disk Io Protocol.
  @param[in]  PartHeader  Partition table header structure

  @retval TRUE      the CRC is valid
  @retval FALSE     the CRC is invalid

**/
STATIC
BOOLEAN
PartitionCheckGptEntryArrayCRC (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
{
  EFI_STATUS  Status;
  UINT8       *Ptr;
  UINT32      Crc;
  UINTN       Size;

  //
  // Read the EFI Partition Entries
  //
  Ptr = AllocatePool (PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry);
  if (Ptr == NULL) {
    DEBUG ((DEBUG_ERROR, " Allocate pool error\n"));
    return FALSE;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     BlockIo->Media->MediaId,
                     MultU64x32 (PartHeader->PartitionEntryLBA, BlockIo->Media->BlockSize),
                     PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                     Ptr
                     );
  if (EFI_ERROR (Status)) {
    FreePool (Ptr);
    return FALSE;
  }

  Size = PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry;

  Status = gBS->CalculateCrc32 (Ptr, Size, &Crc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CheckPEntryArrayCRC: Crc calculation failed\n"));
    FreePool (Ptr);
    return FALSE;
  }

  FreePool (Ptr);

  return (BOOLEAN)(PartHeader->PartitionEntryArrayCRC32 == Crc);
}

/**
  Restore Partition Table to its alternate place
  (Primary -> Backup or Backup -> Primary).

  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  DiskIo      Disk Io Protocol.
  @param[in]  PartHeader  Partition table header structure.

  @retval TRUE      Restoring succeeds
  @retval FALSE     Restoring failed

**/
BOOLEAN
PartitionRestoreGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
{
  EFI_STATUS                  Status;
  UINTN                       BlockSize;
  EFI_PARTITION_TABLE_HEADER  *PartHdr;
  EFI_LBA                     PEntryLBA;
  UINT8                       *Ptr;
  UINT32                      MediaId;

  PartHdr = NULL;
  Ptr     = NULL;

  BlockSize = BlockIo->Media->BlockSize;
  MediaId   = BlockIo->Media->MediaId;

  PartHdr = AllocateZeroPool (BlockSize);

  if (PartHdr == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate pool error\n"));
    return FALSE;
  }

  PEntryLBA = (PartHeader->MyLBA == PRIMARY_PART_HEADER_LBA) ? \
              (PartHeader->LastUsableLBA + 1) : \
              (PRIMARY_PART_HEADER_LBA + 1);

  CopyMem (PartHdr, PartHeader, sizeof (EFI_PARTITION_TABLE_HEADER));

  PartHdr->MyLBA             = PartHeader->AlternateLBA;
  PartHdr->AlternateLBA      = PartHeader->MyLBA;
  PartHdr->PartitionEntryLBA = PEntryLBA;
  PartitionSetCrc ((EFI_TABLE_HEADER *)PartHdr);

  Status = DiskIo->WriteDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (PartHdr->MyLBA, (UINT32)BlockSize),
                     BlockSize,
                     PartHdr
                     );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Ptr = AllocatePool (PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry);
  if (Ptr == NULL) {
    DEBUG ((DEBUG_ERROR, " Allocate pool error\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (PartHeader->PartitionEntryLBA, (UINT32)BlockSize),
                     PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                     Ptr
                     );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = DiskIo->WriteDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (PEntryLBA, (UINT32)BlockSize),
                     PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                     Ptr
                     );

Done:
  FreePool (PartHdr);

  if (Ptr != NULL) {
    FreePool (Ptr);
  }

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  This routine will check GPT partition entry and return entry status.

  Caution: This function may receive untrusted input.
  The GPT partition entry is external input, so this routine
  will do basic validation for GPT partition entry and report status.

  @param[in]    PartHeader    Partition table header structure
  @param[in]    PartEntry     The partition entry array
  @param[out]   PEntryStatus  the partition entry status array
                              recording the status of each partition

**/
VOID
PartitionCheckGptEntry (
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader,
  IN  EFI_PARTITION_ENTRY         *PartEntry,
  OUT EFI_PARTITION_ENTRY_STATUS  *PEntryStatus
  )
{
  EFI_LBA              StartingLBA;
  EFI_LBA              EndingLBA;
  EFI_PARTITION_ENTRY  *Entry;
  UINTN                Index1;
  UINTN                Index2;

  DEBUG ((DEBUG_INFO, " start check partition entries\n"));
  for (Index1 = 0; Index1 < PartHeader->NumberOfPartitionEntries; Index1++) {
    Entry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartEntry + Index1 * PartHeader->SizeOfPartitionEntry);
    if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
      continue;
    }

    StartingLBA = Entry->StartingLBA;
    EndingLBA   = Entry->EndingLBA;
    if ((StartingLBA > EndingLBA) ||
        (StartingLBA < PartHeader->FirstUsableLBA) ||
        (StartingLBA > PartHeader->LastUsableLBA) ||
        (EndingLBA < PartHeader->FirstUsableLBA) ||
        (EndingLBA > PartHeader->LastUsableLBA)
        )
    {
      PEntryStatus[Index1].OutOfRange = TRUE;
      continue;
    }

    if ((Entry->Attributes & BIT1) != 0) {
      //
      // If Bit 1 is set, this indicate that this is an OS specific GUID partition.
      //
      PEntryStatus[Index1].OsSpecific = TRUE;
    }

    for (Index2 = Index1 + 1; Index2 < PartHeader->NumberOfPartitionEntries; Index2++) {
      Entry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartEntry + Index2 * PartHeader->SizeOfPartitionEntry);
      if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
        continue;
      }

      if ((Entry->EndingLBA >= StartingLBA) && (Entry->StartingLBA <= EndingLBA)) {
        //
        // This region overlaps with the Index1'th region
        //
        PEntryStatus[Index1].Overlap = TRUE;
        PEntryStatus[Index2].Overlap = TRUE;
        continue;
      }
    }
  }

  DEBUG ((DEBUG_INFO, " End check partition entries\n"));
}

/**
  Updates the CRC32 value in the table header.

  @param  Hdr    Table to update

**/
STATIC
VOID
PartitionSetCrc (
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
{
  PartitionSetCrcAltSize (Hdr->HeaderSize, Hdr);
}

/**
  Updates the CRC32 value in the table header.

  @param  Size   The size of the table
  @param  Hdr    Table to update

**/
STATIC
VOID
PartitionSetCrcAltSize (
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
{
  UINT32  Crc;

  Hdr->CRC32 = 0;
  gBS->CalculateCrc32 ((UINT8 *)Hdr, Size, &Crc);
  Hdr->CRC32 = Crc;
}

/**
  Checks the CRC32 value in the table header.

  @param  MaxSize   Max Size limit
  @param  Hdr       Table to check

  @return TRUE      CRC Valid
  @return FALSE     CRC Invalid

**/
STATIC
BOOLEAN
PartitionCheckCrc (
  IN UINTN                 MaxSize,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
{
  return PartitionCheckCrcAltSize (MaxSize, Hdr->HeaderSize, Hdr);
}

/**
  Checks the CRC32 value in the table header.

  @param  MaxSize   Max Size limit
  @param  Size      The size of the table
  @param  Hdr       Table to check

  @return TRUE    CRC Valid
  @return FALSE   CRC Invalid

**/
STATIC
BOOLEAN
PartitionCheckCrcAltSize (
  IN UINTN                 MaxSize,
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
{
  UINT32      Crc;
  UINT32      OrgCrc;
  EFI_STATUS  Status;

  Crc = 0;

  if (Size == 0) {
    //
    // If header size is 0 CRC will pass so return FALSE here
    //
    return FALSE;
  }

  if ((MaxSize != 0) && (Size > MaxSize)) {
    DEBUG ((DEBUG_ERROR, "CheckCrc32: Size > MaxSize\n"));
    return FALSE;
  }

  //
  // clear old crc from header
  //
  OrgCrc     = Hdr->CRC32;
  Hdr->CRC32 = 0;

  Status = gBS->CalculateCrc32 ((UINT8 *)Hdr, Size, &Crc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CheckCrc32: Crc calculation failed\n"));
    return FALSE;
  }

  //
  // set results
  //
  Hdr->CRC32 = Crc;

  //
  // return status
  //
  DEBUG_CODE_BEGIN ();
  if (OrgCrc != Crc) {
    DEBUG ((DEBUG_ERROR, "CheckCrc32: Crc check failed\n"));
  }

  DEBUG_CODE_END ();

  return (BOOLEAN)(OrgCrc == Crc);
}
