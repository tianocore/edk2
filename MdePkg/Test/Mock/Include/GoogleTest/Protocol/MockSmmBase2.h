/** @file MockSmmBase2.h
  This file declares a mock of SmmBase2 Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_BASE2_H_
#define MOCK_SMM_BASE2_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SmmBase2.h>
}

struct MockEfiSmmBase2Protocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSmmBase2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    InSmm,
    (
     IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
     OUT BOOLEAN                      *InSmram
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetSmstLocation,
    (
     IN CONST EFI_SMM_BASE2_PROTOCOL  *This,
     IN OUT EFI_SMM_SYSTEM_TABLE2     **Smst
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSmmBase2Protocol);
MOCK_FUNCTION_DEFINITION (MockEfiSmmBase2Protocol, InSmm, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSmmBase2Protocol, GetSmstLocation, 2, EFIAPI);

#define MOCK_EFI_SMM_BASE2_PROTOCOL_INSTANCE(NAME)  \
  EFI_SMM_BASE2_PROTOCOL NAME##_INSTANCE = {        \
    InSmm,                                          \
    GetSmstLocation };                              \
  EFI_SMM_BASE2_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_SMM_BASE2_H_
