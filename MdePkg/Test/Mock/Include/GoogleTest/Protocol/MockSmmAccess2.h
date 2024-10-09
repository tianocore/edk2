/** @file MockSmmAccess2.h
  Declare mock SMM Access2 Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMM_ACCESS2_H_
#define MOCK_SMM_ACCESS2_H_

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

extern "C" {
  extern EFI_SMM_ACCESS2_PROTOCOL  *gSmmAccess2Protocol;
}

#endif
