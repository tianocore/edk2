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

struct MockEfiRscHandlerProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiRscHandlerProtocol);

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

MOCK_INTERFACE_DEFINITION (MockEfiRscHandlerProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiRscHandlerProtocol, Register, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiRscHandlerProtocol, Unregister, 1, EFIAPI);

#define MOCK_EFI_RSC_HANDLER_PROTOCOL_INSTANCE(NAME)  \
  EFI_RSC_HANDLER_PROTOCOL NAME##_INSTANCE = {        \
    Register,                                         \
    Unregister };                                     \
  EFI_RSC_HANDLER_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif
