/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    SalSystemTable.c
    
Abstract:

  GUIDs used for SAL system table entries in the in the EFI 1.0 system table.

  SAL System Table contains Itanium-based processor centric information about
  the system.

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (SalSystemTable)

EFI_GUID  gEfiSalSystemTableGuid = EFI_SAL_SYSTEM_TABLE_GUID;

EFI_GUID_STRING(&gEfiSalSystemTableGuid, "SAL System Table", "SAL System Table GUID in EFI System Table");
