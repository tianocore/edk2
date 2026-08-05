/** @file
  Shared GUID Partition Table (GPT) parsing and validation routines.

  These routines decode and validate a disk partitioned with the GPT scheme
  as described in the UEFI specification. They are shared between the
  Partition driver (which installs child handles) and other consumers that
  need to parse the same on-disk GPT layout.

  Caution: These routines may receive untrusted input. The GPT partition
  table is external input and must be validated carefully to avoid security
  issues like buffer overflow and integer overflow.

Copyright (c) 2026, SUSE LLC. All rights reserved.<BR>
Copyright (c) 2018 Qualcomm Datacenter Technologies, Inc.
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <Guid/Gpt.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>

//
// GPT Partition Entry Status
//
typedef struct {
  BOOLEAN    OutOfRange;
  BOOLEAN    Overlap;
  BOOLEAN    OsSpecific;
} EFI_PARTITION_ENTRY_STATUS;

/**
  Read GPT partition table header from the given LBA and validate it.

  Caution: This function may receive untrusted input.
  The GPT partition table header is external input, so this routine
  will do basic validation for GPT partition table header before return.

  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  DiskIo      Disk Io protocol.
  @param[in]  Lba         The starting Lba of the Partition Table.
  @param[out] PartHeader  Stores the partition table that is read.

  @retval TRUE      The partition table is valid.
  @retval FALSE     The partition table is not valid.

**/
BOOLEAN
PartitionValidGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_LBA                     Lba,
  OUT EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

/**
  Restore Partition Table to its alternate place
  (Primary -> Backup or Backup -> Primary).

  @param[in]  BlockIo     Parent BlockIo interface.
  @param[in]  DiskIo      Disk Io Protocol.
  @param[in]  PartHeader  Partition table header structure.

  @retval TRUE      Restoring succeeds.
  @retval FALSE     Restoring failed.

**/
BOOLEAN
PartitionRestoreGptTable (
  IN  EFI_BLOCK_IO_PROTOCOL       *BlockIo,
  IN  EFI_DISK_IO_PROTOCOL        *DiskIo,
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader
  );

/**
  Check GPT partition entries and report the status of each entry.

  Caution: This function may receive untrusted input.
  The GPT partition entry is external input, so this routine
  will do basic validation for GPT partition entry and report status.

  @param[in]    PartHeader    Partition table header structure.
  @param[in]    PartEntry     The partition entry array.
  @param[out]   PEntryStatus  The partition entry status array
                              recording the status of each partition.

**/
VOID
PartitionCheckGptEntry (
  IN  EFI_PARTITION_TABLE_HEADER  *PartHeader,
  IN  EFI_PARTITION_ENTRY         *PartEntry,
  OUT EFI_PARTITION_ENTRY_STATUS  *PEntryStatus
  );
