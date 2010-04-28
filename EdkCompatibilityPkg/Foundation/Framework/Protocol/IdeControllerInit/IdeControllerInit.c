/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    IdeControllerInit.c
    
Abstract:

    EFI IDE Controller Init Protocol

Revision History

--*/

#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (IdeControllerInit)

EFI_GUID  gEfiIdeControllerInitProtocolGuid = EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiIdeControllerInitProtocolGuid, "IDE Controller Init Protocol", "Platform IDE Init Protocol");
