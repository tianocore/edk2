/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MemoryDiscovered.h
    
Abstract:

  Memory Discovered PPI as defined in Tiano

--*/

#ifndef _PEI_MEMORY_DISCOVERED_PPI_H
#define _PEI_MEMORY_DISCOVERED_PPI_H

#define PEI_PERMANENT_MEMORY_INSTALLED_PPI_GUID \
  { \
    0xf894643d, 0xc449, 0x42d1, {0x8e, 0xa8, 0x85, 0xbd, 0xd8, 0xc6, 0x5b, 0xde} \
  }

EFI_FORWARD_DECLARATION (PEI_PERMANENT_MEMORY_INSTALLED_PPI);

extern EFI_GUID gPeiMemoryDiscoveredPpiGuid;

#endif
