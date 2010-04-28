/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MemoryTypeInformation.c
    
Abstract:

  GUID used for Memory Type Information entries in the HOB list.

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(MemoryTypeInformation)

EFI_GUID gEfiMemoryTypeInformationGuid  = EFI_MEMORY_TYPE_INFORMATION_GUID;

EFI_GUID_STRING(&gEfiMemoryTypeInformationGuid, "Memory Type Information", 
                "Memory Type Information HOB GUID for HOB list.");

