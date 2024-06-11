/** @file MockIsaHc.cpp
  Google Test mock for Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockIsaHc.h>

MOCK_INTERFACE_DEFINITION (MockIsaHcProtocol);
MOCK_FUNCTION_DEFINITION (MockIsaHcProtocol, OpenIoAperture, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIsaHcProtocol, CloseIoAperture, 2, EFIAPI);

EFI_ISA_HC_PROTOCOL  ISA_HC_PROTOCOL_INSTANCE = {
  0,                  // UINT32                 Version;
  OpenIoAperture,     // EFI_ISA_HC_OPEN_IO     OpenIoAperture;
  CloseIoAperture     // EFI_ISA_HC_CLOSE_IO    CloseIoAperture;
};

extern "C" {
  EFI_ISA_HC_PROTOCOL  *gIsaHcProtocol = &ISA_HC_PROTOCOL_INSTANCE;
}
