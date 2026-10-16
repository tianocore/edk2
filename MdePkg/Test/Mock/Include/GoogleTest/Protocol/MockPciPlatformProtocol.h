/** @file
  This file declares a mock of PCI Platform Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/PciPlatform.h>
}

struct MockPciPlatformPhaseNotify {
  MOCK_INTERFACE_DECLARATION (MockPciPlatformPhaseNotify);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPlatformNotify,
    (IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
     IN EFI_HANDLE                                     HostBridge,
     IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
     IN EFI_PCI_EXECUTION_PHASE                        ExecPhase
    )
    );
};

struct MockPciPlatformPreprocessController {
  MOCK_INTERFACE_DECLARATION (MockPciPlatformPreprocessController);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPlatformPrepController,
    (
     IN EFI_PCI_PLATFORM_PROTOCOL                     *This,
     IN EFI_HANDLE                                    HostBridge,
     IN EFI_HANDLE                                    RootBridge,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS   PciAddress,
     IN EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE  Phase,
     IN EFI_PCI_EXECUTION_PHASE                       ExecPhase
    )
    );
};

struct MockPciPlatformGetPlatformPolicy {
  MOCK_INTERFACE_DECLARATION (MockPciPlatformGetPlatformPolicy);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockGetPlatformPolicy,
    (
     IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
     OUT       EFI_PCI_PLATFORM_POLICY    *PciPolicy
    )
    );
};

struct MockPciPlatformGetPciRom {
  MOCK_INTERFACE_DECLARATION (MockPciPlatformGetPciRom);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockGetPciRom,
    (
     IN  CONST EFI_PCI_PLATFORM_PROTOCOL  *This,
     IN        EFI_HANDLE                 PciHandle,
     OUT       VOID                       **RomImage,
     OUT       UINTN                      *RomSize
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockPciPlatformPhaseNotify);
MOCK_FUNCTION_DEFINITION (MockPciPlatformPhaseNotify, MockPlatformNotify, 4, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciPlatformPreprocessController);
MOCK_FUNCTION_DEFINITION (MockPciPlatformPreprocessController, MockPlatformPrepController, 6, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciPlatformGetPlatformPolicy);
MOCK_FUNCTION_DEFINITION (MockPciPlatformGetPlatformPolicy, MockGetPlatformPolicy, 2, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciPlatformGetPciRom);
MOCK_FUNCTION_DEFINITION (MockPciPlatformGetPciRom, MockGetPciRom, 4, EFIAPI);

#define MOCK_EFI_PCI_PLATFORM_PROTOCOL_INSTANCE(NAME)  \
EFI_PCI_PLATFORM_PROTOCOL NAME##_INSTANCE = {          \
  MockPlatformNotify,                                  \
  MockPlatformPrepController,                          \
  MockGetPlatformPolicy,                               \
  MockGetPciRom,                                       \
};                                                     \
EFI_PCI_PLATFORM_PROTOCOL *NAME = &NAME##_INSTANCE;
