/** @file
  This file declares a mock of EFI_PCI_IO_PROTOCOL.

  Only Pci.Read() is mocked so far -- extend with additional
  MOCK_FUNCTION_DECLARATION/DEFINITION pairs (and instance fields) as
  other consumers need more of this protocol's surface.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/PciIo.h>
}

struct MockPciIoProtocol {
  MOCK_INTERFACE_DECLARATION (MockPciIoProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PciRead,
    (IN     EFI_PCI_IO_PROTOCOL           *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH     Width,
     IN     UINT32                        Offset,
     IN     UINTN                         Count,
     IN OUT VOID                          *Buffer)
    );
};

extern "C" {
  extern EFI_PCI_IO_PROTOCOL  *gPciIoProtocol;
}
