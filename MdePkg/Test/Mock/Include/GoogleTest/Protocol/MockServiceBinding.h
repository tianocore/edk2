/** @file MockServiceBinding.h
  This file declares a mock of Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SERVICE_BINDING_H_
#define MOCK_SERVICE_BINDING_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ServiceBinding.h>
}

struct MockEfiServiceBindingProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiServiceBindingProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CreateChild,
    (
     IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
     IN OUT EFI_HANDLE                    *ChildHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    DestroyChild,
    (
     IN EFI_SERVICE_BINDING_PROTOCOL          *This,
     IN EFI_HANDLE                            ChildHandle
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiServiceBindingProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiServiceBindingProtocol, CreateChild, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiServiceBindingProtocol, DestroyChild, 2, EFIAPI);

#define MOCK_EFI_SERVICE_BINDING_PROTOCOL_INSTANCE(NAME)  \
  EFI_SERVICE_BINDING_PROTOCOL NAME##_INSTANCE = {        \
    CreateChild,                                          \
    DestroyChild };                                       \
  EFI_SERVICE_BINDING_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_SERVICE_BINDING_H_
