/** @file
  Migrated FV information

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
#define __EDKII_MIGRATED_FV_INFO_GUID_H__

#define FLAGS_SKIP_FV_MIGRATION        BIT0
#define FLAGS_SKIP_FV_RAW_DATA_COPY    BIT1

///
/// EDKII_TO_MIGRATE_FV_INFO Hob information should be published by platform to indicate
/// one FV is expected to migrate to permarnant memory or not before TempRam tears down.
///
typedef struct {
  UINT32     FvOrgBase;        // original FV address
  //
  // Migration Flags:
  // Bit0: Indicate to skip FV migration or not
  // Bit1: Indicate to skip FV raw data copy or not
  // Others: Reserved bits
  //
  UINT32     MigrationFlags;
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
