/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    PeiFlushInstructionCache.c
    
Abstract:

  GUID for the Instruction Cache Flushing APIs shared between PEI and DXE

--*/

#include "Tiano.h"
#include EFI_GUID_DEFINITION(PeiFlushInstructionCache)

EFI_GUID gEfiPeiFlushInstructionCacheGuid  = EFI_PEI_FLUSH_INSTRUCTION_CACHE_GUID;

EFI_GUID_STRING(&gEfiPeiFlushInstructionCacheGuid, "PEI Flush Instruction Cache", 
                "Flush Instruction Cache APIs from PEI");

