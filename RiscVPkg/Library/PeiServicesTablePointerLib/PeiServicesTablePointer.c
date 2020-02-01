/** @file
  PEI Services Table Pointer Library.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <PiPei.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>
#include <RiscV.h>

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
  IN CONST EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  RISCV_MACHINE_MODE_CONTEXT *Context;

  Context = (RISCV_MACHINE_MODE_CONTEXT *)(UINTN)RiscVGetScratch ();
  //DEBUG ((DEBUG_INFO, "PEI set RISC-V Machine mode context at %x, PEI Service\n", Context, PeiServicesTablePointer));
  Context->PeiService = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiServicesTablePointer;
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
  RISCV_MACHINE_MODE_CONTEXT *Context;
  EFI_PEI_SERVICES **PeiServices;

  Context = (RISCV_MACHINE_MODE_CONTEXT *)(UINTN)RiscVGetScratch ();
  PeiServices = (EFI_PEI_SERVICES **)Context->PeiService;
  //DEBUG ((DEBUG_INFO, "PEI Get RISC-V Machine mode context at %x, PEI Service\n", Context, PeiServices));

  return (CONST EFI_PEI_SERVICES **)PeiServices;
}


/**
  The constructor function caches the pointer to PEI services.
  
  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  SetPeiServicesTablePointer (PeiServices);
  return EFI_SUCCESS;
}

/**
  Perform CPU specific actions required to migrate the PEI Services Table 
  pointer from temporary RAM to permanent RAM.

  For IA32 CPUs, the PEI Services Table pointer is stored in the 4 bytes 
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For X64 CPUs, the PEI Services Table pointer is stored in the 8 bytes 
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For Itanium and ARM CPUs, a the PEI Services Table Pointer is stored in
  a dedicated CPU register.  This means that there is no memory storage 
  associated with storing the PEI Services Table pointer, so no additional 
  migration actions are required for Itanium or ARM CPUs.

**/
VOID
EFIAPI
MigratePeiServicesTablePointer (
  VOID
  )
{
  //
  //  PEI Services Table pointer is cached in the global variable. No additional 
  //  migration actions are required.
  //
  return;
}
