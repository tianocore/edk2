/** @file MockReportStatusCodeLib.cpp
  Google Test mocks for ReportStatusCodeLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockReportStatusCodeLib.h>

MOCK_INTERFACE_DEFINITION (MockReportStatusCodeLib);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCode, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCodeWithDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCodeWithExtendedData, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCodeEx, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportProgressCodeEnabled, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportErrorCodeEnabled, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportDebugCodeEnabled, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, CodeTypeToPostCode, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCodeExtractAssertInfo, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeLib, ReportStatusCodeExtractDebugInfo, 4, EFIAPI);
