/** @file MockReportStatusCodeLib.h
  Google Test mocks for ReportStatusCodeLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
}

struct MockReportStatusCodeLib {
  MOCK_INTERFACE_DECLARATION (MockReportStatusCodeLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ReportProgressCodeEnabled,
    ()
    );
};
