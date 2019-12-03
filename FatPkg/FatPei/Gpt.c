/** @file
  Routines supporting partition discovery and
  logical device reading

Copyright (c) 2019 Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Mbr.h>
#include <Uefi/UefiGpt.h>
#include <Library/BaseLib.h>
#include "FatLitePeim.h"

//
// Assumption: 'a' and 'blocksize' are all UINT32 or UINT64.
// If 'a' and 'blocksize' are not the same type, should use DivU64xU32 to calculate.
//
#define EFI_SIZE_TO_BLOCKS(a, blocksize)  (((a) / (blocksize)) + (((a) % (blocksize)) ? 1 : 0))

//
// GPT Partition Entry Status
//
typedef struct {
  BOOLEAN OutOfRange;
  BOOLEAN Overlap;
  BOOLEAN OsSpecific;
} EFI_PARTITION_ENTRY_STATUS;

/**
  Check if the CRC field in the Partition table header is valid.

  @param[in]  PartHeader  Partition table header structure

  @retval TRUE      the CRC is valid
  @retval FALSE     the CRC is invalid

**/
BOOLEAN
PartitionCheckGptHeaderCRC (
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
{
  UINT32      GptHdrCrc;
  UINT32      Crc;

  GptHdrCrc = PartHeader->Header.CRC32;

  //
  // Set CRC field to zero when doing calculation
  //
  PartHeader->Header.CRC32 = 0;

  Crc = CalculateCrc32 (PartHeader, PartHeader->Header.HeaderSize);

  //
  // Restore Header CRC
  //
  PartHeader->Header.CRC32 = GptHdrCrc;

  return (GptHdrCrc == Crc);
}


/**
  Check if the CRC field in the Partition table header is valid
  for Partition entry array.

  @param[in]  PartHeader  Partition table header structure
  @param[in]  PartEntry   The partition entry array

  @retval TRUE      the CRC is valid
  @retval FALSE     the CRC is invalid

**/
BOOLEAN
PartitionCheckGptEntryArrayCRC (
  IN  EFI_PARTITION_TABLE_HEADER *PartHeader,
  IN  EFI_PARTITION_ENTRY        *PartEntry
  )
{
  UINT32      Crc;
  UINTN       Size;

  Size = (UINTN)MultU64x32(PartHeader->NumberOfPartitionEntries, PartHeader->SizeOfPartitionEntry);
  Crc  = CalculateCrc32 (PartEntry, Size);

  return (BOOLEAN) (PartHeader->PartitionEntryArrayCRC32 == Crc);
}

/**
  The function is used for valid GPT table. Both for Primary and Backup GPT header.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device
  @param[in]  IsPrimaryHeader   Indicate to which header will be checked.
  @param[in]  PartHdr           Stores the partition table that is read

  @retval TRUE      The partition table is valid
  @retval FALSE     The partition table is not valid

**/
BOOLEAN
PartitionCheckGptHeader (
  IN  PEI_FAT_PRIVATE_DATA        *PrivateData,
  IN  UINTN                       ParentBlockDevNo,
  IN  BOOLEAN                     IsPrimaryHeader,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHdr
  )
{
  PEI_FAT_BLOCK_DEVICE            *ParentBlockDev;
  EFI_PEI_LBA                     Lba;
  EFI_PEI_LBA                     AlternateLba;
  EFI_PEI_LBA                     EntryArrayLastLba;

  UINT64                          PartitionEntryArraySize;
  UINT64                          PartitionEntryBlockNumb;
  UINT32                          EntryArraySizeRemainder;

  ParentBlockDev = &(PrivateData->BlockDevice[ParentBlockDevNo]);

  if (IsPrimaryHeader) {
    Lba          = PRIMARY_PART_HEADER_LBA;
    AlternateLba = ParentBlockDev->LastBlock;
  } else {
    Lba          = ParentBlockDev->LastBlock;
    AlternateLba = PRIMARY_PART_HEADER_LBA;
  }

  if ( (PartHdr->Header.Signature != EFI_PTAB_HEADER_ID) ||
       (PartHdr->Header.Revision != 0x00010000) ||
       (PartHdr->Header.HeaderSize < 92) ||
       (PartHdr->Header.HeaderSize > ParentBlockDev->BlockSize) ||
       (!PartitionCheckGptHeaderCRC (PartHdr)) ||
       (PartHdr->Header.Reserved != 0)
     ) {
    DEBUG ((DEBUG_ERROR, "Invalid efi partition table header\n"));
    return FALSE;
  }

  //
  // |    Block0    |    Block1    |Block2 ~ FirstUsableLBA - 1|FirstUsableLBA, ... ,LastUsableLBA|LastUsableLBA+1 ~ LastBlock-1|  LastBlock  |
  // |Protective MBR|Primary Header|Entry Array(At Least 16384)|             Partition            | Entry Array(At Least 16384) |BackUp Header|
  //
  // 1. Protective MBR is fixed at Block 0.
  // 2. Primary Header is fixed at Block 1.
  // 3. Backup Header is fixed at LastBlock.
  // 4. Must be remain 128*128 bytes for primary entry array.
  // 5. Must be remain 128*128 bytes for backup entry array.
  // 6. SizeOfPartitionEntry must be equals to 128 * 2^n.
  //
  if ( (PartHdr->MyLBA != Lba) ||
       (PartHdr->AlternateLBA != AlternateLba) ||
       (PartHdr->FirstUsableLBA < 2 + EFI_SIZE_TO_BLOCKS (EFI_GPT_PART_ENTRY_MIN_SIZE, ParentBlockDev->BlockSize)) ||
       (PartHdr->LastUsableLBA  > ParentBlockDev->LastBlock - 1 - EFI_SIZE_TO_BLOCKS (EFI_GPT_PART_ENTRY_MIN_SIZE, ParentBlockDev->BlockSize)) ||
       (PartHdr->FirstUsableLBA > PartHdr->LastUsableLBA) ||
       (PartHdr->PartitionEntryLBA < 2) ||
       (PartHdr->PartitionEntryLBA > ParentBlockDev->LastBlock - 1) ||
       (PartHdr->PartitionEntryLBA >= PartHdr->FirstUsableLBA && PartHdr->PartitionEntryLBA <= PartHdr->LastUsableLBA) ||
       (PartHdr->SizeOfPartitionEntry%128 != 0) ||
       (PartHdr->SizeOfPartitionEntry != sizeof (EFI_PARTITION_ENTRY))
     ) {
    DEBUG ((DEBUG_ERROR, "Invalid efi partition table header\n"));
    return FALSE;
  }

  //
  // Ensure the NumberOfPartitionEntries * SizeOfPartitionEntry doesn't overflow.
  //
  if (PartHdr->NumberOfPartitionEntries > DivU64x32 (MAX_UINTN, PartHdr->SizeOfPartitionEntry)) {
    DEBUG ((DEBUG_ERROR, "Memory overflow in GPT Entry Array\n"));
    return FALSE;
  }

  PartitionEntryArraySize = MultU64x32 (PartHdr->NumberOfPartitionEntries, PartHdr->SizeOfPartitionEntry);
  EntryArraySizeRemainder = 0;
  PartitionEntryBlockNumb = DivU64x32Remainder (PartitionEntryArraySize, ParentBlockDev->BlockSize, &EntryArraySizeRemainder);
  if (EntryArraySizeRemainder != 0) {
    PartitionEntryBlockNumb++;
  }

  if (IsPrimaryHeader) {
    EntryArrayLastLba = PartHdr->FirstUsableLBA;
  } else {
    EntryArrayLastLba = ParentBlockDev->LastBlock;
  }

  //
  // Make sure partition entry array not overlaps with partition area or the LastBlock.
  //
  if (PartHdr->PartitionEntryLBA + PartitionEntryBlockNumb > EntryArrayLastLba) {
    DEBUG ((DEBUG_ERROR, "GPT Partition Entry Array Error!\n"));
    DEBUG ((DEBUG_ERROR, "PartitionEntryArraySize = %lu.\n", PartitionEntryArraySize));
    DEBUG ((DEBUG_ERROR, "PartitionEntryLBA = %lu.\n", PartHdr->PartitionEntryLBA));
    DEBUG ((DEBUG_ERROR, "PartitionEntryBlockNumb = %lu.\n", PartitionEntryBlockNumb));
    DEBUG ((DEBUG_ERROR, "EntryArrayLastLba = %lu.\n", EntryArrayLastLba));
    return FALSE;
  }

  return TRUE;
}

/**
  This function is used to verify each partition in block device.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device
  @param[in]  PartHdr           Stores the partition table that is read

  @retval TRUE      The partition is valid
  @retval FALSE     The partition is not valid

**/
BOOLEAN
PartitionCheckGptEntryArray (
  IN  PEI_FAT_PRIVATE_DATA        *PrivateData,
  IN  UINTN                       ParentBlockDevNo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHdr
  )
{
  EFI_STATUS                      Status;
  PEI_FAT_BLOCK_DEVICE            *ParentBlockDev;
  PEI_FAT_BLOCK_DEVICE            *BlockDevPtr;

  UINT64                          PartitionEntryArraySize;
  UINT64                          PartitionEntryBlockNumb;
  UINT32                          EntryArraySizeRemainder;

  EFI_PARTITION_ENTRY             *PartitionEntryBuffer;
  EFI_PARTITION_ENTRY_STATUS      *PartitionEntryStatus;

  BOOLEAN                         Found;
  EFI_LBA                         StartingLBA;
  EFI_LBA                         EndingLBA;
  UINTN                           Index;
  UINTN                           Index1;
  UINTN                           Index2;
  EFI_PARTITION_ENTRY             *Entry;

  PartitionEntryBuffer = NULL;
  PartitionEntryStatus = NULL;

  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);
  Found           = FALSE;

  PartitionEntryArraySize = MultU64x32 (PartHdr->NumberOfPartitionEntries, PartHdr->SizeOfPartitionEntry);
  EntryArraySizeRemainder = 0;
  PartitionEntryBlockNumb = DivU64x32Remainder (PartitionEntryArraySize, ParentBlockDev->BlockSize, &EntryArraySizeRemainder);
  if (EntryArraySizeRemainder != 0) {
    PartitionEntryBlockNumb++;
  }
  PartitionEntryArraySize = MultU64x32 (PartitionEntryBlockNumb, ParentBlockDev->BlockSize);

  PartitionEntryBuffer = (EFI_PARTITION_ENTRY *) AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)PartitionEntryArraySize));
  if (PartitionEntryBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate memory error!\n"));
    goto EXIT;
  }

  PartitionEntryStatus = (EFI_PARTITION_ENTRY_STATUS *) AllocatePages (EFI_SIZE_TO_PAGES (PartHdr->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY_STATUS)));
  if (PartitionEntryStatus == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate memory error!\n"));
    goto EXIT;
  }
  ZeroMem (PartitionEntryStatus, PartHdr->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY_STATUS));

  Status = FatReadBlock (
             PrivateData,
             ParentBlockDevNo,
             PartHdr->PartitionEntryLBA,
             (UINTN)PartitionEntryArraySize,
             PartitionEntryBuffer
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Read partition entry array error!\n"));
    goto EXIT;
  }

  if (!PartitionCheckGptEntryArrayCRC (PartHdr, PartitionEntryBuffer)) {
    DEBUG ((DEBUG_ERROR, "Partition entries CRC check fail\n"));
    goto EXIT;
  }

  for (Index1 = 0; Index1 < PartHdr->NumberOfPartitionEntries; Index1++) {
    Entry = (EFI_PARTITION_ENTRY *) ((UINT8 *) PartitionEntryBuffer + Index1 * PartHdr->SizeOfPartitionEntry);
    if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
      continue;
    }

    StartingLBA = Entry->StartingLBA;
    EndingLBA   = Entry->EndingLBA;
    if (StartingLBA > EndingLBA ||
        StartingLBA < PartHdr->FirstUsableLBA ||
        StartingLBA > PartHdr->LastUsableLBA ||
        EndingLBA < PartHdr->FirstUsableLBA ||
        EndingLBA > PartHdr->LastUsableLBA
        ) {
      PartitionEntryStatus[Index1].OutOfRange = TRUE;
      continue;
    }

    if ((Entry->Attributes & BIT1) != 0) {
      //
      // If Bit 1 is set, this indicate that this is an OS specific GUID partition.
      //
      PartitionEntryStatus[Index1].OsSpecific = TRUE;
    }

    for (Index2 = Index1 + 1; Index2 < PartHdr->NumberOfPartitionEntries; Index2++) {
      Entry = (EFI_PARTITION_ENTRY *) ((UINT8 *) PartitionEntryBuffer + Index2 * PartHdr->SizeOfPartitionEntry);
      if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
        continue;
      }

      if (Entry->EndingLBA >= StartingLBA && Entry->StartingLBA <= EndingLBA) {
        //
        // This region overlaps with the Index1'th region
        //
        PartitionEntryStatus[Index1].Overlap  = TRUE;
        PartitionEntryStatus[Index2].Overlap  = TRUE;
        continue;
      }
    }
  }

  for (Index = 0; Index < PartHdr->NumberOfPartitionEntries; Index++) {
    if (CompareGuid (&PartitionEntryBuffer[Index].PartitionTypeGUID, &gEfiPartTypeUnusedGuid)||
        PartitionEntryStatus[Index].OutOfRange ||
        PartitionEntryStatus[Index].Overlap ||
        PartitionEntryStatus[Index].OsSpecific) {
      //
      // Don't use null EFI Partition Entries, Invalid Partition Entries or OS specific
      // partition Entries
      //
      continue;
    }

    if (PrivateData->BlockDeviceCount >= PEI_FAT_MAX_BLOCK_DEVICE) {
      break;
    }

    Found                         = TRUE;
    BlockDevPtr                   = &(PrivateData->BlockDevice[PrivateData->BlockDeviceCount]);

    BlockDevPtr->BlockSize        = ParentBlockDev->BlockSize;
    BlockDevPtr->LastBlock        = PartitionEntryBuffer[Index].EndingLBA;
    BlockDevPtr->IoAlign          = ParentBlockDev->IoAlign;
    BlockDevPtr->Logical          = TRUE;
    BlockDevPtr->PartitionChecked = FALSE;
    BlockDevPtr->StartingPos      = MultU64x32 (
                                      PartitionEntryBuffer[Index].StartingLBA,
                                      ParentBlockDev->BlockSize
                                      );
    BlockDevPtr->ParentDevNo      = ParentBlockDevNo;

    PrivateData->BlockDeviceCount++;

    DEBUG ((DEBUG_INFO, "Find GPT Partition [0x%lx",  PartitionEntryBuffer[Index].StartingLBA, BlockDevPtr->LastBlock));
    DEBUG ((DEBUG_INFO, ", 0x%lx]\n", BlockDevPtr->LastBlock));
    DEBUG ((DEBUG_INFO, "         BlockSize %x\n",  BlockDevPtr->BlockSize));
  }

EXIT:
  if (PartitionEntryBuffer != NULL) {
    FreePages (PartitionEntryBuffer, EFI_SIZE_TO_PAGES ((UINTN)PartitionEntryArraySize));
  }

  if (PartitionEntryStatus != NULL) {
    FreePages (PartitionEntryStatus, EFI_SIZE_TO_PAGES (PartHdr->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY_STATUS)));
  }

  return Found;
}

/**
  The function is used to check GPT structure, include GPT header and GPT entry array.

  1. Check GPT header.
  2. Check partition entry array.
  3. Check each partitions.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device
  @param[in]  IsPrimary         Indicate primary or backup to be check

  @retval TRUE              Primary or backup GPT structure is valid.
  @retval FALSE             Both primary and backup are invalid.

**/
BOOLEAN
PartitionCheckGptStructure (
  IN  PEI_FAT_PRIVATE_DATA      *PrivateData,
  IN  UINTN                     ParentBlockDevNo,
  IN  BOOLEAN                   IsPrimary
  )
{
  EFI_STATUS                    Status;
  PEI_FAT_BLOCK_DEVICE          *ParentBlockDev;
  EFI_PARTITION_TABLE_HEADER    *PartHdr;
  EFI_PEI_LBA                   GptHeaderLBA;

  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);
  PartHdr         = (EFI_PARTITION_TABLE_HEADER *) PrivateData->BlockData;

  if (IsPrimary) {
    GptHeaderLBA = PRIMARY_PART_HEADER_LBA;
  } else {
    GptHeaderLBA = ParentBlockDev->LastBlock;
  }

  Status = FatReadBlock (
             PrivateData,
             ParentBlockDevNo,
             GptHeaderLBA,
             ParentBlockDev->BlockSize,
             PartHdr
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!PartitionCheckGptHeader (PrivateData, ParentBlockDevNo, IsPrimary, PartHdr)) {
    return FALSE;
  }

  if (!PartitionCheckGptEntryArray (PrivateData, ParentBlockDevNo, PartHdr)) {
    return FALSE;
  }

  return TRUE;
}

/**
  This function is used to check protective MBR structure before checking GPT.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device

  @retval TRUE              Valid protective MBR
  @retval FALSE             Invalid MBR
**/
BOOLEAN
PartitionCheckProtectiveMbr (
  IN  PEI_FAT_PRIVATE_DATA    *PrivateData,
  IN  UINTN                   ParentBlockDevNo
  )
{
  EFI_STATUS                  Status;
  MASTER_BOOT_RECORD          *ProtectiveMbr;
  MBR_PARTITION_RECORD        *MbrPartition;
  PEI_FAT_BLOCK_DEVICE        *ParentBlockDev;
  UINTN                       Index;

  ProtectiveMbr   = (MASTER_BOOT_RECORD *) PrivateData->BlockData;
  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);

  //
  // Read Protective MBR
  //
  Status = FatReadBlock (
             PrivateData,
             ParentBlockDevNo,
             0,
             ParentBlockDev->BlockSize,
             ProtectiveMbr
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GPT Error When Read Protective Mbr From Partition!\n"));
    return FALSE;
  }

  if (ProtectiveMbr->Signature != MBR_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "Protective Mbr Signature is invalid!\n"));
    return FALSE;
  }

  //
  // The partition define in UEFI Spec Table 17.
  // Boot Code, Unique MBR Disk Signature, Unknown.
  // These parts will not be used by UEFI, so we skip to check them.
  //
  for (Index = 0; Index < MAX_MBR_PARTITIONS; Index++) {
    MbrPartition = (MBR_PARTITION_RECORD *)&ProtectiveMbr->Partition[Index];
    if (MbrPartition->BootIndicator   == 0x00 &&
        MbrPartition->StartSector     == 0x02 &&
        MbrPartition->OSIndicator     == PMBR_GPT_PARTITION &&
        UNPACK_UINT32 (MbrPartition->StartingLBA) == 1
       ) {
      return TRUE;
    }
  }

  DEBUG ((DEBUG_ERROR, "Protective Mbr, All Partition Entry Are Empty!\n"));
  return FALSE;
}

/**
  This function is used for finding GPT partition on block device.
  As follow UEFI spec we should check protective MBR first and then
  try to check both primary/backup GPT structures.

  @param[in]  PrivateData       The global memory map
  @param[in]  ParentBlockDevNo  The parent block device

  @retval TRUE              New partitions are detected and logical block devices
                            are added to block device array
  @retval FALSE             No new partitions are added

**/
BOOLEAN
FatFindGptPartitions (
  IN  PEI_FAT_PRIVATE_DATA *PrivateData,
  IN  UINTN                ParentBlockDevNo
  )
{
  BOOLEAN                      Found;
  PEI_FAT_BLOCK_DEVICE         *ParentBlockDev;

  if (ParentBlockDevNo > PEI_FAT_MAX_BLOCK_DEVICE - 1) {
    return FALSE;
  }

  ParentBlockDev  = &(PrivateData->BlockDevice[ParentBlockDevNo]);
  if (ParentBlockDev->BlockSize > PEI_FAT_MAX_BLOCK_SIZE) {
    DEBUG ((DEBUG_ERROR, "Device BlockSize %x exceed FAT_MAX_BLOCK_SIZE\n", ParentBlockDev->BlockSize));
    return FALSE;
  }

  if (!PartitionCheckProtectiveMbr (PrivateData, ParentBlockDevNo)) {
    return FALSE;
  }

  Found = PartitionCheckGptStructure (PrivateData, ParentBlockDevNo, TRUE);
  if (!Found) {
    DEBUG ((DEBUG_ERROR, "Primary GPT Header Error, Try to Check Backup GPT Header!\n"));
    Found = PartitionCheckGptStructure (PrivateData, ParentBlockDevNo, FALSE);
  }

  if (Found) {
    ParentBlockDev->PartitionChecked = TRUE;
  }

  return Found;
}
