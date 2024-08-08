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

extern "C" {
  extern EFI_SMM_BASE2_PROTOCOL  *gSmmBase2Protocol;
}

#endif // MOCK_SMM_BASE2_H_
