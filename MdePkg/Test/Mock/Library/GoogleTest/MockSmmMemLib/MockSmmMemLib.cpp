/** @file MockSmmMemLib.cpp
  Google Test mocks for SmmMemLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockSmmMemLib.h>

MOCK_INTERFACE_DEFINITION (MockSmmMemLib);

MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmIsBufferOutsideSmmValid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmCopyMemToSmram, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmCopyMemFromSmram, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmCopyMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmSetMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmMemLib, SmmCommBufferValid, 2, EFIAPI);
