/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    GenericVariable.c
    
Abstract:

    The variable space Guid to pair with a Unicode string name to tag an EFI variable

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (GenericVariable)

EFI_GUID  gEfiGenericVariableGuid = EFI_GENERIC_VARIABLE_GUID;

EFI_GUID_STRING(&gEfiGenericVariableGuid, "GenericVariable", "Generic Variable GUID");
