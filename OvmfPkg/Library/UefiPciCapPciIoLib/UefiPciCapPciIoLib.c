/** @file
  Plug an EFI_PCI_IO_PROTOCOL backend into PciCapLib, for config space access.

  Copyright (C) 2018, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemoryAllocationLib.h>

#include "UefiPciCapPciIoLib.h"

/**
  Transfer bytes between the config space of a given PCI device and a memory
  buffer.

  ProtoDevTransferConfig() performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  @param[in] PciIo             The EFI_PCI_IO_PROTOCOL representation of the
                               PCI device.

  @param[in] TransferFunction  The EFI_PCI_IO_PROTOCOL_CONFIG function that
                               implements the transfer. The direction of the
                               transfer is inherent to TransferFunction.
                               TransferFunction() is required to return an
                               unspecified error if any sub-transfer within
                               Size bytes from ConfigOffset exceeds the config
                               space limit of the PCI device.

  @param[in] ConfigOffset      The offset in the config space of the PCI device
                               at which the transfer should commence.

  @param[in,out] Buffer        The memory buffer where the transfer should
                               occur.

  @param[in] Size              The number of bytes to transfer.

  @retval EFI_SUCCESS  Size bytes have been transferred between config space
                       and Buffer.

  @return              Error codes propagated from TransferFunction(). Fewer
                       than Size bytes may have been transferred.
**/
STATIC
EFI_STATUS
ProtoDevTransferConfig (
  IN     EFI_PCI_IO_PROTOCOL         *PciIo,
  IN     EFI_PCI_IO_PROTOCOL_CONFIG  TransferFunction,
  IN     UINT16                      ConfigOffset,
  IN OUT UINT8                       *Buffer,
  IN     UINT16                      Size
  )
{
  while (Size > 0) {
    EFI_PCI_IO_PROTOCOL_WIDTH  Width;
    UINT16                     Count;
    EFI_STATUS                 Status;
    UINT16                     Progress;

    //
    // Pick the largest access size that is allowed by the remaining transfer
    // Size and by the alignment of ConfigOffset.
    //
    // When the largest access size is available, transfer as many bytes as
    // possible in one iteration of the loop. Otherwise, transfer only one
    // unit, to improve the alignment.
    //
    if ((Size >= 4) && ((ConfigOffset & 3) == 0)) {
      Width = EfiPciIoWidthUint32;
      Count = Size >> Width;
    } else if ((Size >= 2) && ((ConfigOffset & 1) == 0)) {
      Width = EfiPciIoWidthUint16;
      Count = 1;
    } else {
      Width = EfiPciIoWidthUint8;
      Count = 1;
    }

    Status = TransferFunction (PciIo, Width, ConfigOffset, Count, Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Progress      = Count << Width;
    ConfigOffset += Progress;
    Buffer       += Progress;
    Size         -= Progress;
  }

  return EFI_SUCCESS;
}

/**
  Read the config space of a given PCI device (both normal and extended).

  ProtoDevReadConfig() performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  ProtoDevReadConfig() returns an unspecified error if accessing Size bytes
  from SourceOffset exceeds the config space limit of the PCI device. Fewer
  than Size bytes may have been read in this case.

  @param[in] PciDevice           Implementation-specific unique representation
                                 of the PCI device in the PCI hierarchy.

  @param[in] SourceOffset        Source offset in the config space of the PCI
                                 device to start reading from.

  @param[out] DestinationBuffer  Buffer to store the read data to.

  @param[in] Size                The number of bytes to transfer.

  @retval RETURN_SUCCESS  Size bytes have been transferred from config space to
                          DestinationBuffer.

  @return                 Error codes propagated from
                          EFI_PCI_IO_PROTOCOL.Pci.Read(). Fewer than Size bytes
                          may have been read.
**/
STATIC
RETURN_STATUS
EFIAPI
ProtoDevReadConfig (
  IN  PCI_CAP_DEV  *PciDevice,
  IN  UINT16       SourceOffset,
  OUT VOID         *DestinationBuffer,
  IN  UINT16       Size
  )
{
  PROTO_DEV  *ProtoDev;

  ProtoDev = PROTO_DEV_FROM_PCI_CAP_DEV (PciDevice);
  return ProtoDevTransferConfig (
           ProtoDev->PciIo,
           ProtoDev->PciIo->Pci.Read,
           SourceOffset,
           DestinationBuffer,
           Size
           );
}

/**
  Write the config space of a given PCI device (both normal and extended).

  ProtoDevWriteConfig() performs as few config space accesses as possible
  (without attempting 64-bit wide accesses).

  ProtoDevWriteConfig() returns an unspecified error if accessing Size bytes at
  DestinationOffset exceeds the config space limit of the PCI device. Fewer
  than Size bytes may have been written in this case.

  @param[in] PciDevice          Implementation-specific unique representation
                                of the PCI device in the PCI hierarchy.

  @param[in] DestinationOffset  Destination offset in the config space of the
                                PCI device to start writing at.

  @param[in] SourceBuffer       Buffer to read the data to be stored from.

  @param[in] Size               The number of bytes to transfer.

  @retval RETURN_SUCCESS  Size bytes have been transferred from SourceBuffer to
                          config space.

  @return                 Error codes propagated from
                          EFI_PCI_IO_PROTOCOL.Pci.Write(). Fewer than Size
                          bytes may have been written.
**/
STATIC
RETURN_STATUS
EFIAPI
ProtoDevWriteConfig (
  IN PCI_CAP_DEV  *PciDevice,
  IN UINT16       DestinationOffset,
  IN VOID         *SourceBuffer,
  IN UINT16       Size
  )
{
  PROTO_DEV  *ProtoDev;

  ProtoDev = PROTO_DEV_FROM_PCI_CAP_DEV (PciDevice);
  return ProtoDevTransferConfig (
           ProtoDev->PciIo,
           ProtoDev->PciIo->Pci.Write,
           DestinationOffset,
           SourceBuffer,
           Size
           );
}

/**
  Create a PCI_CAP_DEV object from an EFI_PCI_IO_PROTOCOL instance. The config
  space accessors are based upon EFI_PCI_IO_PROTOCOL.Pci.Read() and
  EFI_PCI_IO_PROTOCOL.Pci.Write().

  @param[in] PciIo       EFI_PCI_IO_PROTOCOL representation of the PCI device.

  @param[out] PciDevice  The PCI_CAP_DEV object constructed as described above.
                         PciDevice can be passed to the PciCapLib APIs.

  @retval EFI_SUCCESS           PciDevice has been constructed and output.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
PciCapPciIoDeviceInit (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  OUT PCI_CAP_DEV          **PciDevice
  )
{
  PROTO_DEV  *ProtoDev;

  ProtoDev = AllocatePool (sizeof *ProtoDev);
  if (ProtoDev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ProtoDev->Signature              = PROTO_DEV_SIG;
  ProtoDev->PciIo                  = PciIo;
  ProtoDev->BaseDevice.ReadConfig  = ProtoDevReadConfig;
  ProtoDev->BaseDevice.WriteConfig = ProtoDevWriteConfig;

  *PciDevice = &ProtoDev->BaseDevice;
  return EFI_SUCCESS;
}

/**
  Free the resources used by PciDevice.

  @param[in] PciDevice  The PCI_CAP_DEV object to free, originally produced by
                        PciCapPciIoDeviceInit().
**/
VOID
EFIAPI
PciCapPciIoDeviceUninit (
  IN PCI_CAP_DEV  *PciDevice
  )
{
  PROTO_DEV  *ProtoDev;

  ProtoDev = PROTO_DEV_FROM_PCI_CAP_DEV (PciDevice);
  FreePool (ProtoDev);
}
