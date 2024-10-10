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

struct MockSmmBase2Protocol {
  MOCK_INTERFACE_DECLARATION (MockSmmBase2Protocol);

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

MOCK_INTERFACE_DEFINITION (MockSmmBase2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmBase2Protocol, InSmm, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmBase2Protocol, GetSmstLocation, 2, EFIAPI);

EFI_SMM_BASE2_PROTOCOL  SMMBASE2_PROTOCOL_INSTANCE = {
  InSmm,           // EFI_SMM_INSIDE_OUT2;
  GetSmstLocation  // EFI_SMM_GET_SMST_LOCATION2;
};

extern "C" {
  EFI_SMM_BASE2_PROTOCOL  *gSmmBase2Protocol = &SMMBASE2_PROTOCOL_INSTANCE;
}

#endif // MOCK_SMM_BASE2_H_
