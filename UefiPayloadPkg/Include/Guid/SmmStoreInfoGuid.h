/** @file
  This file defines the hob structure for coreboot's SmmStore.

  Copyright (c) 2022, 9elements GmbH<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMMSTORE_GUID_H_
#define SMMSTORE_GUID_H_

///
/// System Table Information GUID
///
extern EFI_GUID  gEfiSmmStoreInfoHobGuid;

typedef struct {
  UINT64    ComBuffer;
  UINT32    ComBufferSize;
  UINT32    NumBlocks;
  UINT32    BlockSize;
  UINT64    MmioAddress;
  UINT8     ApmCmd;
  UINT8     Reserved0[3];
} SMMSTORE_INFO;

#endif // SMMSTORE_GUID_H_
