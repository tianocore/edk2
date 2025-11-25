/** @file MockServiceBinding.h
  This file declares a mock of Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ServiceBinding.h>
}

struct MockServiceBinding {
  MOCK_INTERFACE_DECLARATION (MockServiceBinding);

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

MOCK_INTERFACE_DEFINITION (MockServiceBinding);
MOCK_FUNCTION_DEFINITION (MockServiceBinding, CreateChild, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockServiceBinding, DestroyChild, 2, EFIAPI);

static EFI_SERVICE_BINDING_PROTOCOL  SERVICE_BINDING_PROTOCOL_INSTANCE = {
  CreateChild,
  DestroyChild
};

extern "C" {
  EFI_SERVICE_BINDING_PROTOCOL  *gServiceBindingProtocol = &SERVICE_BINDING_PROTOCOL_INSTANCE;
}
