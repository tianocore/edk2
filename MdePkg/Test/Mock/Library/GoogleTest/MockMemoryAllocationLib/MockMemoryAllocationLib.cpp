/** @file MockMemoryAllocationLib.cpp
  Google Test mocks for MemoryAllocationLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockMemoryAllocationLib.h>

MOCK_INTERFACE_DEFINITION (MockMemoryAllocationLib);

MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocatePages, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateRuntimePages, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateReservedPages, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, FreePages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateAlignedPages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateAlignedRuntimePages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateAlignedReservedPages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, FreeAlignedPages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocatePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateRuntimePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateReservedPool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateZeroPool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateRuntimeZeroPool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateReservedZeroPool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateCopyPool, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateRuntimeCopyPool, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, AllocateReservedCopyPool, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, ReallocatePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, ReallocateRuntimePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, ReallocateReservedPool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemoryAllocationLib, FreePool, 1, EFIAPI);
