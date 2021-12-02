/** @file
  Intel FSP Non-Volatile Storage (NVS) HOB version 2 definition from
  Intel Firmware Support Package External Architecture Specification v2.3.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_NON_VOLATILE_STORAGE_HOB2_H__
#define __FSP_NON_VOLATILE_STORAGE_HOB2_H__

///
/// The Non-Volatile Storage (NVS) HOB version 2 provides > 64KB buffer support.
///
typedef struct {
  EFI_HOB_GUID_TYPE       GuidHob;
  EFI_PHYSICAL_ADDRESS    NvsDataPtr;
  UINT64                  NvsDataLength;
} FSP_NON_VOLATILE_STORAGE_HOB2;

extern EFI_GUID  gFspNonVolatileStorageHob2Guid;

#endif
