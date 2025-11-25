/** @file
  This file declares a mock of PCI IO Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/PciIo.h>
}

//
// Declarations to handle usage of the Pci Io Protocol by creating mock
//
struct MockPciIoConfigAccess {
  MOCK_INTERFACE_DECLARATION (MockPciIoConfigAccess);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciIoRead,
    (IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT32                       Offset,
     IN     UINTN                        Count,
     IN OUT VOID                         *Buffer)
    );
};

struct MockPciIoProtocol {
  MOCK_INTERFACE_DECLARATION (MockPciIoProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetLocation,
    (
     IN EFI_PCI_IO_PROTOCOL          *This,
     OUT UINTN                       *SegmentNumber,
     OUT UINTN                       *BusNumber,
     OUT UINTN                       *DeviceNumber,
     OUT UINTN                       *FunctionNumber
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Attributes,
    (
     IN EFI_PCI_IO_PROTOCOL                       *This,
     IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
     IN  UINT64                                   Attributes,
     OUT UINT64                                   *Result OPTIONAL
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockPciIoConfigAccess);
MOCK_FUNCTION_DEFINITION (MockPciIoConfigAccess, MockPciIoRead, 5, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockPciIoProtocol);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocol, GetLocation, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocol, Attributes, 4, EFIAPI);

static EFI_PCI_IO_PROTOCOL  PCI_IO_PROTOCOL_MOCK = {
  NULL,
  NULL,
  { NULL,     NULL    },
  { NULL,     NULL    },
  { MockPciIoRead,NULL    },
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  GetLocation,
  Attributes,
  NULL,
  NULL,
  0,
  NULL
};

extern "C" {
  EFI_PCI_IO_PROTOCOL  *gPciIoProtocol = &PCI_IO_PROTOCOL_MOCK;
}
