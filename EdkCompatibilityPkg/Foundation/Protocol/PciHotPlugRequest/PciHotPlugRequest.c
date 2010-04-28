/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciHotPlugRequest.c

Abstract:

  
 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (PciHotPlugRequest)

EFI_GUID  gEfiPciHotPlugRequestProtocolGuid = EFI_PCI_HOTPLUG_REQUEST_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiPciHotPlugRequestProtocolGuid, "Pci Hot Plug Request Protocol", "Tiano Hot Plug Request Protocol");
