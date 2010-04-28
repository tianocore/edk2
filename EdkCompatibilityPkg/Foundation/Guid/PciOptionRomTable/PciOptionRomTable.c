/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciOptionRomTable.c
    
Abstract:

  GUID and data structure used to describe the list of PCI Option ROMs present in a system.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PciOptionRomTable)

EFI_GUID gEfiPciOptionRomTableGuid = EFI_PCI_OPTION_ROM_TABLE_GUID;

EFI_GUID_STRING(&gEfiPciOptionRomTableGuid, "PCI Option ROM Table", "PCI Option ROM Table GUID in EFI System Table");
