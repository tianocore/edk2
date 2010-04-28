/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciRootBridgeIo.c

Abstract:

  PCI Root Bridge I/O protocol as defined in the EFI 1.1 specification.

  PCI Root Bridge I/O protocol is used by PCI Bus Driver to perform PCI Memory, PCI I/O, 
  and PCI Configuration cycles on a PCI Root Bridge. It also provides services to perform 
  defferent types of bus mastering DMA

--*/

#include "EfiSpec.h"

#include EFI_PROTOCOL_DEFINITION (PciRootBridgeIo)

EFI_GUID  gEfiPciRootBridgeIoProtocolGuid = EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPciRootBridgeIoProtocolGuid, "PciRootBridgeIo Protocol", "EFI 1.1 Pci Root Bridge IO Protocol");
