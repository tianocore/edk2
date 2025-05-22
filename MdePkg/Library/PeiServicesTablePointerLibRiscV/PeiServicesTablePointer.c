/** @file
  PEI Services Table Pointer Library.

  According to PI specification, the peiservice pointer is stored in the SSCRATCH
  register of RISC-V architecture.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>

/**
  Caches a pointer PEI Services Table.

  Caches the pointer to the PEI Services Table specified by PeiServicesTablePointer
  in a CPU specific manner as specified in the CPU binding section of the Platform Initialization
  Pre-EFI Initialization Core Interface Specification.

  If PeiServicesTablePointer is NULL, then ASSERT().

  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES  **PeiServicesTablePointer
  )
{
  ASSERT (PeiServicesTablePointer != NULL);
  RiscVSetSupervisorScratch ((UINT64)PeiServicesTablePointer);
}

/**
  Retrieves the cached value of the PEI Services Table pointer.

  Returns the cached value of the PEI Services Table pointer in a CPU specific manner
  as specified in the CPU binding section of the Platform Initialization Pre-EFI
  Initialization Core Interface Specification.

  If the cached PEI Services Table pointer is NULL, then ASSERT().

  @return  The pointer to PeiServices.

**/
CONST EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  return (CONST EFI_PEI_SERVICES **)RiscVGetSupervisorScratch ();
}

/**
  Perform CPU specific actions required to migrate the PEI Services Table
  pointer from temporary RAM to permanent RAM.

**/
VOID
EFIAPI
MigratePeiServicesTablePointer (
  VOID
  )
{
  //
  //  PEI Services Table pointer is stored in CSR_SSCRATCH register. No additional
  //  migration actions are required.
  //
  return;
}
