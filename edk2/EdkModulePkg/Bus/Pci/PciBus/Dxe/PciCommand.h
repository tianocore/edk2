/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PciCommand.h
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_COMMAND_H
#define _EFI_PCI_COMMAND_H

//
// The PCI Command register bits owned by PCI Bus driver.
//
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_COMMAND_BITS_OWNED                          ( \
                EFI_PCI_COMMAND_IO_SPACE                    | \
                EFI_PCI_COMMAND_MEMORY_SPACE                | \
                EFI_PCI_COMMAND_BUS_MASTER                  | \
                EFI_PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE | \
                EFI_PCI_COMMAND_VGA_PALETTE_SNOOP           | \
                EFI_PCI_COMMAND_FAST_BACK_TO_BACK             \
                )

//
// The PCI Bridge Control register bits owned by PCI Bus driver.
// 
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCI_BRIDGE_CONTROL_BITS_OWNED                   ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA_16               | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )

//
// The PCCard Bridge Control register bits owned by PCI Bus driver.
// 
// They should be cleared at the beginning. The other registers
// are owned by chipset, we should not touch them.
//
#define EFI_PCCARD_BRIDGE_CONTROL_BITS_OWNED                ( \
                EFI_PCI_BRIDGE_CONTROL_ISA                  | \
                EFI_PCI_BRIDGE_CONTROL_VGA                  | \
                EFI_PCI_BRIDGE_CONTROL_FAST_BACK_TO_BACK      \
                )


#define EFI_GET_REGISTER      1
#define EFI_SET_REGISTER      2
#define EFI_ENABLE_REGISTER   3
#define EFI_DISABLE_REGISTER  4

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

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description
  Command     - TODO: add argument description
  Offset      - TODO: add argument description
  Operation   - TODO: add argument description
  PtrCommand  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
PciCapabilitySupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
LocateCapabilityRegBlock (
  IN PCI_IO_DEVICE  *PciIoDevice,
  IN UINT8          CapId,
  IN OUT UINT8      *Offset,
  OUT UINT8         *NextRegBlock OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIoDevice   - TODO: add argument description
  CapId         - TODO: add argument description
  Offset        - TODO: add argument description
  NextRegBlock  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;


#define PciReadCommandRegister(a,b) \
        PciOperateRegister (a,0, PCI_COMMAND_OFFSET, EFI_GET_REGISTER, b)

#define PciSetCommandRegister(a,b) \
        PciOperateRegister (a,b, PCI_COMMAND_OFFSET, EFI_SET_REGISTER, NULL)
        
#define PciEnableCommandRegister(a,b) \
        PciOperateRegister (a,b, PCI_COMMAND_OFFSET, EFI_ENABLE_REGISTER, NULL)
        
#define PciDisableCommandRegister(a,b) \
        PciOperateRegister (a,b, PCI_COMMAND_OFFSET, EFI_DISABLE_REGISTER, NULL)

#define PciReadBridgeControlRegister(a,b) \
        PciOperateRegister (a,0, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_GET_REGISTER, b)
        
#define PciSetBridgeControlRegister(a,b) \
        PciOperateRegister (a,b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_SET_REGISTER, NULL)

#define PciEnableBridgeControlRegister(a,b) \
        PciOperateRegister (a,b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_ENABLE_REGISTER, NULL)
        
#define PciDisableBridgeControlRegister(a,b) \
        PciOperateRegister (a,b, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_DISABLE_REGISTER, NULL)

#endif
