/** @file MockSmmAccess2.h
  Declare mock SMM Access2 Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiMultiPhase.h>
  #include <Protocol/SmmAccess2.h>
}

struct MockSmmAccess2Protocol {
  MOCK_INTERFACE_DECLARATION (MockSmmAccess2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Open,
    (IN EFI_MM_ACCESS_PROTOCOL  *This)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Close,
    (IN EFI_MM_ACCESS_PROTOCOL  *This)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Lock,
    (IN EFI_MM_ACCESS_PROTOCOL  *This)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetCapabilities,
    (IN CONST EFI_MM_ACCESS_PROTOCOL    *This,
     IN OUT UINTN                       *MmramMapSize,
     IN OUT EFI_MMRAM_DESCRIPTOR        *MmramMap)
    );
};

MOCK_INTERFACE_DEFINITION (MockSmmAccess2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Open, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Close, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Lock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, GetCapabilities, 3, EFIAPI);

static EFI_SMM_ACCESS2_PROTOCOL  SMM_ACCESS2_PROTOCOL_INSTANCE = {
  Open,
  Close,
  Lock,
  GetCapabilities,
  FALSE,
  FALSE
};

extern "C" {
  EFI_SMM_ACCESS2_PROTOCOL  *gSmmAccess2Protocol = &SMM_ACCESS2_PROTOCOL_INSTANCE;
}
