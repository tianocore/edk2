/** @file
IIO PCI Root Bridge Io Protocol code. Generic enough to work for all IIOs.
Does not support configuration accesses to the extended PCI Express registers yet.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PciRootBridge.h"

//
// Define PCI express offse
//
#define PCIE_OFF(Bus, Device, Function, Register) \
    ((UINT64) ((UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) (Function << 12) + (UINTN) (Register)))

//
// Pci Root Bridge Io Module Variables
//
EFI_METRONOME_ARCH_PROTOCOL *mMetronome;
EFI_CPU_IO2_PROTOCOL        *mCpuIo;

EFI_STATUS
SimpleIioRootBridgeConstructor (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       *Protocol,
  IN EFI_HANDLE                            HostBridgeHandle,
  IN PCI_ROOT_BRIDGE_RESOURCE_APERTURE     *ResAperture,
  UINT64                                   AllocAttributes
  )
/*++

Routine Description:

  Construct the Pci Root Bridge Io protocol.

Arguments:

  Protocol          -  Protocol to initialize.
  HostBridgeHandle  -  Handle to the HostBridge.
  ResAperture       -  Resource apperture of the root bridge.
  AllocAttributes   -  Attribute of resouce allocated.

Returns:

  EFI_SUCCESS  -  Success.
  Others       -  Fail.

--*/
{
  EFI_STATUS                Status;
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  PCI_RESOURCE_TYPE         Index;
  UINT32                    HecBase;
  UINT32                    HecSize;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (Protocol);

  //
  // Initialize the apertures with default values
  //
  CopyMem (
    &PrivateData->Aperture,
    ResAperture,
    sizeof (PCI_ROOT_BRIDGE_RESOURCE_APERTURE)
    );

  for (Index = TypeIo; Index < TypeMax; Index++) {
    PrivateData->ResAllocNode[Index].Type   = Index;
    PrivateData->ResAllocNode[Index].Base   = 0;
    PrivateData->ResAllocNode[Index].Length = 0;
    PrivateData->ResAllocNode[Index].Status = ResNone;
  }

  EfiInitializeLock (&PrivateData->PciLock, TPL_HIGH_LEVEL);
  PrivateData->PciAddress             = 0xCF8;
  PrivateData->PciData                = 0xCFC;

  PrivateData->RootBridgeAllocAttrib  = AllocAttributes;
  PrivateData->Attributes             = 0;
  PrivateData->Supports = EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO |
    EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO |
    EFI_PCI_ATTRIBUTE_ISA_IO_16         |
    EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16 |
    EFI_PCI_ATTRIBUTE_VGA_MEMORY |
    EFI_PCI_ATTRIBUTE_VGA_IO_16;

  //
  // Don't support BASE above 4GB currently
  // Position to bit 39:28
  //
  HecBase = (UINT32) PcdGet64 (PcdPciExpressBaseAddress);
  HecSize = (UINT32) PcdGet64 (PcdPciExpressSize);

  ASSERT ((HecBase & (HecSize - 1)) == 0);
  ASSERT (HecBase != 0);

  PrivateData->HecBase            = HecBase;
  PrivateData->HecLen             = HecSize;

  PrivateData->BusNumberAssigned  = FALSE;
  PrivateData->BusScanCount       = 0;

  Protocol->ParentHandle          = HostBridgeHandle;

  Protocol->PollMem               = RootBridgeIoPollMem;
  Protocol->PollIo                = RootBridgeIoPollIo;

  Protocol->Mem.Read              = RootBridgeIoMemRead;
  Protocol->Mem.Write             = RootBridgeIoMemWrite;

  Protocol->Io.Read               = RootBridgeIoIoRead;
  Protocol->Io.Write              = RootBridgeIoIoWrite;

  Protocol->CopyMem               = RootBridgeIoCopyMem;

  Protocol->Pci.Read              = RootBridgeIoPciRead;
  Protocol->Pci.Write             = RootBridgeIoPciWrite;

  Protocol->Map                   = RootBridgeIoMap;
  Protocol->Unmap                 = RootBridgeIoUnmap;

  Protocol->AllocateBuffer        = RootBridgeIoAllocateBuffer;
  Protocol->FreeBuffer            = RootBridgeIoFreeBuffer;

  Protocol->Flush                 = RootBridgeIoFlush;

  Protocol->GetAttributes         = RootBridgeIoGetAttributes;
  Protocol->SetAttributes         = RootBridgeIoSetAttributes;

  Protocol->Configuration         = RootBridgeIoConfiguration;

  Protocol->SegmentNumber         = 0;

  Status                          = gBS->LocateProtocol (&gEfiMetronomeArchProtocolGuid, NULL, (VOID **) &mMetronome);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiCpuIo2ProtocolGuid,
                  NULL,
                  (VOID **) &mCpuIo
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoPollMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
/*++

Routine Description:

  Poll an address in memory mapped space until an exit condition is met
  or a timeout occurs.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  Width of the memory operation.
  Address  -  The base address of the memory operation.
  Mask     -  Mask used for polling criteria.
  Value    -  Comparison value used for polling exit criteria.
  Delay    -  Number of 100ns units to poll.
  Result   -  Pointer to the last value read from memory location.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_TIMEOUT            -  Delay expired before a match occurred.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32       Remainder;

  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 || Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // No matter what, always do a single poll.
  //
  Status = This->Mem.Read (
                      This,
                      Width,
                      Address,
                      1,
                      Result
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay != 0) {
    //
    // Determine the proper # of metronome ticks to wait for polling the
    // location.  The nuber of ticks is Roundup (Delay / mMetronome->TickPeriod)+1
    // The "+1" to account for the possibility of the first tick being short
    // because we started in the middle of a tick.
    //
    // BugBug: overriding mMetronome->TickPeriod with UINT32 until Metronome
    // protocol definition is updated.
    //
    NumberOfTicks = DivU64x32Remainder (
                      Delay,
                      (UINT32) mMetronome->TickPeriod,
                      &Remainder
                      );
    if (Remainder != 0) {
      NumberOfTicks += 1;
    }

    NumberOfTicks += 1;

    while (NumberOfTicks) {

      mMetronome->WaitForTick (mMetronome, 1);

      Status = This->Mem.Read (
                          This,
                          Width,
                          Address,
                          1,
                          Result
                          );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((*Result & Mask) == Value) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
RootBridgeIoPollIo (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                                 Address,
  IN  UINT64                                 Mask,
  IN  UINT64                                 Value,
  IN  UINT64                                 Delay,
  OUT UINT64                                 *Result
  )
/*++

Routine Description:

  Poll an address in I/O space until an exit condition is met
  or a timeout occurs.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  Width of I/O operation.
  Address  -  The base address of the I/O operation.
  Mask     -  Mask used for polling criteria.
  Value    -  Comparison value used for polling exit criteria.
  Delay    -  Number of 100ns units to poll.
  Result   -  Pointer to the last value read from memory location.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_TIMEOUT            -  Delay expired before a match occurred.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
{
  EFI_STATUS  Status;
  UINT64      NumberOfTicks;
  UINT32       Remainder;

  //
  // No matter what, always do a single poll.
  //
  if (Result == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 || Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = This->Io.Read (
                      This,
                      Width,
                      Address,
                      1,
                      Result
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((*Result & Mask) == Value) {
    return EFI_SUCCESS;
  }

  if (Delay != 0) {
    //
    // Determine the proper # of metronome ticks to wait for polling the
    // location.  The number of ticks is Roundup (Delay / mMetronome->TickPeriod)+1
    // The "+1" to account for the possibility of the first tick being short
    // because we started in the middle of a tick.
    //
    NumberOfTicks = DivU64x32Remainder (
                      Delay,
                      (UINT32) mMetronome->TickPeriod,
                      &Remainder
                      );
    if (Remainder != 0) {
      NumberOfTicks += 1;
    }

    NumberOfTicks += 1;

    while (NumberOfTicks) {

      mMetronome->WaitForTick (mMetronome, 1);

      Status = This->Io.Read (
                          This,
                          Width,
                          Address,
                          1,
                          Result
                          );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((*Result & Mask) == Value) {
        return EFI_SUCCESS;
      }

      NumberOfTicks -= 1;
    }
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allow read from memory mapped I/O space.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  The width of memory operation.
  Address  -  Base address of the memory operation.
  Count    -  Number of memory opeartion to perform.
  Buffer   -  The destination buffer to store data.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 ||
      Width == EfiPciWidthUint64 ||
      Width == EfiPciWidthFifoUint64 ||
      Width == EfiPciWidthFillUint64 ||
      Width >= EfiPciWidthMaximum
      ) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);
  //
  // Check memory access limit
  //
  if (PrivateData->Aperture.Mem64Limit > PrivateData->Aperture.Mem64Base) {
      if (Address > PrivateData->Aperture.Mem64Limit) {
        return EFI_INVALID_PARAMETER;
      }
  } else {
      if (Address > PrivateData->Aperture.Mem32Limit) {
        return EFI_INVALID_PARAMETER;
      }
  }

  return mCpuIo->Mem.Read (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      Address,
                      Count,
                      Buffer
                      );
}

EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Allow write to memory mapped I/O space.

Arguments:

  This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width    -  The width of memory operation.
  Address  -  Base address of the memory operation.
  Count    -  Number of memory opeartion to perform.
  Buffer   -  The source buffer to write data from.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

--*/
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 ||
      Width == EfiPciWidthUint64 ||
      Width == EfiPciWidthFifoUint64 ||
      Width == EfiPciWidthFillUint64 ||
      Width >= EfiPciWidthMaximum
      ) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  //
  // Check memory access limit
  //
  if (PrivateData->Aperture.Mem64Limit > PrivateData->Aperture.Mem64Base) {
      if (Address > PrivateData->Aperture.Mem64Limit) {
        return EFI_INVALID_PARAMETER;
      }
  } else {
      if (Address > PrivateData->Aperture.Mem32Limit) {
        return EFI_INVALID_PARAMETER;
      }
  }

  return mCpuIo->Mem.Write (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      Address,
                      Count,
                      Buffer
                      );
}

EFI_STATUS
EFIAPI
RootBridgeIoIoRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
/*++

Routine Description:

  Enable a PCI driver to read PCI controller registers in the
  PCI root bridge I/O space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The base address of the I/O operation.
  Count    -  The number of I/O operations to perform.
  Buffer   -  The destination buffer to store the results.

Returns:

  EFI_SUCCESS            -  The data was read from the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
{

  UINTN                     AlignMask;
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 ||
      Width == EfiPciWidthUint64 ||
      Width == EfiPciWidthFifoUint64 ||
      Width == EfiPciWidthFillUint64 ||
      Width >= EfiPciWidthMaximum
      ) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  //
  // AlignMask = (1 << Width) - 1;
  //
  AlignMask = (1 << (Width & 0x03)) - 1;

  //
  // check Io access limit
  //
  if (Address > PrivateData->Aperture.IoLimit) {
    return EFI_INVALID_PARAMETER;
  }

  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  return mCpuIo->Io.Read (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      Address,
                      Count,
                      Buffer
                      );

}

EFI_STATUS
EFIAPI
RootBridgeIoIoWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
/*++

Routine Description:

  Enable a PCI driver to write to PCI controller registers in the
  PCI root bridge I/O space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The base address of the I/O operation.
  Count    -  The number of I/O operations to perform.
  Buffer   -  The source buffer to write data from.

Returns:

  EFI_SUCCESS            -  The data was written to the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
{
  UINTN                     AlignMask;
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 ||
      Width == EfiPciWidthUint64 ||
      Width == EfiPciWidthFifoUint64 ||
      Width == EfiPciWidthFillUint64 ||
      Width >= EfiPciWidthMaximum
      ) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  //
  // AlignMask = (1 << Width) - 1;
  //
  AlignMask = (1 << (Width & 0x03)) - 1;

  //
  // Check Io access limit
  //
  if (Address > PrivateData->Aperture.IoLimit) {
    return EFI_INVALID_PARAMETER;
  }

  if (Address & AlignMask) {
    return EFI_INVALID_PARAMETER;
  }

  return mCpuIo->Io.Write (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      Address,
                      Count,
                      Buffer
                      );

}

EFI_STATUS
EFIAPI
RootBridgeIoCopyMem (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL           *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH     Width,
  IN UINT64                                    DestAddress,
  IN UINT64                                    SrcAddress,
  IN UINTN                                     Count
  )
/*++

Routine Description:

  Copy one region of PCI root bridge memory space to be copied to
  another region of PCI root bridge memory space.

Arguments:

  This         -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Width        -  Signifies the width of the memory operation.
  DestAddress  -  Destination address of the memory operation.
  SrcAddress   -  Source address of the memory operation.
  Count        -  Number of memory operations to perform.

Returns:

  EFI_SUCCESS            -  The data was copied successfully.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
{
  EFI_STATUS  Status;
  BOOLEAN     Direction;
  UINTN       Stride;
  UINTN       Index;
  UINT64      Result;

  if (Width < 0 || Width > EfiPciWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestAddress == SrcAddress) {
    return EFI_SUCCESS;
  }

  Stride    = (UINTN)1 << Width;

  Direction = TRUE;
  if ((DestAddress > SrcAddress) && (DestAddress < (SrcAddress + Count * Stride))) {
    Direction   = FALSE;
    SrcAddress  = SrcAddress + (Count - 1) * Stride;
    DestAddress = DestAddress + (Count - 1) * Stride;
  }

  for (Index = 0; Index < Count; Index++) {
    Status = RootBridgeIoMemRead (
              This,
              Width,
              SrcAddress,
              1,
              &Result
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RootBridgeIoMemWrite (
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
      SrcAddress += Stride;
      DestAddress += Stride;
    } else {
      SrcAddress -= Stride;
      DestAddress -= Stride;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
RootBridgeIoPciRW (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN BOOLEAN                                Write,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 UserAddress,
  IN UINTN                                  Count,
  IN OUT VOID                              *UserBuffer
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  PCI_CONFIG_ACCESS_CF8             Pci;
  PCI_CONFIG_ACCESS_CF8             PciAligned;
  UINT32                            Stride;
  UINTN                             PciData;
  UINTN                             PciDataStride;
  PCI_ROOT_BRIDGE_INSTANCE         *PrivateData;

  if (Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS(This);

  ASSERT (((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*)&UserAddress)->ExtendedRegister == 0x00);

  Stride = 1 << Width;

  Pci.Bits.Reg = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &UserAddress)->Register;
  Pci.Bits.Func = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &UserAddress)->Function;
  Pci.Bits.Dev = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &UserAddress)->Device;
  Pci.Bits.Bus = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &UserAddress)->Bus;
  Pci.Bits.Reserved = 0;
  Pci.Bits.Enable = 1;

  //
  // PCI Configure access are all 32-bit aligned, but by accessing the
  //  CONFIG_DATA_REGISTER (0xcfc) with different widths more cycle types
  //  are possible on PCI.
  //
  // To read a byte of PCI configuration space you load 0xcf8 and
  //  read 0xcfc, 0xcfd, 0xcfe, 0xcff
  //
  PciDataStride = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS*) &UserAddress)->Register & 0x03;

  while (Count) {
    PciAligned = Pci;
    PciAligned.Bits.Reg &= 0xfc;
    PciData = PrivateData->PciData + PciDataStride;
    EfiAcquireLock(&PrivateData->PciLock);
    This->Io.Write (This, EfiPciWidthUint32, \
                    PrivateData->PciAddress, 1, &PciAligned);
    if (Write) {
      This->Io.Write (This, Width, PciData, 1, UserBuffer);
    } else {
      This->Io.Read (This, Width, PciData, 1, UserBuffer);
    }
    EfiReleaseLock(&PrivateData->PciLock);
    UserBuffer = ((UINT8 *)UserBuffer) + Stride;
    PciDataStride = (PciDataStride + Stride) % 4;
    Count -= 1;

    //
    // Only increment the PCI address if Width is not a FIFO.
    //
    if (Width >= EfiPciWidthUint8 && Width <= EfiPciWidthUint64) {
      Pci.Bits.Reg += Stride;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoPciRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN     UINT64                                   Address,
  IN     UINTN                                    Count,
  IN OUT VOID                                     *Buffer
  )
/*++

Routine Description:

  Allows read from PCI configuration space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The address within the PCI configuration space
              for the PCI controller.
  Count    -  The number of PCI configuration operations
              to perform.
  Buffer   -  The destination buffer to store the results.

Returns:

  EFI_SUCCESS            -  The data was read from the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINT32                    PciBus;
  UINT32                    PciDev;
  UINT32                    PciFn;
  UINT32                    PciExtReg;
  UINT64                    ExtConfigAdd;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 ||
      Width == EfiPciWidthUint64 ||
      Width == EfiPciWidthFifoUint64 ||
      Width == EfiPciWidthFillUint64 ||
      Width >= EfiPciWidthMaximum
      ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read Pci configuration space
  //
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (PrivateData->HecBase == 0) {
    return RootBridgeIoPciRW (This, FALSE, Width, Address, Count, Buffer);
  }

  if (!((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->ExtendedRegister) {
    PciExtReg = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Register;
  } else {
    PciExtReg = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->ExtendedRegister & 0x0FFF;
  }

  PciBus        = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Bus;
  PciDev        = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Device;
  PciFn         = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Function;

  ExtConfigAdd  = (UINT64) PrivateData->HecBase + PCIE_OFF (PciBus, PciDev, PciFn, PciExtReg);

  return mCpuIo->Mem.Read (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      ExtConfigAdd,
                      Count,
                      Buffer
                      );
}

EFI_STATUS
EFIAPI
RootBridgeIoPciWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL          *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH    Width,
  IN UINT64                                   Address,
  IN UINTN                                    Count,
  IN OUT VOID                                 *Buffer
  )
/*++

Routine Description:

  Allows write to PCI configuration space.

Arguments:

  This     -  A pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
  Width    -  Signifies the width of the memory operation.
  Address  -  The address within the PCI configuration space
              for the PCI controller.
  Count    -  The number of PCI configuration operations
              to perform.
  Buffer   -  The source buffer to get the results.

Returns:

  EFI_SUCCESS            -  The data was written to the PCI root bridge.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to a lack of
                            resources.
--*/
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;
  UINT32                    PciBus;
  UINT32                    PciDev;
  UINT32                    PciFn;
  UINT32                    PciExtReg;
  UINT64                    ExtConfigAdd;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width < 0 || Width >= EfiPciWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Write Pci configuration space
  //
  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (PrivateData->HecBase == 0) {
    return RootBridgeIoPciRW (This, TRUE, Width, Address, Count, Buffer);
  }

  if (!((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->ExtendedRegister) {
    PciExtReg = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Register;
  } else {
    PciExtReg = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->ExtendedRegister & 0x0FFF;
  }

  PciBus        = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Bus;
  PciDev        = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Device;
  PciFn         = ((EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS *) &Address)->Function;

  ExtConfigAdd  = (UINT64) PrivateData->HecBase + PCIE_OFF (PciBus, PciDev, PciFn, PciExtReg);

  return mCpuIo->Mem.Write (
                      mCpuIo,
                      (EFI_CPU_IO_PROTOCOL_WIDTH) Width,
                      ExtConfigAdd,
                      Count,
                      Buffer
                      );
}

EFI_STATUS
EFIAPI
RootBridgeIoMap (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL            *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )
/*++

Routine Description:

  Provides the PCI controller-specific address needed to access
  system memory for DMA.

Arguments:

  This           -  A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  Operation      -  Indicate if the bus master is going to read or write
                    to system memory.
  HostAddress    -  The system memory address to map on the PCI controller.
  NumberOfBytes  -  On input the number of bytes to map.
                    On output the number of bytes that were mapped.
  DeviceAddress  -  The resulting map address for the bus master PCI
                    controller to use to access the system memory's HostAddress.
  Mapping        -  The value to pass to Unmap() when the bus master DMA
                    operation is complete.

Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameters found.
  EFI_UNSUPPORTED        -  The HostAddress cannot be mapped as a common
                            buffer.
  EFI_DEVICE_ERROR       -  The System hardware could not map the requested
                            address.
  EFI_OUT_OF_RESOURCES   -  The request could not be completed due to
                            lack of resources.

--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;
  MAP_INFO              *MapInfo;

  if (NumberOfBytes == NULL || Mapping == NULL || DeviceAddress == NULL || HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the return values to their defaults
  //
  *Mapping = NULL;

  //
  // Make sure that Operation is valid
  //
  if ((Operation < 0) || (Operation > EfiPciOperationBusMasterCommonBuffer64)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Most PCAT like chipsets can not handle performing DMA above 4GB.
  // If any part of the DMA transfer being mapped is above 4GB, then
  // map the DMA transfer to a buffer below 4GB.
  //
  PhysicalAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress;
  if ((PhysicalAddress +*NumberOfBytes) > 0x100000000ULL) {
    //
    // Common Buffer operations can not be remapped.  If the common buffer
    // if above 4GB, then it is not possible to generate a mapping, so return
    // an error.
    //
    if (Operation == EfiPciOperationBusMasterCommonBuffer || Operation == EfiPciOperationBusMasterCommonBuffer64) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if ((PhysicalAddress + *NumberOfBytes) > (DMA_MEMORY_TOP+1)) {

    //
    // Common Buffer operations can not be remapped.
    //
    if (Operation == EfiPciOperationBusMasterCommonBuffer || Operation == EfiPciOperationBusMasterCommonBuffer64) {
      *DeviceAddress = PhysicalAddress;
      return EFI_SUCCESS;
    }
    //
    // Allocate a MAP_INFO structure to remember the mapping when Unmap() is
    // called later.
    //
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (MAP_INFO),
                    (VOID **) &MapInfo
                    );
    if (EFI_ERROR (Status)) {
      *NumberOfBytes = 0;
      return Status;
    }
    //
    // Return a pointer to the MAP_INFO structure in Mapping
    //
    *Mapping = MapInfo;

    //
    // Initialize the MAP_INFO structure
    //
    MapInfo->Operation          = Operation;
    MapInfo->NumberOfBytes      = *NumberOfBytes;
    MapInfo->NumberOfPages      = EFI_SIZE_TO_PAGES (*NumberOfBytes);
    MapInfo->HostAddress        = PhysicalAddress;
    MapInfo->MappedHostAddress  = DMA_MEMORY_TOP;

    //
    // Allocate a buffer below DMA_MEMORY_TOP to map the transfer to.
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    MapInfo->NumberOfPages,
                    &MapInfo->MappedHostAddress
                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (MapInfo);
      *NumberOfBytes = 0;
      return Status;
    }
    //
    // If this is a read operation from the Bus Master's point of view,
    // then copy the contents of the real buffer into the mapped buffer
    // so the Bus Master can read the contents of the real buffer.
    //
    if (Operation == EfiPciOperationBusMasterRead || Operation == EfiPciOperationBusMasterRead64) {
      CopyMem (
        (VOID *) (UINTN) MapInfo->MappedHostAddress,
        (VOID *) (UINTN) MapInfo->HostAddress,
        MapInfo->NumberOfBytes
        );
    }
    //
    // The DeviceAddress is the address of the maped buffer below DMA_MEMORY_TOP
    //
    *DeviceAddress = MapInfo->MappedHostAddress;
  } else {
    //
    // The transfer is below DMA_MEMORY_TOP, so the DeviceAddress is simply the HostAddress
    //
    *DeviceAddress = PhysicalAddress;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoUnmap (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN VOID                             *Mapping
  )
/*++

Routine Description:

  Completes the Map() operation and releases any corresponding resources.

Arguments:

  This     -  Pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Mapping  -  The value returned from Map() operation.

Returns:

  EFI_SUCCESS            -  The range was unmapped successfully.
  EFI_INVALID_PARAMETER  -  Mapping is not a value that was returned
                            by Map operation.
  EFI_DEVICE_ERROR       -  The data was not committed to the target
                            system memory.

--*/
{
  MAP_INFO  *MapInfo;

  //
  // See if the Map() operation associated with this Unmap() required a mapping buffer.
  // If a mapping buffer was not required, then this function simply returns EFI_SUCCESS.
  //
  if (Mapping != NULL) {
    //
    // Get the MAP_INFO structure from Mapping
    //
    MapInfo = (MAP_INFO *) Mapping;

    //
    // If this is a write operation from the Bus Master's point of view,
    // then copy the contents of the mapped buffer into the real buffer
    // so the processor can read the contents of the real buffer.
    //
    if ((MapInfo->Operation == EfiPciOperationBusMasterWrite) ||
        (MapInfo->Operation == EfiPciOperationBusMasterWrite64)
        ) {
      CopyMem (
        (VOID *) (UINTN) MapInfo->HostAddress,
        (VOID *) (UINTN) MapInfo->MappedHostAddress,
        MapInfo->NumberOfBytes
        );
    }
    //
    // Free the mapped buffer and the MAP_INFO structure.
    //
    gBS->FreePages (MapInfo->MappedHostAddress, MapInfo->NumberOfPages);
    gBS->FreePool (Mapping);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoAllocateBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  EFI_ALLOCATE_TYPE                Type,
  IN  EFI_MEMORY_TYPE                  MemoryType,
  IN  UINTN                            Pages,
  OUT VOID                             **HostAddress,
  IN  UINT64                           Attributes
  )
/*++

Routine Description:

  Allocates pages that are suitable for a common buffer mapping.

Arguments:

  This         -  Pointer to EFI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Type         -  Not used and can be ignored.
  MemoryType   -  Type of memory to allocate.
  Pages        -  Number of pages to allocate.
  HostAddress  -  Pointer to store the base system memory address
                  of the allocated range.
  Attributes   -  Requested bit mask of attributes of the allocated
                  range.

Returns:

  EFI_SUCCESS            -  The requested memory range were allocated.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_UNSUPPORTED        -  Attributes is unsupported.

--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysicalAddress;

  //
  // Validate Attributes
  //
  if ((Attributes & EFI_PCI_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }
  //
  // Check for invalid inputs
  //
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  if ((MemoryType != EfiBootServicesData) && (MemoryType != EfiRuntimeServicesData)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Limit allocations to memory below DMA_MEMORY_TOP
  //
  PhysicalAddress = DMA_MEMORY_TOP;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  MemoryType,
                  Pages,
                  &PhysicalAddress
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *HostAddress = (VOID *) (UINTN) PhysicalAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoFreeBuffer (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN  UINTN                            Pages,
  OUT VOID                             *HostAddress
  )
/*++

Routine Description:

  Free memory allocated in AllocateBuffer.

Arguments:

  This         -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
                  instance.
  Pages        -  Number of pages to free.
  HostAddress  -  The base system memory address of the
                  allocated range.

Returns:

  EFI_SUCCESS            -  Requested memory pages were freed.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.

--*/
{
  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) HostAddress, Pages);
}

EFI_STATUS
EFIAPI
RootBridgeIoFlush (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL       *This
  )
/*++

Routine Description:

  Flushes all PCI posted write transactions from a PCI host
  bridge to system memory.

Arguments:

  This  - Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.

Returns:

  EFI_SUCCESS       -  PCI posted write transactions were flushed
                       from PCI host bridge to system memory.
  EFI_DEVICE_ERROR  -  Fail due to hardware error.

--*/
{
  //
  // not supported yet
  //
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoGetAttributes (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT UINT64                           *Supported,
  OUT UINT64                           *Attributes
  )
/*++

Routine Description:

  Get the attributes that a PCI root bridge supports and
  the attributes the PCI root bridge is currently using.

Arguments:

  This        -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL
                 instance.
  Supports    -  A pointer to the mask of attributes that
                 this PCI root bridge supports.
  Attributes  -  A pointer to the mask of attributes that
                 this PCI root bridge is currently using.
Returns:

  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.

--*/

// GC_TODO:    Supported - add argument and description to function comment
//
// GC_TODO:    Supported - add argument and description to function comment
//
// GC_TODO:    Supported - add argument and description to function comment
//
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (Attributes == NULL && Supported == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Set the return value for Supported and Attributes
  //
  if (Supported) {
    *Supported = PrivateData->Supports;
  }

  if (Attributes) {
    *Attributes = PrivateData->Attributes;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoSetAttributes (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  IN     UINT64                           Attributes,
  IN OUT UINT64                           *ResourceBase,
  IN OUT UINT64                           *ResourceLength
  )
/*++

Routine Description:

  Sets the attributes for a resource range on a PCI root bridge.

Arguments:

  This            -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Attributes      -  The mask of attributes to set.
  ResourceBase    -  Pointer to the base address of the resource range
                     to be modified by the attributes specified by Attributes.
  ResourceLength  -  Pointer to the length of the resource range to be modified.

Returns:
  EFI_SUCCESS            -  Success.
  EFI_INVALID_PARAMETER  -  Invalid parameter found.
  EFI_OUT_OF_RESOURCES   -  Not enough resources to set the attributes upon.

--*/

//
// GC_TODO:    EFI_UNSUPPORTED - add return value to function comment
//
{
  PCI_ROOT_BRIDGE_INSTANCE  *PrivateData;

  PrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  if (Attributes != 0) {
    Attributes &= (PrivateData->Supports);
    if (Attributes == 0) {
      return EFI_UNSUPPORTED;
    }
  }

  if (Attributes == PrivateData->Attributes) {
    return EFI_SUCCESS;
  }
  //
  // It is just a trick for some attribute can only be enabled or disabled
  // otherwise it can impact on other devices
  //
  PrivateData->Attributes = Attributes;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
RootBridgeIoConfiguration (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *This,
  OUT VOID                             **Resources
  )
/*++

Routine Description:

  Retrieves the current resource settings of this PCI root bridge
  in the form of a set of ACPI 2.0 resource descriptor.

Arguments:

  This       -  Pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  Resources  -  Pointer to the ACPI 2.0 resource descriptor that
                describe the current configuration of this PCI root
                bridge.

Returns:

  EFI_SUCCESS      -  Success.
  EFI_UNSUPPORTED  -  Current configuration of the PCI root bridge
                      could not be retrieved.

--*/

//
// GC_TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
//
{
  EFI_STATUS                        Status;
  UINTN                             Idx;

  PCI_ROOT_BRIDGE_INSTANCE          *RbPrivateData;
  PCI_RES_NODE                      *ResAllocNode;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Config;

  //
  // Get this instance of the Root Bridge.
  //
  RbPrivateData = DRIVER_INSTANCE_FROM_PCI_ROOT_BRIDGE_IO_THIS (This);

  //
  // If the pointer is not NULL, it points to a buffer already allocated.
  //
  if (RbPrivateData->ConfigBuffer == NULL) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    TypeMax * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR),
                    &RbPrivateData->ConfigBuffer
                    );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  Config = RbPrivateData->ConfigBuffer;

  ZeroMem (Config, TypeMax * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));

  for (Idx = 0; Idx < TypeMax; Idx++) {

    ResAllocNode = &RbPrivateData->ResAllocNode[Idx];

    if (ResAllocNode->Status != ResAllocated) {
      continue;
    }

    switch (ResAllocNode->Type) {

    case TypeIo:
      Config->Desc          = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len           = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType       = ACPI_ADDRESS_SPACE_TYPE_IO;
      Config->AddrRangeMin  = ResAllocNode->Base;
      Config->AddrRangeMax  = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen       = ResAllocNode->Length;
      break;

    case TypeMem32:
      Config->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType               = ACPI_ADDRESS_SPACE_TYPE_MEM;
      Config->AddrSpaceGranularity  = 32;
      Config->AddrRangeMin          = ResAllocNode->Base;
      Config->AddrRangeMax          = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen               = ResAllocNode->Length;
      break;

    case TypePMem32:
      Config->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType               = ACPI_ADDRESS_SPACE_TYPE_MEM;
      Config->SpecificFlag          = 6;
      Config->AddrSpaceGranularity  = 32;
      Config->AddrRangeMin          = ResAllocNode->Base;
      Config->AddrRangeMax          = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen               = ResAllocNode->Length;
      break;

    case TypeMem64:
      Config->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType               = ACPI_ADDRESS_SPACE_TYPE_MEM;
      Config->SpecificFlag          = 6;
      Config->AddrSpaceGranularity  = 64;
      Config->AddrRangeMin          = ResAllocNode->Base;
      Config->AddrRangeMax          = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen               = ResAllocNode->Length;
      break;

    case TypePMem64:
      Config->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len                   = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType               = ACPI_ADDRESS_SPACE_TYPE_MEM;
      Config->SpecificFlag          = 6;
      Config->AddrSpaceGranularity  = 64;
      Config->AddrRangeMin          = ResAllocNode->Base;
      Config->AddrRangeMax          = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen               = ResAllocNode->Length;
      break;

    case TypeBus:
      Config->Desc          = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Config->Len           = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;
      Config->ResType       = ACPI_ADDRESS_SPACE_TYPE_BUS;
      Config->AddrRangeMin  = ResAllocNode->Base;
      Config->AddrRangeMax  = ResAllocNode->Base + ResAllocNode->Length - 1;
      Config->AddrLen       = ResAllocNode->Length;
      break;

    default:
      break;
    }

    Config++;
  }
  //
  // Terminate the entries.
  //
  ((EFI_ACPI_END_TAG_DESCRIPTOR *) Config)->Desc      = ACPI_END_TAG_DESCRIPTOR;
  ((EFI_ACPI_END_TAG_DESCRIPTOR *) Config)->Checksum  = 0x0;

  *Resources = RbPrivateData->ConfigBuffer;
  return EFI_SUCCESS;
}
