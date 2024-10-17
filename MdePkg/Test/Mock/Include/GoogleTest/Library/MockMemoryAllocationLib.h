/** @file MockMemoryAllocationLib.h
  Google Test mocks for MemoryAllocationLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MEMORY_ALLOCATION_LIB_H_
#define MOCK_MEMORY_ALLOCATION_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiPei.h>
  #include <PiDxe.h>
  #include <PiSmm.h>
  #include <PiMm.h>
  #include <Uefi.h>
  #include <Library/MemoryAllocationLib.h>
}

struct MockMemoryAllocationLib {
  MOCK_INTERFACE_DECLARATION (MockMemoryAllocationLib);

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocatePages,
    (IN UINTN  Pages)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateRuntimePages,
    (IN UINTN  Pages)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateReservedPages,
    (IN UINTN  Pages)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    FreePages,
    (IN VOID   *Buffer,
     IN UINTN  Pages)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateAlignedPages,
    (IN UINTN  Pages,
     IN UINTN  Alignment)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateAlignedRuntimePages,
    (IN UINTN  Pages,
     IN UINTN  Alignment)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateAlignedReservedPages,
    (IN UINTN  Pages,
     IN UINTN  Alignment)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    FreeAlignedPages,
    (IN VOID   *Buffer,
     IN UINTN  Pages)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocatePool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateRuntimePool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateReservedPool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateZeroPool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateRuntimeZeroPool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateReservedZeroPool,
    (IN UINTN  AllocationSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateCopyPool,
    (IN       UINTN  AllocationSize,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateRuntimeCopyPool,
    (IN       UINTN  AllocationSize,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocateReservedCopyPool,
    (IN       UINTN  AllocationSize,
     IN CONST VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ReallocatePool,
    (IN UINTN  OldSize,
     IN UINTN  NewSize,
     IN VOID   *OldBuffer OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ReallocateRuntimePool,
    (IN UINTN  OldSize,
     IN UINTN  NewSize,
     IN VOID   *OldBuffer OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    ReallocateReservedPool,
    (IN UINTN  OldSize,
     IN UINTN  NewSize,
     IN VOID   *OldBuffer OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    FreePool,
    (IN VOID  *Buffer)
    );
};

#endif // MOCK_MEMORY_ALLOCATION_LIB_H_
