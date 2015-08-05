/** @file
  x64-specifc functionality for DxeLoad.

Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeIpl.h"
#include "X64/VirtualMemory.h"



/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.
   It also installs EFI_END_OF_PEI_PPI to signal the end of PEI phase.

   @param DxeCoreEntryPoint         The entry point of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList
  )
{
  VOID                            *BaseOfStack;
  VOID                            *TopOfStack;
  EFI_STATUS                      Status;
  UINTN                           PageTables;
  UINT32                          Index;
  EFI_VECTOR_HANDOFF_INFO         *VectorInfo;
  EFI_PEI_VECTOR_HANDOFF_INFO_PPI *VectorHandoffInfoPpi;

  //
  // Get Vector Hand-off Info PPI and build Guided HOB
  //
  Status = PeiServicesLocatePpi (
             &gEfiVectorHandoffInfoPpiGuid,
             0,
             NULL,
             (VOID **)&VectorHandoffInfoPpi
             );
  if (Status == EFI_SUCCESS) {
    DEBUG ((EFI_D_INFO, "Vector Hand-off Info PPI is gotten, GUIDed HOB is created!\n"));
    VectorInfo = VectorHandoffInfoPpi->Info;
    Index = 1;
    while (VectorInfo->Attribute != EFI_VECTOR_HANDOFF_LAST_ENTRY) {
      VectorInfo ++;
      Index ++;
    }
    BuildGuidDataHob (
      &gEfiVectorHandoffInfoPpiGuid,
      VectorHandoffInfoPpi->Info,
      sizeof (EFI_VECTOR_HANDOFF_INFO) * Index
      );
  }

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *) ((UINTN) BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  PageTables = 0;
  if (FeaturePcdGet (PcdDxeIplBuildPageTables)) {
    //
    // Create page table and save PageMapLevel4 to CR3
    //
    PageTables = CreateIdentityMappingPageTables ((EFI_PHYSICAL_ADDRESS) (UINTN) BaseOfStack, STACK_SIZE);
  } else {
    //
    // Set NX for stack feature also require PcdDxeIplBuildPageTables be TRUE
    // for the DxeIpl and the DxeCore are both X64.
    //
    ASSERT (PcdGetBool (PcdSetNxForStack) == FALSE);
  }
  
  //
  // End of PEI phase signal
  //
  Status = PeiServicesInstallPpi (&gEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

  if (FeaturePcdGet (PcdDxeIplBuildPageTables)) {
    AsmWriteCr3 (PageTables);
  }

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //    
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN) BaseOfStack, STACK_SIZE);

  //
  // Transfer the control to the entry point of DxeCore.
  //
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    HobList.Raw,
    NULL,
    TopOfStack
    );
}
