/*++
 
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciIo.c
  
Abstract:

  PCI I/O Abstraction Driver

Revision History

--*/

#include "pcibus.h"

//
// Internal use only
//
STATIC
EFI_STATUS
ReportErrorStatusCode (
  IN PCI_IO_DEVICE               *PciIoDevice,
  IN EFI_STATUS_CODE_VALUE       Code
  );

//
// PCI I/O Support Function Prototypes
//
//
//
// Pci Io Protocol Interface
//
static EFI_PCI_IO_PROTOCOL  PciIoInterface = {
  PciIoPollMem,
  PciIoPollIo,
  {
    PciIoMemRead,
    PciIoMemWrite
  },
  {
    PciIoIoRead,
    PciIoIoWrite
  },
  {
    PciIoConfigRead,
    PciIoConfigWrite
  },
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
  NULL
};

STATIC
EFI_STATUS
ReportErrorStatusCode (
  IN PCI_IO_DEVICE               *PciIoDevice,
  IN EFI_STATUS_CODE_VALUE       Code
  )
/*++

Routine Description:

  report a error Status code of PCI bus driver controller

Arguments:
  
Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Code - add argument and description to function comment
{
  return REPORT_STATUS_CODE_WITH_DEVICE_PATH (
          EFI_ERROR_CODE | EFI_ERROR_MINOR,
          Code,
          PciIoDevice->DevicePath
          );
}

EFI_STATUS
InitializePciIoInstance (
  PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  Initializes a PCI I/O Instance

Arguments:
  
Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  CopyMem (&PciIoDevice->PciIo, &PciIoInterface, sizeof (EFI_PCI_IO_PROTOCOL));
  return EFI_SUCCESS;
}

EFI_STATUS
PciIoVerifyBarAccess (
  PCI_IO_DEVICE                   *PciIoDevice,
  UINT8                           BarIndex,
  PCI_BAR_TYPE                    Type,
  IN EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN UINTN                        Count,
  UINT64                          *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Base Address Register (BAR)

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Type - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (BarIndex == EFI_PCI_IO_PASS_THROUGH_BAR) {
    return EFI_SUCCESS;
  }

  //
  // BarIndex 0-5 is legal
  //
  if (BarIndex >= PCI_MAX_BAR) {
    return EFI_INVALID_PARAMETER;
  }

  if (!CheckBarType (PciIoDevice, BarIndex, Type)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFifoUintX then convert to EfiPciIoWidthUintX
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  if (Width >= EfiPciWidthFifoUint8 && Width <= EfiPciWidthFifoUint64) {
    Count = 1;
  }

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if ((*Offset + Count * (UINTN)(1 << Width)) - 1 >= PciIoDevice->PciBar[BarIndex].Length) {
    return EFI_INVALID_PARAMETER;
  }

  *Offset = *Offset + PciIoDevice->PciBar[BarIndex].BaseAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
PciIoVerifyConfigAccess (
  PCI_IO_DEVICE                 *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  )
/*++

Routine Description:

  Verifies access to a PCI Config Header

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  UINT64  ExtendOffset;

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Width is EfiPciIoWidthFillUintX then convert to EfiPciIoWidthUintX
  //
  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if (PciIoDevice->IsPciExp) {
    if ((*Offset + Count * (UINTN)(1 << Width)) - 1 >= PCI_EXP_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    ExtendOffset  = LShiftU64 (*Offset, 32);
    *Offset       = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, 0);
    *Offset       = (*Offset) | ExtendOffset;

  } else {
    if ((*Offset + Count * (UINTN)(1 << Width)) - 1 >= PCI_MAX_CONFIG_OFFSET) {
      return EFI_UNSUPPORTED;
    }

    *Offset = EFI_PCI_ADDRESS (PciIoDevice->BusNumber, PciIoDevice->DeviceNumber, PciIoDevice->FunctionNumber, *Offset);
  }

  return EFI_SUCCESS;
}

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
/*++

Routine Description:

  Poll PCI Memmory

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Mask - add argument and description to function comment
// TODO:    Value - add argument and description to function comment
// TODO:    Delay - add argument and description to function comment
// TODO:    Result - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoDevice->PciRootBridgeIo->PollMem (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Offset,
                                          Mask,
                                          Value,
                                          Delay,
                                          Result
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

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
/*++

Routine Description:

  Poll PCI IO

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Mask - add argument and description to function comment
// TODO:    Value - add argument and description to function comment
// TODO:    Delay - add argument and description to function comment
// TODO:    Result - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->PollIo (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          Offset,
                                          Mask,
                                          Value,
                                          Delay,
                                          Result
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

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
/*++

Routine Description:

  Performs a PCI Memory Read Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR);
  }

  return Status;
}

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
/*++

Routine Description:

  Performs a PCI Memory Write Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Mem.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoIoRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT8                      BarIndex,
  IN     UINT64                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI I/O Read Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Io.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR);
  }

  return Status;
}

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
/*++

Routine Description:

  Performs a PCI I/O Write Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->Io.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Read Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        Address;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Address     = Offset;
  Status      = PciIoVerifyConfigAccess (PciIoDevice, Width, Count, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIoDevice->PciRootBridgeIo->Pci.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Address,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:

  Performs a PCI Configuration Write Cycle

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    Buffer - add argument and description to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        Address;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Address     = Offset;
  Status      = PciIoVerifyConfigAccess (PciIoDevice, Width, Count, &Address);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIoDevice->PciRootBridgeIo->Pci.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Address,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoCopyMem (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN     UINT8                        DestBarIndex,
  IN     UINT64                       DestOffset,
  IN     UINT8                        SrcBarIndex,
  IN     UINT64                       SrcOffset,
  IN     UINTN                        Count
  )
/*++

Routine Description:

  Copy PCI Memory

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Width - add argument and description to function comment
// TODO:    DestBarIndex - add argument and description to function comment
// TODO:    DestOffset - add argument and description to function comment
// TODO:    SrcBarIndex - add argument and description to function comment
// TODO:    SrcOffset - add argument and description to function comment
// TODO:    Count - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Width < 0 || Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == EfiPciIoWidthFifoUint8  ||
      Width == EfiPciIoWidthFifoUint16 ||
      Width == EfiPciIoWidthFifoUint32 ||
      Width == EfiPciIoWidthFifoUint64 ||
      Width == EfiPciIoWidthFillUint8  ||
      Width == EfiPciIoWidthFillUint16 ||
      Width == EfiPciIoWidthFillUint32 ||
      Width == EfiPciIoWidthFillUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, DestBarIndex, PciBarTypeMem, Width, Count, &DestOffset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, SrcBarIndex, PciBarTypeMem, Width, Count, &SrcOffset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = PciIoDevice->PciRootBridgeIo->CopyMem (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          DestOffset,
                                          SrcOffset,
                                          Count
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoMap (
  IN     EFI_PCI_IO_PROTOCOL            *This,
  IN     EFI_PCI_IO_PROTOCOL_OPERATION  Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS           *DeviceAddress,
  OUT    VOID                           **Mapping
  )
/*++

Routine Description:

  Maps a memory region for DMA

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    HostAddress - add argument and description to function comment
// TODO:    NumberOfBytes - add argument and description to function comment
// TODO:    DeviceAddress - add argument and description to function comment
// TODO:    Mapping - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Operation < 0 || Operation >= EfiPciIoOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) {
    Operation = (EFI_PCI_IO_PROTOCOL_OPERATION) (Operation + EfiPciOperationBusMasterRead64);
  }

  Status = PciIoDevice->PciRootBridgeIo->Map (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION) Operation,
                                          HostAddress,
                                          NumberOfBytes,
                                          DeviceAddress,
                                          Mapping
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
/*++

Routine Description:

  Unmaps a memory region for DMA

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Mapping - add argument and description to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->Unmap (
                                          PciIoDevice->PciRootBridgeIo,
                                          Mapping
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoAllocateBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 Pages,
  OUT VOID                  **HostAddress,
  IN  UINT64                Attributes
  )
/*++

Routine Description:

  Allocates a common buffer for DMA

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Type - add argument and description to function comment
// TODO:    MemoryType - add argument and description to function comment
// TODO:    Pages - add argument and description to function comment
// TODO:    HostAddress - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if (Attributes &
      (~(EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE | EFI_PCI_ATTRIBUTE_MEMORY_CACHED))) {
    return EFI_UNSUPPORTED;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) {
    Attributes |= EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE;
  }

  Status = PciIoDevice->PciRootBridgeIo->AllocateBuffer (
                                          PciIoDevice->PciRootBridgeIo,
                                          Type,
                                          MemoryType,
                                          Pages,
                                          HostAddress,
                                          Attributes
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
/*++

Routine Description:

  Frees a common buffer 

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Pages - add argument and description to function comment
// TODO:    HostAddress - add argument and description to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->FreeBuffer (
                                          PciIoDevice->PciRootBridgeIo,
                                          Pages,
                                          HostAddress
                                          );

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  )
/*++

Routine Description:

  Flushes a DMA buffer

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->Flush (
                                           PciIoDevice->PciRootBridgeIo
                                           );
  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  )
/*++

Routine Description:

  Gets a PCI device's current bus number, device number, and function number.

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Segment - add argument and description to function comment
// TODO:    Bus - add argument and description to function comment
// TODO:    Device - add argument and description to function comment
// TODO:    Function - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Segment == NULL || Bus == NULL || Device == NULL || Function == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Segment  = PciIoDevice->PciRootBridgeIo->SegmentNumber;
  *Bus      = PciIoDevice->BusNumber;
  *Device   = PciIoDevice->DeviceNumber;
  *Function = PciIoDevice->FunctionNumber;

  return EFI_SUCCESS;
}

BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE       *PciIoDevice,
  UINT8                  BarIndex,
  PCI_BAR_TYPE           BarType
  )
/*++

Routine Description:

  Sets a PCI controllers attributes on a resource range

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    BarType - add argument and description to function comment
{
  switch (BarType) {

  case PciBarTypeMem:

    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem32  &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypePMem64 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeMem64    ) {
      return FALSE;
    }

    return TRUE;

  case PciBarTypeIo:
    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo32 &&
        PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeIo16){
      return FALSE;
    }

    return TRUE;

  default:
    break;
  }

  return FALSE;
}

EFI_STATUS
ModifyRootBridgeAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  UINT64                                   Attributes,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
/*++

Routine Description:

  Set new attributes to a Root Bridge

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  UINT64      PciRootBridgeSupports;
  UINT64      PciRootBridgeAttributes;
  UINT64      NewPciRootBridgeAttributes;
  EFI_STATUS  Status;

  //
  // Get the current attributes of this PCI device's PCI Root Bridge
  //
  Status = PciIoDevice->PciRootBridgeIo->GetAttributes (
                                          PciIoDevice->PciRootBridgeIo,
                                          &PciRootBridgeSupports,
                                          &PciRootBridgeAttributes
                                          );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Record the new attribute of the Root Bridge
  //
  if (Operation == EfiPciIoAttributeOperationEnable) {
    NewPciRootBridgeAttributes = PciRootBridgeAttributes | Attributes;
  } else {
    NewPciRootBridgeAttributes = PciRootBridgeAttributes & (~Attributes);
  }
 
  //
  // Call the PCI Root Bridge to attempt to modify the attributes
  //
  if (NewPciRootBridgeAttributes ^ PciRootBridgeAttributes) {

    Status = PciIoDevice->PciRootBridgeIo->SetAttributes (
                                            PciIoDevice->PciRootBridgeIo,
                                            NewPciRootBridgeAttributes,
                                            NULL,
                                            NULL
                                            );
    if (EFI_ERROR (Status)) {
      //
      // The PCI Root Bridge could not modify the attributes, so return the error.
      //
      return EFI_UNSUPPORTED;
    }
  }
    
  //
  // Also update the attributes for this Root Bridge structure
  //
  PciIoDevice->Attributes = NewPciRootBridgeAttributes;
  return EFI_SUCCESS;

}

EFI_STATUS
SupportPaletteSnoopAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
/*++

Routine Description:

  Check whether this device can be enable/disable to snoop

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  PCI_IO_DEVICE *Temp;
  UINT16        VGACommand;

  //
  // Snoop attribute can be only modified by GFX
  //
  if (!IS_PCI_GFX (&PciIoDevice->Pci)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get the boot VGA on the same segement
  //
  Temp = ActiveVGADeviceOnTheSameSegment (PciIoDevice);

  if (!Temp) {
    //
    // If there is no VGA device on the segement, set
    // this graphics card to decode the palette range
    //
    return EFI_SUCCESS;
  }
  
  //
  // Check these two agents are on the same path
  //
  if (!PciDevicesOnTheSamePath (Temp, PciIoDevice)) {
    //
    // they are not on the same path, so snoop can be enabled or disabled
    //
    return EFI_SUCCESS;
  }
  //
  // Check if they are on the same bus
  //
  if (Temp->Parent == PciIoDevice->Parent) {

    PciReadCommandRegister (Temp, &VGACommand);

    //
    // If they are on the same bus, either one can
    // be set to snoop, the other set to decode
    //
    if (VGACommand & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) {
      //
      // VGA has set to snoop, so GFX can be only set to disable snoop
      //
      if (Operation == EfiPciIoAttributeOperationEnable) {
        return EFI_UNSUPPORTED;
      }
    } else {
      //
      // VGA has disabled to snoop, so GFX can be only enabled
      //
      if (Operation == EfiPciIoAttributeOperationDisable) {
        return EFI_UNSUPPORTED;
      }
    }

    return EFI_SUCCESS;
  }
  
  //
  // If they are on  the same path but on the different bus
  // The first agent is set to snoop, the second one set to
  // decode
  //
            
  if (Temp->BusNumber < PciIoDevice->BusNumber) {
    //
    // GFX should be set to decode
    //
    if (Operation == EfiPciIoAttributeOperationDisable) {
      PciEnableCommandRegister (Temp, EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
      Temp->Attributes |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
    } else {
      return EFI_UNSUPPORTED;
    }

  } else {
    //
    // GFX should be set to snoop
    //
    if (Operation == EfiPciIoAttributeOperationEnable) {
      PciDisableCommandRegister (Temp, EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
      Temp->Attributes &= (~EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
    } else {
      return EFI_UNSUPPORTED;
    }

  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    Result - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_STATUS    Status;

  PCI_IO_DEVICE *PciIoDevice;
  PCI_IO_DEVICE *UpStreamBridge;
  PCI_IO_DEVICE *Temp;

  UINT64        Supports;
  UINT64        UpStreamAttributes;
  UINT16        BridgeControl;
  UINT16        Command;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  switch (Operation) {
  case EfiPciIoAttributeOperationGet:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Attributes;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationSupported:
    if (Result == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    *Result = PciIoDevice->Supports;
    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationSet:
    Status = PciIoDevice->PciIo.Attributes (
                                  &(PciIoDevice->PciIo),
                                  EfiPciIoAttributeOperationEnable,
                                  Attributes,
                                  NULL
                                  );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    Status = PciIoDevice->PciIo.Attributes (
                                  &(PciIoDevice->PciIo),
                                  EfiPciIoAttributeOperationDisable,
                                  (~Attributes) & (PciIoDevice->Supports),
                                  NULL
                                  );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    return EFI_SUCCESS;

  case EfiPciIoAttributeOperationEnable:
  case EfiPciIoAttributeOperationDisable:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }
  //
  // Just a trick for ENABLE attribute
  //
  if ((Attributes & EFI_PCI_DEVICE_ENABLE) == EFI_PCI_DEVICE_ENABLE) {
    Attributes &= (PciIoDevice->Supports);

    //
    // Raise the EFI_P_PC_ENABLE Status code
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      EFI_IO_BUS_PCI | EFI_P_PC_ENABLE,
      PciIoDevice->DevicePath
      );
  }

  //
  // If no attributes can be supported, then return.
  // Otherwise, set the attributes that it can support.
  //
  Supports = (PciIoDevice->Supports) & Attributes;
  if (Supports != Attributes) {
    return EFI_UNSUPPORTED;
  }
   
  //
  // For Root Bridge, just call RootBridgeIo to set attributes;
  //
  if (!PciIoDevice->Parent) {
    Status = ModifyRootBridgeAttributes (PciIoDevice, Attributes, Operation);
    return Status;
  }

  Command       = 0;
  BridgeControl = 0;

  //
  // Check VGA and VGA16, they can not be set at the same time
  //
  if (((Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO)         &&
       (Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO_16))         ||
      ((Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO)         &&
       (Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) ||
      ((Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO) &&
       (Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_IO_16))         ||
      ((Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO) &&
       (Attributes & EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) ) {
    return EFI_UNSUPPORTED;
  }

  //
  // For PPB & P2C, set relevant attribute bits
  //
  if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

    if (Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_VGA;
    }

    if (Attributes & EFI_PCI_IO_ATTRIBUTE_ISA_IO) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_ISA;
    }

    if (Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) {
      Command |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
    }

    if (Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16 | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_VGA_16;
    }

  } else {
    //
    // Do with the attributes on VGA
    // Only for VGA's legacy resource, we just can enable once.
    //
    if (Attributes &
        (EFI_PCI_IO_ATTRIBUTE_VGA_IO    |
         EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 |
         EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY)) {
      //
      // Check if a VGA has been enabled before enabling a new one
      //
      if (Operation == EfiPciIoAttributeOperationEnable) {
        //
        // Check if there have been an active VGA device on the same segment
        //
        Temp = ActiveVGADeviceOnTheSameSegment (PciIoDevice);
        if (Temp && Temp != PciIoDevice) {
          //
          // An active VGA has been detected, so can not enable another
          //
          return EFI_UNSUPPORTED;
        }
      }
    }
    
    //
    // Do with the attributes on GFX
    //
    if (Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) {

      if (Operation == EfiPciIoAttributeOperationEnable) {
        //
        // Check if snoop can be enabled in current configuration
        //
        Status = SupportPaletteSnoopAttributes (PciIoDevice, Operation);

        if (EFI_ERROR (Status)) {
        
          //
          // Enable operation is forbidden, so mask the bit in attributes
          // so as to keep consistent with the actual Status
          //
          // Attributes &= (~EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO);
          //
          //
          //
          return EFI_UNSUPPORTED;

        }
      }

      //
      // It can be supported, so get ready to set the bit
      //
      Command |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
    }
  }

  if (Attributes & EFI_PCI_IO_ATTRIBUTE_IO) {
    Command |= EFI_PCI_COMMAND_IO_SPACE;
  }

  if (Attributes & EFI_PCI_IO_ATTRIBUTE_MEMORY) {
    Command |= EFI_PCI_COMMAND_MEMORY_SPACE;
  }

  if (Attributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) {
    Command |= EFI_PCI_COMMAND_BUS_MASTER;
  }
  //
  // The upstream bridge should be also set to revelant attribute
  // expect for IO, Mem and BusMaster
  //
  UpStreamAttributes = Attributes & 
                       (~(EFI_PCI_IO_ATTRIBUTE_IO     |
                          EFI_PCI_IO_ATTRIBUTE_MEMORY |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER
                          )
                        );
  UpStreamBridge = PciIoDevice->Parent;

  if (Operation == EfiPciIoAttributeOperationEnable) {
    //
    // Enable relevant attributes to command register and bridge control register
    //
    Status = PciEnableCommandRegister (PciIoDevice, Command);
    if (BridgeControl) {
      Status = PciEnableBridgeControlRegister (PciIoDevice, BridgeControl);
    }

    PciIoDevice->Attributes |= Attributes;

    //
    // Enable attributes of the upstream bridge
    //
    Status = UpStreamBridge->PciIo.Attributes (
                                    &(UpStreamBridge->PciIo),
                                    EfiPciIoAttributeOperationEnable,
                                    UpStreamAttributes,
                                    NULL
                                    );
  } else {
    
    //
    // Disable relevant attributes to command register and bridge control register
    //
    Status = PciDisableCommandRegister (PciIoDevice, Command);
    if (BridgeControl) {
      Status = PciDisableBridgeControlRegister (PciIoDevice, BridgeControl);
    }

    PciIoDevice->Attributes &= (~Attributes);
    Status = EFI_SUCCESS;

  }

  if (EFI_ERROR (Status)) {
    ReportErrorStatusCode (PciIoDevice, EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR);
  }

  return Status;
}

EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Supports - add argument and description to function comment
// TODO:    Resources - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_OUT_OF_RESOURCES - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{

  UINT8                             *Configuration;
  UINT8                             NumConfig;
  PCI_IO_DEVICE                     *PciIoDevice;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Ptr;
  EFI_ACPI_END_TAG_DESCRIPTOR       *PtrEnd;

  NumConfig   = 0;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Supports == NULL && Resources == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BarIndex >= PCI_MAX_BAR) {
    return EFI_UNSUPPORTED;
  }

  //
  // This driver does not support modifications to the WRITE_COMBINE or
  // CACHED attributes for BAR ranges.
  //
  if (Supports != NULL) {
    *Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;
  }

  if (Resources != NULL) {

    if (PciIoDevice->PciBar[BarIndex].BarType != PciBarTypeUnknown) {
      NumConfig = 1;
    }

    Configuration = AllocatePool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Configuration == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (
      Configuration,
      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * NumConfig + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR)
      );

    Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;

    if (NumConfig == 1) {
      Ptr->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      Ptr->Len          = sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3;

      Ptr->AddrRangeMin = PciIoDevice->PciBar[BarIndex].BaseAddress;
      Ptr->AddrLen      = PciIoDevice->PciBar[BarIndex].Length;
      Ptr->AddrRangeMax = PciIoDevice->PciBar[BarIndex].Alignment;

      switch (PciIoDevice->PciBar[BarIndex].BarType) {
      case PciBarTypeIo16:
      case PciBarTypeIo32:
        //
        // Io
        //
        Ptr->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
        break;

      case PciBarTypeMem32:
        //
        // Mem
        //
        Ptr->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // 32 bit
        //
        Ptr->AddrSpaceGranularity = 32;
        break;

      case PciBarTypePMem32:
        //
        // Mem
        //
        Ptr->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // prefechable
        //
        Ptr->SpecificFlag = 0x6;
        //
        // 32 bit
        //
        Ptr->AddrSpaceGranularity = 32;
        break;

      case PciBarTypeMem64:
        //
        // Mem
        //
        Ptr->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // 64 bit
        //
        Ptr->AddrSpaceGranularity = 64;
        break;

      case PciBarTypePMem64:
        //
        // Mem
        //
        Ptr->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
        //
        // prefechable
        //
        Ptr->SpecificFlag = 0x6;
        //
        // 64 bit
        //
        Ptr->AddrSpaceGranularity = 64;
        break;

      default:
        break;
      }

      Ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) ((UINT8 *) Ptr + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR));
    }
    
    //
    // put the checksum
    //
    PtrEnd            = (EFI_ACPI_END_TAG_DESCRIPTOR *) ((UINT8 *) Ptr);
    PtrEnd->Desc      = ACPI_END_TAG_DESCRIPTOR;
    PtrEnd->Checksum  = 0;

    *Resources        = Configuration;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
/*++

Routine Description:


Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    BarIndex - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Length - add argument and description to function comment
// TODO:    EFI_INVALID_PARAMETER - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;
  UINT64        NonRelativeOffset;
  UINT64        Supports;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  //
  // Make sure Offset and Length are not NULL
  //
  if (Offset == NULL || Length == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown) {
    return EFI_UNSUPPORTED;
  }
  //
  // This driver does not support setting the WRITE_COMBINE or the CACHED attributes.
  // If Attributes is not 0, then return EFI_UNSUPPORTED.
  //
  Supports = PciIoDevice->Supports & EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED & EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE;

  if (Attributes != (Attributes & Supports)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Attributes must be supported.  Make sure the BAR range describd by BarIndex, Offset, and
  // Length are valid for this PCI device.
  //
  NonRelativeOffset = *Offset;
  Status = PciIoVerifyBarAccess (
            PciIoDevice,
            BarIndex,
            PciBarTypeMem,
            EfiPciIoWidthUint8,
            (UINT32) *Length,
            &NonRelativeOffset
            );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
UpStreamBridgesAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    Attributes - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  PCI_IO_DEVICE       *Parent;
  EFI_PCI_IO_PROTOCOL *PciIo;

  Parent = PciIoDevice->Parent;

  while (Parent && IS_PCI_BRIDGE (&Parent->Pci)) {

    //
    // Get the PciIo Protocol
    //
    PciIo = &Parent->PciIo;

    PciIo->Attributes (PciIo, Operation, Attributes, NULL);

    Parent = Parent->Parent;
  }

  return EFI_SUCCESS;
}

BOOLEAN
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE        *PciDevice1,
  IN PCI_IO_DEVICE        *PciDevice2
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciDevice1 - add argument and description to function comment
// TODO:    PciDevice2 - add argument and description to function comment
{
  BOOLEAN   Existed1;
  BOOLEAN   Existed2;

  if (PciDevice1->Parent == PciDevice2->Parent) {
    return TRUE;
  }

  Existed1 = PciDeviceExisted (PciDevice1->Parent, PciDevice2);
  Existed2 = PciDeviceExisted (PciDevice2->Parent, PciDevice1);

  return (BOOLEAN) (Existed1 || Existed2);
}
