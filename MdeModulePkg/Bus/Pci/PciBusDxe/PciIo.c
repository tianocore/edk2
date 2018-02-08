/** @file
  EFI PCI IO protocol functions implementation for PCI Bus module.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PciBus.h"

extern EDKII_IOMMU_PROTOCOL                          *mIoMmuProtocol;

//
// Pci Io Protocol Interface
//
EFI_PCI_IO_PROTOCOL  mPciIoInterface = {
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

/**
  Initializes a PCI I/O Instance.

  @param PciIoDevice    Pci device instance.

**/
VOID
InitializePciIoInstance (
  IN PCI_IO_DEVICE               *PciIoDevice
  )
{
  CopyMem (&PciIoDevice->PciIo, &mPciIoInterface, sizeof (EFI_PCI_IO_PROTOCOL));
}

/**
  Verifies access to a PCI Base Address Register (BAR).

  @param PciIoDevice  Pci device instance.
  @param BarIndex     The BAR index of the standard PCI Configuration header to use as the
                      base address for the memory or I/O operation to perform.
  @param Type         Operation type could be memory or I/O.
  @param Width        Signifies the width of the memory or I/O operations.
  @param Count        The number of memory or I/O operations to perform.
  @param Offset       The offset within the PCI configuration space for the PCI controller.

  @retval EFI_INVALID_PARAMETER Invalid Width/BarIndex or Bar type.
  @retval EFI_SUCCESS           Successfully verified.

**/
EFI_STATUS
PciIoVerifyBarAccess (
  IN PCI_IO_DEVICE                   *PciIoDevice,
  IN UINT8                           BarIndex,
  IN PCI_BAR_TYPE                    Type,
  IN IN EFI_PCI_IO_PROTOCOL_WIDTH    Width,
  IN IN UINTN                        Count,
  IN UINT64                          *Offset
  )
{
  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
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
  if (Width >= EfiPciIoWidthFifoUint8 && Width <= EfiPciIoWidthFifoUint64) {
    Count = 1;
  }

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & 0x03);

  if ((*Offset + Count * (UINTN)(1 << Width)) - 1 >= PciIoDevice->PciBar[BarIndex].Length) {
    return EFI_INVALID_PARAMETER;
  }

  *Offset = *Offset + PciIoDevice->PciBar[BarIndex].BaseAddress;

  return EFI_SUCCESS;
}

/**
  Verifies access to a PCI Configuration Header.

  @param PciIoDevice  Pci device instance.
  @param Width        Signifies the width of the memory or I/O operations.
  @param Count        The number of memory or I/O operations to perform.
  @param Offset       The offset within the PCI configuration space for the PCI controller.

  @retval EFI_INVALID_PARAMETER  Invalid Width
  @retval EFI_UNSUPPORTED        Offset overflowed.
  @retval EFI_SUCCESS            Successfully verified.

**/
EFI_STATUS
PciIoVerifyConfigAccess (
  IN PCI_IO_DEVICE              *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN UINTN                      Count,
  IN UINT64                     *Offset
  )
{
  UINT64  ExtendOffset;

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
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

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
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
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Status  = PciIoMemRead (This, Width, BarIndex, Offset, 1, Result);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if ((*Result & Mask) == Value || Delay == 0) {
        return EFI_SUCCESS;
      }
      do {
        //
        // Stall 10 us = 100 * 100ns
        //
        gBS->Stall (10);

        Status  = PciIoMemRead (This, Width, BarIndex, Offset, 1, Result);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        if ((*Result & Mask) == Value) {
          return EFI_SUCCESS;
        }
        if (Delay <= 100) {
          return EFI_TIMEOUT;
        }
        Delay -= 100;
      } while (TRUE);
    }
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
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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

  @retval EFI_SUCCESS           The last data returned from the access matched the poll exit criteria.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       Offset is not valid for the BarIndex of this PCI controller.
  @retval EFI_TIMEOUT           Delay expired before a match occurred.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.

**/
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
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width > EfiPciIoWidthUint64) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, 1, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Status  = PciIoIoRead (This, Width, BarIndex, Offset, 1, Result);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if ((*Result & Mask) == Value || Delay == 0) {
        return EFI_SUCCESS;
      }
      do {
        //
        // Stall 10 us = 100 * 100ns
        //
        gBS->Stall (10);

        Status  = PciIoIoRead (This, Width, BarIndex, Offset, 1, Result);
        if (EFI_ERROR (Status)) {
          return Status;
        }
        if ((*Result & Mask) == Value) {
          return EFI_SUCCESS;
        }
        if (Delay <= 100) {
          return EFI_TIMEOUT;
        }
        Delay -= 100;
      } while (TRUE);
    }
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
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }  
  

  Status = PciIoDevice->PciRootBridgeIo->Mem.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeMem, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }

  Status = PciIoDevice->PciRootBridgeIo->Mem.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }    

  Status = PciIoDevice->PciRootBridgeIo->Io.Read (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PciIoVerifyBarAccess (PciIoDevice, BarIndex, PciBarTypeIo, Width, Count, &Offset);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }  

  Status = PciIoDevice->PciRootBridgeIo->Io.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Offset,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
EFIAPI
PciIoConfigRead (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
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
  
  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }    

  Status = PciIoDevice->PciRootBridgeIo->Pci.Read (
                                               PciIoDevice->PciRootBridgeIo,
                                               (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                               Address,
                                               Count,
                                               Buffer
                                               );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_READ_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Enable a PCI driver to access PCI controller registers in PCI configuration space.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Width                 Signifies the width of the memory operations.
  @param  Offset                The offset within the PCI configuration space for the PCI controller.
  @param  Count                 The number of PCI configuration operations to perform.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.


  @retval EFI_SUCCESS           The data was read from or written to the PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by Offset, Width, and Count is not
                                valid for the PCI configuration header of the PCI controller.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_INVALID_PARAMETER Buffer is NULL or Width is invalid.

**/
EFI_STATUS
EFIAPI
PciIoConfigWrite (
  IN     EFI_PCI_IO_PROTOCOL        *This,
  IN     EFI_PCI_IO_PROTOCOL_WIDTH  Width,
  IN     UINT32                     Offset,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
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

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((Offset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }  
  
  Status = PciIoDevice->PciRootBridgeIo->Pci.Write (
                                              PciIoDevice->PciRootBridgeIo,
                                              (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                              Address,
                                              Count,
                                              Buffer
                                              );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_WRITE_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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

  @retval EFI_SUCCESS           The data was copied from one memory region to another memory region.
  @retval EFI_UNSUPPORTED       DestBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       SrcBarIndex not valid for this PCI controller.
  @retval EFI_UNSUPPORTED       The address range specified by DestOffset, Width, and Count
                                is not valid for the PCI BAR specified by DestBarIndex.
  @retval EFI_UNSUPPORTED       The address range specified by SrcOffset, Width, and Count is
                                not valid for the PCI BAR specified by SrcBarIndex.
  @retval EFI_INVALID_PARAMETER Width is invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
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
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Width >= EfiPciIoWidthMaximum) {
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

  //
  // If request is not aligned, then convert request to EfiPciIoWithXXXUint8
  //  
  if (FeaturePcdGet (PcdUnalignedPciIoEnable)) {
    if ((SrcOffset & ((1 << (Width & 0x03)) - 1)) != 0 || (DestOffset & ((1 << (Width & 0x03)) - 1)) != 0) {
      Count *=  (UINTN)(1 << (Width & 0x03));
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH) (Width & (~0x03));
    }
  }  

  Status = PciIoDevice->PciRootBridgeIo->CopyMem (
                                          PciIoDevice->PciRootBridgeIo,
                                          (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH) Width,
                                          DestOffset,
                                          SrcOffset,
                                          Count
                                          );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

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
{
  EFI_STATUS                                 Status;
  PCI_IO_DEVICE                              *PciIoDevice;
  UINT64                                     IoMmuAttribute;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION  RootBridgeIoOperation;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((UINT32)Operation >= EfiPciIoOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RootBridgeIoOperation = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION)Operation;
  if ((PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) != 0) {
    RootBridgeIoOperation = (EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_OPERATION)(Operation + EfiPciOperationBusMasterRead64);
  }

  Status = PciIoDevice->PciRootBridgeIo->Map (
                                          PciIoDevice->PciRootBridgeIo,
                                          RootBridgeIoOperation,
                                          HostAddress,
                                          NumberOfBytes,
                                          DeviceAddress,
                                          Mapping
                                          );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  if (mIoMmuProtocol != NULL) {
    if (!EFI_ERROR (Status)) {
      switch (Operation) {
      case EfiPciIoOperationBusMasterRead:
        IoMmuAttribute = EDKII_IOMMU_ACCESS_READ;
        break;
      case EfiPciIoOperationBusMasterWrite:
        IoMmuAttribute = EDKII_IOMMU_ACCESS_WRITE;
        break;
      case EfiPciIoOperationBusMasterCommonBuffer:
        IoMmuAttribute = EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE;
        break;
      default:
        ASSERT(FALSE);
        return EFI_INVALID_PARAMETER;
      }
      mIoMmuProtocol->SetAttribute (
                        mIoMmuProtocol,
                        PciIoDevice->Handle,
                        *Mapping,
                        IoMmuAttribute
                        );
    }
  }

  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
PciIoUnmap (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  IN  VOID                 *Mapping
  )
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (mIoMmuProtocol != NULL) {
    mIoMmuProtocol->SetAttribute (
                      mIoMmuProtocol,
                      PciIoDevice->Handle,
                      Mapping,
                      0
                      );
  }

  Status = PciIoDevice->PciRootBridgeIo->Unmap (
                                          PciIoDevice->PciRootBridgeIo,
                                          Mapping
                                          );

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Allocates pages that are suitable for an EfiPciIoOperationBusMasterCommonBuffer
  or EfiPciOperationBusMasterCommonBuffer64 mapping.

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
                                MEMORY_WRITE_COMBINE, MEMORY_CACHED and DUAL_ADDRESS_CYCLE.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
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
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  if ((Attributes &
      (~(EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE | EFI_PCI_ATTRIBUTE_MEMORY_CACHED))) != 0){
    return EFI_UNSUPPORTED;
  }

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if ((PciIoDevice->Attributes & EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE) != 0) {
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
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
PciIoFreeBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *This,
  IN  UINTN                 Pages,
  IN  VOID                  *HostAddress
  )
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
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Flushes all PCI posted write transactions from a PCI host bridge to system memory.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The PCI posted write transactions were flushed from the PCI host
                                bridge to system memory.
  @retval EFI_DEVICE_ERROR      The PCI posted write transactions were not flushed from the PCI
                                host bridge due to a hardware error.

**/
EFI_STATUS
EFIAPI
PciIoFlush (
  IN  EFI_PCI_IO_PROTOCOL  *This
  )
{
  EFI_STATUS    Status;
  PCI_IO_DEVICE *PciIoDevice;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  Status = PciIoDevice->PciRootBridgeIo->Flush (
                                           PciIoDevice->PciRootBridgeIo
                                           );
  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
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
EFI_STATUS
EFIAPI
PciIoGetLocation (
  IN  EFI_PCI_IO_PROTOCOL  *This,
  OUT UINTN                *Segment,
  OUT UINTN                *Bus,
  OUT UINTN                *Device,
  OUT UINTN                *Function
  )
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

/**
  Check BAR type for PCI resource.

  @param PciIoDevice   PCI device instance.
  @param BarIndex      The BAR index of the standard PCI Configuration header to use as the
                       base address for the memory or I/O operation to perform.
  @param BarType       Memory or I/O.

  @retval TRUE         Pci device's bar type is same with input BarType.
  @retval TRUE         Pci device's bar type is not same with input BarType.

**/
BOOLEAN
CheckBarType (
  IN PCI_IO_DEVICE          *PciIoDevice,
  IN UINT8                  BarIndex,
  IN PCI_BAR_TYPE           BarType
  )
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

/**
  Set/Disable new attributes to a Root Bridge.

  @param  PciIoDevice  Pci device instance.
  @param  Attributes   New attribute want to be set.
  @param  Operation    Set or Disable.

  @retval  EFI_UNSUPPORTED  If root bridge does not support change attribute.
  @retval  EFI_SUCCESS      Successfully set new attributs.

**/
EFI_STATUS
ModifyRootBridgeAttributes (
  IN  PCI_IO_DEVICE                            *PciIoDevice,
  IN  UINT64                                   Attributes,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
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
  // Mask off attributes not supported by PCI root bridge.
  //
  Attributes &= ~(UINT64)(EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE |
                          EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM |
                          EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE);

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
  if ((NewPciRootBridgeAttributes ^ PciRootBridgeAttributes) != 0) {

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

/**
  Check whether this device can be enable/disable to snoop.

  @param PciIoDevice  Pci device instance.
  @param Operation    Enable/Disable.

  @retval EFI_UNSUPPORTED  Pci device is not GFX device or not support snoop.
  @retval EFI_SUCCESS      Snoop can be supported.

**/
EFI_STATUS
SupportPaletteSnoopAttributes (
  IN PCI_IO_DEVICE                            *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation
  )
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

  if (Temp == NULL) {
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

    PCI_READ_COMMAND_REGISTER (Temp, &VGACommand);

    //
    // If they are on the same bus, either one can
    // be set to snoop, the other set to decode
    //
    if ((VGACommand & EFI_PCI_COMMAND_VGA_PALETTE_SNOOP) != 0) {
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
      PCI_ENABLE_COMMAND_REGISTER (Temp, EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
      Temp->Attributes |= EFI_PCI_COMMAND_VGA_PALETTE_SNOOP;
    } else {
      return EFI_UNSUPPORTED;
    }

  } else {
    //
    // GFX should be set to snoop
    //
    if (Operation == EfiPciIoAttributeOperationEnable) {
      PCI_DISABLE_COMMAND_REGISTER (Temp, EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
      Temp->Attributes &= (~(UINT64)EFI_PCI_COMMAND_VGA_PALETTE_SNOOP);
    } else {
      return EFI_UNSUPPORTED;
    }

  }

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
EFI_STATUS
EFIAPI
PciIoAttributes (
  IN EFI_PCI_IO_PROTOCOL                       * This,
  IN  EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN  UINT64                                   Attributes,
  OUT UINT64                                   *Result OPTIONAL
  )
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
  // EFI_PCI_DEVICE_ENABLE is not defined in UEFI spec, which is the internal usage.
  // So, this logic doesn't confrom to UEFI spec, which should be removed.
  // But this trick logic is still kept for some binary drivers that depend on it.
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
  // Check VGA and VGA16, they can not be set at the same time
  //
  if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO)) != 0) {
    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) != 0) {
      return EFI_UNSUPPORTED;
    }
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
  if (PciIoDevice->Parent == NULL) {
    Status = ModifyRootBridgeAttributes (PciIoDevice, Attributes, Operation);
    return Status;
  }

  Command       = 0;
  BridgeControl = 0;

  //
  // For PPB & P2C, set relevant attribute bits
  //
  if (IS_PCI_BRIDGE (&PciIoDevice->Pci) || IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_IO | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) != 0) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_VGA;
    }

    if ((Attributes & EFI_PCI_IO_ATTRIBUTE_ISA_IO) != 0) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_ISA;
    }

    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) != 0) {
      Command |= EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO;
    }

    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16 | EFI_PCI_IO_ATTRIBUTE_VGA_IO_16)) != 0) {
      BridgeControl |= EFI_PCI_BRIDGE_CONTROL_VGA_16;
    }

  } else {
    //
    // Do with the attributes on VGA
    // Only for VGA's legacy resource, we just can enable once.
    //
    if ((Attributes &
        (EFI_PCI_IO_ATTRIBUTE_VGA_IO    |
         EFI_PCI_IO_ATTRIBUTE_VGA_IO_16 |
         EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY)) != 0) {
      //
      // Check if a VGA has been enabled before enabling a new one
      //
      if (Operation == EfiPciIoAttributeOperationEnable) {
        //
        // Check if there have been an active VGA device on the same segment
        //
        Temp = ActiveVGADeviceOnTheSameSegment (PciIoDevice);
        if (Temp != NULL && Temp != PciIoDevice) {
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
    if ((Attributes & (EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO | EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16)) != 0) {

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

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_IO) != 0) {
    Command |= EFI_PCI_COMMAND_IO_SPACE;
  }

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_MEMORY) != 0) {
    Command |= EFI_PCI_COMMAND_MEMORY_SPACE;
  }

  if ((Attributes & EFI_PCI_IO_ATTRIBUTE_BUS_MASTER) != 0) {
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
    Status = PCI_ENABLE_COMMAND_REGISTER (PciIoDevice, Command);
    if (BridgeControl != 0) {
      Status = PCI_ENABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, BridgeControl);
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
    Status = PCI_DISABLE_COMMAND_REGISTER (PciIoDevice, Command);
    if (BridgeControl != 0) {
      Status = PCI_DISABLE_BRIDGE_CONTROL_REGISTER (PciIoDevice, BridgeControl);
    }

    PciIoDevice->Attributes &= (~Attributes);
    Status = EFI_SUCCESS;

  }

  if (EFI_ERROR (Status)) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      EFI_IO_BUS_PCI | EFI_IOB_EC_CONTROLLER_ERROR,
      PciIoDevice->DevicePath
      );
  }

  return Status;
}

/**
  Retrieve the AddrTranslationOffset from RootBridgeIo for the
  specified range.

  @param RootBridgeIo    Root Bridge IO instance.
  @param AddrRangeMin    The base address of the MMIO.
  @param AddrLen         The length of the MMIO.

  @retval The AddrTranslationOffset from RootBridgeIo for the 
          specified range, or (UINT64) -1 if the range is not
          found in RootBridgeIo.
**/
UINT64
GetMmioAddressTranslationOffset (
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL   *RootBridgeIo,
  UINT64                            AddrRangeMin,
  UINT64                            AddrLen
  )
{
  EFI_STATUS                        Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Configuration;

  Status = RootBridgeIo->Configuration (
                           RootBridgeIo,
                           (VOID **) &Configuration
                           );
  if (EFI_ERROR (Status)) {
    return (UINT64) -1;
  }

  // According to UEFI 2.7, EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL::Configuration()
  // returns host address instead of device address, while AddrTranslationOffset
  // is not zero, and device address = host address + AddrTranslationOffset, so
  // we convert host address to device address for range compare.
  while (Configuration->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    if ((Configuration->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) &&
        (Configuration->AddrRangeMin + Configuration->AddrTranslationOffset <= AddrRangeMin) &&
        (Configuration->AddrRangeMin + Configuration->AddrLen + Configuration->AddrTranslationOffset >= AddrRangeMin + AddrLen)
        ) {
      return Configuration->AddrTranslationOffset;
    }
    Configuration++;
  }

  //
  // The resource occupied by BAR should be in the range reported by RootBridge.
  //
  ASSERT (FALSE);
  return (UINT64) -1;
}

/**
  Gets the attributes that this PCI controller supports setting on a BAR using
  SetBarAttributes(), and retrieves the list of resource descriptors for a BAR.

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  BarIndex              The BAR index of the standard PCI Configuration header to use as the
                                base address for resource range. The legal range for this field is 0..5.
  @param  Supports              A pointer to the mask of attributes that this PCI controller supports
                                setting for this BAR with SetBarAttributes().
  @param  Resources             A pointer to the resource descriptors that describe the current
                                configuration of this BAR of the PCI controller.

  @retval EFI_SUCCESS           If Supports is not NULL, then the attributes that the PCI
                                controller supports are returned in Supports. If Resources
                                is not NULL, then the resource descriptors that the PCI
                                controller is currently using are returned in Resources.
  @retval EFI_INVALID_PARAMETER Both Supports and Attributes are NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to allocate
                                Resources.

**/
EFI_STATUS
EFIAPI
PciIoGetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL             * This,
  IN  UINT8                          BarIndex,
  OUT UINT64                         *Supports, OPTIONAL
  OUT VOID                           **Resources OPTIONAL
  )
{
  PCI_IO_DEVICE                     *PciIoDevice;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  EFI_ACPI_END_TAG_DESCRIPTOR       *End;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_IO_THIS (This);

  if (Supports == NULL && Resources == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BarIndex >= PCI_MAX_BAR) || (PciIoDevice->PciBar[BarIndex].BarType == PciBarTypeUnknown)) {
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
    Descriptor = AllocateZeroPool (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
    if (Descriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Resources   = Descriptor;

    Descriptor->Desc         = ACPI_ADDRESS_SPACE_DESCRIPTOR;
    Descriptor->Len          = (UINT16) (sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) - 3);
    Descriptor->AddrRangeMin = PciIoDevice->PciBar[BarIndex].BaseAddress;
    Descriptor->AddrLen      = PciIoDevice->PciBar[BarIndex].Length;
    Descriptor->AddrRangeMax = PciIoDevice->PciBar[BarIndex].Alignment;

    switch (PciIoDevice->PciBar[BarIndex].BarType) {
    case PciBarTypeIo16:
    case PciBarTypeIo32:
      //
      // Io
      //
      Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_IO;
      break;

    case PciBarTypePMem32:
      //
      // prefechable
      //
      Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      //
      // Fall through
      //
    case PciBarTypeMem32:
      //
      // Mem
      //
      Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 32 bit
      //
      Descriptor->AddrSpaceGranularity = 32;
      break;

    case PciBarTypePMem64:
      //
      // prefechable
      //
      Descriptor->SpecificFlag = EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE;
      //
      // Fall through
      //
    case PciBarTypeMem64:
      //
      // Mem
      //
      Descriptor->ResType = ACPI_ADDRESS_SPACE_TYPE_MEM;
      //
      // 64 bit
      //
      Descriptor->AddrSpaceGranularity = 64;
      break;

    default:
      break;
    }

    //
    // put the checksum
    //
    End           = (EFI_ACPI_END_TAG_DESCRIPTOR *) (Descriptor + 1);
    End->Desc     = ACPI_END_TAG_DESCRIPTOR;
    End->Checksum = 0;

    //
    // Get the Address Translation Offset
    //
    if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
      Descriptor->AddrTranslationOffset = GetMmioAddressTranslationOffset (
                                            PciIoDevice->PciRootBridgeIo,
                                            Descriptor->AddrRangeMin,
                                            Descriptor->AddrLen
                                            );
      if (Descriptor->AddrTranslationOffset == (UINT64) -1) {
        FreePool (Descriptor);
        return EFI_UNSUPPORTED;
      }
    }

    // According to UEFI spec 2.7, we need return host address for
    // PciIo->GetBarAttributes, and host address = device address - translation.
    Descriptor->AddrRangeMin -= Descriptor->AddrTranslationOffset;
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

  @retval EFI_SUCCESS           The set of attributes specified by Attributes for the resource
                                range specified by BarIndex, Offset, and Length were
                                set on the PCI controller, and the actual resource range is returned
                                in Offset and Length.
  @retval EFI_INVALID_PARAMETER Offset or Length is NULL.
  @retval EFI_UNSUPPORTED       BarIndex not valid for this PCI controller.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to set the attributes on the
                                resource range specified by BarIndex, Offset, and
                                Length.

**/
EFI_STATUS
EFIAPI
PciIoSetBarAttributes (
  IN EFI_PCI_IO_PROTOCOL              *This,
  IN     UINT64                       Attributes,
  IN     UINT8                        BarIndex,
  IN OUT UINT64                       *Offset,
  IN OUT UINT64                       *Length
  )
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

/**
  Program parent bridge's attribute recurrently.

  @param PciIoDevice  Child Pci device instance
  @param Operation    The operation to perform on the attributes for this PCI controller.
  @param Attributes   The mask of attributes that are used for Set, Enable, and Disable
                      operations.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       one or more of the bits set in
                                Attributes are not supported by this PCI controller or one of
                                its parent bridges when Operation is Set, Enable or Disable.

**/
EFI_STATUS
UpStreamBridgesAttributes (
  IN PCI_IO_DEVICE                            *PciIoDevice,
  IN EFI_PCI_IO_PROTOCOL_ATTRIBUTE_OPERATION  Operation,
  IN UINT64                                   Attributes
  )
{
  PCI_IO_DEVICE       *Parent;
  EFI_PCI_IO_PROTOCOL *PciIo;

  Parent = PciIoDevice->Parent;

  while (Parent != NULL && IS_PCI_BRIDGE (&Parent->Pci)) {

    //
    // Get the PciIo Protocol
    //
    PciIo = &Parent->PciIo;

    PciIo->Attributes (PciIo, Operation, Attributes, NULL);

    Parent = Parent->Parent;
  }

  return EFI_SUCCESS;
}

/**
  Test whether two Pci devices has same parent bridge.

  @param PciDevice1  The first pci device for testing.
  @param PciDevice2  The second pci device for testing.

  @retval TRUE       Two Pci device has the same parent bridge.
  @retval FALSE      Two Pci device has not the same parent bridge.

**/
BOOLEAN
PciDevicesOnTheSamePath (
  IN PCI_IO_DEVICE        *PciDevice1,
  IN PCI_IO_DEVICE        *PciDevice2
  )
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

