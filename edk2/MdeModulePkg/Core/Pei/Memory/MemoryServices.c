/** @file
  EFI PEI Core memory services
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PeiMain.h>

/**

  Initialize the memory services.


  @param PrivateData     Add parameter description
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param OldCoreData     Pointer to the PEI Core data.
                         NULL if being run in non-permament memory mode.

**/
VOID
InitializeMemoryServices (
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *OldCoreData
  )
{
  
  PrivateData->SwitchStackSignal      = FALSE;

  //
  // First entering PeiCore, following code will initialized some field
  // in PeiCore's private data according to hand off data from sec core.
  //
  if (OldCoreData == NULL) {

    PrivateData->PeiMemoryInstalled = FALSE;

    PrivateData->BottomOfCarHeap        = SecCoreData->PeiTemporaryRamBase; 
    PrivateData->TopOfCarHeap           = (VOID *)((UINTN)(PrivateData->BottomOfCarHeap) + SecCoreData->PeiTemporaryRamSize);
    PrivateData->SizeOfTemporaryMemory  = SecCoreData->TemporaryRamSize;
    PrivateData->StackSize              = (UINT64) SecCoreData->StackSize;
    
    DEBUG_CODE_BEGIN ();
      PrivateData->SizeOfCacheAsRam = SecCoreData->PeiTemporaryRamSize + SecCoreData->StackSize;
      PrivateData->MaxTopOfCarHeap  = (VOID *) ((UINTN) PrivateData->BottomOfCarHeap + (UINTN) PrivateData->SizeOfCacheAsRam);
      PrivateData->StackBase        = (EFI_PHYSICAL_ADDRESS) (UINTN) SecCoreData->StackBase;
      PrivateData->StackSize        = (UINT64) SecCoreData->StackSize;
    DEBUG_CODE_END ();

    PrivateData->HobList.Raw = PrivateData->BottomOfCarHeap;
    
    PeiCoreBuildHobHandoffInfoTable (
      BOOT_WITH_FULL_CONFIGURATION,
      (EFI_PHYSICAL_ADDRESS) (UINTN) PrivateData->BottomOfCarHeap,
      (UINTN) SecCoreData->PeiTemporaryRamSize
      );

    //
    // Set PS to point to ServiceTableShadow in Cache
    //
    PrivateData->PS = &(PrivateData->ServiceTableShadow);
  }
  
  return;
}

/**

  Install the permanent memory is now available.
  Creates HOB (PHIT and Stack).

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryBegin        Start of memory address.
  @param MemoryLength       Length of memory.

  @return EFI_SUCCESS Always success.

**/
EFI_STATUS
EFIAPI
PeiInstallPeiMemory (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    MemoryBegin,
  IN UINT64                  MemoryLength
  )
{
  PEI_CORE_INSTANCE                     *PrivateData;

  DEBUG ((EFI_D_INFO, "PeiInstallPeiMemory MemoryBegin 0x%LX, MemoryLength 0x%LX\n", MemoryBegin, MemoryLength));
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  PrivateData->PhysicalMemoryBegin   = MemoryBegin;
  PrivateData->PhysicalMemoryLength  = MemoryLength;
  PrivateData->FreePhysicalMemoryTop = MemoryBegin + MemoryLength;
   
  PrivateData->SwitchStackSignal      = TRUE;

  return EFI_SUCCESS;   
}

/**

  Memory allocation service on permanent memory,
  not usable prior to the memory installation.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryType      Type of memory to allocate.
  @param Pages           Number of pages to allocate.
  @param Memory          Pointer of memory allocated.

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_INVALID_PARAMETER    Only AllocateAnyAddress is supported.
  @retval EFI_NOT_AVAILABLE_YET    Called with permanent memory not available
  @retval EFI_OUT_OF_RESOURCES     There is not enough HOB heap to satisfy the requirement
                                   to allocate the number of pages.

**/
EFI_STATUS
EFIAPI
PeiAllocatePages (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  OUT EFI_PHYSICAL_ADDRESS      *Memory
  )
{
  PEI_CORE_INSTANCE                       *PrivateData;
  EFI_PEI_HOB_POINTERS                    Hob;
  EFI_PHYSICAL_ADDRESS                    Offset;
  EFI_PHYSICAL_ADDRESS                    *FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS                    *FreeMemoryBottom;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  Hob.Raw     = PrivateData->HobList.Raw;
  
  //
  // Check if Hob already available
  //
  if (!PrivateData->PeiMemoryInstalled) {
    //
    // When PeiInstallMemory is called but CAR has *not* been moved to temporary memory,
    // the AllocatePage will dependent the field of PEI_CORE_INSTANCE structure.
    //
    if (!PrivateData->SwitchStackSignal) {
      return EFI_NOT_AVAILABLE_YET;
    } else {
      FreeMemoryTop     = &(PrivateData->FreePhysicalMemoryTop);
      FreeMemoryBottom  = &(PrivateData->PhysicalMemoryBegin);
    }
  } else {
    FreeMemoryTop     = &(Hob.HandoffInformationTable->EfiFreeMemoryTop);
    FreeMemoryBottom  = &(Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  }

  //
  // Check to see if on 4k boundary
  //
  Offset = *(FreeMemoryTop) & 0xFFF;
  
  //
  // If not aligned, make the allocation aligned.
  //
  if (Offset != 0) {
    *(FreeMemoryTop) -= Offset;
  }
  
  //
  // Verify that there is sufficient memory to satisfy the allocation
  //
  if (*(FreeMemoryTop) - ((Pages * EFI_PAGE_SIZE) + sizeof (EFI_HOB_MEMORY_ALLOCATION)) < 
      *(FreeMemoryBottom)) {
    DEBUG ((EFI_D_ERROR, "AllocatePages failed: No 0x%x Pages is available.\n", Pages));
    DEBUG ((EFI_D_ERROR, "There is only left 0x%x pages memory resource to be allocated.\n", \
    EFI_SIZE_TO_PAGES ((UINTN) (*(FreeMemoryTop) - *(FreeMemoryBottom)))));
    return  EFI_OUT_OF_RESOURCES;
  } else {
    //
    // Update the PHIT to reflect the memory usage
    //
    *(FreeMemoryTop) -= Pages * EFI_PAGE_SIZE;

    //
    // Update the value for the caller
    //
    *Memory = *(FreeMemoryTop);

    //
    // Create a memory allocation HOB.
    //
    BuildMemoryAllocationHob (
      *(FreeMemoryTop),
      Pages * EFI_PAGE_SIZE,
      MemoryType
      );

    return EFI_SUCCESS;
  }
}

/**

  Memory allocation service on the CAR.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Size            Amount of memory required
  @param Buffer          Address of pointer to the buffer

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_OUT_OF_RESOURCES     There is not enough heap to satisfy the requirement
                                   to allocate the requested size.

**/
EFI_STATUS
EFIAPI
PeiAllocatePool (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN UINTN                      Size,
  OUT VOID                      **Buffer
  )
{
  EFI_STATUS               Status;
  EFI_HOB_MEMORY_POOL      *Hob;

 //
 // If some "post-memory" PEIM wishes to allocate larger pool,
 // it should use AllocatePages service instead.
 //
 ASSERT (Size < 0x10000 - sizeof (EFI_HOB_MEMORY_POOL));
 Status = PeiServicesCreateHob (
             EFI_HOB_TYPE_MEMORY_POOL,
             (UINT16)(sizeof (EFI_HOB_MEMORY_POOL) + Size),
             (VOID **)&Hob
             );
  *Buffer = Hob+1;  


  return Status;
}
