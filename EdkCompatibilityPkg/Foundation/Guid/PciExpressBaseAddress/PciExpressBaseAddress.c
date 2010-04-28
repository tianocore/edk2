/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PciExpressBaseAddress.c
    
Abstract:


   GUIDs used for PciExpress Base Address

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (PciExpressBaseAddress)

EFI_GUID  gEfiPciExpressBaseAddressGuid = EFI_PCI_EXPRESS_BASE_ADDRESS_GUID;

EFI_GUID_STRING(&gEfiPciExpressBaseAddressGuid, "PCI Express Base Address", "PCI Express Base Address GUID");
