/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeHotRelocateEx.c

Abstract:

    Fixes IPF specific relocation types


Revision History

--*/

#include "Runtime.h"

//
// Cache Flush Routine.
//
EFI_STATUS
FlushCpuCache (
  IN EFI_PHYSICAL_ADDRESS          Start,
  IN UINT64                        Length
  )
/*++

Routine Description:

  Flush cache with specified range.

Arguments:

  Start   - Start address
  Length  - Length in bytes

Returns:

  Status code
  
  EFI_SUCCESS - success

--*/
{
  SalFlushCache (Start, Length);
  return EFI_SUCCESS;
}
