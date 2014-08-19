/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciEmulation.h"

BOOLEAN
PciRootBridgeMemAddressValid (
  IN PCI_ROOT_BRIDGE  *Private,
  IN UINT64           Address
  )
{
  if ((Address >= Private->MemoryStart) && (Address < (Private->MemoryStart + Private->MemorySize))) {
    return TRUE;
  }

  return FALSE;
}


EFI_STATUS
PciRootBridgeIoMemRW (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINTN                                  Count,
  IN  BOOLEAN                                InStrideFlag,
  IN  PTR                                    In,
  IN  BOOLEAN                                OutStrideFlag,
  OUT PTR                                    Out
  )
{
  UINTN  Stride;
  UINTN  InStride;
  UINTN  OutStride;


  Width     = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) (Width & 0x03);
  Stride    = (UINTN)1 << Width;
  InStride  = InStrideFlag  ? Stride : 0;
  OutStride = OutStrideFlag ? Stride : 0;

  //
  // Loop for each iteration and move the data
  //
  switch (Width) {
  case EfiPciWidthUint8:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui8 = *Out.ui8;
    }
    break;
  case EfiPciWidthUint16:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui16 = *Out.ui16;
    }
    break;
  case EfiPciWidthUint32:
    for (;Count > 0; Count--, In.buf += InStride, Out.buf += OutStride) {
      *In.ui32 = *Out.ui32;
    }
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PciRootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                               *UserBuffer
  )
{
  return EFI_SUCCESS;
}

/**
  Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory operations.
  @param  Address               The base address of the memory operations.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciRootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  PCI_ROOT_BRIDGE   *Private;
  UINTN             AlignMask;
  PTR               In;
  PTR               Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  Private = INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (!PciRootBridgeMemAddressValid (Private, Address)) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  In.buf  = Buffer;
  Out.buf = (VOID *)(UINTN) Address;

  switch (Width) {
  case EfiPciWidthUint8:
  case EfiPciWidthUint16:
  case EfiPciWidthUint32:
  case EfiPciWidthUint64:
    return PciRootBridgeIoMemRW (Width, Count, TRUE, In, TRUE, Out);

  case EfiPciWidthFifoUint8:
  case EfiPciWidthFifoUint16:
  case EfiPciWidthFifoUint32:
  case EfiPciWidthFifoUint64:
    return PciRootBridgeIoMemRW (Width, Count, TRUE, In, FALSE, Out);

  case EfiPciWidthFillUint8:
  case EfiPciWidthFillUint16:
  case EfiPciWidthFillUint32:
  case EfiPciWidthFillUint64:
    return PciRootBridgeIoMemRW (Width, Count, FALSE, In, TRUE, Out);

  default:
    break;
  }

  return EFI_INVALID_PARAMETER;
}



/**
  Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory operations.
  @param  Address               The base address of the memory operations.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciRootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  PCI_ROOT_BRIDGE *Private;
  UINTN  AlignMask;
  PTR    In;
  PTR    Out;

  if ( Buffer == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  Private = INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (!PciRootBridgeMemAddressValid (Private, Address)) {
    return EFI_INVALID_PARAMETER;
  }

  AlignMask = (1 << (Width & 0x03)) - 1;
  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  In.buf  = (VOID *)(UINTN) Address;
  Out.buf = Buffer;

  switch (Width) {
  case EfiPciWidthUint8:
  case EfiPciWidthUint16:
  case EfiPciWidthUint32:
  case EfiPciWidthUint64:
    return PciRootBridgeIoMemRW (Width, Count, TRUE, In, TRUE, Out);

  case EfiPciWidthFifoUint8:
  case EfiPciWidthFifoUint16:
  case EfiPciWidthFifoUint32:
  case EfiPciWidthFifoUint64:
    return PciRootBridgeIoMemRW (Width, Count, FALSE, In, TRUE, Out);

  case EfiPciWidthFillUint8:
  case EfiPciWidthFillUint16:
  case EfiPciWidthFillUint32:
  case EfiPciWidthFillUint64:
    return PciRootBridgeIoMemRW (Width, Count, TRUE, In, FALSE, Out);

  default:
    break;
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory operations.
  @param  Address               The base address of the memory operations.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciRootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PciRootBridgeIoPciRW (This, FALSE, Width, Address, Count, Buffer);
}



/**
  Enables a PCI driver to access PCI controller registers in the PCI root bridge memory space.

  @param  This                  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  @param  Width                 Signifies the width of the memory operations.
  @param  Address               The base address of the memory operations.
  @param  Count                 The number of memory operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The data was read from or written to the PCI root bridge.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
EFI_STATUS
EFIAPI
PciRootBridgeIoPciWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PciRootBridgeIoPciRW (This, TRUE, Width, Address, Count, Buffer);
}


