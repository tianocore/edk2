/** @file MockRng.h
  This file declares a mock of Rng Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_RNG_H_
#define MOCK_RNG_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/Rng.h>
}

struct MockEfiRngProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiRngProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetInfo,
    (
     IN EFI_RNG_PROTOCOL             *This,
     IN OUT UINTN                    *RNGAlgorithmListSize,
     OUT EFI_RNG_ALGORITHM           *RNGAlgorithmList
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetRng,
    (
     IN EFI_RNG_PROTOCOL            *This,
     IN EFI_RNG_ALGORITHM           *RNGAlgorithm,
     IN UINTN                       RNGValueLength,
     OUT UINT8                      *RNGValue
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiRngProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiRngProtocol, GetInfo, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiRngProtocol, GetRng, 4, EFIAPI);

#define MOCK_EFI_RNG_PROTOCOL_INSTANCE(NAME)  \
  EFI_RNG_PROTOCOL NAME##_INSTANCE = {        \
    GetInfo,                                  \
    GetRng };                                 \
  EFI_RNG_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_RNG_H_
