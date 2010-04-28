/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeServices.c
    
Abstract:

  GUID used for the DXE Services Table

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION (DxeServices)

EFI_GUID  gEfiDxeServicesTableGuid = EFI_DXE_SERVICES_TABLE_GUID;

EFI_GUID_STRING(&gEfiDxeServicesTableGuid, "DXE Services Table", "DXE Services Table GUID in EFI System Table");
