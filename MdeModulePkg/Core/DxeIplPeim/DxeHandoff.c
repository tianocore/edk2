/** @file
  Generic version of arch-specific functionality for DxeLoad.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2023, Google, LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeIpl.h"

#include <Ppi/MemoryAttribute.h>

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
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  VOID                        *BaseOfStack;
  VOID                        *TopOfStack;
  EFI_STATUS                  Status;
  EDKII_MEMORY_ATTRIBUTE_PPI  *MemoryPpi;

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  if (PcdGetBool (PcdSetNxForStack)) {
    Status = PeiServicesLocatePpi (
               &gEdkiiMemoryAttributePpiGuid,
               0,
               NULL,
               (VOID **)&MemoryPpi
               );
    ASSERT_EFI_ERROR (Status);

    Status = MemoryPpi->SetPermissions (
                          MemoryPpi,
                          (UINTN)BaseOfStack,
                          STACK_SIZE,
                          EFI_MEMORY_XP,
                          EFI_MEMORY_XP
                          );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // End of PEI phase signal
  //
  Status = PeiServicesInstallPpi (&gEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack, STACK_SIZE);

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
