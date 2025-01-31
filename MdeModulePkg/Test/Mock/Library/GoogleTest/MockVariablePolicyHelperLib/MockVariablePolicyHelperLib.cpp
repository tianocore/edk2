/** @file MockVariablePolicyHelperLib.cpp
  Google Test mocks for VariablePolicyHelperLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockVariablePolicyHelperLib.h>

MOCK_INTERFACE_DEFINITION (MockVariablePolicyHelperLib);
MOCK_FUNCTION_DEFINITION (MockVariablePolicyHelperLib, CreateBasicVariablePolicy, 8, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockVariablePolicyHelperLib, CreateVarStateVariablePolicy, 10, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockVariablePolicyHelperLib, RegisterBasicVariablePolicy, 8, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockVariablePolicyHelperLib, RegisterVarStateVariablePolicy, 10, EFIAPI);
