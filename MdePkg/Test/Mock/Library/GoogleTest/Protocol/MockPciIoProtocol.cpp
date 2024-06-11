/** @file MockPciIoProtocol.cpp
  Google Test mock for Pci Io Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockPciIoProtocol.h>

MOCK_INTERFACE_DEFINITION (MockPciIoConfigAccess);
MOCK_FUNCTION_DEFINITION (MockPciIoConfigAccess, MockPciIoRead, 5, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockPciIoProtocol);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocol, GetLocation, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocol, Attributes, 4, EFIAPI);

EFI_PCI_IO_PROTOCOL  PCI_IO_PROTOCOL_MOCK = {
  NULL,                    // EFI_PCI_IO_PROTOCOL_POLL_IO_MEM           PollMem;
  NULL,                    // EFI_PCI_IO_PROTOCOL_POLL_IO_MEM           PollIo;
  { NULL,          NULL }, // EFI_PCI_IO_PROTOCOL_ACCESS                Mem;
  { NULL,          NULL }, // EFI_PCI_IO_PROTOCOL_ACCESS                Io;
  { MockPciIoRead, NULL }, // EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS         Pci;
  NULL,                    // EFI_PCI_IO_PROTOCOL_COPY_MEM              CopyMem;
  NULL,                    // EFI_PCI_IO_PROTOCOL_MAP                   Map;
  NULL,                    // EFI_PCI_IO_PROTOCOL_UNMAP                 Unmap;
  NULL,                    // EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER       AllocateBuffer;
  NULL,                    // EFI_PCI_IO_PROTOCOL_FREE_BUFFER           FreeBuffer;
  NULL,                    // EFI_PCI_IO_PROTOCOL_FLUSH                 Flush;
  GetLocation,             // EFI_PCI_IO_PROTOCOL_GET_LOCATION          GetLocation;
  Attributes,              // EFI_PCI_IO_PROTOCOL_ATTRIBUTES            Attributes;
  NULL,                    // EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES    GetBarAttributes;
  NULL,                    // EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES    SetBarAttributes;
  0,                       // UINT64                                    RomSize;
  NULL,                    // VOID    *RomImage;
};

extern "C" {
  EFI_PCI_IO_PROTOCOL  *gPciIoProtocol = &PCI_IO_PROTOCOL_MOCK;
};
