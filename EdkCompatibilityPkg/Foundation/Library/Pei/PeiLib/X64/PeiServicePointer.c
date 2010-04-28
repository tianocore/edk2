/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

    PeiServicePointer.c

Abstract:

--*/

#include "Tiano.h"
#include "PeiApi.h"


#if (PI_SPECIFICATION_VERSION >= 0x00010000)

VOID
SetPeiServicesTablePointer (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Save PeiService pointer so that it can be retrieved anywhere.

Arguments:

  PeiServices     - The direct pointer to PeiServiceTable.
  PhyscialAddress - The physcial address of variable PeiServices.
  
Returns:
  NONE
  
--*/        
{
  return;
}

EFI_PEI_SERVICES **
GetPeiServicesTablePointer (
  VOID 
  )
/*++

Routine Description:

  Get PeiService pointer.

Arguments:

  NONE.
  
Returns:
  The direct pointer to PeiServiceTable.
  
--*/          
{
  return NULL;
}


VOID
MigrateIdtTable (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Migrate IDT from temporary memory to real memory where preceded with 4 bytes for
  storing PeiService pointer.

Arguments:

  PeiServices   - The direct pointer to PeiServiceTable.
  
Returns:

  NONE.
  
--*/  
{
  return;
}

#endif

