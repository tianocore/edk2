/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_PAGE_TABLE_SUPPORT_H_
#define CPU_PAGE_TABLE_SUPPORT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <time.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestHostBaseLib.h>
#include <Library/BaseCryptLib.h>
#include "../CpuPageTable.h"

#define UNIT_TEST_APP_NAME     "Cpu Page Table Lib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

//
// Random Options
//

//
// Only test one-one mapping case
//
#define ONLY_ONE_ONE_MAPPING  0x00000001

//
// Change page table without using function PageTableMap, and use the modified page table as input
//
#define MANUAL_CHANGE_PAGE_TABLE  0x00000002

//
// Use pre-generated random number array to generate random number
//
#define USE_RANDOM_ARRAY  0x00000004

typedef struct {
  PAGING_MODE    PagingMode;
  UINTN          TestCount;
  UINTN          TestRangeCount;
  UINTN          RandomOption;
} CPU_PAGE_TABLE_LIB_RANDOM_TEST_CONTEXT;

/**
  Random Test

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestCaseforRandomTest (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Init global data

  @param[in]   MemorySpace    Memory space
**/
VOID
InitGlobalData (
  UINTN  MemorySpace
  );

/**
  Check if the Page table is valid

  @param[in]   PageTable      The pointer to the page table.
  @param[in]   PagingMode     The paging mode.

  @retval  UNIT_TEST_PASSED   It is a valid Page Table
**/
UNIT_TEST_STATUS
IsPageTableValid (
  IN     UINTN        PageTable,
  IN     PAGING_MODE  PagingMode
  );

/**
  Get max physical adrress supported by specific page mode

  @param[in]  Mode           The paging mode.

  @retval  max address.
**/
UINT64
GetMaxAddress (
  IN PAGING_MODE  Mode
  );

#endif
