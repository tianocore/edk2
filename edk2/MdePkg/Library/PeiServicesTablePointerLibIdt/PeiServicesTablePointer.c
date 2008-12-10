/** @file
  PEI Services Table Pointer Library for IA-32 and x64.

  According to PI specification, the peiservice pointer is stored prior at IDT
  table in IA32 and x64 architecture.
  
  Copyright (c) 2006 - 2007, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

/**
  The function returns the pointer to PEI services.

  The function returns the pointer to PEI services.
  It will ASSERT() if the pointer to PEI services is NULL.

  @retval  The pointer to PeiServices.

**/
CONST EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  CONST EFI_PEI_SERVICES  **PeiServices;
  IA32_DESCRIPTOR   Idtr;
  
  AsmReadIdtr (&Idtr);
  PeiServices = (CONST EFI_PEI_SERVICES **) (*(UINTN*)(Idtr.Base - sizeof (UINTN)));
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

/**
  The function set the pointer of PEI services immediately preceding the IDT table
  according to PI specification.
  
  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  IA32_DESCRIPTOR   Idtr;
  
  AsmReadIdtr (&Idtr);
  (*(UINTN*)(Idtr.Base - sizeof (UINTN))) = (UINTN)PeiServicesTablePointer;
}


