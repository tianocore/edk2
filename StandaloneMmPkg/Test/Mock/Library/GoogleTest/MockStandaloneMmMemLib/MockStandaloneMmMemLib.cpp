/** @file MockStandaloneMmMemLib.cpp
  Google Test mocks for StandaloneMmMemLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockStandaloneMmMemLib.h>

MOCK_INTERFACE_DEFINITION (MockMmMemLib);

MOCK_FUNCTION_DEFINITION (MockMmMemLib, MmIsBufferOutsideMmValid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmMemLib, MmCopyMemToMmram, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmMemLib, MmCopyMemFromMmram, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmMemLib, MmCopyMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmMemLib, MmSetMem, 3, EFIAPI);
