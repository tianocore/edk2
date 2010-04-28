/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  BootState.c
    
Abstract:

  GUID for use conveying the boot-state to PEI

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (BootState)

EFI_GUID gEfiBootStateGuid = EFI_BOOT_STATE_VARIABLE_GUID;

//
// GUID for frequency selection HOB
//
EFI_GUID_STRING(&gEfiBootStateGuid, "Boot State", "Boot State");
