/** @file MockIoLib.cpp
  Google Test mocks for IoLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockIoLib.h>

MOCK_INTERFACE_DEFINITION (MockIoLib);

MOCK_FUNCTION_DEFINITION (MockIoLib, MmioAndThenOr32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoWrite8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoRead8, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoRead64, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, IoWrite64, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioRead8, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioWrite8, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioRead16, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioWrite16, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioRead32, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioWrite32, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioRead64, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIoLib, MmioWrite64, 2, EFIAPI);
