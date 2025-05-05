/** @file MockSmmSxDispatch2.h
  This file declares a mock of SMM Software Dispatch Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_SX_DISPATCH2_H_
#define MOCK_SMM_SX_DISPATCH2_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmSxDispatch2.h>
}

// Declarations to handle usage of the EFI_SMM_SX_DISPATCH2_PROTOCOL
struct MockEfiSmmSxDispatch2Protocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSmmSxDispatch2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Register,
    (
     IN  CONST EFI_MM_SX_DISPATCH_PROTOCOL  *This,
     IN        EFI_MM_HANDLER_ENTRY_POINT   DispatchFunction,
     IN  CONST EFI_MM_SX_REGISTER_CONTEXT   *RegisterContext,
     OUT       EFI_HANDLE                   *DispatchHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UnRegister,
    (
     IN CONST EFI_MM_SX_DISPATCH_PROTOCOL  *This,
     IN       EFI_HANDLE                   DispatchHandle)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSmmSxDispatch2Protocol);
MOCK_FUNCTION_DEFINITION (MockEfiSmmSxDispatch2Protocol, Register, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSmmSxDispatch2Protocol, UnRegister, 2, EFIAPI);

#define MOCK_EFI_SMM_SX_DISPATCH2_PROTOCOL_INSTANCE(NAME)  \
 EFI_SMM_SX_DISPATCH2_PROTOCOL NAME##_INSTANCE = {          \
  Register,                                            \
  UnRegister };                                        \
 EFI_SMM_SX_DISPATCH2_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_SMM_SW_DISPATCH2_H_
