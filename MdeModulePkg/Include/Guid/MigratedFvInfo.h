/** @file
  Migrated FV information

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
#define __EDKII_MIGRATED_FV_INFO_GUID_H__

typedef struct {
  UINT32    FvOrgBase;         // original FV address
  UINT32    FvNewBase;         // new FV address
  UINT32    FvDataBase;        // original FV data
  UINT32    FvLength;          // Fv Length
} EDKII_MIGRATED_FV_INFO;

extern EFI_GUID  gEdkiiMigratedFvInfoGuid;

#endif // #ifndef __EDKII_MIGRATED_FV_INFO_GUID_H__
