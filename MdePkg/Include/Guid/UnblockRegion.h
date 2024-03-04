/** @file
  Defines the HOB GUID for UnblockRegion.
  Copyright (c) 2015 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef MM_UNBLOCK_REGION_H_
#define MM_UNBLOCK_REGION_H_
#define MM_UNBLOCK_REGION_GUID \
  { \
    0x7c316fb3, 0x849e, 0x4ee7, {0x87, 0xfc, 0x16, 0x2d, 0x0b, 0x03, 0x42, 0xbf } \
  }
extern EFI_GUID  gMmUnblockRegionHobGuid;

typedef struct {
	EFI_MEMORY_DESCRIPTOR MemoryDescriptor;
	EFI_GUID          IdentifierGuid;
} MM_UNBLOCK_REGION;
#endif
