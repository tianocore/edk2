/** @file MockIsaHc.h
  This file declares a mock of Isa Hc Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_ISA_HC_PROTOCOL_H_
#define MOCK_ISA_HC_PROTOCOL_H_

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

EFI_ISA_HC_PROTOCOL  ISA_HC_PROTOCOL_INSTANCE = {
  0,                  // UINT32                 Version;
  OpenIoAperture,     // EFI_ISA_HC_OPEN_IO     OpenIoAperture;
  CloseIoAperture     // EFI_ISA_HC_CLOSE_IO    CloseIoAperture;
};

extern "C" {
  EFI_ISA_HC_PROTOCOL  *gIsaHcProtocol = &ISA_HC_PROTOCOL_INSTANCE;
}

#endif // MOCK_ISA_HC_PROTOCOL_H_
