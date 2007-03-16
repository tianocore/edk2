/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Gpt.c
  
Abstract:

  Decode a hard disk partitioned with the GPT scheme in the EFI 1.0 
  specification.

--*/

#include "Partition.h"

STATIC
BOOLEAN
PartitionValidGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

STATIC
BOOLEAN
PartitionCheckGptEntryArrayCRC (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

STATIC
BOOLEAN
PartitionRestoreGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

STATIC
VOID
PartitionCheckGptEntry (
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader,
  IN  EFI_PARTITION_ENTRY         *PartEntry,
  OUT EFI_PARTITION_ENTRY_STATUS  *PEntryStatus
  );

STATIC
BOOLEAN
PartitionCheckCrcAltSize (
  IN UINTN                 MaxSize,
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

STATIC
BOOLEAN
PartitionCheckCrc (
  IN UINTN                 MaxSize,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

STATIC
VOID
PartitionSetCrcAltSize (
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  );

STATIC
VOID
PartitionSetCrc (
  IN OUT EFI_TABLE_HEADER *Hdr
  );

EFI_STATUS
PartitionInstallGptChildHandles (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DISK_IO_PROTOCOL         *DiskIo,
  IN  EFI_BLOCK_IO_PROTOCOL        *BlockIo,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
/*++

Routine Description:
  Install child handles if the Handle supports GPT partition structure.

Arguments:       
  This       - Calling context.
  Handle     - Parent Handle 
  DiskIo     - Parent DiskIo interface
  BlockIo    - Parent BlockIo interface
  DevicePath - Parent Device Path

Returns:
  EFI_SUCCESS  - Valid GPT disk
  EFI_MEDIA_CHANGED - Media changed Detected
  !EFI_SUCCESS - Not a valid GPT disk

--*/
{
  EFI_STATUS                  Status;
  UINT32                      BlockSize;
  EFI_LBA                     LastBlock;
  MASTER_BOOT_RECORD          *ProtectiveMbr;
  EFI_PARTITION_TABLE_HEADER  *PrimaryHeader;
  EFI_PARTITION_TABLE_HEADER  *BackupHeader;
  EFI_PARTITION_ENTRY         *PartEntry;
  EFI_PARTITION_ENTRY_STATUS  *PEntryStatus;
  UINTN                       Index;
  EFI_STATUS                  GptValid;
  HARDDRIVE_DEVICE_PATH       HdDev;

  ProtectiveMbr = NULL;
  PrimaryHeader = NULL;
  BackupHeader  = NULL;
  PartEntry     = NULL;
  PEntryStatus  = NULL;

  BlockSize     = BlockIo->Media->BlockSize;
  LastBlock     = BlockIo->Media->LastBlock;

  DEBUG ((EFI_D_INFO, " BlockSize : %d \n", BlockSize));
  DEBUG ((EFI_D_INFO, " LastBlock : %x \n", LastBlock));

  GptValid = EFI_NOT_FOUND;

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
  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      BlockIo->Media->MediaId,
                      0,
                      BlockIo->Media->BlockSize,
                      ProtectiveMbr
                      );
  if (EFI_ERROR (Status)) {
    GptValid = Status;
    goto Done;
  }
  //
  // Verify that the Protective MBR is valid
  //
  if (ProtectiveMbr->Partition[0].BootIndicator != 0x00 ||
      ProtectiveMbr->Partition[0].OSIndicator != PMBR_GPT_PARTITION ||
      UNPACK_UINT32 (ProtectiveMbr->Partition[0].StartingLBA) != 1
      ) {
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
    DEBUG ((EFI_D_INFO, " Not Valid primary partition table\n"));

    if (!PartitionValidGptTable (BlockIo, DiskIo, LastBlock, BackupHeader)) {
      DEBUG ((EFI_D_INFO, " Not Valid backup partition table\n"));
      goto Done;
    } else {
      DEBUG ((EFI_D_INFO, " Valid backup partition table\n"));
      DEBUG ((EFI_D_INFO, " Restore primary partition table by the backup\n"));
      if (!PartitionRestoreGptTable (BlockIo, DiskIo, BackupHeader)) {
        DEBUG ((EFI_D_INFO, " Restore primary partition table error\n"));
      }

      if (PartitionValidGptTable (BlockIo, DiskIo, BackupHeader->AlternateLBA, PrimaryHeader)) {
        DEBUG ((EFI_D_INFO, " Restore backup partition table success\n"));
      }
    }
  } else if (!PartitionValidGptTable (BlockIo, DiskIo, PrimaryHeader->AlternateLBA, BackupHeader)) {
    DEBUG ((EFI_D_INFO, " Valid primary and !Valid backup partition table\n"));
    DEBUG ((EFI_D_INFO, " Restore backup partition table by the primary\n"));
    if (!PartitionRestoreGptTable (BlockIo, DiskIo, PrimaryHeader)) {
      DEBUG ((EFI_D_INFO, " Restore  backup partition table error\n"));
    }

    if (PartitionValidGptTable (BlockIo, DiskIo, PrimaryHeader->AlternateLBA, BackupHeader)) {
      DEBUG ((EFI_D_INFO, " Restore backup partition table success\n"));
    }

  }

  DEBUG ((EFI_D_INFO, " Valid primary and Valid backup partition table\n"));

  //
  // Read the EFI Partition Entries
  //
  PartEntry = AllocatePool (PrimaryHeader->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY));
  if (PartEntry == NULL) {
    DEBUG ((EFI_D_ERROR, "Allocate pool error\n"));
    goto Done;
  }

  Status = DiskIo->ReadDisk (
                    DiskIo,
                    BlockIo->Media->MediaId,
                    MultU64x32(PrimaryHeader->PartitionEntryLBA, BlockSize),
                    PrimaryHeader->NumberOfPartitionEntries * (PrimaryHeader->SizeOfPartitionEntry),
                    PartEntry
                    );
  if (EFI_ERROR (Status)) {
    GptValid = Status;
    DEBUG ((EFI_D_INFO, " Partition Entry ReadBlocks error\n"));
    goto Done;
  }

  DEBUG ((EFI_D_INFO, " Partition entries read block success\n"));

  DEBUG ((EFI_D_INFO, " Number of partition entries: %d\n", PrimaryHeader->NumberOfPartitionEntries));

  PEntryStatus = AllocateZeroPool (PrimaryHeader->NumberOfPartitionEntries * sizeof (EFI_PARTITION_ENTRY_STATUS));
  if (PEntryStatus == NULL) {
    DEBUG ((EFI_D_ERROR, "Allocate pool error\n"));
    goto Done;
  }

  //
  // Check the integrity of partition entries
  //
  PartitionCheckGptEntry (PrimaryHeader, PartEntry, PEntryStatus);

  //
  // If we got this far the GPT layout of the disk is valid and we should return true
  //
  GptValid = EFI_SUCCESS;

  //
  // Create child device handles
  //
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    if (CompareGuid (&PartEntry[Index].PartitionTypeGUID, &gEfiPartTypeUnusedGuid) ||
        PEntryStatus[Index].OutOfRange ||
        PEntryStatus[Index].Overlap
        ) {
      //
      // Don't use null EFI Partition Entries or Invalid Partition Entries
      //
      continue;
    }

    ZeroMem (&HdDev, sizeof (HdDev));
    HdDev.Header.Type     = MEDIA_DEVICE_PATH;
    HdDev.Header.SubType  = MEDIA_HARDDRIVE_DP;
    SetDevicePathNodeLength (&HdDev.Header, sizeof (HdDev));

    HdDev.PartitionNumber = (UINT32) Index + 1;
    HdDev.MBRType         = MBR_TYPE_EFI_PARTITION_TABLE_HEADER;
    HdDev.SignatureType   = SIGNATURE_TYPE_GUID;
    HdDev.PartitionStart  = PartEntry[Index].StartingLBA;
    HdDev.PartitionSize   = PartEntry[Index].EndingLBA - PartEntry[Index].StartingLBA + 1;
    CopyMem (HdDev.Signature, &PartEntry[Index].UniquePartitionGUID, sizeof (EFI_GUID));

    DEBUG ((EFI_D_INFO, " Index : %d\n", Index));
    DEBUG ((EFI_D_INFO, " Start LBA : %x\n", HdDev.PartitionStart));
    DEBUG ((EFI_D_INFO, " End LBA : %x\n", PartEntry[Index].EndingLBA));
    DEBUG ((EFI_D_INFO, " Partition size: %x\n", HdDev.PartitionSize));
    DEBUG ((EFI_D_INFO, " Start : %x", MultU64x32 (PartEntry[Index].StartingLBA, BlockSize)));
    DEBUG ((EFI_D_INFO, " End : %x\n", MultU64x32 (PartEntry[Index].EndingLBA, BlockSize)));

    Status = PartitionInstallChildHandle (
              This,
              Handle,
              DiskIo,
              BlockIo,
              DevicePath,
              (EFI_DEVICE_PATH_PROTOCOL *) &HdDev,
              PartEntry[Index].StartingLBA,
              PartEntry[Index].EndingLBA,
              BlockSize,
              CompareGuid(&PartEntry[Index].PartitionTypeGUID, &gEfiPartTypeSystemPartGuid)
              );
  }

  DEBUG ((EFI_D_INFO, "Prepare to Free Pool\n"));

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

  return GptValid;
}

STATIC
BOOLEAN
PartitionValidGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
/*++

Routine Description:
  Check if the GPT partition table is valid

Arguments:       
  BlockIo   - Parent BlockIo interface
  DiskIo    - Disk Io protocol.
  Lba       - The starting Lba of the Partition Table
  PartHeader   - Stores the partition table that is read

Returns:
  TRUE       - The partition table is valid
  FALSE      - The partition table is not valid

--*/
{
  EFI_STATUS                  Status;
  UINT32                      BlockSize;
  EFI_PARTITION_TABLE_HEADER  *PartHdr;

  BlockSize = BlockIo->Media->BlockSize;

  PartHdr   = AllocateZeroPool (BlockSize);

  if (PartHdr == NULL) {
    DEBUG ((EFI_D_ERROR, "Allocate pool error\n"));
    return FALSE;
  }
  //
  // Read the EFI Partition Table Header
  //
  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      BlockIo->Media->MediaId,
                      Lba,
                      BlockSize,
                      PartHdr
                      );
  if (EFI_ERROR (Status)) {
    FreePool (PartHdr);
    return FALSE;
  }

  if (CompareMem (&PartHdr->Header.Signature, EFI_PTAB_HEADER_ID, sizeof (UINT64)) != 0 ||
      !PartitionCheckCrc (BlockSize, &PartHdr->Header) ||
      PartHdr->MyLBA != Lba
      ) {
    DEBUG ((EFI_D_INFO, " !Valid efi partition table header\n"));
    FreePool (PartHdr);
    return FALSE;
  }

  CopyMem (PartHeader, PartHdr, sizeof (EFI_PARTITION_TABLE_HEADER));
  if (!PartitionCheckGptEntryArrayCRC (BlockIo, DiskIo, PartHeader)) {
    FreePool (PartHdr);
    return FALSE;
  }

  DEBUG ((EFI_D_INFO, " Valid efi partition table header\n"));
  FreePool (PartHdr);
  return TRUE;
}

STATIC
BOOLEAN
PartitionCheckGptEntryArrayCRC (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
/*++

Routine Description:

  Check if the CRC field in the Partition table header is valid 
  for Partition entry array

Arguments:

  BlockIo   - parent BlockIo interface 
  DiskIo    - Disk Io Protocol.
  PartHeader   - Partition table header structure

Returns:
  
  TRUE      - the CRC is valid
  FALSE     - the CRC is invalid

--*/
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
    DEBUG ((EFI_D_ERROR, " Allocate pool error\n"));
    return FALSE;
  }

  Status = DiskIo->ReadDisk (
                    DiskIo,
                    BlockIo->Media->MediaId,
                    MultU64x32(PartHeader->PartitionEntryLBA, BlockIo->Media->BlockSize),
                    PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                    Ptr
                    );
  if (EFI_ERROR (Status)) {
    FreePool (Ptr);
    return FALSE;
  }

  Size    = PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry;

  Status  = gBS->CalculateCrc32 (Ptr, Size, &Crc);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "CheckPEntryArrayCRC: Crc calculation failed\n"));
    FreePool (Ptr);
    return FALSE;
  }

  FreePool (Ptr);

  return (BOOLEAN) (PartHeader->PartitionEntryArrayCRC32 == Crc);
}

STATIC
BOOLEAN
PartitionRestoreGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  )
/*++

Routine Description:

  Restore Partition Table to its alternate place
  (Primary -> Backup or Backup -> Primary)

Arguments:

  BlockIo   - parent BlockIo interface 
  DiskIo    - Disk Io Protocol.
  PartHeader   - the source Partition table header structure

Returns:
  
  TRUE      - Restoring succeeds
  FALSE     - Restoring failed

--*/
{
  EFI_STATUS                  Status;
  UINTN                       BlockSize;
  EFI_PARTITION_TABLE_HEADER  *PartHdr;
  EFI_LBA                     PEntryLBA;
  UINT8                       *Ptr;

  PartHdr   = NULL;
  Ptr       = NULL;

  BlockSize = BlockIo->Media->BlockSize;

  PartHdr   = AllocateZeroPool (BlockSize);

  if (PartHdr == NULL) {
    DEBUG ((EFI_D_ERROR, "Allocate pool error\n"));
    return FALSE;
  }

  PEntryLBA = (PartHeader->MyLBA == PRIMARY_PART_HEADER_LBA) ? \
                             (PartHeader->LastUsableLBA + 1) : \
                             (PRIMARY_PART_HEADER_LBA + 1);

  CopyMem (PartHdr, PartHeader, sizeof (EFI_PARTITION_TABLE_HEADER));

  PartHdr->MyLBA              = PartHeader->AlternateLBA;
  PartHdr->AlternateLBA       = PartHeader->MyLBA;
  PartHdr->PartitionEntryLBA  = PEntryLBA;
  PartitionSetCrc ((EFI_TABLE_HEADER *) PartHdr);

  Status = BlockIo->WriteBlocks (BlockIo, BlockIo->Media->MediaId, PartHdr->MyLBA, BlockSize, PartHdr);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Ptr = AllocatePool (PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry);
  if (Ptr == NULL) {
    DEBUG ((EFI_D_ERROR, " Allocate pool effor\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = DiskIo->ReadDisk (
                    DiskIo,
                    BlockIo->Media->MediaId,
                    MultU64x32(PartHeader->PartitionEntryLBA, BlockIo->Media->BlockSize),
                    PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                    Ptr
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = DiskIo->WriteDisk (
                    DiskIo,
                    BlockIo->Media->MediaId,
                    MultU64x32(PEntryLBA, BlockIo->Media->BlockSize),
                    PartHeader->NumberOfPartitionEntries * PartHeader->SizeOfPartitionEntry,
                    Ptr
                    );

Done:
  FreePool (PartHdr);
  FreePool (Ptr);

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

STATIC
VOID
PartitionCheckGptEntry (
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader,
  IN  EFI_PARTITION_ENTRY         *PartEntry,
  OUT EFI_PARTITION_ENTRY_STATUS  *PEntryStatus
  )
/*++

Routine Description:

  Check each partition entry for its range

Arguments:

  PartHeader       - the partition table header
  PartEntry        - the partition entry array
  PEntryStatus  - the partition entry status array recording the status of
                  each partition

Returns:
  VOID

--*/
{
  EFI_LBA StartingLBA;
  EFI_LBA EndingLBA;
  UINTN   Index1;
  UINTN   Index2;

  DEBUG ((EFI_D_INFO, " start check partition entries\n"));
  for (Index1 = 0; Index1 < PartHeader->NumberOfPartitionEntries; Index1++) {
    if (CompareGuid (&PartEntry[Index1].PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
      continue;
    }

    StartingLBA = PartEntry[Index1].StartingLBA;
    EndingLBA   = PartEntry[Index1].EndingLBA;
    if (StartingLBA > EndingLBA ||
        StartingLBA < PartHeader->FirstUsableLBA ||
        StartingLBA > PartHeader->LastUsableLBA ||
        EndingLBA < PartHeader->FirstUsableLBA ||
        EndingLBA > PartHeader->LastUsableLBA
        ) {
      PEntryStatus[Index1].OutOfRange = TRUE;
      continue;
    }

    for (Index2 = Index1 + 1; Index2 < PartHeader->NumberOfPartitionEntries; Index2++) {

      if (CompareGuid (&PartEntry[Index2].PartitionTypeGUID, &gEfiPartTypeUnusedGuid)) {
        continue;
      }

      if (PartEntry[Index2].EndingLBA >= StartingLBA && PartEntry[Index2].StartingLBA <= EndingLBA) {
        //
        // This region overlaps with the Index1'th region
        //
        PEntryStatus[Index1].Overlap  = TRUE;
        PEntryStatus[Index2].Overlap  = TRUE;
        continue;

      }
    }
  }

  DEBUG ((EFI_D_INFO, " End check partition entries\n"));
}

STATIC
VOID
PartitionSetCrc (
  IN OUT EFI_TABLE_HEADER *Hdr
  )
/*++

Routine Description:

  Updates the CRC32 value in the table header

Arguments:

  Hdr     - The table to update

Returns:

  None

--*/
{
  PartitionSetCrcAltSize (Hdr->HeaderSize, Hdr);
}

STATIC
VOID
PartitionSetCrcAltSize (
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
/*++

Routine Description:

  Updates the CRC32 value in the table header

Arguments:

  Size    - The size of the table
  Hdr     - The table to update

Returns:

  None

--*/
{
  UINT32  Crc;

  Hdr->CRC32 = 0;
  gBS->CalculateCrc32 ((UINT8 *) Hdr, Size, &Crc);
  Hdr->CRC32 = Crc;
}

STATIC
BOOLEAN
PartitionCheckCrc (
  IN UINTN                 MaxSize,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
/*++

Routine Description:

  Checks the CRC32 value in the table header

Arguments:
  
  MaxSize - Max Size limit
  Hdr     - The table to check

Returns:

  TRUE if the CRC is OK in the table

--*/
{
  return PartitionCheckCrcAltSize (MaxSize, Hdr->HeaderSize, Hdr);
}

STATIC
BOOLEAN
PartitionCheckCrcAltSize (
  IN UINTN                 MaxSize,
  IN UINTN                 Size,
  IN OUT EFI_TABLE_HEADER  *Hdr
  )
/*++

Routine Description:

  Checks the CRC32 value in the table header

Arguments:
  
  MaxSize - Max Size Limit
  Size    - The size of the table
  Hdr     - The table to check

Returns:

  TRUE if the CRC is OK in the table

--*/
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

  if (MaxSize && Size > MaxSize) {
    DEBUG ((EFI_D_ERROR, "CheckCrc32: Size > MaxSize\n"));
    return FALSE;
  }
  //
  // clear old crc from header
  //
  OrgCrc      = Hdr->CRC32;
  Hdr->CRC32  = 0;

  Status      = gBS->CalculateCrc32 ((UINT8 *) Hdr, Size, &Crc);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "CheckCrc32: Crc calculation failed\n"));
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
      DEBUG ((EFI_D_ERROR, "CheckCrc32: Crc check failed\n"));
    }
  DEBUG_CODE_END ();

  return (BOOLEAN) (OrgCrc == Crc);
}
