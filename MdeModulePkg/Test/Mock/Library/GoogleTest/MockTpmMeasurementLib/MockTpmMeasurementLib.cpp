/** @file MockTpmMeasurementlib.cpp
  Google Test mocks for TpmMeasurementlib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockTpmMeasurementLib.h>

MOCK_INTERFACE_DEFINITION (MockTpmMeasurementLib);
MOCK_FUNCTION_DEFINITION (MockTpmMeasurementLib, TpmMeasureAndLogData, 6, EFIAPI);
