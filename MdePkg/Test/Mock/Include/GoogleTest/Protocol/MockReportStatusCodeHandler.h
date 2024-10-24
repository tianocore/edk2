/** @file MockReportStatusCodeHandler.h
  This file declares a mock of Report Status Code Handler Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H
#define MOCK_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
  #include <Protocol/ReportStatusCodeHandler.h>
}

struct MockReportStatusCodeHandler {
  MOCK_INTERFACE_DECLARATION (MockReportStatusCodeHandler);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Register,
    (
     IN EFI_RSC_HANDLER_CALLBACK   Callback,
     IN EFI_TPL                    Tpl)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Unregister,
    (IN EFI_RSC_HANDLER_CALLBACK Callback)
    );
};

MOCK_INTERFACE_DEFINITION (MockReportStatusCodeHandler);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Register, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockReportStatusCodeHandler, Unregister, 1, EFIAPI);

EFI_RSC_HANDLER_PROTOCOL  RscHandlerProtocol = {
  Register,
  Unregister
};

extern "C" {
  EFI_RSC_HANDLER_PROTOCOL  *RscHandlerProtocolServices = &RscHandlerProtocol;
}

#endif
