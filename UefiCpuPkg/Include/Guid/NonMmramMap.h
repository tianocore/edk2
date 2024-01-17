/** @file
  This file defines a NON_MMRAM_MAP structure which is used
  as interface for SMM CPU driver to apply read only or read
  proctection for specific non-mmram memory ranges.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NON_MMRAM_MAP_H_
#define NON_MMRAM_MAP_H_

#define NON_MMRAM_MAP_GUID \
  { \
    0x4a743869, 0xed0c, 0x4ef8, {0x94, 0x4e, 0x82, 0xdf, 0x6c, 0x8b, 0xcb, 0x9d}  \
  }

typedef struct {
  ///
  /// Physical address of the first byte in the memory region. PhysicalStart must be
  /// aligned on a 4 KiB boundary, and must not be above 0xfffffffffffff000.
  ///
  EFI_PHYSICAL_ADDRESS    PhysicalStart;
  ///
  /// NumberOfPagesNumber of 4 KiB pages in the memory region.
  /// NumberOfPages must not be 0, and must not be any value
  /// that would represent a memory page with a start address,
  /// either physical or virtual, above 0xfffffffffffff000.
  ///
  UINT64                  NumberOfPages;
  ///
  /// Attributes of the memory region that describe the bit mask of capabilities
  /// for that memory region. Only EFI_MEMORY_RP and EFI_MEMORY_RO bits are valid.
  /// EFI_MEMORY_XP is ignored since NX is natural applied for all non-mmram memory.
  ///
  UINT64                  Attribute;
} NON_MMRAM_MAP_ENTRY;

typedef struct {
  ///
  /// Describes the number of NON_MMRAM_MAP_ENTRY.
  ///
  UINT64                 NumberOfEntry;
  ///
  /// Non SMM memory.
  ///
  NON_MMRAM_MAP_ENTRY    Entry[0];
} NON_MMRAM_MAP;

extern EFI_GUID  gNonMmramMapGuid;
#endif
