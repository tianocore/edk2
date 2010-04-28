/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PciIo.h
    
Abstract:

    EFI PCI I/O Protocol

Revision History

--*/

#ifndef _EFI_PCI_IO_H
#define _EFI_PCI_IO_H

//
// Global ID for the PCI I/O Protocol
//
#define EFI_PCI_IO_PROTOCOL_GUID \
  { \
    0x4cf5b200, 0x68b8, 0x4ca5, {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x2, 0x9a} \
  }

EFI_FORWARD_DECLARATION (EFI_PCI_IO_PROTOCOL);

//
// Prototypes for the PCI I/O Protocol
//
typedef enum {
  EfiPciIoWidthUint8      = 0,
  EfiPciIoWidthUint16,
  EfiPciIoWidthUint32,
  EfiPciIoWidthUint64,
  EfiPciIoWidthFifoUint8,
  EfiPciIoWidthFifoUint16,
  EfiPciIoWidthFifoUint32,
  EfiPciIoWidthFifoUint64,
  EfiPciIoWidthFillUint8,
  EfiPciIoWidthFillUint16,
  EfiPciIoWidthFillUint32,
  EfiPciIoWidthFillUint64,
  EfiPciIoWidthMaximum
} EFI_PCI_IO_PROTOCOL_WIDTH;

//
// Complete PCI address generater
//
#define EFI_PCI_IO_PASS_THROUGH_BAR               0xff    // Special BAR that passes a memory or I/O cycle through unchanged
#define EFI_PCI_IO_ATTRIBUTE_MASK                 0x077f  // All the following I/O and Memory cycles
#define EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO   0x0001  // I/O cycles 0x0000-0x00FF (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_ISA_IO               0x0002  // I/O cycles 0x0000-0x03FF (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO       0x0004  // I/O cycles 0x3C6, 0x3C8, 0x3C9 (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY           0x0008  // MEM cycles 0xA0000-0xBFFFF (24 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_VGA_IO               0x0010  // I/O cycles 0x3B0-0x3BB and 0x3C0-0x3DF (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO       0x0020  // I/O cycles 0x1F0-0x1F7, 0x3F6, 0x3F7 (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO     0x0040  // I/O cycles 0x170-0x177, 0x376, 0x377 (10 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE 0x0080  // Map a memory range so write are combined
#define EFI_PCI_IO_ATTRIBUTE_IO                   0x0100  // Enable the I/O decode bit in the PCI Config Header
#define EFI_PCI_IO_ATTRIBUTE_MEMORY               0x0200  // Enable the Memory decode bit in the PCI Config Header
#define EFI_PCI_IO_ATTRIBUTE_BUS_MASTER           0x0400  // Enable the DMA bit in the PCI Config Header
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED        0x0800  // Map a memory range so all r/w accesses are cached
#define EFI_PCI_IO_ATTRIBUTE_MEMORY_DISABLE       0x1000  // Disable a memory range
#define EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE      0x2000  // Clear for an add-in PCI Device
#define EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM         0x4000  // Clear for a physical PCI Option ROM accessed through ROM BAR
#define EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE   0x8000  // Clear for PCI controllers that can not genrate a DAC
//
// The following definition is added in EFI1.1 spec update and UEFI2.0 spec.
//
#define EFI_PCI_IO_ATTRIBUTE_ISA_IO_16            0x10000 // I/O cycles 0x0100-0x03FF (16 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16    0x20000 // I/O cycles 0x3C6, 0x3C8, 0x3C9 (16 bit decode)
#define EFI_PCI_IO_ATTRIBUTE_VGA_IO_16            0x40000 // I/O cycles 0x3B0-0x3BB and 0x3C0-0x3DF (16 bit decode)

#define EFI_PCI_DEVICE_ENABLE                     (EFI_PCI_IO_ATTRIBUTE_IO | EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER)
#define EFI_VGA_DEVICE_ENABLE                     (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY | EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_IO)

//
// *******************************************************
// EFI_PCI_IO_PROTOCOL_OPERATION
// *******************************************************
//
typedef enum {
  EfiPciIoOperationBusMasterRead,
  EfiPciIoOperationBusMasterWrite,
  EfiPciIoOperationBusMasterCommonBuffer,
  EfiPciIoOperationMaximum
} EFI_PCI_IO_PROTOCOL_OPERATION;

//
// *******************************************************
// EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION
// *******************************************************
//
typedef enum {
  EfiPciIoAttributeOperationGet,
  EfiPciIoAttributeOperationSet,
  EfiPciIoAttributeOperationEnable,
  EfiPciIoAttributeOperationDisable,
  EfiPciIoAttributeOperationSupported,
  EfiPciIoAttributeOperationMaximum
} EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_POLL_IO_MEM) (
  IN EFI_PCI_IO_PROTOCOL           * This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_IO_MEM) (
  IN EFI_PCI_IO_PROTOCOL              * This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_PCI_IO_PROTOCOL_IO_MEM  Read;
  EFI_PCI_IO_PROTOCOL_IO_MEM  Write;
} EFI_PCI_IO_PROTOCOL_ACCESS;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_CONFIG) (
  IN EFI_PCI_IO_PROTOCOL              * This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  );

typedef struct {
  EFI_PCI_IO_PROTOCOL_CONFIG  Read;
  EFI_PCI_IO_PROTOCOL_CONFIG  Write;
} EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS;

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_COPY_MEM) (
  IN EFI_PCI_IO_PROTOCOL              * This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_MAP) (
  IN EFI_PCI_IO_PROTOCOL                * This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           * DeviceAddress,
  OUT    VOID                           **Mapping
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_UNMAP) (
  IN EFI_PCI_IO_PROTOCOL           * This,
  IN  VOID                         *Mapping
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER) (
  IN EFI_PCI_IO_PROTOCOL           * This,
  IN  EFI_ALLOCATE_TYPE            Type,
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress,
  IN  UINT64                       Attributes
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_FREE_BUFFER) (
  IN EFI_PCI_IO_PROTOCOL           * This,
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_FLUSH) (
  IN EFI_PCI_IO_PROTOCOL  * This
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_GET_LOCATION) (
  IN EFI_PCI_IO_PROTOCOL          * This,
  OUT UINTN                       *SegmentNumber,
  OUT UINTN                       *BusNumber,
  OUT UINTN                       *DeviceNumber,
  OUT UINTN                       *FunctionNumber
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES) (
  IN EFI_PCI_IO_PROTOCOL              * This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  );

//
// Interface structure for the PCI I/O Protocol
//
struct _EFI_PCI_IO_PROTOCOL {
  EFI_PCI_IO_PROTOCOL_POLL_IO_MEM         PollMem;
  EFI_PCI_IO_PROTOCOL_POLL_IO_MEM         PollIo;
  EFI_PCI_IO_PROTOCOL_ACCESS              Mem;
  EFI_PCI_IO_PROTOCOL_ACCESS              Io;
  EFI_PCI_IO_PROTOCOL_CONFIG_ACCESS       Pci;
  EFI_PCI_IO_PROTOCOL_COPY_MEM            CopyMem;
  EFI_PCI_IO_PROTOCOL_MAP                 Map;
  EFI_PCI_IO_PROTOCOL_UNMAP               Unmap;
  EFI_PCI_IO_PROTOCOL_ALLOCATE_BUFFER     AllocateBuffer;
  EFI_PCI_IO_PROTOCOL_FREE_BUFFER         FreeBuffer;
  EFI_PCI_IO_PROTOCOL_FLUSH               Flush;
  EFI_PCI_IO_PROTOCOL_GET_LOCATION        GetLocation;
  EFI_PCI_IO_PROTOCOL_ATTRIBUTES          Attributes;
  EFI_PCI_IO_PROTOCOL_GET_BAR_ATTRIBUTES  GetBarAttributes;
  EFI_PCI_IO_PROTOCOL_SET_BAR_ATTRIBUTES  SetBarAttributes;
  UINT64                                  RomSize;
  VOID                                    *RomImage;
};

extern EFI_GUID gEfiPciIoProtocolGuid;

#endif
