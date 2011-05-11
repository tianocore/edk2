/** @file
  PEI Services Table Pointer Library for PEI Core.
  
  This library is used for PEI Core which does executed from flash device directly but
  executed in memory. When the PEI Core does a Set of the PEI Service table pointer 
  a PPI is reinstalled so that PEIMs can update the copy of the PEI Services table 
  they have cached. 

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portiions copyrigth (c) 2011, Apple Inc. All rights reserved. 
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/DebugLib.h>

#include <Ppi/EmuPeiServicesTableUpdate.h>


CONST EFI_PEI_SERVICES  **gPeiServices = NULL;

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
  IN CONST EFI_PEI_SERVICES **PeiServicesTablePointer
  )
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  VOID                    *NotUsed;

  gPeiServices = PeiServicesTablePointer; 
  
  Status = (*PeiServicesTablePointer)->LocatePpi (
                                        PeiServicesTablePointer,
                                        &gEmuPeiServicesTableUpdatePpiGuid, // GUID
                                        0,                 // INSTANCE
                                        &PpiDescriptor,    // EFI_PEI_PPI_DESCRIPTOR
                                        &NotUsed           // PPI
                                        );
  if (!EFI_ERROR (Status)) {
    //
    // Standard PI Mechanism is to use negative offset from IDT. 
    // We can't do that in the emulator, so we make up a constant location
    // that every one can use. The first try may fail as the PEI Core is still
    // initializing its self, but that is OK. 
    //

    // Reinstall PPI to consumers know to update PEI Services pointer
    Status = (*PeiServicesTablePointer)->ReInstallPpi (
                                            PeiServicesTablePointer,
                                            PpiDescriptor,
                                            PpiDescriptor
                                            );

  }

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
  ASSERT (gPeiServices != NULL);
  ASSERT (*gPeiServices != NULL);
  return gPeiServices;
}


