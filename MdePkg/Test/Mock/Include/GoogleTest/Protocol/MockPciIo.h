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
struct MockEfiPciIoProtocolAccess {
  MOCK_INTERFACE_DECLARATION (MockEfiPciIoProtocolAccess);

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

struct MockEfiPciIoProtocolConfigAccess {
  MOCK_INTERFACE_DECLARATION (MockEfiPciIoProtocolConfigAccess);

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

struct MockEfiPciIoProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiPciIoProtocol);

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

MOCK_INTERFACE_DEFINITION (MockEfiPciIoProtocolAccess);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocolAccess, PciIoProtocolRead, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocolAccess, PciIoProtocolWrite, 6, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockEfiPciIoProtocolConfigAccess);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocolConfigAccess, PciIoRead, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocolConfigAccess, PciIoWrite, 5, EFIAPI);

MOCK_INTERFACE_DEFINITION (MockEfiPciIoProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, PollIoMem, 8, EFIAPI); // todo I can just reuse this?
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, MockCopyMem, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, Map, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, Unmap, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, AllocateBuffer, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, FreeBuffer, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, Flush, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, GetLocation, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, Attributes, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, GetBarAttributes, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiPciIoProtocol, SetBarAttributes, 5, EFIAPI);

#define MOCK_EFI_PCI_IO_PROTOCOL_INSTANCE(NAME) \
  EFI_PCI_IO_PROTOCOL NAME##_INSTANCE = {       \
    PollIoMem,                                  \
    PollIoMem,                                  \
    { PciIoProtocolRead, PciIoProtocolWrite },  \
    { PciIoProtocolRead, PciIoProtocolWrite },  \
    { PciIoRead,         PciIoWrite         },  \
    MockCopyMem,                                \
    Map,                                        \
    Unmap,                                      \
    AllocateBuffer,                             \
    FreeBuffer,                                 \
    Flush,                                      \
    GetLocation,                                \
    Attributes,                                 \
    GetBarAttributes,                           \
    SetBarAttributes,                           \
    0,                                          \
    NULL, };                                    \
  EFI_PCI_IO_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif //MOCK_PCI_IO_H_
