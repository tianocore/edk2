/** @file
  Internal header for CpuPageTableLib.

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_PAGE_TABLE_H_
#define CPU_PAGE_TABLE_H_

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuPageTableLib.h>

#define REGION_LENGTH(l)  LShiftU64 (1, (l) * 9 + 3)

#define MAX_PAE_PDPTE_NUM  4

/**
  Return TRUE when the page table entry is a leaf entry that points to the physical address memory.
  Return FALSE when the page table entry is a non-leaf entry that points to the page table entries.

  @param[in] PagingEntry Pointer to the page table entry.
  @param[in] Level       Page level where the page table entry resides in.

  @retval TRUE  It's a leaf entry.
  @retval FALSE It's a non-leaf entry.
**/
BOOLEAN
IsPle (
  IN     IA32_PAGING_ENTRY  *PagingEntry,
  IN     UINTN              Level
  );

/**
  Return the attribute of a 2M/1G page table entry.

  @param[in] PleB               Pointer to a 2M/1G page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the 2M/1G page table entry.
**/
UINT64
PageTableLibGetPleBMapAttribute (
  IN IA32_PAGE_LEAF_ENTRY_BIG_PAGESIZE  *PleB,
  IN IA32_MAP_ATTRIBUTE                 *ParentMapAttribute
  );

/**
  Return the attribute of a non-leaf page table entry.

  @param[in] Pnle               Pointer to a non-leaf page table entry.
  @param[in] ParentMapAttribute Pointer to the parent attribute.

  @return Attribute of the non-leaf page table entry.
**/
UINT64
PageTableLibGetPnleMapAttribute (
  IN IA32_PAGE_NON_LEAF_ENTRY  *Pnle,
  IN IA32_MAP_ATTRIBUTE        *ParentMapAttribute
  );

#endif
