/** @file MockPeiReportStatusCodeHandler.h
  This file declares a mock of Report Status Code Handler PPI.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PEI_REPORT_STATUS_CODE_HANDLER_PPI_H
#define MOCK_PEI_REPORT_STATUS_CODE_HANDLER_PPI_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiPeiCis.h>
  #include <Ppi/ReportStatusCodeHandler.h>
}

struct MockPeiReportStatusCodeHandler {
  MOCK_INTERFACE_DECLARATION (MockPeiReportStatusCodeHandler);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Register,
    (IN EFI_PEI_RSC_HANDLER_CALLBACK Callback)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Unregister,
    (IN EFI_PEI_RSC_HANDLER_CALLBACK Callback)
    );
};

MOCK_INTERFACE_DEFINITION (MockPeiReportStatusCodeHandler);
MOCK_FUNCTION_DEFINITION (MockPeiReportStatusCodeHandler, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPeiReportStatusCodeHandler, Unregister, 1, EFIAPI);

EFI_PEI_RSC_HANDLER_PPI  PeiRscHandlerPpi = {
  Register,
  Unregister
};

extern "C" {
  EFI_PEI_RSC_HANDLER_PPI  *PeiRscHandlerPpiServices = &PeiRscHandlerPpi;
}

#endif
