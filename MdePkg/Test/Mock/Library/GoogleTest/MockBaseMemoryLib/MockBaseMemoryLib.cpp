/** @file MockBaseMemoryLib.cpp
  Google Test mocks for BaseMemoryLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockBaseMemoryLib.h>

MOCK_INTERFACE_DEFINITION (MockBaseMemoryLib);

MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, CopyMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, SetMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, SetMem16, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, SetMem32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, SetMem64, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, SetMemN, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ZeroMem, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, CompareMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanMem8, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanMem16, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanMem32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanMem64, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanMemN, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, CopyGuid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, CompareGuid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, ScanGuid, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, IsZeroGuid, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBaseMemoryLib, IsZeroBuffer, 2, EFIAPI);
