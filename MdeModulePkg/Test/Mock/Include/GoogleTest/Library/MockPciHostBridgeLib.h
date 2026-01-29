/** @file
  Google Test mocks for PciHostBridgeLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PCI_HOST_BRIDGE_LIB_H_
#define MOCK_PCI_HOST_BRIDGE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PciHostBridgeLib.h>
}

struct MockPciHostBridgeLib {
  MOCK_INTERFACE_DECLARATION (MockPciHostBridgeLib);

  MOCK_FUNCTION_DECLARATION (
    PCI_ROOT_BRIDGE *,
    PciHostBridgeGetRootBridges,
    (UINTN  *Count)
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    PciHostBridgeFreeRootBridges,
    (PCI_ROOT_BRIDGE  *Bridges,
     UINTN            Count)
    );
  MOCK_FUNCTION_DECLARATION (
    VOID,
    PciHostBridgeResourceConflict,
    (EFI_HANDLE  HostBridgeHandle,
     VOID        *Configuration)
    );
};

#endif
