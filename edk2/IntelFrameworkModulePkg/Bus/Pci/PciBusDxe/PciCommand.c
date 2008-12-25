/** @file
  This module implement Pci register operation interface for 
  Pci device.
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include "PciBus.h"

/**
  Operate the PCI register via PciIo function interface.
  
  @param PciIoDevice    Pointer to instance of PCI_IO_DEVICE
  @param Command        Operator command
  @param Offset         The address within the PCI configuration space for the PCI controller.
  @param Operation      Type of Operation
  @param PtrCommand     Return buffer holding old PCI command, if operation is not EFI_SET_REGISTER
  
  @return status of PciIo operation
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

/**
  check the cpability of this device supports
  
  @param PciIoDevice  Pointer to instance of PCI_IO_DEVICE
  
  @retval TRUE  Support
  @retval FALSE Not support.
**/
BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{

  if (PciIoDevice->Pci.Hdr.Status & EFI_PCI_STATUS_CAPABILITY) {
    return TRUE;
  }

  return FALSE;
}

/**
  Locate cap reg.
  
  @param PciIoDevice         - A pointer to the PCI_IO_DEVICE.
  @param CapId               - The cap ID.
  @param Offset              - A pointer to the offset.
  @param NextRegBlock        - A pointer to the next block.
  
  @retval EFI_UNSUPPORTED  Pci device does not support
  @retval EFI_NOT_FOUND    Pci device support but can not find register block.
  @retval EFI_SUCCESS      Success to locate capability register block
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
                  PCI_CAPBILITY_POINTER_OFFSET,
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

