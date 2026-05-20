/** @file
  This file defines the hob structure for coreboot's SmmStore.

  Copyright (c) 2022, 9elements GmbH<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
} SMMSTORE_INFO;
