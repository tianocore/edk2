/** @file
  This file declares a mock of PCI Root Bridge IO Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C"
{
  #include <Uefi.h>
  #include <Protocol/PciRootBridgeIo.h>
}

//
// Declarations to handle usage of the Pci Root Bridge Io Protocol by creating mock
//
struct MockPciRootBridgeIoPollIoMem {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoPollIoMem);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgePollMem,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINT64 Mask,
     IN UINT64 Value,
     IN UINT64 Delay,
     OUT UINT64 *Result)
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgePollIo,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINT64 Mask,
     IN UINT64 Value,
     IN UINT64 Delay,
     OUT UINT64 *Result)
    );
};

struct MockPciRootBridgeIoConfigAccess {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoConfigAccess);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeMemRead,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeMemWrite,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeIoRead,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeIoWrite,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgePciRead,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgePciWrite,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 Address,
     IN UINTN Count,
     IN OUT VOID *Buffer)
    );
};

struct MockPciRootBridgeIoCopyMem {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoCopyMem);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeCopyMem,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH Width,
     IN UINT64 DestAddress,
     IN UINT64 SrcAddress,
     IN UINTN Count)
    );
};

struct MockPciRootBridgeIoMap {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoMap);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeMap,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * This,
     IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION Operation,
     IN VOID *HostAddress,
     IN OUT UINTN *NumberOfBytes,
     OUT EFI_PHYSICAL_ADDRESS *DeviceAddress,
     OUT VOID **Mapping)
    );
};

struct MockPciRootBridgeIoUnMap {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoUnMap);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeUnMap,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
     IN  VOID                                     *Mapping)
    );
};

struct MockPciRootBridgeIoAllocateBuffer {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoAllocateBuffer);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeAllocateBuffer,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
     IN     EFI_ALLOCATE_TYPE                        Type,
     IN     EFI_MEMORY_TYPE                          MemoryType,
     IN     UINTN                                    Pages,
     IN OUT VOID                                     **HostAddress,
     IN     UINT64                                   Attributes)
    );
};

struct MockPciRootBridgeIoFreeBuffer {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoFreeBuffer);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeFreeBuffer,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
     IN  UINTN                                    Pages,
     IN  VOID                                     *HostAddress)
    );
};

struct MockPciRootBridgeIoFlush {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoFlush);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeFlush,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This)
    );
};

struct MockPciRootBridgeIoGetAttributes {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoGetAttributes);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeGetAttributes,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
     OUT UINT64                                   *Supports,
     OUT UINT64                                   *Attributes)
    );
};

struct MockPciRootBridgeIoSetAttributes {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoSetAttributes);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeSetAttributes,
    (IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
     IN     UINT64                                   Attributes,
     IN OUT UINT64                                   *ResourceBase,
     IN OUT UINT64                                   *ResourceLength)
    );
};

struct MockPciRootBridgeIoConfiguration {
  MOCK_INTERFACE_DECLARATION (MockPciRootBridgeIoConfiguration);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockPciRootBridgeConfiguration,
    (IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
     OUT VOID                                     **Resources)
    );
};

MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoPollIoMem);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoPollIoMem, MockPciRootBridgePollMem, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoPollIoMem, MockPciRootBridgePollIo, 7, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoConfigAccess);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgeMemRead, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgeMemWrite, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgeIoRead, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgeIoWrite, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgePciRead, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfigAccess, MockPciRootBridgePciWrite, 5, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoCopyMem);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoCopyMem, MockPciRootBridgeCopyMem, 5, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoMap);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoMap, MockPciRootBridgeMap, 6, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoUnMap);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoUnMap, MockPciRootBridgeUnMap, 2, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoAllocateBuffer);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoAllocateBuffer, MockPciRootBridgeAllocateBuffer, 6, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoFreeBuffer);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoFreeBuffer, MockPciRootBridgeFreeBuffer, 3, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoFlush);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoFlush, MockPciRootBridgeFlush, 1, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoGetAttributes);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoGetAttributes, MockPciRootBridgeGetAttributes, 3, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoSetAttributes);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoSetAttributes, MockPciRootBridgeSetAttributes, 4, EFIAPI);
MOCK_INTERFACE_DEFINITION (MockPciRootBridgeIoConfiguration);
MOCK_FUNCTION_DEFINITION (MockPciRootBridgeIoConfiguration, MockPciRootBridgeConfiguration, 2, EFIAPI);

#define MOCK_PCI_ROOT_BRIDGE_IO_PROTOCOL_INSTANCE(NAME)   \
EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL NAME##_INSTANCE = {       \
  NULL,                                                   \
  MockPciRootBridgePollMem,                               \
  MockPciRootBridgePollIo,                                \
  { MockPciRootBridgeMemRead, MockPciRootBridgeMemWrite },\
  { MockPciRootBridgeIoRead,  MockPciRootBridgeIoWrite  },\
  { MockPciRootBridgePciRead, MockPciRootBridgePciWrite },\
  MockPciRootBridgeCopyMem,                               \
  MockPciRootBridgeMap,                                   \
  MockPciRootBridgeUnMap,                                 \
  MockPciRootBridgeAllocateBuffer,                        \
  MockPciRootBridgeFreeBuffer,                            \
  MockPciRootBridgeFlush,                                 \
  MockPciRootBridgeGetAttributes,                         \
  MockPciRootBridgeSetAttributes,                         \
  MockPciRootBridgeConfiguration,                         \
};                                                        \
EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *NAME = &NAME##_INSTANCE;
