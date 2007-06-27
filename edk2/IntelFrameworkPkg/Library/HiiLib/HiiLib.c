/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  HiiLib.c

**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

/**
  This function allocates pool for an EFI_HII_PACKAGES structure
  with enough space for the variable argument list of package pointers.
  The allocated structure is initialized using NumberOfPackages, Guid, 
  and the variable length argument list of package pointers.

  @param  NumberOfPackages The number of HII packages to prepare.
  @param  Guid Package GUID.

  @return The allocated and initialized packages.

**/
EFI_HII_PACKAGES *
EFIAPI
PreparePackages (
  IN UINTN           NumberOfPackages,
  IN CONST EFI_GUID  *Guid OPTIONAL,
  ...
  )
{
  VA_LIST           Args;
  EFI_HII_PACKAGES  *HiiPackages;
  VOID              **Package;
  UINTN             Index;

  ASSERT (NumberOfPackages > 0);

  HiiPackages                   = AllocateZeroPool (sizeof (EFI_HII_PACKAGES) + NumberOfPackages * sizeof (VOID *));
  ASSERT (HiiPackages != NULL);

  HiiPackages->GuidId           = (EFI_GUID *) Guid;
  HiiPackages->NumberOfPackages = NumberOfPackages;
  Package                       = (VOID **) (((UINT8 *) HiiPackages) + sizeof (EFI_HII_PACKAGES));

  VA_START (Args, Guid);

  for (Index = 0; Index < NumberOfPackages; Index++) {
    *Package = VA_ARG (Args, VOID *);
    Package++;
  }

  VA_END (Args);

  return HiiPackages;

}
