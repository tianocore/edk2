/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiInMemory.h
    
Abstract:

  
--*/

#ifndef _PEI_IN_MEMORY_H
#define _PEI_IN_MEMORY_H
        
#define PEI_IN_MEMORY_GUID \
  {0x643b8786, 0xb417, 0x48d2, {0x8f, 0x5e, 0x78, 0x19, 0x93, 0x1c, 0xae, 0xd8}}
  
extern EFI_GUID gPeiInMemoryGuid;

#endif
