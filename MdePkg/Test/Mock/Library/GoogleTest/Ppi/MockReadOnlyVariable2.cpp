/** @file MockReadOnlyVariable2.cpp
  Google Test mock for ReadOnlyVariable2

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Ppi/MockReadOnlyVariable2.h>

MOCK_INTERFACE_DEFINITION (MockReadOnlyVariable2);
MOCK_FUNCTION_DEFINITION (MockReadOnlyVariable2, GetVariable, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReadOnlyVariable2, NextVariableName, 4, EFIAPI);

// Normally PpiVariableServices is "found"
// This will be defined INSIDE the test, with its definition pointing to the mock function GetVariable
EFI_PEI_READ_ONLY_VARIABLE2_PPI  PeiReadOnlyVariablePpi = {
  GetVariable,      // EFI_PEI_GET_VARIABLE2
  NextVariableName  // EFI_PEI_GET_NEXT_VARIABLE_NAME2
};

extern "C" {
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PpiReadOnlyVariableServices = &PeiReadOnlyVariablePpi;
}
