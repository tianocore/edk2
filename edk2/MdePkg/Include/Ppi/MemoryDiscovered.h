/** @file
  This file declares Memory Discovered PPI.
  This PPI is installed by the PEI Foundation at the point of system 
  evolution when the permanent memory size has been registered and 
  waiting PEIMs can use the main memory store. 

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PEI CIS
  Version 0.91.

**/

#ifndef __PEI_MEMORY_DISCOVERED_PPI_H__
#define __PEI_MEMORY_DISCOVERED_PPI_H__

#define EFI_PEI_PERMANENT_MEMORY_INSTALLED_PPI_GUID \
  { \
    0xf894643d, 0xc449, 0x42d1, {0x8e, 0xa8, 0x85, 0xbd, 0xd8, 0xc6, 0x5b, 0xde } \
  }

extern EFI_GUID gEfiPeiMemoryDiscoveredPpiGuid;

#endif
