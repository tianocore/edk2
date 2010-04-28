/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    SmBios.c
    
Abstract:

  GUIDs used to locate the SMBIOS tables in the EFI 1.0 system table.

  This GUID in the system table is the only legal way to search for and 
  locate the SMBIOS tables. Do not search the 0xF0000 segment to find SMBIOS
  tables.

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (SmBios)

EFI_GUID  gEfiSmbiosTableGuid = EFI_SMBIOS_TABLE_GUID;

EFI_GUID_STRING(&gEfiSmbiosTableGuid, "SMBIOS Table", "SMBIOS Table GUID in EFI System Table");
