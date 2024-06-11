/** @file MockServiceBinding.cpp
  Google Test mock for Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockServiceBinding.h>

MOCK_INTERFACE_DEFINITION (MockServiceBinding);
MOCK_FUNCTION_DEFINITION (MockServiceBinding, CreateChild, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockServiceBinding, DestroyChild, 2, EFIAPI);

EFI_SERVICE_BINDING_PROTOCOL  SERVICE_BINDING_PROTOCOL_INSTANCE = {
  CreateChild,    // EFI_SERVICE_BINDING_CREATE_CHILD
  DestroyChild,   // EFI_SERVICE_BINDING_DESTROY_CHILD
};

extern "C" {
  EFI_SERVICE_BINDING_PROTOCOL  *gServiceBindingProtocol = &SERVICE_BINDING_PROTOCOL_INSTANCE;
}
