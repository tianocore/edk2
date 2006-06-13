/** @file
  PEI Services Table Pointer Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PeiServicesTablePointer.c

**/



static EFI_PEI_SERVICES  **gPeiServices;

EFI_PEI_SERVICES **
GetPeiServicesTablePointer (
  VOID
  )
{
  ASSERT (gPeiServices != NULL);
  return gPeiServices;
}

/**
**/
EFI_STATUS
PeiServicesTablePointerLibConstructor (
  IN EFI_FFS_FILE_HEADER  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  gPeiServices = PeiServices;
  return EFI_SUCCESS;
}
