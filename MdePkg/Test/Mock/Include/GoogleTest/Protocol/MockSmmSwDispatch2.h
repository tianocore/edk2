/** @file MockSmmSwDispatch2.h
  This file declares a mock of SMM Software Dispatch Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmSwDispatch2.h>
}

// Declarations to handle usage of the EFI_SMM_SW_DISPATCH2_PROTOCOL
struct MockEfiSmmSwDispatch2Protocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSmmSwDispatch2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockRegister,
    (
     IN  CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
     IN        EFI_SMM_HANDLER_ENTRY_POINT2   DispatchFunction,
     IN  OUT   EFI_SMM_SW_REGISTER_CONTEXT    *RegisterContext,
     OUT       EFI_HANDLE                     *DispatchHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockUnRegister,
    (
     IN CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
     IN       EFI_HANDLE                     DispatchHandle)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSmmSwDispatch2Protocol);
MOCK_FUNCTION_DEFINITION (MockEfiSmmSwDispatch2Protocol, MockRegister, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSmmSwDispatch2Protocol, MockUnRegister, 2, EFIAPI);

#define MOCK_EFI_SMM_SW_DISPATCH2_PROTOCOL_INSTANCE(NAME)  \
 EFI_SMM_SW_DISPATCH2_PROTOCOL NAME##_INSTANCE = {          \
   MockRegister,                                            \
   MockUnRegister };                                        \
 EFI_SMM_SW_DISPATCH2_PROTOCOL  *NAME = &NAME##_INSTANCE;
