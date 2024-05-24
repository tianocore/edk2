/** @file
  Defines the HOB GUID for UnblockRegion.

  MM_UNBLOCK_REGION structure including following parameters:
  1.MemoryDescriptor: It's to describe the memory region that need to be unblocked.
  2.IdentifierGuid: It indicates which driver unblock the memory region.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MM_UNBLOCK_REGION_H_
#define MM_UNBLOCK_REGION_H_

#define MM_UNBLOCK_REGION_GUID \
  { \
    0x7c316fb3, 0x849e, 0x4ee7, {0x87, 0xfc, 0x16, 0x2d, 0x0b, 0x03, 0x42, 0xbf } \
  }

typedef struct {
  EFI_MEMORY_DESCRIPTOR    MemoryDescriptor;
  EFI_GUID                 IdentifierGuid;
} MM_UNBLOCK_REGION;

extern EFI_GUID  gMmUnblockRegionHobGuid;

#endif
