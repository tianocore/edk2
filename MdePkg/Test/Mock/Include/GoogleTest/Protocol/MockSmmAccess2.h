/** @file
  EFI SMM Access2 Protocol as defined in the PI 1.2 specification.

  This protocol is used to control the visibility of the SMRAM on the platform.
  It abstracts the location and characteristics of SMRAM.  The expectation is
  that the north bridge or memory controller would publish this protocol.

  The principal functionality found in the memory controller includes the following:
  - Exposing the SMRAM to all non-SMM agents, or the "open" state
  - Shrouding the SMRAM to all but the SMM agents, or the "closed" state
  - Preserving the system integrity, or "locking" the SMRAM, such that the settings cannot be
    perturbed by either boot service or runtime agents

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
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

MOCK_INTERFACE_DEFINITION (MockSmmAccess2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Open, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Close, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Lock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, GetCapabilities, 3, EFIAPI);

EFI_SMM_ACCESS2_PROTOCOL  SMM_ACCESS2_PROTOCOL_INSTANCE = {
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

#endif
