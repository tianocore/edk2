/** @file
  PCI command register operations supporting functions implementation for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"

/**
  Operate the PCI register via PciIo function interface.

  @param PciIoDevice    Pointer to instance of PCI_IO_DEVICE.
  @param Command        Operator command.
  @param Offset         The address within the PCI configuration space for the PCI controller.
  @param Operation      Type of Operation.
  @param PtrCommand     Return buffer holding old PCI command, if operation is not EFI_SET_REGISTER.

  @return Status of PciIo operation.

**/
EFI_STATUS
PciOperateRegister (
  IN  PCI_IO_DEVICE *PciIoDevice,
  IN  UINT16        Command,
  IN  UINT8         Offset,
  IN  UINT8         Operation,
  OUT UINT16        *PtrCommand
  )
{
  UINT16              OldCommand;
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;

  OldCommand  = 0;
  PciIo       = &PciIoDevice->PciIo;

  if (Operation != EFI_SET_REGISTER) {
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint16,
                          Offset,
                          1,
                          &OldCommand
                          );

    if (Operation == EFI_GET_REGISTER) {
      *PtrCommand = OldCommand;
      return Status;
    }
  }

  if (Operation == EFI_ENABLE_REGISTER) {
    OldCommand = (UINT16) (OldCommand | Command);
  } else if (Operation == EFI_DISABLE_REGISTER) {
    OldCommand = (UINT16) (OldCommand & ~(Command));
  } else {
    OldCommand = Command;
  }

  return PciIo->Pci.Write (
                      PciIo,
                      EfiPciIoWidthUint16,
                      Offset,
                      1,
                      &OldCommand
                      );
}

/**
  Check the capability supporting by given device.

  @param PciIoDevice   Pointer to instance of PCI_IO_DEVICE.

  @retval TRUE         Capability supported.
  @retval FALSE        Capability not supported.

**/
BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  if ((PciIoDevice->Pci.Hdr.Status & EFI_PCI_STATUS_CAPABILITY) != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Locate capability register block per capability ID.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param CapId             The capability ID.
  @param Offset            A pointer to the offset returned.
  @param NextRegBlock      A pointer to the next block returned.

  @retval EFI_SUCCESS      Successfully located capability register block.
  @retval EFI_UNSUPPORTED  Pci device does not support capability.
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.

**/
EFI_STATUS
LocateCapabilityRegBlock (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          CapId,
  IN OUT UINT8      *Offset,
  OUT UINT8         *NextRegBlock OPTIONAL
  )
{
  UINT8   CapabilityPtr;
  UINT16  CapabilityEntry;
  UINT8   CapabilityID;

  //
  // To check the capability of this device supports
  //
  if (!PciCapabilitySupport (PciIoDevice)) {
    return EFI_UNSUPPORTED;
  }

  if (*Offset != 0) {
    CapabilityPtr = *Offset;
  } else {

    CapabilityPtr = 0;
    if (IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

      PciIoDevice->PciIo.Pci.Read (
                               &PciIoDevice->PciIo,
                               EfiPciIoWidthUint8,
                               EFI_PCI_CARDBUS_BRIDGE_CAPABILITY_PTR,
                               1,
                               &CapabilityPtr
                               );
    } else {

      PciIoDevice->PciIo.Pci.Read (
                               &PciIoDevice->PciIo,
                               EfiPciIoWidthUint8,
                               PCI_CAPBILITY_POINTER_OFFSET,
                               1,
                               &CapabilityPtr
                               );
    }
  }

  while ((CapabilityPtr >= 0x40) && ((CapabilityPtr & 0x03) == 0x00)) {
    PciIoDevice->PciIo.Pci.Read (
                             &PciIoDevice->PciIo,
                             EfiPciIoWidthUint16,
                             CapabilityPtr,
                             1,
                             &CapabilityEntry
                             );

    CapabilityID = (UINT8) CapabilityEntry;

    if (CapabilityID == CapId) {
      *Offset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (UINT8) (CapabilityEntry >> 8);
      }

      return EFI_SUCCESS;
    }

    //
    // Certain PCI device may incorrectly have capability pointing to itself,
    // break to avoid dead loop.
    //
    if (CapabilityPtr == (UINT8) (CapabilityEntry >> 8)) {
      break;
    }

    CapabilityPtr = (UINT8) (CapabilityEntry >> 8);
  }

  return EFI_NOT_FOUND;
}

/**
  Locate PciExpress capability register block per capability ID.

  @param PciIoDevice       A pointer to the PCI_IO_DEVICE.
  @param CapId             The capability ID.
  @param Offset            A pointer to the offset returned.
  @param NextRegBlock      A pointer to the next block returned.

  @retval EFI_SUCCESS      Successfully located capability register block.
  @retval EFI_UNSUPPORTED  Pci device does not support capability.
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.

**/
EFI_STATUS
LocatePciExpressCapabilityRegBlock (
  IN     PCI_IO_DEVICE *PciIoDevice,
  IN     UINT16        CapId,
  IN OUT UINT32        *Offset,
     OUT UINT32        *NextRegBlock OPTIONAL
  )
{
  EFI_STATUS           Status;
  UINT32               CapabilityPtr;
  UINT32               CapabilityEntry;
  UINT16               CapabilityID;

  //
  // To check the capability of this device supports
  //
  if (!PciIoDevice->IsPciExp) {
    return EFI_UNSUPPORTED;
  }

  if (*Offset != 0) {
    CapabilityPtr = *Offset;
  } else {
    CapabilityPtr = EFI_PCIE_CAPABILITY_BASE_OFFSET;
  }

  while (CapabilityPtr != 0) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFFC;
    Status = PciIoDevice->PciIo.Pci.Read (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint32,
                                      CapabilityPtr,
                                      1,
                                      &CapabilityEntry
                                      );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (CapabilityEntry == MAX_UINT32) {
      DEBUG ((
        DEBUG_WARN,
        "%a: [%02x|%02x|%02x] failed to access config space at offset 0x%x\n",
        __FUNCTION__,
        PciIoDevice->BusNumber,
        PciIoDevice->DeviceNumber,
        PciIoDevice->FunctionNumber,
        CapabilityPtr
        ));
      break;
    }

    CapabilityID = (UINT16) CapabilityEntry;

    if (CapabilityID == CapId) {
      *Offset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (CapabilityEntry >> 20) & 0xFFF;
      }

      return EFI_SUCCESS;
    }

    CapabilityPtr = (CapabilityEntry >> 20) & 0xFFF;
  }

  return EFI_NOT_FOUND;
}
