/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NonDiscoverablePciDeviceIo.h"

#include <IndustryStandard/Acpi.h>

#include <Protocol/PciRootBridgeIo.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS            AllocAddress;
  VOID                            *HostAddress;
  EFI_PCI_IO_PROTOCOL_OPERATION   Operation;
  UINTN                           NumberOfBytes;
} NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO;

//
// Get the resource associated with BAR number 'BarIndex'.
//
STATIC
EFI_STATUS
GetBarResource (
  IN  NON_DISCOVERABLE_PCI_DEVICE         *Dev,
  IN  UINT8                               BarIndex,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   **Descriptor
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;

  if (BarIndex < Dev->BarOffset) {
    return EFI_NOT_FOUND;
  }

  BarIndex -= Dev->BarOffset;

  for (Desc = Dev->Device->Resources;
       Desc->Desc != ACPI_END_TAG_DESCRIPTOR;
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3)) {

    if (BarIndex == 0) {
      *Descriptor = Desc;
      return EFI_SUCCESS;
    }

    BarIndex -= 1;
  }
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL         *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH   Width,
  IN  UINT8                       BarIndex,
  IN  UINT64                      Offset,
  IN  UINT64                      Mask,
  IN  UINT64                      Value,
  IN  UINT64                      Delay,
  OUT UINT64                      *Result
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL         *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH   Width,
  IN  UINT8                       BarIndex,
  IN  UINT64                      Offset,
  IN  UINT64                      Mask,
  IN  UINT64                      Value,
  IN  UINT64                      Delay,
  OUT UINT64                      *Result
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
PciIoMemRW (
  IN  EFI_PCI_IO_PROTOCOL_WIDTH   Width,
  IN  UINTN                       Count,
  IN  UINTN                       DstStride,
  IN  VOID                        *Dst,
  IN  UINTN                       SrcStride,
  OUT CONST VOID                  *Src
  )
{
  volatile UINT8             *Dst8;
  volatile UINT16            *Dst16;
  volatile UINT32            *Dst32;
  volatile CONST UINT8       *Src8;
  volatile CONST UINT16      *Src16;
  volatile CONST UINT32      *Src32;

  //
  // Loop for each iteration and move the data
  //
  switch (Width & 0x3) {
  case EfiPciWidthUint8:
    Dst8 = (UINT8 *)Dst;
    Src8 = (UINT8 *)Src;
    for (;Count > 0; Count--, Dst8 += DstStride, Src8 += SrcStride) {
      *Dst8 = *Src8;
    }
    break;
  case EfiPciWidthUint16:
    Dst16 = (UINT16 *)Dst;
    Src16 = (UINT16 *)Src;
    for (;Count > 0; Count--, Dst16 += DstStride, Src16 += SrcStride) {
      *Dst16 = *Src16;
    }
    break;
  case EfiPciWidthUint32:
    Dst32 = (UINT32 *)Dst;
    Src32 = (UINT32 *)Src;
    for (;Count > 0; Count--, Dst32 += DstStride, Src32 += SrcStride) {
      *Dst32 = *Src32;
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL          *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE         *Dev;
  UINTN                               AlignMask;
  VOID                                *Address;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;
  EFI_STATUS                          Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);

  //
  // Only allow accesses to the BARs we emulate
  //
  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  Address = (VOID *)(UINTN)(Desc->AddrRangeMin + Offset);
  AlignMask = (1 << (Width & 0x03)) - 1;
  if ((UINTN)Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Width) {
  case EfiPciWidthUint8:
  case EfiPciWidthUint16:
  case EfiPciWidthUint32:
  case EfiPciWidthUint64:
    return PciIoMemRW (Width, Count, 1, Buffer, 1, Address);

  case EfiPciWidthFifoUint8:
  case EfiPciWidthFifoUint16:
  case EfiPciWidthFifoUint32:
  case EfiPciWidthFifoUint64:
    return PciIoMemRW (Width, Count, 1, Buffer, 0, Address);

  case EfiPciWidthFillUint8:
  case EfiPciWidthFillUint16:
  case EfiPciWidthFillUint32:
  case EfiPciWidthFillUint64:
    return PciIoMemRW (Width, Count, 0, Buffer, 1, Address);

  default:
    break;
  }
  return EFI_INVALID_PARAMETER;
}

STATIC
EFI_STATUS
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL          *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE         *Dev;
  UINTN                               AlignMask;
  VOID                                *Address;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;
  EFI_STATUS                          Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);

  //
  // Only allow accesses to the BARs we emulate
  //
  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  Address = (VOID *)(UINTN)(Desc->AddrRangeMin + Offset);
  AlignMask = (1 << (Width & 0x03)) - 1;
  if ((UINTN)Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Width) {
  case EfiPciWidthUint8:
  case EfiPciWidthUint16:
  case EfiPciWidthUint32:
  case EfiPciWidthUint64:
    return PciIoMemRW (Width, Count, 1, Address, 1, Buffer);

  case EfiPciWidthFifoUint8:
  case EfiPciWidthFifoUint16:
  case EfiPciWidthFifoUint32:
  case EfiPciWidthFifoUint64:
    return PciIoMemRW (Width, Count, 0, Address, 1, Buffer);

  case EfiPciWidthFillUint8:
  case EfiPciWidthFillUint16:
  case EfiPciWidthFillUint32:
  case EfiPciWidthFillUint64:
    return PciIoMemRW (Width, Count, 1, Address, 0, Buffer);

  default:
    break;
  }
  return EFI_INVALID_PARAMETER;
}

STATIC
EFI_STATUS
PciIoIoRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL          *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
PciIoPciRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE   *Dev;
  VOID                          *Address;
  UINTN                         Length;

  if (Width < 0 || Width >= EfiPciIoWidthMaximum || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);
  Address = (UINT8 *)&Dev->ConfigSpace + Offset;
  Length = Count << ((UINTN)Width & 0x3);

  if (Offset + Length > sizeof (Dev->ConfigSpace)) {
    //
    // Read all zeroes for config space accesses beyond the first
    // 64 bytes
    //
    Length -= sizeof (Dev->ConfigSpace) - Offset;
    ZeroMem ((UINT8 *)Buffer + sizeof (Dev->ConfigSpace) - Offset, Length);

    Count -= Length >> ((UINTN)Width & 0x3);
  }
  return PciIoMemRW (Width, Count, 1, Buffer, 1, Address);
}

STATIC
EFI_STATUS
PciIoPciWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE   *Dev;
  VOID                          *Address;

  if (Width < 0 || Width >= EfiPciIoWidthMaximum || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);
  Address = (UINT8 *)&Dev->ConfigSpace + Offset;

  if (Offset + (Count << ((UINTN)Width & 0x3)) > sizeof (Dev->ConfigSpace)) {
    return EFI_UNSUPPORTED;
  }

  return PciIoMemRW (Width, Count, 1, Address, 1, Buffer);
}

STATIC
EFI_STATUS
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
CoherentPciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  NON_DISCOVERABLE_PCI_DEVICE           *Dev;
  EFI_STATUS                            Status;
  NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO  *MapInfo;

  //
  // If HostAddress exceeds 4 GB, and this device does not support 64-bit DMA
  // addressing, we need to allocate a bounce buffer and copy over the data.
  //
  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);
  if ((Dev->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0 &&
      (UINTN)HostAddress + *NumberOfBytes > SIZE_4GB) {

    //
    // Bounce buffering is not possible for consistent mappings
    //
    if (Operation == EfiPciIoOperationBusMasterCommonBuffer) {
      return EFI_UNSUPPORTED;
    }

    MapInfo = AllocatePool (sizeof *MapInfo);
    if (MapInfo == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    MapInfo->AllocAddress = MAX_UINT32;
    MapInfo->HostAddress = HostAddress;
    MapInfo->Operation = Operation;
    MapInfo->NumberOfBytes = *NumberOfBytes;

    Status = gBS->AllocatePages (AllocateMaxAddress, EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
                    &MapInfo->AllocAddress);
    if (EFI_ERROR (Status)) {
      //
      // If we fail here, it is likely because the system has no memory below
      // 4 GB to begin with. There is not much we can do about that other than
      // fail the map request.
      //
      FreePool (MapInfo);
      return EFI_DEVICE_ERROR;
    }
    if (Operation == EfiPciIoOperationBusMasterRead) {
      gBS->CopyMem ((VOID *)(UINTN)MapInfo->AllocAddress, HostAddress,
             *NumberOfBytes);
    }
    *DeviceAddress = MapInfo->AllocAddress;
    *Mapping = MapInfo;
  } else {
    *DeviceAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
    *Mapping = NULL;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CoherentPciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL          *This,
  IN  VOID                         *Mapping
  )
{
  NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO  *MapInfo;

  MapInfo = Mapping;
  if (MapInfo != NULL) {
    if (MapInfo->Operation == EfiPciIoOperationBusMasterWrite) {
      gBS->CopyMem (MapInfo->HostAddress, (VOID *)(UINTN)MapInfo->AllocAddress,
             MapInfo->NumberOfBytes);
    }
    gBS->FreePages (MapInfo->AllocAddress,
           EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes));
    FreePool (MapInfo);
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CoherentPciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL         *This,
  IN  EFI_ALLOCATE_TYPE           Type,
  IN  EFI_MEMORY_TYPE             MemoryType,
  IN  UINTN                       Pages,
  OUT VOID                        **HostAddress,
  IN  UINT64                      Attributes
  )
{
  NON_DISCOVERABLE_PCI_DEVICE       *Dev;
  EFI_PHYSICAL_ADDRESS              AllocAddress;
  EFI_ALLOCATE_TYPE                 AllocType;
  EFI_STATUS                        Status;

  if ((Attributes & ~(EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE |
                      EFI_PCI_ATTRIBUTE_MEMORY_CACHED)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate below 4 GB if the dual address cycle attribute has not
  // been set. If the system has no memory available below 4 GB, there
  // is little we can do except propagate the error.
  //
  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);
  if ((Dev->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    AllocAddress = MAX_UINT32;
    AllocType = AllocateMaxAddress;
  } else {
    AllocType = AllocateAnyPages;
  }

  Status = gBS->AllocatePages (AllocType, MemoryType, Pages, &AllocAddress);
  if (!EFI_ERROR (Status)) {
    *HostAddress = (VOID *)(UINTN)AllocAddress;
  }
  return Status;
}

STATIC
EFI_STATUS
CoherentPciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL         *This,
  IN  UINTN                       Pages,
  IN  VOID                        *HostAddress
  )
{
  FreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
PciIoFlush (
  IN EFI_PCI_IO_PROTOCOL          *This
  )
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciIoGetLocation (
  IN   EFI_PCI_IO_PROTOCOL  *This,
  OUT  UINTN                *SegmentNumber,
  OUT  UINTN                *BusNumber,
  OUT  UINTN                *DeviceNumber,
  OUT  UINTN                *FunctionNumber
  )
{
  if (SegmentNumber == NULL ||
      BusNumber == NULL ||
      DeviceNumber == NULL ||
      FunctionNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *SegmentNumber  = 0;
  *BusNumber      = 0xff;
  *DeviceNumber   = 0;
  *FunctionNumber = 0;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciIoAttributes (
  IN  EFI_PCI_IO_PROTOCOL                      *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
{
  NON_DISCOVERABLE_PCI_DEVICE   *Dev;
  BOOLEAN                       Enable;

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);

  Enable = FALSE;
  switch (Operation) {
  case EfiPciIoAttributeOperationGet:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    *Result = Dev->Attributes;
    break;

  case EfiPciIoAttributeOperationSupported:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    *Result = EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE;
    break;

  case EfiPciIoAttributeOperationEnable:
    Attributes |= Dev->Attributes;
  case EfiPciIoAttributeOperationSet:
    Enable = ((~Dev->Attributes & Attributes) & EFI_PCI_DEVICE_ENABLE) != 0;
    Dev->Attributes = Attributes;
    break;

  case EfiPciIoAttributeOperationDisable:
    Dev->Attributes &= ~Attributes;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  };

  //
  // If we're setting any of the EFI_PCI_DEVICE_ENABLE bits, perform
  // the device specific initialization now.
  //
  if (Enable && !Dev->Enabled && Dev->Device->Initialize != NULL) {
    Dev->Device->Initialize (Dev->Device);
    Dev->Enabled = TRUE;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports OPTIONAL,
  OUT VOID                           **Resources OPTIONAL
  )
{
  NON_DISCOVERABLE_PCI_DEVICE       *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor, *BarDesc;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;
  EFI_STATUS                        Status;

  if (Supports == NULL && Resources == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO(This);

  Status = GetBarResource (Dev, BarIndex, &BarDesc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Don't expose any configurable attributes for our emulated BAR
  //
  if (Supports != NULL) {
    *Supports = 0;
  }

  if (Resources != NULL) {
    Descriptor = AllocatePool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) +
                               sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Descriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Descriptor, BarDesc, sizeof *Descriptor);

    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Descriptor + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;

    *Resources = Descriptor;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciIoSetBarAttributes (
  IN     EFI_PCI_IO_PROTOCOL          *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC CONST EFI_PCI_IO_PROTOCOL PciIoTemplate =
{
  PciIoPollMem,
  PciIoPollIo,
  { PciIoMemRead, PciIoMemWrite },
  { PciIoIoRead,  PciIoIoWrite },
  { PciIoPciRead, PciIoPciWrite },
  PciIoCopyMem,
  CoherentPciIoMap,
  CoherentPciIoUnmap,
  CoherentPciIoAllocateBuffer,
  CoherentPciIoFreeBuffer,
  PciIoFlush,
  PciIoGetLocation,
  PciIoAttributes,
  PciIoGetBarAttributes,
  PciIoSetBarAttributes,
  0,
  0
};

VOID
InitializePciIoProtocol (
  NON_DISCOVERABLE_PCI_DEVICE     *Dev
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;
  INTN                                Idx;

  Dev->ConfigSpace.Hdr.VendorId = PCI_ID_VENDOR_UNKNOWN;
  Dev->ConfigSpace.Hdr.DeviceId = PCI_ID_DEVICE_DONTCARE;

  // Copy protocol structure
  CopyMem(&Dev->PciIo, &PciIoTemplate, sizeof PciIoTemplate);

  if (CompareGuid (Dev->Device->Type, &gEdkiiNonDiscoverableAhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_MASS_STORAGE_AHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_MASS_STORAGE_SATADPA;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset = 5;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableEhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_EHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableNvmeDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x2; // PCI_IF_NVMHCI
    Dev->ConfigSpace.Hdr.ClassCode[1] = 0x8; // PCI_CLASS_MASS_STORAGE_NVM
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableOhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_OHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableSdhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x0; // don't care
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_SUBCLASS_SD_HOST_CONTROLLER;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SYSTEM_PERIPHERAL;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableXhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_XHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableUhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_UHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset = 0;
  } else if (CompareGuid (Dev->Device->Type,
                          &gEdkiiNonDiscoverableUfsDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x0; // don't care
    Dev->ConfigSpace.Hdr.ClassCode[1] = 0x9; // UFS controller subclass;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset = 0;
  } else {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
  }

  //
  // Iterate over the resources to populate the virtual BARs
  //
  Idx = Dev->BarOffset;
  for (Desc = Dev->Device->Resources, Dev->BarCount = 0;
       Desc->Desc != ACPI_END_TAG_DESCRIPTOR;
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3)) {

    ASSERT (Desc->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR);
    ASSERT (Desc->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM);

    if (Idx >= PCI_MAX_BARS ||
        (Idx == PCI_MAX_BARS - 1 && Desc->AddrSpaceGranularity == 64)) {
      DEBUG ((DEBUG_ERROR,
        "%a: resource count exceeds number of emulated BARs\n",
        __FUNCTION__));
      ASSERT (FALSE);
      break;
    }

    Dev->ConfigSpace.Device.Bar[Idx] = (UINT32)Desc->AddrRangeMin;
    Dev->BarCount++;

    if (Desc->AddrSpaceGranularity == 64) {
      Dev->ConfigSpace.Device.Bar[Idx] |= 0x4;
      Dev->ConfigSpace.Device.Bar[++Idx] = (UINT32)RShiftU64 (
                                                     Desc->AddrRangeMin, 32);
    }
  }
}
