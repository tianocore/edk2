/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ComponentName.c
    
Abstract:

    EFI Component Name Protocol

Revision History

--*/

#include "EfiSpec.h"
#include EFI_PROTOCOL_DEFINITION (ComponentName)

EFI_GUID  gEfiComponentNameProtocolGuid = EFI_COMPONENT_NAME_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiComponentNameProtocolGuid, "Component Name Protocol", "EFI 1.1 Component Name Protocol");
