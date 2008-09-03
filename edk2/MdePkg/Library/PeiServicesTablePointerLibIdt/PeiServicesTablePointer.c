/** @file
  PEI Services Table Pointer Library for IA-32 and X64.

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
  
  The function returns the pointer to PeiServicee following
  PI1.0.
  
  For IA32, the four-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**.
  For x64, the eight-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**
  
  @return  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_PEI_SERVICES  **PeiServices;
  IA32_DESCRIPTOR   Idtr;
  
  AsmReadIdtr (&Idtr);
  PeiServices = (EFI_PEI_SERVICES **) (*(UINTN*)(Idtr.Base - sizeof (UINTN)));
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

/**
  
  The function sets the pointer to PeiServicee following
  PI1.0.
  
  For IA32, the four-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**.
  For x64, the eight-bytes field immediately prior to new IDT
  base addres is used to save the EFI_PEI_SERVICES**
  
  @param PeiServicesTablePointer  The pointer to PeiServices.

**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  IA32_DESCRIPTOR   Idtr;
  
  AsmReadIdtr (&Idtr);
  (*(UINTN*)(Idtr.Base - sizeof (UINTN))) = (UINTN)PeiServicesTablePointer;
}


