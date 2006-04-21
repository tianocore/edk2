/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ia32Fwh.c
    
Abstract:

Revision History

--*/

#include "FWBlockService.h"

EFI_STATUS
FvbSpecificInitialize (
  IN  ESAL_FWB_GLOBAL   *mFvbModuleGlobal
  )
/*++

Routine Description:
  Additional initialize code for IA32 platform.

Arguments:
  ESAL_FWB_GLOBAL       - Global pointer that points to the instance data

Returns: 
  EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}
