/** @file
  RISC-V specific functionality for DxeLoad.

  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2023, Rivos Inc

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include "UefiPayloadEntry.h"

#define STACK_SIZE  0x20000

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
  VOID  *BaseOfStack;
  VOID  *TopOfStack;

  //
  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  if (BaseOfStack == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Can't allocate memory for stack.", __func__));
    ASSERT (FALSE);
  }

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack, STACK_SIZE);

  DEBUG ((DEBUG_INFO, "DXE Core new stack at %llx, stack pointer at %llx\n", BaseOfStack, TopOfStack));

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
