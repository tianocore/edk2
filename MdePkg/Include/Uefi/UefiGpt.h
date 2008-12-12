/** @file
  EFI Guid Partition Table Format Definition.

  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_GPT_H__
#define __UEFI_GPT_H__

///
/// The primary GUID Partition Table Header must be
/// located in LBA 1 (i.e., the second logical block).
///
#define PRIMARY_PART_HEADER_LBA 1
///
/// EFI Partition Table Signature: "EFI PART"
/// 
#define EFI_PTAB_HEADER_ID      SIGNATURE_64 ('E','F','I',' ','P','A','R','T')

#pragma pack(1)

///
/// GPT Partition Table Header
///
typedef struct {
  ///
  /// The table header for the GPT partition Table.
  /// This header contains EFI_PTAB_HEADER_ID
  ///
  EFI_TABLE_HEADER  Header;
  ///
  /// The LBA that contains this data structure.
  ///
  EFI_LBA           MyLBA;
  ///
  /// LBA address of the alternate GUID Partition Table Header.
  ///
  EFI_LBA           AlternateLBA;
  ///
  /// The first usable logical block that may be used
  /// by a partition described by a GUID Partition Entry.
  ///
  EFI_LBA           FirstUsableLBA;
  ///
  /// The last usable logical block that may be used
  /// by a partition described by a GUID Partition Entry.
  ///
  EFI_LBA           LastUsableLBA;
  ///
  /// GUID that can be used to uniquely identify the disk.
  ///
  EFI_GUID          DiskGUID;
  ///
  /// The starting LBA of the GUID Partition Entry array.
  ///
  EFI_LBA           PartitionEntryLBA;
  ///
  /// The number of Partition Entries in the GUID Partition Entry array.
  ///
  UINT32            NumberOfPartitionEntries;
  ///
  /// The size, in bytes, of each the GUID Partition
  /// Entry structures in the GUID Partition Entry
  /// array. Must be a multiple of 8.
  ///
  UINT32            SizeOfPartitionEntry;
  ///
  /// The CRC32 of the GUID Partition Entry array.
  /// Starts at PartitionEntryLBA and is
  /// computed over a byte length of
  /// NumberOfPartitionEntries * SizeOfPartitionEntry.
  ///
  UINT32            PartitionEntryArrayCRC32;
} EFI_PARTITION_TABLE_HEADER;

///
/// GPT Partition Entry
///
typedef struct {
  ///
  /// Unique ID that defines the purpose and type of this Partition. A value of
  /// zero defines that this partition entry is not being used.
  ///
  EFI_GUID  PartitionTypeGUID;
  ///
  /// GUID that is unique for every partition entry. Every partition ever
  /// created will have a unique GUID.
  /// This GUID must be assigned when the GUID Partition Entry is created.
  ///
  EFI_GUID  UniquePartitionGUID;
  ///
  /// Starting LBA of the partition defined by this entry
  ///
  EFI_LBA   StartingLBA;
  ///
  /// Ending LBA of the partition defined by this entry.
  ///
  EFI_LBA   EndingLBA;
  ///
  /// Attribute bits, all bits reserved by UEFI
  /// Bit 0 Required for the platform to function.
  /// Bits 1-47 Undefined and must be zero.
  /// Bits 48-63 Reserved for GUID specific use.
  ///
  UINT64    Attributes;
  ///
  /// Unicode string.
  ///
  CHAR16    PartitionName[36];
} EFI_PARTITION_ENTRY;

#pragma pack()
#endif


