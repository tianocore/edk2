/** @file
  PEI Services Table Pointer Library for IA-32 and X64.

  Copyright (c) 2006 - 2007, Intel Corporation.<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "InternalPeiServicesTablePointer.h"


/**
  
  The function returns the pointer to PeiServicee following
  PI1.0.
  
  For IA32, the four-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**.
  For x64, the eight-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**
  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **) AsmPeiSevicesTablePointer ();
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

