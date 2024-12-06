/** @file MockSmmReportStatusCodeHandler.h
  This file declares a mock of SMM Report Status Code Handler Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H
#define MOCK_SMM_REPORT_STATUS_CODE_HANDLER_PROTOCOL_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/ReportStatusCodeLib.h>
  #include <Protocol/SmmReportStatusCodeHandler.h>
}

struct MockEfiSmmRscHandlerProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSmmRscHandlerProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Register,
    (
     IN EFI_SMM_RSC_HANDLER_CALLBACK   Callback)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Unregister,
    (IN EFI_SMM_RSC_HANDLER_CALLBACK Callback)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSmmRscHandlerProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiSmmRscHandlerProtocol, Register, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSmmRscHandlerProtocol, Unregister, 1, EFIAPI);

#define MOCK_EFI_SMM_RSC_HANDLER_PROTOCOL_INSTANCE(NAME)  \
  EFI_SMM_RSC_HANDLER_PROTOCOL NAME##_INSTANCE = {        \
    Register,                                             \
    Unregister };                                         \
  EFI_SMM_RSC_HANDLER_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif
