/** @file
  Shared mock disk and GPT construction helpers used by the GptLib
  host-based unit tests.

  Copyright (c) 2026, SUSE LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Library/GptLib.h>

#define SECTOR_SIZE            512U
#define DISK_SECTORS           4096U
#define DISK_IMAGE_SIZE        (DISK_SECTORS * SECTOR_SIZE)
#define LAST_LBA               (DISK_SECTORS - 1U)
#define NUM_PARTITION_ENTRIES  128U
#define PARTITION_ENTRY_SIZE   128U
#define PART_ARRAY_SIZE        (NUM_PARTITION_ENTRIES * PARTITION_ENTRY_SIZE)
#define FIRST_USABLE_LBA       34U
#define LAST_USABLE_LBA        (DISK_SECTORS - 34U)

//
// The only GPT header revision defined by the UEFI specification, and the
// minimum on-disk header size (through PartitionEntryArrayCRC32).
//
#define TEST_GPT_REVISION_V1      0x00010000U
#define TEST_GPT_HEADER_MIN_SIZE  92U

extern UINT8  mDiskImage[DISK_IMAGE_SIZE];

extern EFI_BLOCK_IO_MEDIA     mBlockIoMedia;
extern EFI_BLOCK_IO_PROTOCOL  mBlockIo;
extern EFI_DISK_IO_PROTOCOL   mDiskIo;
extern BOOLEAN                mWriteProtected;

extern CONST EFI_GUID  mEspTypeGuid;
extern CONST EFI_GUID  mPartitionGuid1;
extern CONST EFI_GUID  mPartitionGuid2;

UINT8 *
GetDiskLba (
  IN EFI_LBA  Lba
  );

EFI_PARTITION_TABLE_HEADER *
GetPrimaryHeader (
  VOID
  );

VOID
UpdateGptHeaderCrc (
  IN OUT EFI_PARTITION_TABLE_HEADER  *Header
  );

VOID
FillPartitionEntry (
  IN OUT EFI_PARTITION_ENTRY  *Entry,
  IN CONST EFI_GUID           *UniquePartitionGuid,
  IN EFI_LBA                  StartingLba,
  IN EFI_LBA                  EndingLba
  );

/**
  Write a self-consistent GPT table (header plus partition entry array) to the
  mock disk at the requested locations, computing correct header and
  entry-array CRC32 values.

  @param[in]  HeaderLba      LBA where the GPT header is written. Also stored
                             in the header's MyLBA field.
  @param[in]  AlternateLba   Value stored in the header's AlternateLBA field.
  @param[in]  EntryArrayLba  LBA where the partition entry array is written.
  @param[in]  NumEntries     NumberOfPartitionEntries value.
  @param[in]  EntrySize      SizeOfPartitionEntry value.
  @param[in]  NumPartitions  Number of non-empty partition entries to populate.
**/
VOID
WriteGptTableAt (
  IN EFI_LBA  HeaderLba,
  IN EFI_LBA  AlternateLba,
  IN EFI_LBA  EntryArrayLba,
  IN UINT32   NumEntries,
  IN UINT32   EntrySize,
  IN UINT32   NumPartitions
  );

/**
  Reset the mock disk to a single valid primary GPT: header at LBA 1, a
  128 x 128-byte entry array at LBA 2 holding two partitions.
**/
VOID
SetupDiskWithValidPrimary (
  VOID
  );

BOOLEAN
ValidateAt (
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *Header
  );

VOID
InitCheckEntryHeader (
  OUT EFI_PARTITION_TABLE_HEADER  *Header,
  IN  UINT32                      NumEntries,
  IN  UINT32                      EntrySize
  );
