/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCI_ROOT_BRIDGE_H_
#define _PCI_ROOT_BRIDGE_H_

#include <PiDxe.h>

#include <TPS65950.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OmapDmaLib.h>
#include <Library/DmaLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Acpi.h>

#include <Omap3530/Omap3530.h>



#define EFI_RESOURCE_NONEXISTENT  0xFFFFFFFFFFFFFFFFULL
#define EFI_RESOURCE_LESS         0xFFFFFFFFFFFFFFFEULL
#define EFI_RESOURCE_SATISFIED    0x0000000000000000ULL


typedef struct {
  ACPI_HID_DEVICE_PATH      AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;


#define ACPI_CONFIG_IO    0
#define ACPI_CONFIG_MMIO  1
#define ACPI_CONFIG_BUS   2

typedef struct {
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR Desc[3];
  EFI_ACPI_END_TAG_DESCRIPTOR       EndDesc;
} ACPI_CONFIG_INFO;


#define PCI_ROOT_BRIDGE_SIGNATURE SIGNATURE_32 ('P', 'c', 'i', 'F')

typedef struct {
  UINT32                                            Signature;
  EFI_HANDLE                                        Handle;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                   Io;
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH                   DevicePath;

  UINT8   StartBus;
  UINT8   EndBus;
  UINT16  Type;
  UINT32  MemoryStart;
  UINT32  MemorySize;
  UINTN   IoOffset;
  UINT32  IoStart;
  UINT32  IoSize;
  UINT64  PciAttributes;

  ACPI_CONFIG_INFO  *Config;

} PCI_ROOT_BRIDGE;


#define INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(a) CR (a, PCI_ROOT_BRIDGE, Io, PCI_ROOT_BRIDGE_SIGNATURE)


typedef union {
  UINT8   volatile  *buf;
  UINT8   volatile  *ui8;
  UINT16  volatile  *ui16;
  UINT32  volatile  *ui32;
  UINT64  volatile  *ui64;
  UINTN   volatile  ui;
} PTR;



EFI_STATUS
EFIAPI
PciRootBridgeIoPollMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoPollIo (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 UserAddress,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *UserBuffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 DestAddress,
  IN UINT64                                 SrcAddress,
  IN UINTN                                  Count
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoUnmap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength
  );

EFI_STATUS
EFIAPI
PciRootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  );

//
// Private Function Prototypes
//
EFI_STATUS
EFIAPI
PciRootBridgeIoMemRW (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                                  Count,
  IN  BOOLEAN                                InStrideFlag,
  IN  PTR                                    In,
  IN  BOOLEAN                                OutStrideFlag,
  OUT PTR                                    Out
  );

BOOLEAN
PciIoMemAddressValid (
  IN EFI_PCI_IO_PROTOCOL  *This,
  IN UINT64               Address
  );

EFI_STATUS
EmulatePciIoForEhci (
  INTN    MvPciIfMaxIf
  );

#endif

