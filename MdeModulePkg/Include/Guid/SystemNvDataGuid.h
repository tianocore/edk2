/** @file
  This file defines NvDataFv GUID and FTW working block structures.
  The NvDataFv GUID can be used as FileSystemGuid in EFI_FIRMWARE_VOLUME_HEADER if
  this FV image contains NV data, such as NV variable data.
  This file also defines WorkingBlockSignature GUID for FTW working block signature.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SYSTEM_NV_DATA_GUID_H__
#define __SYSTEM_NV_DATA_GUID_H__

#define EFI_SYSTEM_NV_DATA_FV_GUID \
  {0xfff12b8d, 0x7696, 0x4c8b, {0xa9, 0x85, 0x27, 0x47, 0x7, 0x5b, 0x4f, 0x50} }

#define EDKII_WORKING_BLOCK_SIGNATURE_GUID \
  {0x9e58292b, 0x7c68, 0x497d, {0xa0, 0xce, 0x65,  0x0, 0xfd, 0x9f, 0x1b, 0x95} }

extern EFI_GUID gEfiSystemNvDataFvGuid;
extern EFI_GUID gEdkiiWorkingBlockSignatureGuid;

#define WORKING_BLOCK_VALID   0x1
#define WORKING_BLOCK_INVALID 0x2

///
/// The EDKII Fault tolerant working block header.
/// The header is immediately followed by the write queue data.
///
typedef struct {
  ///
  /// FTW working block signature.
  /// Its value has be updated from gEfiSystemNvDataFvGuid to gEdkiiWorkingBlockSignatureGuid,
  /// because its write queue data format has been updated to support the crossing archs.
  ///
  EFI_GUID  Signature;
  ///
  /// 32bit CRC calculated for this header.
  ///
  UINT32    Crc;
  ///
  /// Working block valid bit.
  ///
  UINT8     WorkingBlockValid : 1;
  UINT8     WorkingBlockInvalid : 1;
  UINT8     Reserved : 6;
  UINT8     Reserved3[3];
  ///
  /// Total size of the following write queue range.
  ///
  UINT64    WriteQueueSize;
  ///
  /// Write Queue data.
  ///
  /// EFI_FAULT_TOLERANT_WRITE_HEADER FtwHeader;
  /// EFI_FAULT_TOLERANT_WRITE_RECORD FtwRecord[FtwHeader.NumberOfWrites]
  /// EFI_FAULT_TOLERANT_WRITE_HEADER FtwHeader2;
  /// EFI_FAULT_TOLERANT_WRITE_RECORD FtwRecord2[FtwHeader2.NumberOfWrites]
  /// ...
  ///
} EFI_FAULT_TOLERANT_WORKING_BLOCK_HEADER;

#define FTW_VALID_STATE     0
#define FTW_INVALID_STATE   1

//
// EFI Fault tolerant block update write queue entry.
//
typedef struct {
  UINT8     HeaderAllocated : 1;
  UINT8     WritesAllocated : 1;
  UINT8     Complete : 1;
  UINT8     Reserved : 5;
  EFI_GUID  CallerId;
  UINT64    NumberOfWrites;
  UINT64    PrivateDataSize;
} EFI_FAULT_TOLERANT_WRITE_HEADER;

//
// EFI Fault tolerant block update write queue record.
//
typedef struct {
  UINT8   BootBlockUpdate : 1;
  UINT8   SpareComplete : 1;
  UINT8   DestinationComplete : 1;
  UINT8   Reserved : 5;
  EFI_LBA Lba;
  UINT64  Offset;
  UINT64  Length;
  //
  // Relative offset to spare block.
  //
  INT64   RelativeOffset;
  //
  // UINT8    PrivateData[PrivateDataSize]
  //
} EFI_FAULT_TOLERANT_WRITE_RECORD;

#define FTW_RECORD_SIZE(PrivateDataSize)  (sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD) + (UINTN) PrivateDataSize)

#define FTW_RECORD_TOTAL_SIZE(NumberOfWrites, PrivateDataSize) \
    ((UINTN) (NumberOfWrites) * (sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD) + (UINTN) PrivateDataSize))

#define FTW_WRITE_TOTAL_SIZE(NumberOfWrites, PrivateDataSize) \
    ( \
      sizeof (EFI_FAULT_TOLERANT_WRITE_HEADER) + (UINTN) (NumberOfWrites) * \
      (sizeof (EFI_FAULT_TOLERANT_WRITE_RECORD) + (UINTN) PrivateDataSize) \
    )

#endif
