/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PciCommand.c

Abstract:

  PCI Bus Driver

Revision History

--*/

#include "pcibus.h"

EFI_STATUS
PciOperateRegister (
  IN  PCI_IO_DEVICE *PciIoDevice,
  IN  UINT16        Command,
  IN  UINT8         Offset,
  IN  UINT8         Operation,
  OUT UINT16        *PtrCommand
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
// TODO:    Command - add argument and description to function comment
// TODO:    Offset - add argument and description to function comment
// TODO:    Operation - add argument and description to function comment
// TODO:    PtrCommand - add argument and description to function comment
{
  UINT16              OldCommand;
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;

  OldCommand  = 0;
  PciIo       = &PciIoDevice->PciIo;

  if (Operation != EFI_SET_REGISTER) {
    Status = PciIoRead (
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

  return PciIoWrite (
                      PciIo,
                      EfiPciIoWidthUint16,
                      Offset,
                      1,
                      &OldCommand
                    );
}

BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    PciIoDevice - add argument and description to function comment
{

  if (PciIoDevice->Pci.Hdr.Status & EFI_PCI_STATUS_CAPABILITY) {
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
LocateCapabilityRegBlock (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          CapId,
  IN OUT UINT8      *Offset,
  OUT UINT8         *NextRegBlock OPTIONAL
  )
/*++

Routine Description:
  Locate cap reg.

Arguments:
  PciIoDevice         - A pointer to the PCI_IO_DEVICE.
  CapId               - The cap ID.
  Offset              - A pointer to the offset.
  NextRegBlock        - A pointer to the next block.

Returns:

  None

--*/
// TODO:    EFI_UNSUPPORTED - add return value to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
// TODO:    EFI_NOT_FOUND - add return value to function comment
{
  UINT8   CapabilityPtr;
  UINT16  CapabilityEntry;
  UINT8   CapabilityID;

  //
  // To check the cpability of this device supports
  //
  if (!PciCapabilitySupport (PciIoDevice)) {
    return EFI_UNSUPPORTED;
  }

  if (*Offset != 0) {
    CapabilityPtr = *Offset;
  } else {

    CapabilityPtr = 0;
    if (IS_CARDBUS_BRIDGE (&PciIoDevice->Pci)) {

      PciIoRead (
                  &PciIoDevice->PciIo,
                  EfiPciIoWidthUint8,
                  EFI_PCI_CARDBUS_BRIDGE_CAPABILITY_PTR,
                  1,
                  &CapabilityPtr
                );
    } else {

      PciIoRead (
                  &PciIoDevice->PciIo,
                  EfiPciIoWidthUint8,
                  EFI_PCI_CAPABILITY_PTR,
                  1,
                  &CapabilityPtr
                );
    }
  }

  while (CapabilityPtr > 0x3F) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFC;
    PciIoRead (
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

    CapabilityPtr = (UINT8) (CapabilityEntry >> 8);
  }

  return EFI_NOT_FOUND;
}
