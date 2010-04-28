/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    GlobalVariable.c
    
Abstract:

  GUID for EFI (NVRAM) Variables. Defined in EFI 1.0.

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (GlobalVariable)

EFI_GUID  gEfiGlobalVariableGuid = EFI_GLOBAL_VARIABLE_GUID;

EFI_GUID_STRING(&gEfiGlobalVariableGuid, "Efi", "Efi Variable GUID")
