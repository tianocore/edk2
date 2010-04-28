/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciHostBridgeResourceAllocation.c

Abstract:


--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (PciHostBridgeResourceAllocation)

EFI_GUID  gEfiPciHostBridgeResourceAllocationProtocolGuid = EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL_GUID;

EFI_GUID_STRING
  (
    &gEfiPciHostBridgeResourceAllocationProtocolGuid, "PciHostBridgeRresourceAllocation Protocol",
      "Pci Host Bridge Resource Allocation Protocol"
  );
