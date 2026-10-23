/** @file MockIsaHc.h
  This file declares a mock of Isa Hc Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/IsaHc.h>
}

struct MockIsaHcProtocol {
  MOCK_INTERFACE_DECLARATION (MockIsaHcProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    OpenIoAperture,
    (
     IN CONST EFI_ISA_HC_PROTOCOL  *This,
     IN UINT16                     IoAddress,
     IN UINT16                     IoLength,
     OUT UINT64                    *IoApertureHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CloseIoAperture,
    (
     IN CONST EFI_ISA_HC_PROTOCOL      *This,
     IN UINT64                         IoApertureHandle
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockIsaHcProtocol);
MOCK_FUNCTION_DEFINITION (MockIsaHcProtocol, OpenIoAperture, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockIsaHcProtocol, CloseIoAperture, 2, EFIAPI);

static EFI_ISA_HC_PROTOCOL  ISA_HC_PROTOCOL_INSTANCE = {
  0,
  OpenIoAperture,
  CloseIoAperture
};

extern "C" {
  EFI_ISA_HC_PROTOCOL  *gIsaHcProtocol = &ISA_HC_PROTOCOL_INSTANCE;
}
