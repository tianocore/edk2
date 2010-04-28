/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    IncompatiblePciDeviceSupport.c
    
Abstract:

    EFI Incompatible PCI Device Support Protocol

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (IncompatiblePciDeviceSupport)

EFI_GUID  gEfiIncompatiblePciDeviceSupportProtocolGuid = EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL_GUID;

EFI_GUID_STRING
  (
    &gEfiIncompatiblePciDeviceSupportProtocolGuid, "Incompatible PCI Device Support Protocol",
      "Tiano Incompatible PCI Device Support Protocol"
  );
