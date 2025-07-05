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

struct MockEfiPeiRscHandlerPpi {
  MOCK_INTERFACE_DECLARATION (MockEfiPeiRscHandlerPpi);

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

MOCK_INTERFACE_DEFINITION (MockEfiPeiRscHandlerPpi);
MOCK_FUNCTION_DEFINITION (MockEfiPeiRscHandlerPpi, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPeiRscHandlerPpi, Unregister, 1, EFIAPI);

#define MOCK_EFI_PEI_RSC_HANDLER_PPI_INSTANCE(NAME) \
  EFI_PEI_RSC_HANDLER_PPI NAME##_INSTANCE = {       \
    Register,                                       \
    Unregister };                                   \
  EFI_PEI_RSC_HANDLER_PPI  *NAME = &NAME##_INSTANCE;

#endif
