/** @file
  This file defines the hob structure for system tables like ACPI, SMBIOS tables.

  Copyright (c) 2020, 9elements Agency GmbH<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMMSTORE_GUID_H__
#define __SMMSTORE_GUID_H__

///
/// System Table Information GUID
///
extern EFI_GUID gEfiSMMSTOREInfoHobGuid;

typedef struct {
  UINT64    ComBuffer;
  UINT32    ComBufferSize;
  UINT32    NumBlocks;
  UINT32    BlockSize;
  UINT64    MmioAddress;
  UINT8     ApmCmd;
  UINT8     Reserved0[3];
} SMMSTORE_INFO;

#endif
