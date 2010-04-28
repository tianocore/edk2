/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugImageInfoTable.c
    
Abstract:

  GUID used to locate the Debug Image Info table in the EFI 1.0 system table.

--*/

#include "EfiSpec.h"
#include EFI_GUID_DEFINITION (DebugImageInfoTable)

EFI_GUID  gEfiDebugImageInfoTableGuid = EFI_DEBUG_IMAGE_INFO_TABLE_GUID;

EFI_GUID_STRING
  (&gEfiDebugImageInfoTableGuid, "Debug Image Info Table", "Debug Image Info Table GUID in EFI System Table");
