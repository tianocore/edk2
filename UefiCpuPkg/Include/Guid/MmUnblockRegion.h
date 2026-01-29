/** @file
  Defines the GUIDed HOB that describes the memory region to be unblocked in MM environment.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MM_UNBLOCK_REGION_H_
#define MM_UNBLOCK_REGION_H_

///
/// The GUID of the UnblockRegion GUIDed HOB.
///
#define MM_UNBLOCK_REGION_HOB_GUID \
  { \
    0x7c316fb3, 0x849e, 0x4ee7, {0x87, 0xfc, 0x16, 0x2d, 0x0b, 0x03, 0x42, 0xbf } \
  }

///
/// The structure defines the data layout of the UnblockRegion GUIDed HOB.
///
typedef struct {
  ///
  /// Physical address of the first byte in the memory region. PhysicalStart must be
  /// aligned on a 4 KiB boundary.
  ///
  EFI_PHYSICAL_ADDRESS    PhysicalStart;

  ///
  /// Number of 4 KiB pages in the memory region.
  ///
  UINT64                  NumberOfPages;

  ///
  /// GUID to identify the memory region.
  ///
  EFI_GUID                IdentifierGuid;
} MM_UNBLOCK_REGION;

extern EFI_GUID  gMmUnblockRegionHobGuid;

#endif
