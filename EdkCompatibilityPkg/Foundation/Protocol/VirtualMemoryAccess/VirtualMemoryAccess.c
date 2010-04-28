/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  VirtualMemoryAccess.c

Abstract:

  
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (VirtualMemoryAccess)

EFI_GUID  gEfiVirtualMemoryAccessProtocolGuid = EFI_VIRTUAL_MEMORY_ACCESS_PROTOCOL_GUID;

EFI_GUID_STRING
  (&gEfiVirtualMemoryAccessProtocolGuid, "Virtual Memory Access Protocol", "Tiano Virtual Memory Access Protocol");
