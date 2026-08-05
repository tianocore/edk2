/** @file
  Install GPT partition child handles for the Partition driver.

  Caution: This file requires additional review when modified.
  This driver will have external input - disk partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  PartitionInstallGptChildHandles() routine will read disk partition content and
  do basic validation before PartitionInstallChildHandle(). The GPT table
  parsing and validation helpers it relies on live in the shared GPT parser
  (GptLib/Gpt.c).

Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Partition.h"

/**
  Install child handles if the Handle supports GPT partition structure.

  Caution: This function may receive untrusted input.
  The GPT partition table is external input, so this routine
  will do basic validation for GPT partition table before install
  child handle for each GPT partition.

  @param[in]  This       Calling context.
  @param[in]  Handle     Parent Handle.
  @param[in]  DiskIo     Parent DiskIo interface.
  @param[in]  DiskIo2    Parent DiskIo2 interface.
  @param[in]  BlockIo    Parent BlockIo interface.
  @param[in]  BlockIo2   Parent BlockIo2 interface.
  @param[in]  DevicePath Parent Device Path.

  @retval EFI_SUCCESS           Valid GPT disk.
  @retval EFI_MEDIA_CHANGED     Media changed Detected.
  @retval other                 Not a valid GPT disk.

**/
EFI_STATUS
PartitionInstallGptChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_DISK_IO2_PROTOCOL        *DiskIo2,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_BLOCK_IO2_PROTOCOL       *BlockIo2,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  EFI_STATUS                   Status;
  UINT32                       BlockSize;
  EFI_LBA                      LastBlock;
  MASTER_BOOT_RECORD           *ProtectiveMbr;
  EFI_PARTITION_TABLE_HEADER   *PrimaryHeader;
  EFI_PARTITION_TABLE_HEADER   *BackupHeader;
  EFI_PARTITION_ENTRY          *PartEntry;
  EFI_PARTITION_ENTRY          *Entry;
  EFI_PARTITION_ENTRY_STATUS   *PEntryStatus;
  UINTN                        Index;
  EFI_STATUS                   GptValidStatus;
  HARDDRIVE_DEVICE_PATH        HdDev;
  UINT32                       MediaId;
  EFI_PARTITION_INFO_PROTOCOL  PartitionInfo;

  ProtectiveMbr = NULL;
  PrimaryHeader = NULL;
  BackupHeader  = NULL;
  PartEntry     = NULL;
  PEntryStatus  = NULL;

  BlockSize = BlockIo->Media->BlockSize;
  LastBlock = BlockIo->Media->LastBlock;
  MediaId   = BlockIo->Media->MediaId;

  DEBUG ((DEBUG_INFO, " BlockSize : %d \n", BlockSize));
  DEBUG ((DEBUG_INFO, " LastBlock : %lx \n", LastBlock));

  GptValidStatus = EFI_NOT_FOUND;

  //
  // Ensure the block size can hold the MBR
  //
  if (BlockSize < sizeof (MASTER_BOOT_RECORD)) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate a buffer for the Protective MBR
  //
  ProtectiveMbr = AllocatePool (BlockSize);
  if (ProtectiveMbr == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Read the Protective MBR from LBA #0
  //
  Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     0,
                     BlockSize,
                     ProtectiveMbr
                     );
  if (EFI_ERROR (Status)) {
    GptValidStatus = Status;
    goto Done;
  }

  //
  // Verify that the Protective MBR is valid
  //
  for (Index = 0; Index < MAX_MBR_PARTITIONS; Index++) {
    if ((ProtectiveMbr->Partition[Index].OSIndicator == PMBR_GPT_PARTITION) &&
        (UNPACK_UINT32 (ProtectiveMbr->Partition[Index].StartingLBA) == 1)
        )
    {
      break;
    }
  }

  if (Index == MAX_MBR_PARTITIONS) {
    goto Done;
  }

  //
  // Allocate the GPT structures
  //
  PrimaryHeader = AllocateZeroPool (sizeof (EFI_PARTITION_TABLE_HEADER));
  if (PrimaryHeader == NULL) {
    goto Done;
  }

  BackupHeader = AllocateZeroPool (sizeof (EFI_PARTITION_TABLE_HEADER));
  if (BackupHeader == NULL) {
    goto Done;
  }

  //
  // Check primary and backup partition tables
  //
  if (!PartitionValidGptTable (BlockIo, DiskIo, PRIMARY_PART_HEADER_LBA, PrimaryHeader)) {
    DEBUG ((DEBUG_INFO, " Not Valid primary partition table\n"));

    if (!PartitionValidGptTable (BlockIo, DiskIo, LastBlock, BackupHeader)) {
      DEBUG ((DEBUG_INFO, " Not Valid backup partition table\n"));
      goto Done;
    }

    DEBUG ((DEBUG_INFO, " Valid backup partition table\n"));
    DEBUG ((DEBUG_INFO, " Restore primary partition table by the backup\n"));
    //
    // Both the restore write and the subsequent validation can fail (for
    // example on write-protected media, or when the backup header's
    // AlternateLBA points beyond the end of the device).  Never fall through
    // with PrimaryHeader left in an unrestored/invalid state; bail out so the
    // partition table that gets used is always one that passed validation.
    //
    if (!PartitionRestoreGptTable (BlockIo, DiskIo, BackupHeader)) {
      DEBUG ((DEBUG_INFO, " Restore primary partition table error\n"));
      goto Done;
    }

    if (!PartitionValidGptTable (BlockIo, DiskIo, BackupHeader->AlternateLBA, PrimaryHeader)) {
      DEBUG ((DEBUG_INFO, " Not Valid restored primary partition table\n"));
      goto Done;
    }

    DEBUG ((DEBUG_INFO, " Restore primary partition table success\n"));
  } else if (!PartitionValidGptTable (BlockIo, DiskIo, PrimaryHeader->AlternateLBA, BackupHeader)) {
    DEBUG ((DEBUG_INFO, " Valid primary and !Valid backup partition table\n"));
    DEBUG ((DEBUG_INFO, " Restore backup partition table by the primary\n"));
    if (!PartitionRestoreGptTable (BlockIo, DiskIo, PrimaryHeader)) {
      DEBUG ((DEBUG_INFO, " Restore backup partition table error\n"));
    }

    if (PartitionValidGptTable (BlockIo, DiskIo, PrimaryHeader->AlternateLBA, BackupHeader)) {
      DEBUG ((DEBUG_INFO, " Restore backup partition table success\n"));
    }
  }

  DEBUG ((DEBUG_INFO, " Valid primary and Valid backup partition table\n"));

  //
  // Read the EFI Partition Entries
  //
  PartEntry = AllocatePool (PrimaryHeader->NumberOfPartitionEntries * PrimaryHeader->SizeOfPartitionEntry);
  if (PartEntry == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate pool error\n"));
    goto Done;
  }

  Status = DiskIo->ReadDisk (
                     DiskIo,
                     MediaId,
                     MultU64x32 (PrimaryHeader->PartitionEntryLBA, BlockSize),
                     PrimaryHeader->NumberOfPartitionEntries * (PrimaryHeader->SizeOfPartitionEntry),
                     PartEntry
                     );
  if (EFI_ERROR (Status)) {
    GptValidStatus = Status;
    DEBUG ((DEBUG_ERROR, " Partition Entry ReadDisk error\n"));
    goto Done;
  }

  DEBUG ((DEBUG_INFO, " Partition entries read block success\n"));

  DEBUG ((DEBUG_INFO, " Number of partition entries: %d\n", PrimaryHeader->NumberOfPartitionEntries));

  PEntryStatus = AllocateZeroPool (PrimaryHeader->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY_STATUS));
  if (PEntryStatus == NULL) {
    DEBUG ((DEBUG_ERROR, "Allocate pool error\n"));
    goto Done;
  }

  //
  // Check the integrity of partition entries
  //
  PartitionCheckGptEntry (PrimaryHeader, PartEntry, PEntryStatus);

  //
  // If we got this far the GPT layout of the disk is valid and we should return true
  //
  GptValidStatus = EFI_SUCCESS;

  //
  // Create child device handles
  //
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    Entry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartEntry + Index * PrimaryHeader->SizeOfPartitionEntry);
    if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeUnusedGuid) ||
        PEntryStatus[Index].OutOfRange ||
        PEntryStatus[Index].Overlap ||
        PEntryStatus[Index].OsSpecific
        )
    {
      //
      // Don't use null EFI Partition Entries, Invalid Partition Entries or OS specific
      // partition Entries
      //
      continue;
    }

    ZeroMem (&HdDev, sizeof (HdDev));
    HdDev.Header.Type    = MEDIA_DEVICE_PATH;
    HdDev.Header.SubType = MEDIA_HARDDRIVE_DP;
    SetDevicePathNodeLength (&HdDev.Header, sizeof (HdDev));

    HdDev.PartitionNumber = (UINT32)Index + 1;
    HdDev.MBRType         = MBR_TYPE_EFI_PARTITION_TABLE_HEADER;
    HdDev.SignatureType   = SIGNATURE_TYPE_GUID;
    HdDev.PartitionStart  = Entry->StartingLBA;
    HdDev.PartitionSize   = Entry->EndingLBA - Entry->StartingLBA + 1;
    CopyMem (HdDev.Signature, &Entry->UniquePartitionGUID, sizeof (EFI_GUID));

    ZeroMem (&PartitionInfo, sizeof (EFI_PARTITION_INFO_PROTOCOL));
    PartitionInfo.Revision = EFI_PARTITION_INFO_PROTOCOL_REVISION;
    PartitionInfo.Type     = PARTITION_TYPE_GPT;
    if (CompareGuid (&Entry->PartitionTypeGUID, &gEfiPartTypeSystemPartGuid)) {
      PartitionInfo.System = 1;
    }

    CopyMem (&PartitionInfo.Info.Gpt, Entry, sizeof (EFI_PARTITION_ENTRY));

    DEBUG ((DEBUG_INFO, " Index : %d\n", (UINT32)Index));
    DEBUG ((DEBUG_INFO, " Start LBA : %lx\n", (UINT64)HdDev.PartitionStart));
    DEBUG ((DEBUG_INFO, " End LBA : %lx\n", (UINT64)Entry->EndingLBA));
    DEBUG ((DEBUG_INFO, " Partition size: %lx\n", (UINT64)HdDev.PartitionSize));
    DEBUG ((DEBUG_INFO, " Start : %lx", MultU64x32 (Entry->StartingLBA, BlockSize)));
    DEBUG ((DEBUG_INFO, " End : %lx\n", MultU64x32 (Entry->EndingLBA, BlockSize)));

    Status = PartitionInstallChildHandle (
               This,
               Handle,
               DiskIo,
               DiskIo2,
               BlockIo,
               BlockIo2,
               DevicePath,
               (EFI_DEVICE_PATH_PROTOCOL *)&HdDev,
               &PartitionInfo,
               Entry->StartingLBA,
               Entry->EndingLBA,
               BlockSize,
               &Entry->PartitionTypeGUID
               );
  }

  DEBUG ((DEBUG_INFO, "Prepare to Free Pool\n"));

Done:
  if (ProtectiveMbr != NULL) {
    FreePool (ProtectiveMbr);
  }

  if (PrimaryHeader != NULL) {
    FreePool (PrimaryHeader);
  }

  if (BackupHeader != NULL) {
    FreePool (BackupHeader);
  }

  if (PartEntry != NULL) {
    FreePool (PartEntry);
  }

  if (PEntryStatus != NULL) {
    FreePool (PEntryStatus);
  }

  return GptValidStatus;
}
