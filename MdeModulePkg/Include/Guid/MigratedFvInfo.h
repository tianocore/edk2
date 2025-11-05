/** @file
  Migrated FV information

Copyright (c) 2020 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
#define __EDKII_MIGRATED_FV_INFO_GUID_H__

//
// FLAGS_FV_RAW_DATA_COPY indicates FV raw data will be copied to permanent memory
// or not. When FV is migrated to permanent memory, it will be rebased and raw
// data will be lost. This bit can be configured as below values:
// 0: FV raw data will not be used in later phase, and the copy will be skipped to
//    optimize boot performance.
// 1: FV raw data will be copied to permanent memory for later phase use (such as
//    FV measurement).
//
#define FLAGS_FV_RAW_DATA_COPY                    BIT0
#define FLAGS_FV_MIGRATE_BEFORE_PEI_CORE_REENTRY  BIT1

///
/// In real use cases, not all FVs need migrate to permanent memory before TempRam tears
/// down. EDKII_MIGRATION_INFO hob should be published by platform to indicate which
/// FVs need migration to optimize boot performance. If this hob is not detected by Pei
/// Core, all FVs on TempRam will be migrated and FV raw data will also be copied.
/// Only one EDKII_MIGRATION_INFO hob should be published by platform, and this hob will
/// take effect only when migration feature is enabled by PCD.
///
typedef struct {
  UINT32    FvOrgBaseOnTempRam;        // Original FV address on Temporary Ram
  //
  // FV Migration Flags:
  // Bit0: Indicate to copy FV raw data or not
  // Others: Reserved bits
  //
  UINT32    FvMigrationFlags;
} TO_MIGRATE_FV_INFO;

typedef struct {
  BOOLEAN    MigrateAll;                    // Migrate all FVs and also copy FV raw data
  //
  // ToMigrateFvCount and ToMigrateFvInfo array indicate which FVs need be migrated, and
  // these info should be ignored when MigrateAll field is set to TRUE.
  //
  UINT32     ToMigrateFvCount;
  // TO_MIGRATE_FV_INFO    ToMigrateFvInfo[];
} EDKII_MIGRATION_INFO;

typedef struct {
  UINT32    FvOrgBase;         // original FV address
  UINT32    FvNewBase;         // new FV address, 0 means rebased data is not copied
  UINT32    FvDataBase;        // original FV data, 0 means raw data is not copied
  UINT32    FvLength;          // Fv Length
} EDKII_MIGRATED_FV_INFO;

extern EFI_GUID  gEdkiiMigrationInfoGuid;
extern EFI_GUID  gEdkiiMigratedFvInfoGuid;

#endif // #ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
