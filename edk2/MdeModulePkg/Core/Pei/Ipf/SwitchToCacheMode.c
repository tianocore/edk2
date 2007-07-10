/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SwitchToCacheMode.c

Abstract:

  Ipf CAR specific function used to switch to cache mode for the later memory access

Revision History

--*/

#include "IpfPeiMain.h"
#include "IpfCpuCore.i"

VOID
SwitchToCacheMode (
  IN PEI_CORE_INSTANCE           *CoreData
  )
/*++

Routine Description:

 Switch the PHIT pointers to cache mode after InstallPeiMemory in CAR.

Arguments:

  CoreData   - The PEI core Private Data

Returns:

--*/
{
  EFI_HOB_HANDOFF_INFO_TABLE    *Phit;

  if (CoreData == NULL) {
    //
    // the first call with CoreData as NULL.
    //
    return;
  }
  
  if ((GetHandOffStatus().r10 & 0xFF) == RecoveryFn) {
    CoreData->StackBase = CoreData->StackBase &  CACHE_MODE_ADDRESS_MASK;
    CoreData->HobList.Raw = (UINT8 *)((UINTN)CoreData->HobList.Raw & CACHE_MODE_ADDRESS_MASK);

    //
    // Change the PHIT pointer value to cache mode
    //
    Phit = CoreData->HobList.HandoffInformationTable;

    Phit->EfiMemoryTop        = Phit->EfiMemoryTop & CACHE_MODE_ADDRESS_MASK;
    Phit->EfiFreeMemoryTop    = Phit->EfiFreeMemoryTop & CACHE_MODE_ADDRESS_MASK;
    Phit->EfiMemoryBottom     = Phit->EfiMemoryBottom  & CACHE_MODE_ADDRESS_MASK;
    Phit->EfiFreeMemoryBottom = Phit->EfiFreeMemoryBottom & CACHE_MODE_ADDRESS_MASK;
    Phit->EfiEndOfHobList     = Phit->EfiEndOfHobList & CACHE_MODE_ADDRESS_MASK;
  }

  return;
}
