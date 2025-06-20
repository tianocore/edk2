/** @file
  Google Test mocks for UefiRuntimeLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockUefiRuntimeLib.h>

//
// Global Variables that are not const
//

MOCK_INTERFACE_DEFINITION (MockUefiRuntimeLib);

MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiAtRuntime, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGoneVirtual, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGetTime, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiSetTime, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGetWakeupTime, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiSetWakeupTime, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGetVariable, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGetNextVariableName, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiSetVariable, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiGetNextHighMonotonicCount, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiResetSystem, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiConvertPointer, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiConvertFunctionPointer, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiSetVirtualAddressMap, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiConvertList, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiUpdateCapsule, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiQueryCapsuleCapabilities, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiRuntimeLib, EfiQueryVariableInfo, 4, EFIAPI);
