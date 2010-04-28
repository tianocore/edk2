/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SystemNvDataGuid.c
    
Abstract:

  GUIDs used for System Non Volatile HOB entries in the in the HOB list and FV Guids carrying
  the System specific information.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(SystemNvDataGuid)


EFI_GUID gEfiSystemNvDataHobGuid  = EFI_SYSTEM_NV_DATA_HOB_GUID;
EFI_GUID gEfiSystemNvDataFvGuid  = EFI_SYSTEM_NV_DATA_FV_GUID;

EFI_GUID_STRING(&gEfiSystemNvDataHobGuid, "SYSTEM NV DATA HOB", "SYSTEM NV DATA HOB GUID for HOB list.");
EFI_GUID_STRING(&gEfiSystemNvDataFvGuid, "SYSTEM NV DATA FV", "SYSTEM NV DATA FV GUID");

