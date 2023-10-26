/** @file
  Migrated FV information

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
#define __EDKII_MIGRATED_FV_INFO_GUID_H__

//
// FLAGS_SKIP_FV_RAW_DATA_COPY indicates FV raw data will be skipped copy to permanent
// memory or not. When FV is migrated to permanent memory, it will be rebased and raw
// data will be lost. This bit can be configured as below values:
// 0: FV raw data will be copied to permanent memory for later phase use.
// 1: FV raw data will not be consumed in later phase, and the copy will be skipped to
//    optimize boot performance.
//
#define FLAGS_SKIP_FV_RAW_DATA_COPY  BIT0

///
/// In real use cases, not all FVs need migrate to permanent memory before TempRam tears
/// down. EDKII_TO_MIGRATE_FV_INFO hob can be published by platform to indicate only selected
/// FVs need migration, and other FVs should be skipped to optimize boot performance.
/// If no such hobs are published, PEI Core should still migrate all FVs on TempRam when this
/// feature is enabled.
///
typedef struct {
  UINT32    FvTemporaryRamBase;        // Original FV address on Temporary Ram
  //
  // Migration Flags:
  // Bit0: Indicate to skip FV raw data copy or not
  // Others: Reserved bits
  //
  UINT32    MigrationFlags;
} EDKII_TO_MIGRATE_FV_INFO;

typedef struct {
  UINT32    FvOrgBase;         // original FV address
  UINT32    FvNewBase;         // new FV address
  UINT32    FvDataBase;        // original FV data
  UINT32    FvLength;          // Fv Length
} EDKII_MIGRATED_FV_INFO;

extern EFI_GUID  gEdkiiToMigrateFvInfoGuid;
extern EFI_GUID  gEdkiiMigratedFvInfoGuid;

#endif // #ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
