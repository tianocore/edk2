/** @file MockGenericMemoryTest.h
  This file declares a mock of generic memory test Protocol.

  Copyright (c) 2024, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/GenericMemoryTest.h>
}
struct MockMemTestProtocol {
  MOCK_INTERFACE_DECLARATION (MockMemTestProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MemoryTestInit,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     IN  EXTENDMEM_COVERAGE_LEVEL                 Level,
     OUT BOOLEAN                                  *RequireSoftECCInit
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PerformMemoryTest,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     OUT UINT64                                   *TestedMemorySize,
     OUT UINT64                                   *TotalMemorySize,
     OUT BOOLEAN                                  *ErrorOut,
     IN  BOOLEAN                                  IfTestAbort
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Finished,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CompatibleRangeTest,
    (
     IN  EFI_GENERIC_MEMORY_TEST_PROTOCOL         *This,
     IN  EFI_PHYSICAL_ADDRESS                     StartAddress,
     IN  UINT64                                   Length
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockMemTestProtocol);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, MemoryTestInit, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, PerformMemoryTest, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, Finished, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMemTestProtocol, CompatibleRangeTest, 3, EFIAPI);

static EFI_GENERIC_MEMORY_TEST_PROTOCOL  MEMORY_TEST_PROTOCOL_INSTANCE = {
  MemoryTestInit,                                                                       // EFI_MEMORY_TEST_INIT
  PerformMemoryTest,                                                                    // EFI_PERFORM_MEMORY_TEST
  Finished,                                                                             // EFI_MEMORY_TEST_FINISHED
  CompatibleRangeTest                                                                   // EFI_MEMORY_TEST_COMPATIBLE_RANGE
};

extern "C" {
  EFI_GENERIC_MEMORY_TEST_PROTOCOL  *gMemTestProtocol = &MEMORY_TEST_PROTOCOL_INSTANCE;
}
