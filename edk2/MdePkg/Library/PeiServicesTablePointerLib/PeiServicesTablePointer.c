/** @file
  PEI Services Table Pointer Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

#include "PeiServicesTablePointerInternal.h"

static EFI_PEI_SERVICES  **gPeiServices;

/**
  The function set the pointer of PEI services immediately preceding the IDT table
  according to PI specification.
  
  @param    PeiServices   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  EFI_PEI_SERVICES  **PeiServices
  )
{
  gPeiServices = PeiServices;
}

/**
  The function returns the pointer to PEI services.

  The function returns the pointer to PEI services.
  It will ASSERT() if the pointer to PEI services is NULL.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
GetPeiServicesTablePointer (
  VOID
  )
{
  ASSERT (gPeiServices != NULL);
  return gPeiServices;
}


/**
  The constructor function caches the pointer to PEI services.
  
  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_PEI_FILE_HANDLE  *FfsHeader,
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  gPeiServices = PeiServices;
  return EFI_SUCCESS;
}

/**
  After memory initialization in PEI phase, the IDT table in temporary memory should 
  be migrated to memory, and the address of PeiServicesPointer also need to be updated  
  immediately preceding the new IDT table.
  
  @param    PeiServices   The address of PeiServices pointer.
**/
VOID
MigrateIdtTable (
  IN EFI_PEI_SERVICES  **PeiServices
  )
{
}

