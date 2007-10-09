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


#include <PiPei.h>


#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>

VOID
EFIAPI
SetPeiServicesTablePointer (
  IN EFI_PEI_SERVICES     **PeiServices
  )
{
  AsmWriteMm7 ((UINT64)(UINTN)PeiServices);
}

/**
  The function returns the pointer to PeiServices.

  The function returns the pointer to PeiServices.
  It will ASSERT() if the pointer to PeiServices is NULL.

  @retval  The pointer to PeiServices.

**/
EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_PEI_SERVICES  **PeiServices;

  PeiServices = (EFI_PEI_SERVICES **)(UINTN)AsmReadMm7 ();
  ASSERT (PeiServices != NULL);
  return PeiServices;
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
  AsmWriteMm7 ((UINT64)(UINTN)PeiServices);
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
  UINTN           Size;
  VOID            *NewBase;
  EFI_STATUS      Status;
  IA32_DESCRIPTOR Idtr;
  
  AsmReadIdtr (&Idtr);
  
  Size = Idtr.Limit + 1;
  
  Status = PeiServicesAllocatePool (Size, &NewBase);
  ASSERT_EFI_ERROR (Status);
  
  CopyMem (NewBase, (VOID*)Idtr.Base, Size);
  
  Idtr.Base = (UINTN)NewBase;
  AsmWriteIdtr (&Idtr);
}

