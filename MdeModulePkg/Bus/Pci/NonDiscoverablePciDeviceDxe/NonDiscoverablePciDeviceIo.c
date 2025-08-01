/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NonDiscoverablePciDeviceIo.h"

#include <Library/DxeServicesTableLib.h>

#include <IndustryStandard/Acpi.h>

#include <Protocol/PciRootBridgeIo.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS             AllocAddress;
  VOID                             *HostAddress;
  EFI_PCI_IO_PROTOCOL_OPERATION    Operation;
  UINTN                            NumberOfBytes;
} NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO;

/**
  Get the resource associated with BAR number 'BarIndex'.

  @param  Dev           Point to the NON_DISCOVERABLE_PCI_DEVICE instance.
  @param  BarIndex      The BAR index of the standard PCI Configuration header to use as the
                        base address for the memory operation to perform.
  @param  Descriptor    Points to the address space descriptor
**/
STATIC
EFI_STATUS
GetBarResource (
  IN  NON_DISCOVERABLE_PCI_DEVICE        *Dev,
  IN  UINT8                              BarIndex,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptor
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;

  if (BarIndex < Dev->BarOffset) {
    return EFI_NOT_FOUND;
  }

  BarIndex -= (UINT8)Dev->BarOffset;

  if (BarIndex >= Dev->BarCount) {
    return EFI_UNSUPPORTED;
  }

  for (Desc = Dev->Device->Resources;
       Desc->Desc != ACPI_END_TAG_DESCRIPTOR;
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3))
  {
    if (BarIndex == 0) {
      *Descriptor = Desc;
      return EFI_SUCCESS;
    }

    BarIndex -= 1;
  }

  return EFI_NOT_FOUND;
}

/**
  Reads from the memory space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoPollMem (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  UINTN                              Count;
  EFI_STATUS                         Status;

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev   = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  Count = 1;

  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Reads from the memory space of a PCI controller. Returns either when the polling exit criteria is
  satisfied or after a defined duration.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory operation.
  @param  Mask                  Mask used for the polling criteria.
  @param  Value                 The comparison value used for the polling exit criteria.
  @param  Delay                 The number of 100 ns units to poll.
  @param  Result                Pointer to the last value read from the memory location.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoPollIo (
  IN  EFI_PCI_IO_PROTOCOL        *This,
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINT8                      BarIndex,
  IN  UINT64                     Offset,
  IN  UINT64                     Mask,
  IN  UINT64                     Value,
  IN  UINT64                     Delay,
  OUT UINT64                     *Result
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  UINTN                              Count;
  EFI_STATUS                         Status;

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev   = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  Count = 1;

  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  Width         Signifies the width of the memory or I/O operations.
  @param  Count         The number of memory or I/O operations to perform.
  @param  DstStride     The stride of the destination buffer.
  @param  Dst           For read operations, the destination buffer to store the results. For write
                        operations, the destination buffer to write data to.
  @param  SrcStride     The stride of the source buffer.
  @param  Src           For read operations, the source buffer to read data from. For write
                        operations, the source buffer to write data from.

  @retval EFI_SUCCESS            The data was read from or written to the PCI controller.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoMemRW (
  IN  EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                      Count,
  IN  UINTN                      DstStride,
  IN  VOID                       *Dst,
  IN  UINTN                      SrcStride,
  OUT CONST VOID                 *Src
  )
{
  volatile UINT8         *Dst8;
  volatile UINT16        *Dst16;
  volatile UINT32        *Dst32;
  volatile CONST UINT8   *Src8;
  volatile CONST UINT16  *Src16;
  volatile CONST UINT32  *Src32;

  //
  // Loop for each iteration and move the data
  //
  switch (Width & 0x3) {
    case EfiPciWidthUint8:
      Dst8 = (UINT8 *)Dst;
      Src8 = (UINT8 *)Src;
      for ( ; Count > 0; Count--, Dst8 += DstStride, Src8 += SrcStride) {
        *Dst8 = *Src8;
      }

      break;
    case EfiPciWidthUint16:
      Dst16 = (UINT16 *)Dst;
      Src16 = (UINT16 *)Src;
      for ( ; Count > 0; Count--, Dst16 += DstStride, Src16 += SrcStride) {
        *Dst16 = *Src16;
      }

      break;
    case EfiPciWidthUint32:
      Dst32 = (UINT32 *)Dst;
      Src32 = (UINT32 *)Src;
      for ( ; Count > 0; Count--, Dst32 += DstStride, Src32 += SrcStride) {
        *Dst32 = *Src32;
      }

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoMemRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  UINTN                              AlignMask;
  VOID                               *Address;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  EFI_STATUS                         Status;

  Desc = NULL;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

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

  Address   = (VOID *)(UINTN)(Desc->AddrRangeMin + Offset);
  AlignMask = (1 << (Width & 0x03)) - 1;
  if ((UINTN)Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Width) {
    case EfiPciIoWidthUint8:
    case EfiPciIoWidthUint16:
    case EfiPciIoWidthUint32:
    case EfiPciIoWidthUint64:
      return PciIoMemRW (Width, Count, 1, Buffer, 1, Address);

    case EfiPciIoWidthFifoUint8:
    case EfiPciIoWidthFifoUint16:
    case EfiPciIoWidthFifoUint32:
    case EfiPciIoWidthFifoUint64:
      return PciIoMemRW (Width, Count, 1, Buffer, 0, Address);

    case EfiPciIoWidthFillUint8:
    case EfiPciIoWidthFillUint16:
    case EfiPciIoWidthFillUint32:
    case EfiPciIoWidthFillUint64:
      return PciIoMemRW (Width, Count, 0, Buffer, 1, Address);

    default:
      break;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoMemWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  UINTN                              AlignMask;
  VOID                               *Address;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  EFI_STATUS                         Status;

  Desc = NULL; // MS_CHANGE for vs2017

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

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

  Address   = (VOID *)(UINTN)(Desc->AddrRangeMin + Offset);
  AlignMask = (1 << (Width & 0x03)) - 1;
  if ((UINTN)Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Width) {
    case EfiPciIoWidthUint8:
    case EfiPciIoWidthUint16:
    case EfiPciIoWidthUint32:
    case EfiPciIoWidthUint64:
      return PciIoMemRW (Width, Count, 1, Address, 1, Buffer);

    case EfiPciIoWidthFifoUint8:
    case EfiPciIoWidthFifoUint16:
    case EfiPciIoWidthFifoUint32:
    case EfiPciIoWidthFifoUint64:
      return PciIoMemRW (Width, Count, 0, Address, 1, Buffer);

    case EfiPciIoWidthFillUint8:
    case EfiPciIoWidthFillUint16:
    case EfiPciIoWidthFillUint32:
    case EfiPciIoWidthFillUint64:
      return PciIoMemRW (Width, Count, 1, Address, 0, Buffer);

    default:
      break;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoIoRead (
  IN EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  EFI_STATUS                         Status;

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Enable a PCI driver to access PCI controller registers in the PCI memory or I/O space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for the memory or I/O operation to perform.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoIoWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  EFI_STATUS                         Status;

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Enable a PCI driver to access PCI config space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoPciRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;
  VOID                         *Address;
  UINTN                        Length;

  if ((Width < 0) || (Width >= EfiPciIoWidthMaximum) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Dev     = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  Address = (UINT8 *)&Dev->ConfigSpace + Offset;
  Length  = Count << ((UINTN)Width & 0x3);

  if (Offset >= sizeof (Dev->ConfigSpace)) {
    ZeroMem (Buffer, Length);
    return EFI_SUCCESS;
  }

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

/**
  Enable a PCI driver to access PCI config space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory or I/O operations.
  @param  Offset                The offset within the selected BAR to start the memory or I/O operation.
  @param  Count                 The number of memory or I/O operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from

  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI BAR specified by BarIndex.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoPciWrite (
  IN EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;
  VOID                         *Address;

  if ((Width < 0) || (Width >= EfiPciIoWidthMaximum) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Dev     = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  Address = (UINT8 *)&Dev->ConfigSpace + Offset;

  if (Offset + (Count << ((UINTN)Width & 0x3)) > sizeof (Dev->ConfigSpace)) {
    return EFI_UNSUPPORTED;
  }

  return PciIoMemRW (Width, Count, 1, Address, 1, Buffer);
}

/**
  Enables a PCI driver to copy one region of PCI memory space to another region of PCI
  memory space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  DestBarIndex          The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  DestOffset            The destination offset within the BAR specified by DestBarIndex to
                                start the memory writes for the copy operation.
  @param  SrcBarIndex           The BAR index in the standard PCI Configuration header to use as the
                                base address for the memory operation to perform.
  @param  SrcOffset             The source offset within the BAR specified by SrcBarIndex to start
                                the memory reads for the copy operation.
  @param  Count                 The number of memory operations to perform. Bytes moved is Width
                                size * Count, starting at DestOffset and SrcOffset.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      DestBarIndex,
  IN     UINT64                     DestOffset,
  IN     UINT8                      SrcBarIndex,
  IN     UINT64                     SrcOffset,
  IN     UINTN                      Count
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *DestDesc;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *SrcDesc;
  EFI_STATUS                         Status;

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  Status = GetBarResource (Dev, DestBarIndex, &DestDesc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (DestOffset + (Count << (Width & 0x3)) > DestDesc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  Status = GetBarResource (Dev, SrcBarIndex, &SrcDesc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (SrcOffset + (Count << (Width & 0x3)) > SrcDesc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Provides the PCI controller-specific addresses needed to access system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
STATIC
EFI_STATUS
EFIAPI
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

  if ((Operation != EfiPciIoOperationBusMasterRead) &&
      (Operation != EfiPciIoOperationBusMasterWrite) &&
      (Operation != EfiPciIoOperationBusMasterCommonBuffer))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((HostAddress   == NULL) ||
      (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) ||
      (Mapping       == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If HostAddress exceeds 4 GB, and this device does not support 64-bit DMA
  // addressing, we need to allocate a bounce buffer and copy over the data.
  //
  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  if (((Dev->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) &&
      ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress + *NumberOfBytes > SIZE_4GB))
  {
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

    MapInfo->AllocAddress  = MAX_UINT32;
    MapInfo->HostAddress   = HostAddress;
    MapInfo->Operation     = Operation;
    MapInfo->NumberOfBytes = *NumberOfBytes;

    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
                    &MapInfo->AllocAddress
                    );
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
      gBS->CopyMem (
             (VOID *)(UINTN)MapInfo->AllocAddress,
             HostAddress,
             *NumberOfBytes
             );
    }

    *DeviceAddress = MapInfo->AllocAddress;
    *Mapping       = MapInfo;
  } else {
    *DeviceAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
    *Mapping       = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.

**/
STATIC
EFI_STATUS
EFIAPI
CoherentPciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
{
  NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO  *MapInfo;

  MapInfo = Mapping;
  if (MapInfo != NULL) {
    if (MapInfo->Operation == EfiPciIoOperationBusMasterWrite) {
      gBS->CopyMem (
             MapInfo->HostAddress,
             (VOID *)(UINTN)MapInfo->AllocAddress,
             MapInfo->NumberOfBytes
             );
    }

    gBS->FreePages (
           MapInfo->AllocAddress,
           EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes)
           );
    FreePool (MapInfo);
  }

  return EFI_SUCCESS;
}

/**
  Allocates pages.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
STATIC
EFI_STATUS
EFIAPI
CoherentPciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;
  EFI_PHYSICAL_ADDRESS         AllocAddress;
  EFI_ALLOCATE_TYPE            AllocType;
  EFI_STATUS                   Status;

  if ((Attributes & ~(EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE |
                      EFI_PCI_ATTRIBUTE_MEMORY_CACHED)) != 0)
  {
    return EFI_UNSUPPORTED;
  }

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MemoryType != EfiBootServicesData) &&
      (MemoryType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate below 4 GB if the dual address cycle attribute has not
  // been set. If the system has no memory available below 4 GB, there
  // is little we can do except propagate the error.
  //
  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  if ((Dev->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0) {
    AllocAddress = MAX_UINT32;
    AllocType    = AllocateMaxAddress;
  } else {
    AllocType = AllocateAnyPages;
  }

  Status = gBS->AllocatePages (AllocType, MemoryType, Pages, &AllocAddress);
  if (!EFI_ERROR (Status)) {
    *HostAddress = (VOID *)(UINTN)AllocAddress;
  }

  return Status;
}

/**
  Frees memory that was allocated in function CoherentPciIoAllocateBuffer ().

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.

**/
STATIC
EFI_STATUS
EFIAPI
CoherentPciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  UINTN                Pages,
  IN  VOID                 *HostAddress
  )
{
  FreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}

/**
  Frees memory that was allocated in function NonCoherentPciIoAllocateBuffer ().

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval others                The operation contain some errors.

**/
STATIC
EFI_STATUS
EFIAPI
NonCoherentPciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  UINTN                Pages,
  IN  VOID                 *HostAddress
  )
{
  NON_DISCOVERABLE_PCI_DEVICE                  *Dev;
  LIST_ENTRY                                   *Entry;
  EFI_STATUS                                   Status;
  NON_DISCOVERABLE_DEVICE_UNCACHED_ALLOCATION  *Alloc;
  NON_DISCOVERABLE_DEVICE_UNCACHED_ALLOCATION  *AllocHead;
  NON_DISCOVERABLE_DEVICE_UNCACHED_ALLOCATION  *AllocTail;
  BOOLEAN                                      Found;
  UINTN                                        StartPages;
  UINTN                                        EndPages;

  if (HostAddress != ALIGN_POINTER (HostAddress, EFI_PAGE_SIZE)) {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  Found = FALSE;
  Alloc = NULL;

  AllocHead = NULL;
  AllocTail = NULL;

  //
  // Find the uncached allocation list entry associated
  // with this allocation
  //
  for (Entry = Dev->UncachedAllocationList.ForwardLink;
       Entry != &Dev->UncachedAllocationList;
       Entry = Entry->ForwardLink)
  {
    Alloc = BASE_CR (Entry, NON_DISCOVERABLE_DEVICE_UNCACHED_ALLOCATION, List);

    StartPages = 0;
    if (Alloc->HostAddress < HostAddress) {
      StartPages = EFI_SIZE_TO_PAGES (
                     (UINTN)HostAddress - (UINTN)Alloc->HostAddress
                     );
    }

    if ((Alloc->HostAddress <= HostAddress) &&
        (Alloc->NumPages >= (Pages + StartPages)))
    {
      //
      // We are freeing at least part of what we were given
      // before by AllocateBuffer()
      //
      Found = TRUE;
      break;
    }
  }

  if (!Found || (Alloc == NULL)) {
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  EndPages = Alloc->NumPages - (Pages + StartPages);

  if (StartPages != 0) {
    AllocHead = AllocatePool (sizeof *AllocHead);
    if (AllocHead == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AllocHead->HostAddress = Alloc->HostAddress;

    AllocHead->NumPages   = StartPages;
    AllocHead->Attributes = Alloc->Attributes;
  }

  if (EndPages != 0) {
    AllocTail = AllocatePool (sizeof *AllocTail);
    if (AllocTail == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    AllocTail->HostAddress = (UINT8 *)Alloc->HostAddress +
                             EFI_PAGES_TO_SIZE (Pages + StartPages);

    AllocTail->NumPages   = EndPages;
    AllocTail->Attributes = Alloc->Attributes;
  }

  RemoveEntryList (&Alloc->List);
  //
  // Record this new sub allocations in the linked list, so we
  // can restore the memory space attributes later
  //
  if (AllocHead != NULL) {
    InsertHeadList (&Dev->UncachedAllocationList, &AllocHead->List);
  }

  if (AllocTail != NULL) {
    InsertHeadList (&Dev->UncachedAllocationList, &AllocTail->List);
  }

  Status = gDS->SetMemorySpaceAttributes (
                  (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress,
                  EFI_PAGES_TO_SIZE (Pages),
                  Alloc->Attributes
                  );
  if (EFI_ERROR (Status)) {
    goto FreeAlloc;
  }

  //
  // If we fail to restore the original attributes, it is better to leak the
  // memory than to return it to the heap
  //
  FreePages (HostAddress, Pages);

FreeAlloc:
  FreePool (Alloc);
  return Status;
}

/**
  Allocates pages.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Type                  This parameter is not used and must be ignored.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
STATIC
EFI_STATUS
EFIAPI
NonCoherentPciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE    Type,
  IN  EFI_MEMORY_TYPE      MemoryType,
  IN  UINTN                Pages,
  OUT VOID                 **HostAddress,
  IN  UINT64               Attributes
  )
{
  NON_DISCOVERABLE_PCI_DEVICE                  *Dev;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR              GcdDescriptor;
  EFI_STATUS                                   Status;
  UINT64                                       MemType;
  NON_DISCOVERABLE_DEVICE_UNCACHED_ALLOCATION  *Alloc;
  VOID                                         *AllocAddress;

  MemType      = EFI_MEMORY_XP;
  AllocAddress = NULL; // MS_CHANGE for vs2017

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  Status = CoherentPciIoAllocateBuffer (
             This,
             Type,
             MemoryType,
             Pages,
             &AllocAddress,
             Attributes
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->GetMemorySpaceDescriptor (
                  (EFI_PHYSICAL_ADDRESS)(UINTN)AllocAddress,
                  &GcdDescriptor
                  );
  if (EFI_ERROR (Status)) {
    goto FreeBuffer;
  }

  if ((GcdDescriptor.Capabilities & (EFI_MEMORY_WC | EFI_MEMORY_UC)) == 0) {
    Status = EFI_UNSUPPORTED;
    goto FreeBuffer;
  }

  //
  // Set the preferred memory attributes
  //
  if (((Attributes & EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE) != 0) ||
      ((GcdDescriptor.Capabilities & EFI_MEMORY_UC) == 0))
  {
    //
    // Use write combining if it was requested, or if it is the only
    // type supported by the region.
    //
    MemType |= EFI_MEMORY_WC;
  } else {
    MemType |= EFI_MEMORY_UC;
  }

  Alloc = AllocatePool (sizeof *Alloc);
  if (Alloc == NULL) {
    goto FreeBuffer;
  }

  Alloc->HostAddress = AllocAddress;
  Alloc->NumPages    = Pages;
  Alloc->Attributes  = GcdDescriptor.Attributes;

  //
  // Record this allocation in the linked list, so we
  // can restore the memory space attributes later
  //
  InsertHeadList (&Dev->UncachedAllocationList, &Alloc->List);

  //
  // Ensure that EFI_MEMORY_XP is in the capability set
  //
  if ((GcdDescriptor.Capabilities & EFI_MEMORY_XP) != EFI_MEMORY_XP) {
    Status = gDS->SetMemorySpaceCapabilities (
                    (PHYSICAL_ADDRESS)(UINTN)AllocAddress,
                    EFI_PAGES_TO_SIZE (Pages),
                    GcdDescriptor.Capabilities | EFI_MEMORY_XP
                    );

    // if we were to fail setting the capability, this would indicate an internal failure of the GCD code. We should
    // assert here to let a platform know something went crazy, but for a release build we can let the allocation occur
    // without the EFI_MEMORY_XP bit set, as that was the existing behavior
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed to set EFI_MEMORY_XP capability on 0x%llx for length 0x%llx. Attempting to allocate without XP set.\n",
        __func__,
        AllocAddress,
        EFI_PAGES_TO_SIZE (Pages)
        ));

      ASSERT_EFI_ERROR (Status);

      MemType &= ~EFI_MEMORY_XP;
    }
  }

  Status = gDS->SetMemorySpaceAttributes (
                  (EFI_PHYSICAL_ADDRESS)(UINTN)AllocAddress,
                  EFI_PAGES_TO_SIZE (Pages),
                  MemType
                  );
  if (EFI_ERROR (Status)) {
    goto RemoveList;
  }

  Status = mCpu->FlushDataCache (
                   mCpu,
                   (EFI_PHYSICAL_ADDRESS)(UINTN)AllocAddress,
                   EFI_PAGES_TO_SIZE (Pages),
                   EfiCpuFlushTypeInvalidate
                   );
  if (EFI_ERROR (Status)) {
    goto RemoveList;
  }

  *HostAddress = AllocAddress;

  return EFI_SUCCESS;

RemoveList:
  RemoveEntryList (&Alloc->List);
  FreePool (Alloc);

FreeBuffer:
  CoherentPciIoFreeBuffer (This, Pages, AllocAddress);
  return Status;
}

/**
  Provides the PCI controller-specific addresses needed to access system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
STATIC
EFI_STATUS
EFIAPI
NonCoherentPciIoMap (
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
  UINTN                                 AlignMask;
  VOID                                  *AllocAddress;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR       GcdDescriptor;
  BOOLEAN                               Bounce;

  AllocAddress = NULL; // MS_CHANGE for vs2017

  if ((HostAddress   == NULL) ||
      (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) ||
      (Mapping       == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Operation != EfiPciIoOperationBusMasterRead) &&
      (Operation != EfiPciIoOperationBusMasterWrite) &&
      (Operation != EfiPciIoOperationBusMasterCommonBuffer))
  {
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = AllocatePool (sizeof *MapInfo);
  if (MapInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MapInfo->HostAddress   = HostAddress;
  MapInfo->Operation     = Operation;
  MapInfo->NumberOfBytes = *NumberOfBytes;

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  //
  // If this device does not support 64-bit DMA addressing, we need to allocate
  // a bounce buffer and copy over the data in case HostAddress >= 4 GB.
  //
  Bounce = ((Dev->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) == 0 &&
            (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress + *NumberOfBytes > SIZE_4GB);

  if (!Bounce) {
    switch (Operation) {
      case EfiPciIoOperationBusMasterRead:
      case EfiPciIoOperationBusMasterWrite:
        //
        // For streaming DMA, it is sufficient if the buffer is aligned to
        // the CPUs DMA buffer alignment.
        //
        AlignMask = mCpu->DmaBufferAlignment - 1;
        if ((((UINTN)HostAddress | *NumberOfBytes) & AlignMask) == 0) {
          break;
        }

      // fall through

      case EfiPciIoOperationBusMasterCommonBuffer:
        //
        // Check whether the host address refers to an uncached mapping.
        //
        Status = gDS->GetMemorySpaceDescriptor (
                        (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress,
                        &GcdDescriptor
                        );
        if (EFI_ERROR (Status) ||
            ((GcdDescriptor.Attributes & (EFI_MEMORY_WB|EFI_MEMORY_WT)) != 0))
        {
          Bounce = TRUE;
        }

        break;

      default:
        ASSERT (FALSE);
    }
  }

  if (Bounce) {
    if (Operation == EfiPciIoOperationBusMasterCommonBuffer) {
      Status = EFI_DEVICE_ERROR;
      goto FreeMapInfo;
    }

    Status = NonCoherentPciIoAllocateBuffer (
               This,
               AllocateAnyPages,
               EfiBootServicesData,
               EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
               &AllocAddress,
               EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE
               );
    if (EFI_ERROR (Status)) {
      goto FreeMapInfo;
    }

    MapInfo->AllocAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocAddress;
    if (Operation == EfiPciIoOperationBusMasterRead) {
      gBS->CopyMem (AllocAddress, HostAddress, *NumberOfBytes);
    }

    *DeviceAddress = MapInfo->AllocAddress;
  } else {
    MapInfo->AllocAddress = 0;
    *DeviceAddress        = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;

    //
    // We are not using a bounce buffer: the mapping is sufficiently
    // aligned to allow us to simply flush the caches. Note that cleaning
    // the caches is necessary for both data directions:
    // - for bus master read, we want the latest data to be present
    //   in main memory
    // - for bus master write, we don't want any stale dirty cachelines that
    //   may be written back unexpectedly, and clobber the data written to
    //   main memory by the device.
    //
    mCpu->FlushDataCache (
            mCpu,
            (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress,
            *NumberOfBytes,
            EfiCpuFlushTypeWriteBack
            );
  }

  *Mapping = MapInfo;
  return EFI_SUCCESS;

FreeMapInfo:
  FreePool (MapInfo);

  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.

**/
STATIC
EFI_STATUS
EFIAPI
NonCoherentPciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
{
  NON_DISCOVERABLE_PCI_DEVICE_MAP_INFO  *MapInfo;

  if (Mapping == NULL) {
    return EFI_DEVICE_ERROR;
  }

  MapInfo = Mapping;
  if (MapInfo->AllocAddress != 0) {
    //
    // We are using a bounce buffer: copy back the data if necessary,
    // and free the buffer.
    //
    if (MapInfo->Operation == EfiPciIoOperationBusMasterWrite) {
      gBS->CopyMem (
             MapInfo->HostAddress,
             (VOID *)(UINTN)MapInfo->AllocAddress,
             MapInfo->NumberOfBytes
             );
    }

    NonCoherentPciIoFreeBuffer (
      This,
      EFI_SIZE_TO_PAGES (MapInfo->NumberOfBytes),
      (VOID *)(UINTN)MapInfo->AllocAddress
      );
  } else {
    //
    // We are *not* using a bounce buffer: if this is a bus master write,
    // we have to invalidate the caches so the CPU will see the uncached
    // data written by the device.
    //
    if (MapInfo->Operation == EfiPciIoOperationBusMasterWrite) {
      mCpu->FlushDataCache (
              mCpu,
              (EFI_PHYSICAL_ADDRESS)(UINTN)MapInfo->HostAddress,
              MapInfo->NumberOfBytes,
              EfiCpuFlushTypeInvalidate
              );
    }
  }

  FreePool (MapInfo);
  return EFI_SUCCESS;
}

/**
  Flushes all PCI posted write transactions from a PCI host bridge to system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoFlush (
  IN EFI_PCI_IO_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Retrieves this PCI controller's current PCI bus number, device number, and function number.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  SegmentNumber         The PCI controller's current PCI segment number.
  @param  BusNumber             The PCI controller's current PCI bus number.
  @param  DeviceNumber          The PCI controller's current PCI device number.
  @param  FunctionNumber        The PCI controller's current PCI function number.

  @retval EFI_SUCCESS           The PCI controller location was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN   EFI_PCI_IO_PROTOCOL  *This,
  OUT  UINTN                *SegmentNumber,
  OUT  UINTN                *BusNumber,
  OUT  UINTN                *DeviceNumber,
  OUT  UINTN                *FunctionNumber
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;

  if ((SegmentNumber == NULL) ||
      (BusNumber == NULL) ||
      (DeviceNumber == NULL) ||
      (FunctionNumber == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  *SegmentNumber  = 0xff;
  *BusNumber      = Dev->UniqueId >> 5;
  *DeviceNumber   = Dev->UniqueId & 0x1f;
  *FunctionNumber = 0;

  return EFI_SUCCESS;
}

/**
  Performs an operation on the attributes that this PCI controller supports. The operations include
  getting the set of supported attributes, retrieving the current attributes, setting the current
  attributes, enabling attributes, and disabling attributes.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Operation             The operation to perform on the attributes for this PCI controller.
  @param  Attributes            The mask of attributes that are used for Set, Enable, and Disable
                                operations.
  @param  Result                A pointer to the result mask of attributes that are returned for the Get
                                and Supported operations.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       one or more of the bits set in
                                Attributes are not supported by this PCI controller or one of
                                its parent bridges when Operation is Set, Enable or Disable.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoAttributes (
  IN  EFI_PCI_IO_PROTOCOL                      *This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;
  BOOLEAN                      Enable;

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

  if ((Attributes & (~(DEV_SUPPORTED_ATTRIBUTES))) != 0) {
    return EFI_UNSUPPORTED;
  }

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

      *Result = DEV_SUPPORTED_ATTRIBUTES;
      break;

    case EfiPciIoAttributeOperationEnable:
      Attributes |= Dev->Attributes;
    case EfiPciIoAttributeOperationSet:
      Enable          = ((~Dev->Attributes & Attributes) & EFI_PCI_DEVICE_ENABLE) != 0;
      Dev->Attributes = Attributes;
      break;

    case EfiPciIoAttributeOperationDisable:
      Dev->Attributes &= ~Attributes;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // If we're setting any of the EFI_PCI_DEVICE_ENABLE bits, perform
  // the device specific initialization now.
  //
  if (Enable && !Dev->Enabled && (Dev->Device->Initialize != NULL)) {
    Dev->Device->Initialize (Dev->Device);
    Dev->Enabled = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Gets the attributes that this PCI controller supports setting on a BAR using
  SetBarAttributes(), and retrieves the list of resource descriptors for a BAR.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Supports              A pointer to the mask of attributes that this PCI controller supports
                                setting for this BAR with SetBarAttributes().
  @param  Resources             A pointer to the ACPI 2.0 resource descriptors that describe the current
                                configuration of this BAR of the PCI controller.

  @retval EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI
                                controller supports are returned in Supports. If Resources
                                is not NULL, then the ACPI 2.0 resource descriptors that the PCI
                                controller is currently using are returned in Resources.
  @retval EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to allocate
                                Resources.

**/
STATIC
EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL  *This,
  IN  UINT8               BarIndex,
  OUT UINT64              *Supports OPTIONAL,
  OUT VOID                **Resources OPTIONAL
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *BarDesc;
  EFI_ACPI_END_TAG_DESCRIPTOR        *End;
  EFI_STATUS                         Status;

  BarDesc = NULL; // MS_CHANGE for vs2017

  if ((Supports == NULL) && (Resources == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);

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
    Descriptor = AllocatePool (
                   sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) +
                   sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
                   );
    if (Descriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Descriptor, BarDesc, sizeof *Descriptor);

    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *)(Descriptor + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;

    *Resources = Descriptor;
  }

  return EFI_SUCCESS;
}

/**
  Sets the attributes for a range of a BAR on a PCI controller.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Attributes            The mask of attributes to set for the resource range specified by
                                BarIndex, Offset, and Length.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Offset                A pointer to the BAR relative base address of the resource range to be
                                modified by the attributes specified by Attributes.
  @param  Length                A pointer to the length of the resource range to be modified by the
                                attributes specified by Attributes.
**/
STATIC
EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN     EFI_PCI_IO_PROTOCOL  *This,
  IN     UINT64               Attributes,
  IN     UINT8                BarIndex,
  IN OUT UINT64               *Offset,
  IN OUT UINT64               *Length
  )
{
  NON_DISCOVERABLE_PCI_DEVICE        *Dev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  EFI_PCI_IO_PROTOCOL_WIDTH          Width;
  UINTN                              Count;
  EFI_STATUS                         Status;

  if ((Attributes & (~DEV_SUPPORTED_ATTRIBUTES)) != 0) {
    return EFI_UNSUPPORTED;
  }

  if ((Offset == NULL) || (Length == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Dev   = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (This);
  Width = EfiPciIoWidthUint8;
  Count = (UINT32)*Length;

  Status = GetBarResource (Dev, BarIndex, &Desc);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*Offset + (Count << (Width & 0x3)) > Desc->AddrLen) {
    return EFI_UNSUPPORTED;
  }

  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

STATIC CONST EFI_PCI_IO_PROTOCOL  PciIoTemplate =
{
  PciIoPollMem,
  PciIoPollIo,
  { PciIoMemRead,             PciIoMemWrite    },
  { PciIoIoRead,              PciIoIoWrite     },
  { PciIoPciRead,             PciIoPciWrite    },
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

/**
  Initialize PciIo Protocol.

  @param  Dev      Point to NON_DISCOVERABLE_PCI_DEVICE instance.

**/
VOID
InitializePciIoProtocol (
  NON_DISCOVERABLE_PCI_DEVICE  *Dev
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  INTN                               Idx;

  InitializeListHead (&Dev->UncachedAllocationList);

  Dev->ConfigSpace.Hdr.VendorId = PCI_ID_VENDOR_UNKNOWN;
  Dev->ConfigSpace.Hdr.DeviceId = PCI_ID_DEVICE_DONTCARE;

  // Copy protocol structure
  CopyMem (&Dev->PciIo, &PciIoTemplate, sizeof PciIoTemplate);

  if (Dev->Device->DmaType == NonDiscoverableDeviceDmaTypeNonCoherent) {
    Dev->PciIo.AllocateBuffer = NonCoherentPciIoAllocateBuffer;
    Dev->PciIo.FreeBuffer     = NonCoherentPciIoFreeBuffer;
    Dev->PciIo.Map            = NonCoherentPciIoMap;
    Dev->PciIo.Unmap          = NonCoherentPciIoUnmap;
  }

  if (CompareGuid (Dev->Device->Type, &gEdkiiNonDiscoverableAhciDeviceGuid)) {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_MASS_STORAGE_AHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_MASS_STORAGE_SATADPA;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset                    = 5;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableEhciDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_EHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableNvmeDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x2; // PCI_IF_NVMHCI
    Dev->ConfigSpace.Hdr.ClassCode[1] = 0x8; // PCI_CLASS_MASS_STORAGE_NVM
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableOhciDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_OHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableSdhciDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x0; // don't care
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_SUBCLASS_SD_HOST_CONTROLLER;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SYSTEM_PERIPHERAL;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableXhciDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_XHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableUhciDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = PCI_IF_UHCI;
    Dev->ConfigSpace.Hdr.ClassCode[1] = PCI_CLASS_SERIAL_USB;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_SERIAL;
    Dev->BarOffset                    = 0;
  } else if (CompareGuid (
               Dev->Device->Type,
               &gEdkiiNonDiscoverableUfsDeviceGuid
               ))
  {
    Dev->ConfigSpace.Hdr.ClassCode[0] = 0x0; // don't care
    Dev->ConfigSpace.Hdr.ClassCode[1] = 0x9; // UFS controller subclass;
    Dev->ConfigSpace.Hdr.ClassCode[2] = PCI_CLASS_MASS_STORAGE;
    Dev->BarOffset                    = 0;
  } else {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
  }

  //
  // Iterate over the resources to populate the virtual BARs
  //
  Idx = Dev->BarOffset;
  for (Desc = Dev->Device->Resources, Dev->BarCount = 0;
       Desc->Desc != ACPI_END_TAG_DESCRIPTOR;
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3))
  {
    ASSERT (Desc->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR);
    ASSERT (Desc->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM);

    if ((Idx >= PCI_MAX_BAR) ||
        ((Idx == PCI_MAX_BAR - 1) && (Desc->AddrSpaceGranularity == 64)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: resource count exceeds number of emulated BARs\n",
        __func__
        ));
      ASSERT (FALSE);
      break;
    }

    Dev->ConfigSpace.Device.Bar[Idx] = (UINT32)Desc->AddrRangeMin;
    Dev->BarCount++;

    if (Desc->AddrSpaceGranularity == 64) {
      Dev->ConfigSpace.Device.Bar[Idx]  |= 0x4;
      Dev->ConfigSpace.Device.Bar[++Idx] = (UINT32)RShiftU64 (
                                                     Desc->AddrRangeMin,
                                                     32
                                                     );
    }
  }
}
