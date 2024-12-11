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

struct MockEfiIsaHcProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiIsaHcProtocol);

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

MOCK_INTERFACE_DEFINITION (MockEfiIsaHcProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiIsaHcProtocol, OpenIoAperture, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiIsaHcProtocol, CloseIoAperture, 2, EFIAPI);

#define MOCK_EFI_ISA_HC_PROTOCOL_INSTANCE(NAME) \
  EFI_ISA_HC_PROTOCOL NAME##_INSTANCE = {       \
    0,                                          \
    OpenIoAperture,                             \
    CloseIoAperture };                          \
  EFI_ISA_HC_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_ISA_HC_PROTOCOL_H_
