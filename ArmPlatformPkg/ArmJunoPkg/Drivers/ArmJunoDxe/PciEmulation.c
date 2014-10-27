/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciEmulation.h"

#define HOST_CONTROLLER_OPERATION_REG_SIZE  0x44

typedef struct {
  ACPI_HID_DEVICE_PATH      AcpiDevicePath;
  PCI_DEVICE_PATH           PciDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} EFI_PCI_IO_DEVICE_PATH;

typedef struct {
  UINT32                  Signature;
  EFI_PCI_IO_DEVICE_PATH  DevicePath;
  EFI_PCI_IO_PROTOCOL     PciIoProtocol;
  PCI_TYPE00              *ConfigSpace;
  PCI_ROOT_BRIDGE         RootBridge;
  UINTN                   Segment;
} EFI_PCI_IO_PRIVATE_DATA;

#define EFI_PCI_IO_PRIVATE_DATA_SIGNATURE     SIGNATURE_32('p', 'c', 'i', 'o')
#define EFI_PCI_IO_PRIVATE_DATA_FROM_THIS(a)  CR (a, EFI_PCI_IO_PRIVATE_DATA, PciIoProtocol, EFI_PCI_IO_PRIVATE_DATA_SIGNATURE)

EFI_PCI_IO_DEVICE_PATH PciIoDevicePathTemplate =
{
  {
    { ACPI_DEVICE_PATH, ACPI_DP, { sizeof (ACPI_HID_DEVICE_PATH), 0 } },
    EISA_PNP_ID(0x0A03),  // HID
    0                     // UID
  },
  {
    { HARDWARE_DEVICE_PATH, HW_PCI_DP, { sizeof (PCI_DEVICE_PATH), 0 } },
    0,
    0
  },
  { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0} }
};

STATIC
VOID
ConfigureUSBHost (
  VOID
  )
{
}


EFI_STATUS
PciIoPollMem (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
PciIoPollIo (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN  UINT8                        BarIndex,
  IN  UINT64                       Offset,
  IN  UINT64                       Mask,
  IN  UINT64                       Value,
  IN  UINT64                       Delay,
  OUT UINT64                       *Result
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
PciIoMemRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  EFI_PCI_IO_PRIVATE_DATA *Private = EFI_PCI_IO_PRIVATE_DATA_FROM_THIS (This);

  return PciRootBridgeIoMemRead (&Private->RootBridge.Io,
                                (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                Private->ConfigSpace->Device.Bar[BarIndex] + Offset, //Fix me ConfigSpace
                                Count,
                                Buffer
                                );
}

EFI_STATUS
PciIoMemWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        BarIndex,
  IN     UINT64                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  EFI_PCI_IO_PRIVATE_DATA *Private = EFI_PCI_IO_PRIVATE_DATA_FROM_THIS (This);

  return PciRootBridgeIoMemWrite (&Private->RootBridge.Io,
                                 (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                 Private->ConfigSpace->Device.Bar[BarIndex] + Offset,  //Fix me ConfigSpace
                                 Count,
                                 Buffer
                                 );
}

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

EFI_STATUS
PciIoIoWrite (
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

EFI_STATUS
PciIoPciRead (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  EFI_PCI_IO_PRIVATE_DATA *Private = EFI_PCI_IO_PRIVATE_DATA_FROM_THIS (This);
  EFI_STATUS Status;

  Status = PciRootBridgeIoMemRW ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH)Width,
                               Count,
                               TRUE,
                               (PTR)(UINTN)Buffer,
                               TRUE,
                               (PTR)(UINTN)(((UINT8 *)Private->ConfigSpace) + Offset)  //Fix me ConfigSpace
                              );
  return Status;
}

EFI_STATUS
PciIoPciWrite (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT32                       Offset,
  IN     UINTN                        Count,
  IN OUT VOID                         *Buffer
  )
{
  EFI_PCI_IO_PRIVATE_DATA *Private = EFI_PCI_IO_PRIVATE_DATA_FROM_THIS (This);

  return PciRootBridgeIoMemRW ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                               Count,
                               TRUE,
                               (PTR)(UINTN)(((UINT8 *)Private->ConfigSpace) + Offset),  //Fix me ConfigSpace
                               TRUE,
                               (PTR)(UINTN)Buffer
                               );
}

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

EFI_STATUS
PciIoMap (
  IN EFI_PCI_IO_PROTOCOL                *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  DMA_MAP_OPERATION   DmaOperation;

  if (Operation == EfiPciIoOperationBusMasterRead) {
    DmaOperation = MapOperationBusMasterRead;
  } else if (Operation == EfiPciIoOperationBusMasterWrite) {
    DmaOperation = MapOperationBusMasterWrite;
  } else if (Operation == EfiPciIoOperationBusMasterCommonBuffer) {
    DmaOperation = MapOperationBusMasterCommonBuffer;
  } else {
    return EFI_INVALID_PARAMETER;
  }
  return DmaMap (DmaOperation, HostAddress, NumberOfBytes, DeviceAddress, Mapping);
}

EFI_STATUS
PciIoUnmap (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  VOID                         *Mapping
  )
{
  return DmaUnmap (Mapping);
}

EFI_STATUS
PciIoAllocateBuffer (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  EFI_ALLOCATE_TYPE            Type,
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress,
  IN  UINT64                       Attributes
  )
{
  if (Attributes & EFI_PCI_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) {
    // Check this
    return EFI_UNSUPPORTED;
  }

  return DmaAllocateBuffer (MemoryType, Pages, HostAddress);
}


EFI_STATUS
PciIoFreeBuffer (
  IN EFI_PCI_IO_PROTOCOL           *This,
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  )
{
  return DmaFreeBuffer (Pages, HostAddress);
}


EFI_STATUS
PciIoFlush (
  IN EFI_PCI_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
PciIoGetLocation (
  IN EFI_PCI_IO_PROTOCOL          *This,
  OUT UINTN                       *SegmentNumber,
  OUT UINTN                       *BusNumber,
  OUT UINTN                       *DeviceNumber,
  OUT UINTN                       *FunctionNumber
  )
{
  EFI_PCI_IO_PRIVATE_DATA *Private = EFI_PCI_IO_PRIVATE_DATA_FROM_THIS (This);

  if (SegmentNumber != NULL) {
    *SegmentNumber = Private->Segment;
  }

  if (BusNumber != NULL) {
    *BusNumber = 0xff;
  }

  if (DeviceNumber != NULL) {
    *DeviceNumber = 0;
  }

  if (FunctionNumber != NULL) {
    *FunctionNumber = 0;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
{
  switch (Operation) {
  case EfiPciIoAttributeOperationGet:
  case EfiPciIoAttributeOperationSupported:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    // We are not a real PCI device so just say things we kind of do
    *Result = EFI_PCI_IO_ATTRIBUTE_MEMORY | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER | EFI_PCI_DEVICE_ENABLE;
    break;

  case EfiPciIoAttributeOperationSet:
  case EfiPciIoAttributeOperationEnable:
  case EfiPciIoAttributeOperationDisable:
    // Since we are not a real PCI device no enable/set or disable operations exist.
    return EFI_SUCCESS;

  default:
  ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  };
  return EFI_SUCCESS;
}

EFI_STATUS
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             *This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_PCI_IO_PROTOCOL PciIoTemplate =
{
  PciIoPollMem,
  PciIoPollIo,
  { PciIoMemRead, PciIoMemWrite },
  { PciIoIoRead,  PciIoIoWrite },
  { PciIoPciRead, PciIoPciWrite },
  PciIoCopyMem,
  PciIoMap,
  PciIoUnmap,
  PciIoAllocateBuffer,
  PciIoFreeBuffer,
  PciIoFlush,
  PciIoGetLocation,
  PciIoAttributes,
  PciIoGetBarAttributes,
  PciIoSetBarAttributes,
  0,
  0
};

EFI_STATUS
PciInstallDevice (
  IN UINTN            DeviceId,
  IN PHYSICAL_ADDRESS MemoryStart,
  IN UINT64           MemorySize,
  IN UINTN            ClassCode1,
  IN UINTN            ClassCode2,
  IN UINTN            ClassCode3
  )
{
  EFI_STATUS              Status;
  EFI_HANDLE              Handle;
  EFI_PCI_IO_PRIVATE_DATA *Private;

  // Configure USB host
  ConfigureUSBHost ();

  // Create a private structure
  Private = AllocatePool (sizeof (EFI_PCI_IO_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Private->Signature              = EFI_PCI_IO_PRIVATE_DATA_SIGNATURE;  // Fill in signature
  Private->RootBridge.Signature   = PCI_ROOT_BRIDGE_SIGNATURE;          // Fake Root Bridge structure needs a signature too
  Private->RootBridge.MemoryStart = MemoryStart; // Get the USB capability register base
  Private->Segment                = 0;                                  // Default to segment zero

  // Calculate the total size of the USB controller (OHCI + EHCI).
  Private->RootBridge.MemorySize = MemorySize; //CapabilityLength + (HOST_CONTROLLER_OPERATION_REG_SIZE + ((4 * PhysicalPorts) - 1));

  // Create fake PCI config space: OHCI + EHCI
  Private->ConfigSpace = AllocateZeroPool (sizeof (PCI_TYPE00));
  if (Private->ConfigSpace == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    FreePool (Private);
    return Status;
  }

  //
  // Configure PCI config space: OHCI + EHCI
  //
  Private->ConfigSpace->Hdr.VendorId = 0x3530; //TODO: Define one
  Private->ConfigSpace->Hdr.DeviceId = 0x3530; //TODO: Define one
  Private->ConfigSpace->Hdr.ClassCode[0] = ClassCode1;
  Private->ConfigSpace->Hdr.ClassCode[1] = ClassCode2;
  Private->ConfigSpace->Hdr.ClassCode[2] = ClassCode3;
  Private->ConfigSpace->Device.Bar[0] = MemoryStart;

  Handle = NULL;

  // Unique device path.
  CopyMem (&Private->DevicePath, &PciIoDevicePathTemplate, sizeof (PciIoDevicePathTemplate));
  Private->DevicePath.AcpiDevicePath.UID = 0;
  Private->DevicePath.PciDevicePath.Device = DeviceId;

  // Copy protocol structure
  CopyMem (&Private->PciIoProtocol, &PciIoTemplate, sizeof (PciIoTemplate));

  Status = gBS->InstallMultipleProtocolInterfaces (&Handle,
                                                  &gEfiPciIoProtocolGuid,       &Private->PciIoProtocol,
                                                  &gEfiDevicePathProtocolGuid,  &Private->DevicePath,
                                                  NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "PciEmulationEntryPoint InstallMultipleProtocolInterfaces () failed.\n"));
  }

  return Status;
}

EFI_STATUS
PciEmulationEntryPoint (
  VOID
  )
{
  EFI_STATUS              Status;

  Status = PciInstallDevice (0, FixedPcdGet32 (PcdSynopsysUsbOhciBaseAddress), SIZE_64KB, PCI_IF_OHCI, PCI_CLASS_SERIAL_USB, PCI_CLASS_SERIAL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "PciEmulation: failed to install OHCI device.\n"));
  }

  Status = PciInstallDevice (1, FixedPcdGet32 (PcdSynopsysUsbEhciBaseAddress), SIZE_64KB, PCI_IF_EHCI, PCI_CLASS_SERIAL_USB, PCI_CLASS_SERIAL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "PciEmulation: failed to install EHCI device.\n"));
  }

  return Status;
}
