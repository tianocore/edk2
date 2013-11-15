/** @file
  PEI Services Table Pointer Library for IA-32 and x64.

  According to PI specification, the peiservice pointer is stored prior at IDT
  table in IA32 and x64 architecture.
  
  Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php.                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Ppi/TemporaryRamSupport.h>

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
  CONST EFI_PEI_SERVICES  **PeiServices;
  IA32_DESCRIPTOR   Idtr;
  
  AsmReadIdtr (&Idtr);
  PeiServices = (CONST EFI_PEI_SERVICES **) (*(UINTN*)(Idtr.Base - sizeof (UINTN)));
  ASSERT (PeiServices != NULL);
  return PeiServices;
}

/**
  Caches a pointer PEI Services Table. 
 
  Caches the pointer to the PEI Services Table specified by PeiServicesTablePointer 
  in a CPU specific manner as specified in the CPU binding section of the Platform Initialization 
  Pre-EFI Initialization Core Interface Specification. 
  The function set the pointer of PEI services immediately preceding the IDT table
  according to PI specification.
  
  If PeiServicesTablePointer is NULL, then ASSERT().
  
  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES ** PeiServicesTablePointer
  )
{
  IA32_DESCRIPTOR        Idtr;
  EFI_PHYSICAL_ADDRESS   IdtBase;
  EFI_STATUS             Status;
  EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI   *TemporaryRamSupportPpi;
  
  ASSERT (PeiServicesTablePointer != NULL);
  AsmReadIdtr (&Idtr);
  if ((*(UINTN*)(Idtr.Base - sizeof (UINTN))) != (UINTN)PeiServicesTablePointer) {
    (*(UINTN*)(Idtr.Base - sizeof (UINTN))) = (UINTN)PeiServicesTablePointer;
    Status = (*PeiServicesTablePointer)->LocatePpi (
                                          PeiServicesTablePointer, 
                                          &gEfiTemporaryRamSupportPpiGuid, 
                                          0, 
                                          NULL, 
                                          (VOID**)&TemporaryRamSupportPpi
                                         );
                                      
    if (EFI_ERROR (Status)) {
      //
      // If TemporaryRamSupportPpi is not found, Idt table needs to be migrated into memory.
      //
      Status = (*PeiServicesTablePointer)->AllocatePages (
                                            PeiServicesTablePointer, 
                                            EfiBootServicesCode,
                                            EFI_SIZE_TO_PAGES(Idtr.Limit + 1 + sizeof (UINTN)),
                                            &IdtBase
                                            );
      if (!EFI_ERROR (Status)) {
        //
        // Migrate Idt table
        //
        CopyMem ((VOID *) (UINTN) IdtBase, (VOID *) (Idtr.Base - sizeof (UINTN)), Idtr.Limit + 1 + sizeof (UINTN));
        Idtr.Base = (UINTN) IdtBase + sizeof (UINTN);
        AsmWriteIdtr (&Idtr);
      }
    }
  }
}


