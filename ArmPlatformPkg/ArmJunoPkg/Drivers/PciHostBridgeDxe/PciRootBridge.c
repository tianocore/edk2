/** @file
*  Implementation of the PCI Root Bridge Protocol for XPress-RICH3 PCIe Root Complex
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

#include "PciHostBridge.h"

#include <Library/DevicePathLib.h>
#include <Library/DmaLib.h>

#define CPUIO_FROM_ROOT_BRIDGE_INSTANCE(Instance) (Instance->HostBridge->CpuIo)
#define METRONOME_FROM_ROOT_BRIDGE_INSTANCE(Instance) (Instance->HostBridge->Metronome)

/**
 * PCI Root Bridge Instance Templates
 */
STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  gDevicePathTemplate = {
    {
      { ACPI_DEVICE_PATH,
        ACPI_DP,
        { (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) }
      },
      EISA_PNP_ID (0x0A03),
      0
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { END_DEVICE_PATH_LENGTH, 0 }
    }
};

STATIC CONST EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL gIoTemplate = {
    0,
    PciRbPollMem,
    PciRbPollIo,
    {
      PciRbMemRead,
      PciRbMemWrite
    },
    {
      PciRbIoRead,
      PciRbIoWrite
    },
    {
      PciRbPciRead,
      PciRbPciWrite
    },
    PciRbCopyMem,
    PciRbMap,
    PciRbUnMap,
    PciRbAllocateBuffer,
    PciRbFreeBuffer,
    PciRbFlush,
    PciRbGetAttributes,
    PciRbSetAttributes,
    PciRbConfiguration,
    0
  };

typedef struct {
    EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR     SpaceDesp[ResTypeMax+1];
    EFI_ACPI_END_TAG_DESCRIPTOR           EndDesp;
} RESOURCE_CONFIGURATION;


RESOURCE_CONFIGURATION Configuration = {
   {{ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_IO , 0, 0, 0, 0, 0, 0, 0},
    {ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_MEM, 0, 0, 32, 0, 0, 0, 0},
    {ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_MEM, 0, 6, 32, 0, 0, 0, 0},
    {ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_MEM, 0, 0, 64, 0, 0, 0, 0},
    {ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_MEM, 0, 6, 64, 0, 0, 0, 0},
    {ACPI_ADDRESS_SPACE_DESCRIPTOR, 0x2B, ACPI_ADDRESS_SPACE_TYPE_BUS, 0, 0, 0, 0, 255, 0, 255}},
    {ACPI_END_TAG_DESCRIPTOR, 0}
};


EFI_STATUS
PciRbPollMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN  UINT64                                   Address,
  IN  UINT64                                   Mask,
  IN  UINT64                                   Value,
  IN  UINT64                                   Delay,
  OUT UINT64                                   *Result
  )
{
  EFI_STATUS                      Status;
  UINT64                          NumberOfTicks;
  UINT32                          Remainder;
  PCI_ROOT_BRIDGE_INSTANCE        *RootBridgeInstance;
  EFI_METRONOME_ARCH_PROTOCOL     *Metronome;

  PCI_TRACE ("PciRbPollMem()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  Metronome = METRONOME_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  // No matter what, always do a single poll.
  Status = This->Mem.Read (This, Width, Address, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  }

  NumberOfTicks = DivU64x32Remainder (Delay, (UINT32) Metronome->TickPeriod, &Remainder);
  if (Remainder != 0) {
    NumberOfTicks += 1;
  }
  NumberOfTicks += 1;

  while (NumberOfTicks) {
    Metronome->WaitForTick (Metronome, 1);

    Status = This->Mem.Read (This, Width, Address, 1, Result);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((*Result & Mask) == Value) {
      return EFI_SUCCESS;
    }

    NumberOfTicks -= 1;
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
PciRbPollIo (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN  UINT64                                   Address,
  IN  UINT64                                   Mask,
  IN  UINT64                                   Value,
  IN  UINT64                                   Delay,
  OUT UINT64                                   *Result
  )
{
  EFI_STATUS                      Status;
  UINT64                          NumberOfTicks;
  UINT32                          Remainder;
  PCI_ROOT_BRIDGE_INSTANCE        *RootBridgeInstance;
  EFI_METRONOME_ARCH_PROTOCOL     *Metronome;

  PCI_TRACE ("PciRbPollIo()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  Metronome = METRONOME_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  // No matter what, always do a single poll.
  Status = This->Io.Read (This, Width, Address, 1, Result);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay == 0) {
    return EFI_SUCCESS;
  }

  NumberOfTicks = DivU64x32Remainder (Delay, (UINT32) Metronome->TickPeriod, &Remainder);
  if (Remainder != 0) {
    NumberOfTicks += 1;
  }
  NumberOfTicks += 1;

  while (NumberOfTicks) {
    Metronome->WaitForTick (Metronome, 1);

    Status = This->Io.Read (This, Width, Address, 1, Result);
    if (EFI_ERROR (Status)) {
        return Status;
    }

    if ((*Result & Mask) == Value) {
        return EFI_SUCCESS;
    }

    NumberOfTicks -= 1;
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
PciRbMemRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;
  EFI_CPU_IO2_PROTOCOL        *CpuIo;

  PCI_TRACE ("PciRbMemRead()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  CpuIo = CPUIO_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (((Address < PCI_MEM32_BASE) || (Address > (PCI_MEM32_BASE + PCI_MEM32_SIZE))) &&
      ((Address < PCI_MEM64_BASE) || (Address > (PCI_MEM64_BASE + PCI_MEM64_SIZE)))) {
    return EFI_INVALID_PARAMETER;
  }

  return CpuIo->Mem.Read (CpuIo, (EFI_CPU_IO_PROTOCOL_WIDTH)Width, Address, Count, Buffer);
}

EFI_STATUS
PciRbMemWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;
  EFI_CPU_IO2_PROTOCOL        *CpuIo;

  PCI_TRACE ("PciRbMemWrite()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  CpuIo = CPUIO_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (((Address < PCI_MEM32_BASE) || (Address > (PCI_MEM32_BASE + PCI_MEM32_SIZE))) &&
      ((Address < PCI_MEM64_BASE) || (Address > (PCI_MEM64_BASE + PCI_MEM64_SIZE)))) {
    return EFI_INVALID_PARAMETER;
  }

  return CpuIo->Mem.Write (CpuIo, (EFI_CPU_IO_PROTOCOL_WIDTH)Width, Address, Count, Buffer);
}

EFI_STATUS
PciRbIoRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  PCI_TRACE ("PciRbIoRead()");

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  // IO currently unsupported
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
PciRbIoWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  PCI_TRACE ("PciRbIoWrite()");

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  // IO currently unsupported
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
PciRbPciRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   EfiAddress,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  UINT32                      Offset;
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;
  EFI_CPU_IO2_PROTOCOL        *CpuIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *EfiPciAddress;
  UINT64                      Address;

  EfiPciAddress  = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *)&EfiAddress;
  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  CpuIo = CPUIO_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiPciAddress->ExtendedRegister) {
    Offset = EfiPciAddress->ExtendedRegister;
  } else {
    Offset = EfiPciAddress->Register;
  }

  // The UEFI PCI enumerator scans for devices at all possible addresses,
  // and ignores some PCI rules - this results in some hardware being
  // detected multiple times. We work around this by faking absent
  // devices
  if ((EfiPciAddress->Bus == 0) && ((EfiPciAddress->Device != 0) || (EfiPciAddress->Function != 0))) {
    *((UINT32 *)Buffer) = 0xffffffff;
    return EFI_SUCCESS;
  }
  if ((EfiPciAddress->Bus == 1) && ((EfiPciAddress->Device != 0) || (EfiPciAddress->Function != 0))) {
    *((UINT32 *)Buffer) = 0xffffffff;
    return EFI_SUCCESS;
  }

  // Work around incorrect class ID in the root bridge
  if ((EfiPciAddress->Bus == 0) && (EfiPciAddress->Device == 0) && (EfiPciAddress->Function == 0) && (Offset == 8)) {
    *((UINT32 *)Buffer) = 0x06040001;
    return EFI_SUCCESS;
   }

  Address = PCI_ECAM_BASE + ((EfiPciAddress->Bus << 20) |
                         (EfiPciAddress->Device << 15) |
                         (EfiPciAddress->Function << 12) | Offset);

  if ((Address < PCI_ECAM_BASE) || (Address > PCI_ECAM_BASE + PCI_ECAM_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  return CpuIo->Mem.Read (CpuIo, (EFI_CPU_IO_PROTOCOL_WIDTH)Width, Address, Count, Buffer);
}

EFI_STATUS
PciRbPciWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   EfiAddress,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
{
  UINT32                      Offset;
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;
  EFI_CPU_IO2_PROTOCOL        *CpuIo;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *EfiPciAddress;
  UINT64                      Address;

  EfiPciAddress  = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *)&EfiAddress;
  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);
  CpuIo = CPUIO_FROM_ROOT_BRIDGE_INSTANCE (RootBridgeInstance);

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiPciAddress->ExtendedRegister)
    Offset = EfiPciAddress->ExtendedRegister;
  else
    Offset = EfiPciAddress->Register;

  Address = PCI_ECAM_BASE + ((EfiPciAddress->Bus << 20) |
                         (EfiPciAddress->Device << 15) |
                         (EfiPciAddress->Function << 12) | Offset);

  if (Address < PCI_ECAM_BASE || Address > PCI_ECAM_BASE + PCI_ECAM_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  return CpuIo->Mem.Write (CpuIo, (EFI_CPU_IO_PROTOCOL_WIDTH)Width, Address, Count, Buffer);
}

EFI_STATUS
PciRbCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   DestAddress,
  IN     UINT64                                   SrcAddress,
  IN     UINTN                                    Count
  )
{
  EFI_STATUS  Status;
  BOOLEAN     Direction;
  UINTN       Stride;
  UINTN       Index;
  UINT64      Result;

  PCI_TRACE ("PciRbCopyMem()");

  if (Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestAddress == SrcAddress) {
    return EFI_SUCCESS;
  }

  Stride = (UINTN)(1 << Width);

  Direction = TRUE;
  if ((DestAddress > SrcAddress) && (DestAddress < (SrcAddress + Count * Stride))) {
    Direction   = FALSE;
    SrcAddress  = SrcAddress  + (Count-1) * Stride;
    DestAddress = DestAddress + (Count-1) * Stride;
  }

  for (Index = 0; Index < Count; Index++) {
    Status = PciRbMemRead (
               This,
               Width,
               SrcAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = PciRbMemWrite (
               This,
               Width,
               DestAddress,
               1,
               &Result
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (Direction) {
      SrcAddress  += Stride;
      DestAddress += Stride;
    } else {
      SrcAddress  -= Stride;
      DestAddress -= Stride;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
PciRbMap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL                *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )
{
  DMA_MAP_OPERATION   DmaOperation;

  PCI_TRACE ("PciRbMap()");

  if (Operation == EfiPciOperationBusMasterRead) {
    DmaOperation = MapOperationBusMasterRead;
  } else if (Operation == EfiPciOperationBusMasterWrite) {
    DmaOperation = MapOperationBusMasterWrite;
  } else if (Operation == EfiPciOperationBusMasterCommonBuffer) {
    DmaOperation = MapOperationBusMasterCommonBuffer;
  } else {
    return EFI_INVALID_PARAMETER;
  }
  return DmaMap (DmaOperation, HostAddress, NumberOfBytes, DeviceAddress, Mapping);
}

EFI_STATUS
PciRbUnMap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  VOID                                     *Mapping
  )
{
  PCI_TRACE ("PciRbUnMap()");
  return DmaUnmap (Mapping);
}

EFI_STATUS
PciRbAllocateBuffer (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     EFI_ALLOCATE_TYPE                        Type,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  )
{
  PCI_TRACE ("PciRbAllocateBuffer()");

  if (Attributes & EFI_PCI_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) {
    return EFI_UNSUPPORTED;
  }

  return DmaAllocateBuffer (MemoryType, Pages, HostAddress);
}

EFI_STATUS
PciRbFreeBuffer (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  )
{
  PCI_TRACE ("PciRbFreeBuffer()");
  return DmaFreeBuffer (Pages, HostAddress);
}

EFI_STATUS
PciRbFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This
  )
{
  PCI_TRACE ("PciRbFlush()");

  //TODO: Not supported yet

  return EFI_SUCCESS;
}

EFI_STATUS
PciRbSetAttributes (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL              *This,
  IN     UINT64                                   Attributes,
  IN OUT UINT64                                   *ResourceBase,
  IN OUT UINT64                                   *ResourceLength
  )
{
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;

  PCI_TRACE ("PciRbSetAttributes()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);

  if (Attributes) {
    if ((Attributes & (~(RootBridgeInstance->Supports))) != 0) {
      return EFI_UNSUPPORTED;
    }
  }

  //TODO: Cannot allowed to change attributes
  if (Attributes & ~RootBridgeInstance->Attributes) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciRbGetAttributes (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  OUT UINT64                                   *Supported,
  OUT UINT64                                   *Attributes
  )
{
  PCI_ROOT_BRIDGE_INSTANCE    *RootBridgeInstance;

  PCI_TRACE ("PciRbGetAttributes()");

  RootBridgeInstance = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);

  if (Attributes == NULL && Supported == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Set the return value for Supported and Attributes
  if (Supported) {
    *Supported  = RootBridgeInstance->Supports;
  }

  if (Attributes) {
    *Attributes = RootBridgeInstance->Attributes;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciRbConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  OUT VOID                                     **Resources
  )
{
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridge;
  UINTN                                 Index;

  PCI_TRACE ("PciRbConfiguration()");

  RootBridge = INSTANCE_FROM_ROOT_BRIDGE_IO_THIS (This);

  for (Index = 0; Index < ResTypeMax; Index++) {
    //if (ResAlloc[Index].Length != 0) => Resource allocated
    if (RootBridge->ResAlloc[Index].Length != 0) {
      Configuration.SpaceDesp[Index].AddrRangeMin = RootBridge->ResAlloc[Index].Base;
      Configuration.SpaceDesp[Index].AddrRangeMax = RootBridge->ResAlloc[Index].Base + RootBridge->ResAlloc[Index].Length - 1;
      Configuration.SpaceDesp[Index].AddrLen      = RootBridge->ResAlloc[Index].Length;
    }
  }

  // Set up Configuration for the bus
  Configuration.SpaceDesp[Index].AddrRangeMin = RootBridge->BusStart;
  Configuration.SpaceDesp[Index].AddrLen      = RootBridge->BusLength;

  *Resources = &Configuration;
  return EFI_SUCCESS;
}

EFI_STATUS
PciRbConstructor (
  IN  PCI_HOST_BRIDGE_INSTANCE *HostBridge,
  IN  UINT32 PciAcpiUid,
  IN  UINT64 MemAllocAttributes
  )
{
  PCI_ROOT_BRIDGE_INSTANCE* RootBridge;
  EFI_STATUS Status;

  PCI_TRACE ("PciRbConstructor()");

  // Allocate Memory for the Instance from a Template
  RootBridge = AllocateZeroPool (sizeof (PCI_ROOT_BRIDGE_INSTANCE));
  if (RootBridge == NULL) {
    PCI_TRACE ("PciRbConstructor(): ERROR: Out of Resources");
    return EFI_OUT_OF_RESOURCES;
  }
  RootBridge->Signature = PCI_ROOT_BRIDGE_SIGNATURE;
  CopyMem (&(RootBridge->DevicePath), &gDevicePathTemplate, sizeof (EFI_PCI_ROOT_BRIDGE_DEVICE_PATH));
  CopyMem (&(RootBridge->Io), &gIoTemplate, sizeof (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL));

  // Set Parent Handle
  RootBridge->Io.ParentHandle = HostBridge->Handle;

  // Attach the Root Bridge to the PCI Host Bridge Instance
  RootBridge->HostBridge = HostBridge;

  // Set Device Path for this Root Bridge
  RootBridge->DevicePath.Acpi.UID = PciAcpiUid;

  RootBridge->BusStart  = FixedPcdGet32 (PcdPciBusMin);
  RootBridge->BusLength = FixedPcdGet32 (PcdPciBusMax) - FixedPcdGet32 (PcdPciBusMin) + 1;

  // PCI Attributes
  RootBridge->Supports = 0;
  RootBridge->Attributes = 0;

  // Install Protocol Instances. It will also generate a device handle for the PCI Root Bridge
  Status = gBS->InstallMultipleProtocolInterfaces (
                      &RootBridge->Handle,
                      &gEfiDevicePathProtocolGuid, &RootBridge->DevicePath,
                      &gEfiPciRootBridgeIoProtocolGuid, &RootBridge->Io,
                      NULL
                      );
  ASSERT (RootBridge->Signature == PCI_ROOT_BRIDGE_SIGNATURE);
  if (EFI_ERROR (Status)) {
    PCI_TRACE ("PciRbConstructor(): ERROR: Fail to install Protocol Interfaces");
    FreePool (RootBridge);
    return EFI_DEVICE_ERROR;
  }

  HostBridge->RootBridge = RootBridge;
  return EFI_SUCCESS;
}

EFI_STATUS
PciRbDestructor (
  IN  PCI_ROOT_BRIDGE_INSTANCE* RootBridge
  )
{
  EFI_STATUS Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                        RootBridge->Handle,
                        &gEfiDevicePathProtocolGuid, &RootBridge->DevicePath,
                        &gEfiPciRootBridgeIoProtocolGuid, &RootBridge->Io,
                        NULL
                        );

  FreePool (RootBridge);

  return Status;
}
