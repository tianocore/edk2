/** @file
  Internal header for Random test.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RANDOM_TEST_H_
#define RANDOM_TEST_H_

#include "CpuPageTableLibUnitTest.h"

typedef struct _ALLOCATE_PAGE_RECORDS ALLOCATE_PAGE_RECORDS;

typedef
VOID *
(EFIAPI *ALLOCATE_PAGES)(
  IN ALLOCATE_PAGE_RECORDS  *PagesRecord,
  IN UINTN                  Pages
  );

typedef struct {
  VOID     *Buffer;
  UINTN    Pages;
} ALLOCATE_PAGE_RECORD;

struct _ALLOCATE_PAGE_RECORDS {
  UINTN                   Count;
  UINTN                   MaxCount;
  ALLOCATE_PAGES          AllocatePagesForPageTable;
  ALLOCATE_PAGE_RECORD    Records[0];
};

typedef struct {
  UINT64                LinearAddress;
  UINT64                Length;
  IA32_MAP_ATTRIBUTE    Attribute;
  IA32_MAP_ATTRIBUTE    Mask;
} MAP_ENTRY;

typedef struct {
  UINTN        Count;
  UINTN        InitCount;
  UINTN        MaxCount;
  MAP_ENTRY    Maps[10];
} MAP_ENTRYS;

UINT64
GetEntryFromPageTable (
  IN     UINTN        PageTable,
  IN     PAGING_MODE  PagingMode,
  IN     UINT64       Address,
  OUT    UINTN        *Level
  );

#endif
