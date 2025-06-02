/** @file MockSxDispatch.h
  This file declares a mock of SMM/MM Sx Dispatch Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SX_DISPATCH_H_
#define MOCK_SX_DISPATCH_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmSxDispatch2.h>
}

// Declarations to handle usage of the Sx Dispatch Protocol
struct MockEfiSxDispatch2Protocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSxDispatch2Protocol);

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

MOCK_INTERFACE_DEFINITION (MockEfiSxDispatch2Protocol);
MOCK_FUNCTION_DEFINITION (MockEfiSxDispatch2Protocol, Register, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSxDispatch2Protocol, UnRegister, 2, EFIAPI);

#define MOCK_EFI_SMM_SX_DISPATCH2_PROTOCOL_INSTANCE(NAME)  \
 EFI_SMM_SX_DISPATCH2_PROTOCOL NAME##_INSTANCE = {          \
  Register,                                            \
  UnRegister };                                        \
 EFI_SMM_SX_DISPATCH2_PROTOCOL  *NAME = &NAME##_INSTANCE;

#define MOCK_EFI_MM_SX_DISPATCH_PROTOCOL_INSTANCE(NAME)  \
 EFI_MM_SX_DISPATCH_PROTOCOL NAME##_INSTANCE = {          \
  Register,                                            \
  UnRegister };                                        \
 EFI_MM_SX_DISPATCH_PROTOCOL  *NAME = &NAME##_INSTANCE;
#endif // MOCK_SX_DISPATCH_H_
