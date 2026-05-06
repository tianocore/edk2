/** @file
  Google Test mocks for PerformanceLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPerformanceLib.h>

MOCK_INTERFACE_DEFINITION (MockPerformanceLib);

MOCK_FUNCTION_DEFINITION (MockPerformanceLib, StartPerformanceMeasurement, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, EndPerformanceMeasurement, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, GetPerformanceMeasurement, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, StartPerformanceMeasurementEx, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, EndPerformanceMeasurementEx, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, GetPerformanceMeasurementEx, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, PerformanceMeasurementEnabled, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, LogPerformanceMeasurementEnabled, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPerformanceLib, LogPerformanceMeasurement, 5, EFIAPI);
