/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    Hob.c
    
Abstract:

  GUIDs used for HOB List in the EFI 1.0 system table

  These GUIDs point the HOB List passed in from PEI to DXE.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (Hob)

EFI_GUID  gEfiHobListGuid = EFI_HOB_LIST_GUID;

EFI_GUID_STRING(&gEfiHobListGuid, "HOB List", "HOB List GUID in EFI System Table");
