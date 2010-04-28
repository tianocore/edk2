/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciRomTable.h
  
Abstract:

  Option Rom Support for PCI Bus Driver

Revision History

--*/

#ifndef _EFI_PCI_ROM_TABLE_H
#define _EFI_PCI_ROM_TABLE_H


EFI_STATUS
PciRomLoadEfiDriversFromOptionRomTable (
  IN EFI_DRIVER_BINDING_PROTOCOL      *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo
  );

EFI_STATUS
PciRomGetRomResourceFromPciOptionRomTable (
  IN EFI_DRIVER_BINDING_PROTOCOL      *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  PCI_IO_DEVICE                       *PciIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This            - TODO: add argument description
  PciRootBridgeIo - TODO: add argument description
  PciIoDevice     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;
#endif
