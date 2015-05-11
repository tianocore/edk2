/** @file
*  Header containing the structure specific to the Xpress-RICH3 PCIe Root Complex
*
*  Copyright (c) 2011-2015, ARM Ltd. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __PCIHOSTBRIDGE_H
#define __PCIHOSTBRIDGE_H

#include <PiDxe.h>

#include "XPressRich3.h"

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/Metronome.h>

#define PCI_TRACE(txt)  DEBUG((EFI_D_VERBOSE, "ARM_PCI: " txt "\n"))

#define PCIE_ROOTPORT_WRITE32(Add, Val) { UINT32 Value = (UINT32)(Val); CpuIo->Mem.Write (CpuIo,EfiCpuIoWidthUint32,(UINT64)(PcdGet64 (PcdPcieRootPortBaseAddress)+(Add)),1,&Value); }
#define PCIE_ROOTPORT_READ32(Add, Val) { CpuIo->Mem.Read (CpuIo,EfiCpuIoWidthUint32,(UINT64)(PcdGet64 (PcdPcieRootPortBaseAddress)+(Add)),1,&Val); }

#define PCIE_CONTROL_WRITE32(Add, Val) { UINT32 Value = (UINT32)(Val); CpuIo->Mem.Write (CpuIo,EfiCpuIoWidthUint32,(UINT64)(PcdGet64 (PcdPcieControlBaseAddress)+(Add)),1,&Value); }
#define PCIE_CONTROL_READ32(Add, Val) { CpuIo->Mem.Read (CpuIo,EfiCpuIoWidthUint32,(UINT64)(PcdGet64 (PcdPcieControlBaseAddress)+(Add)),1,&Val); }

/**
 * PCI Root Bridge Device Path (ACPI Device Node + End Node)
 */
typedef struct {
  ACPI_HID_DEVICE_PATH          Acpi;
  EFI_DEVICE_PATH_PROTOCOL      End;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;

typedef enum {
  ResTypeIo = 0,
  ResTypeMem32,
  ResTypePMem32,
  ResTypeMem64,
  ResTypePMem64,
  ResTypeMax
} PCI_RESOURCE_TYPE;

#define ACPI_SPECFLAG_PREFETCHABLE      0x06
#define EFI_RESOURCE_NONEXISTENT        0xFFFFFFFFFFFFFFFFULL
#define EFI_RESOURCE_LESS               0xFFFFFFFFFFFFFFFEULL

typedef struct {
  UINT64  Base;
  UINT64  Length;
  UINT64  Alignment;
} PCI_RESOURCE_ALLOC;

typedef struct _PCI_HOST_BRIDGE_INSTANCE PCI_HOST_BRIDGE_INSTANCE;

/**
 * PCI Root Bridge Instance structure
 **/
typedef struct {
  UINTN                            Signature;
  EFI_HANDLE                       Handle;
  PCI_HOST_BRIDGE_INSTANCE        *HostBridge;
  //
  // Set Type of memory allocation (could be EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM
  // and EFI_PCI_HOST_BRIDGE_MEM64_DECODE).
  //
  UINT64                           MemAllocAttributes;
  PCI_RESOURCE_ALLOC               ResAlloc[ResTypeMax];
  UINTN                            BusStart;
  UINTN                            BusLength;
  UINT64                           Supports;
  UINT64                           Attributes;
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  DevicePath;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  Io;
} PCI_ROOT_BRIDGE_INSTANCE;

/**
 * PCI Host Bridge Instance structure
 **/
struct _PCI_HOST_BRIDGE_INSTANCE {
  UINTN                                             Signature;
  EFI_HANDLE                                        Handle;
  EFI_HANDLE                                        ImageHandle;
  PCI_ROOT_BRIDGE_INSTANCE                         *RootBridge;
  //
  // The enumeration cannot be restarted after the process goes into the non initial
  // enumeration phase.
  //
  BOOLEAN                                           CanRestarted;
  EFI_CPU_IO2_PROTOCOL                             *CpuIo;
  EFI_METRONOME_ARCH_PROTOCOL                      *Metronome;
  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  ResAlloc;
};

#define PCI_HOST_BRIDGE_SIGNATURE                   SIGNATURE_32 ('e', 'h', 's', 't')
#define PCI_ROOT_BRIDGE_SIGNATURE                   SIGNATURE_32 ('e', '2', 'p', 'b')
#define INSTANCE_FROM_RESOURCE_ALLOCATION_THIS(a)   CR (a, PCI_HOST_BRIDGE_INSTANCE, ResAlloc, PCI_HOST_BRIDGE_SIGNATURE)
#define INSTANCE_FROM_ROOT_BRIDGE_IO_THIS(a)        CR (a, PCI_ROOT_BRIDGE_INSTANCE, Io, PCI_ROOT_BRIDGE_SIGNATURE)

/**
 * PCI Host Bridge Resource Allocator Functions
 **/
EFI_STATUS PciHbRaNotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE     Phase
  );

EFI_STATUS PciHbRaGetNextRootBridge (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN OUT EFI_HANDLE                                    *RootBridgeHandle
  );

EFI_STATUS PciHbRaGetAllocAttributes (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                        RootBridgeHandle,
  OUT UINT64                                           *Attributes
  );

EFI_STATUS PciHbRaStartBusEnumeration (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                        RootBridgeHandle,
  OUT VOID                                            **Configuration
  );

EFI_STATUS PciHbRaSetBusNumbers (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN EFI_HANDLE                                         RootBridgeHandle,
  IN VOID                                              *Configuration
  );

EFI_STATUS PciHbRaSubmitResources (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN EFI_HANDLE                                         RootBridgeHandle,
  IN VOID                                              *Configuration
  );

EFI_STATUS PciHbRaGetProposedResources (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                        RootBridgeHandle,
  OUT VOID                                            **Configuration
  );

EFI_STATUS PciHbRaPreprocessController (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                        RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS       PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE      Phase
  );


/**
 * PCI Root Bridge
 **/
EFI_STATUS PciRbPollMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN  UINT64                                   Address,
  IN  UINT64                                   Mask,
  IN  UINT64                                   Value,
  IN  UINT64                                   Delay,
  OUT UINT64                                   *Result
  );

EFI_STATUS PciRbPollIo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN  UINT64                                   Address,
  IN  UINT64                                   Mask,
  IN  UINT64                                   Value,
  IN  UINT64                                   Delay,
  OUT UINT64                                   *Result
  );

EFI_STATUS PciRbMemRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbMemWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbIoRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbIoWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbPciRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbPciWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  );

EFI_STATUS PciRbCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   DestAddress,
  IN     UINT64                                   SrcAddress,
  IN     UINTN                                    Count
  );

EFI_STATUS PciRbMap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

EFI_STATUS PciRbUnMap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  VOID                                     *Mapping
  );

EFI_STATUS PciRbAllocateBuffer (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_ALLOCATE_TYPE                        Type,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  );

EFI_STATUS PciRbFreeBuffer (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  );

EFI_STATUS PciRbFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  );

EFI_STATUS PciRbSetAttributes (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     UINT64                                   Attributes,
  IN OUT UINT64                                   *ResourceBase,
  IN OUT UINT64                                   *ResourceLength
  );

EFI_STATUS PciRbGetAttributes (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  OUT UINT64                                   *Supports,
  OUT UINT64                                   *Attributes
  );

EFI_STATUS PciRbConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  OUT VOID                                     **Resources
  );

/**
 * PCI Root Bridge Functions
 **/
EFI_STATUS
PciRbConstructor (
  IN  PCI_HOST_BRIDGE_INSTANCE *HostBridge,
  IN  UINT32 PciAcpiUid,
  IN  UINT64 MemAllocAttributes
  );

EFI_STATUS
PciRbDestructor (
  IN  PCI_ROOT_BRIDGE_INSTANCE* RootBridge
  );

EFI_STATUS
HWPciRbInit (
  IN  EFI_CPU_IO2_PROTOCOL *CpuIo
  );

#endif
