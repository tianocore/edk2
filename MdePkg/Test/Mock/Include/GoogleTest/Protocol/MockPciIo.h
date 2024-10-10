/** @file MockPciIo.h
  Google Test mocks for PciIo

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_PCI_IO_H_
#define MOCK_PCI_IO_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Protocol/PciIo.h>
}

struct MockPciIoProtocolAccess {
  MOCK_INTERFACE_DECLARATION (MockPciIoProtocolAccess);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PciIoProtocolRead,
    (IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT8                        BarIndex,
     IN     UINT64                       Offset,
     IN     UINTN                        Count,
     IN OUT VOID                         *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PciIoProtocolWrite,
    (IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT8                        BarIndex,
     IN     UINT64                       Offset,
     IN     UINTN                        Count,
     IN OUT VOID                         *Buffer)
    );
};

struct MockPciIoConfigAccess {
  MOCK_INTERFACE_DECLARATION (MockPciIoConfigAccess);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PciIoRead,
    (IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT32                       Offset,
     IN     UINTN                        Count,
     IN OUT VOID                         *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PciIoWrite,
    (IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT32                       Offset,
     IN     UINTN                        Count,
     IN OUT VOID                         *Buffer)
    );
};

struct MockPciIo {
  MOCK_INTERFACE_DECLARATION (MockPciIo);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PollIoMem,
    (
     IN EFI_PCI_IO_PROTOCOL           *This,
     IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN  UINT8                        BarIndex,
     IN  UINT64                       Offset,
     IN  UINT64                       Mask,
     IN  UINT64                       Value,
     IN  UINT64                       Delay,
     OUT UINT64                       *Result
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockCopyMem, // adding mock to the name to avoid conflict with CopyMem in Uefi.h
    (
     IN EFI_PCI_IO_PROTOCOL              *This,
     IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
     IN     UINT8                        DestBarIndex,
     IN     UINT64                       DestOffset,
     IN     UINT8                        SrcBarIndex,
     IN     UINT64                       SrcOffset,
     IN     UINTN                        Count
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Map,
    (
     IN EFI_PCI_IO_PROTOCOL                *This,
     IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
     IN     VOID                           *HostAddress,
     IN OUT UINTN                          *NumberOfBytes,
     OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
     OUT    VOID                           **Mapping
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Unmap,
    (
     IN EFI_PCI_IO_PROTOCOL           *This,
     IN  VOID                         *Mapping
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AllocateBuffer,
    (
     IN EFI_PCI_IO_PROTOCOL           *This,
     IN  EFI_ALLOCATE_TYPE            Type,
     IN  EFI_MEMORY_TYPE              MemoryType,
     IN  UINTN                        Pages,
     OUT VOID                         **HostAddress,
     IN  UINT64                       Attributes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FreeBuffer,
    (
     IN EFI_PCI_IO_PROTOCOL           *This,
     IN  UINTN                        Pages,
     IN  VOID                         *HostAddress
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Flush,
    (
     IN EFI_PCI_IO_PROTOCOL  *This
    )
    );

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

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetBarAttributes,
    (
     IN EFI_PCI_IO_PROTOCOL             *This,
     IN  UINT8                          BarIndex,
     OUT UINT64                         *Supports  OPTIONAL,
     OUT VOID                           **Resources OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetBarAttributes,
    (
     IN EFI_PCI_IO_PROTOCOL              *This,
     IN     UINT64                       Attributes,
     IN     UINT8                        BarIndex,
     IN OUT UINT64                       *Offset,
     IN OUT UINT64                       *Length
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockPciIoProtocolAccess);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocolAccess, PciIoProtocolRead, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIoProtocolAccess, PciIoProtocolWrite, 6, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockPciIoConfigAccess);
MOCK_FUNCTION_DEFINITION (MockPciIoConfigAccess, PciIoRead, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIoConfigAccess, PciIoWrite, 5, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockPciIo);
MOCK_FUNCTION_DEFINITION (MockPciIo, PollIoMem, 8, EFIAPI); // todo I can just reuse this?
MOCK_FUNCTION_DEFINITION (MockPciIo, MockCopyMem, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, Map, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, Unmap, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, AllocateBuffer, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, FreeBuffer, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, Flush, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, GetLocation, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, Attributes, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, GetBarAttributes, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciIo, SetBarAttributes, 5, EFIAPI);

EFI_PCI_IO_PROTOCOL  PCI_IO_PROTOCOL_MOCK = {
  PollIoMem,                                 // EFI_PCI_IO_PROTOCOL_POLL_IO_MEM           PollMem;
  PollIoMem,                                 // EFI_PCI_IO_PROTOCOL_POLL_IO_MEM           PollIo;
  { PciIoProtocolRead, PciIoProtocolWrite }, // EFI_PCI_IO_PROTOCOL_ACCESS                Mem;
  { PciIoProtocolRead, PciIoProtocolWrite }, // EFI_PCI_IO_PROTOCOL_ACCESS                Io;
  { PciIoRead,         PciIoWrite         }, // EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS         Pci;
  MockCopyMem,                               // EFI_PCI_IO_PROTOCOL_COPY_MEM              CopyMem;
  Map,                                       // EFI_PCI_IO_PROTOCOL_MAP                   Map;
  Unmap,                                     // EFI_PCI_IO_PROTOCOL_UNMAP                 Unmap;
  AllocateBuffer,                            // EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER       AllocateBuffer;
  FreeBuffer,                                // EFI_PCI_IO_PROTOCOL_FREE_BUFFER           FreeBuffer;
  Flush,                                     // EFI_PCI_IO_PROTOCOL_FLUSH                 Flush;
  GetLocation,                               // EFI_PCI_IO_PROTOCOL_GET_LOCATION          GetLocation;
  Attributes,                                // EFI_PCI_IO_PROTOCOL_ATTRIBUTES            Attributes;
  GetBarAttributes,                          // EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES    GetBarAttributes;
  SetBarAttributes,                          // EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES    SetBarAttributes;
  0,                                         // UINT64                                    RomSize;
  NULL,                                      // VOID    *RomImage;
};

extern "C" {
  EFI_PCI_IO_PROTOCOL  *gPciIoProtocol = &PCI_IO_PROTOCOL_MOCK;
};

#endif //MOCK_PCI_IO_H_
