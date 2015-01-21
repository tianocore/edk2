/** @file
  C functions in SEC

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/TemporaryRamSupport.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugAgentLib.h>

/**
  Switch the stack in the temporary memory to the one in the permanent memory.

  This function must be invoked after the memory migration immediately. The relative
  position of the stack in the temporary and permanent memory is same.

  @param TemporaryMemoryBase  Base address of the temporary memory.
  @param PermenentMemoryBase  Base address of the permanent memory.
**/
VOID
EFIAPI
SecSwitchStack (
  UINT32   TemporaryMemoryBase,
  UINT32   PermenentMemoryBase
  );

/**
  This service of the TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.
  @param TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param CopySize               Amount of memory to migrate from temporary to permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize > TemporaryMemoryBase when
                                TemporaryMemoryBase > PermanentMemoryBase.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  IA32_DESCRIPTOR   IdtDescriptor;
  VOID*             OldHeap;
  VOID*             NewHeap;
  VOID*             OldStack;
  VOID*             NewStack;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  DebugAgentContext;
  BOOLEAN           OldStatus;
  UINTN             PeiStackSize;

  PeiStackSize = (UINTN)PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (PeiStackSize == 0) {
    PeiStackSize = (CopySize >> 1);
  }

  ASSERT (PeiStackSize < CopySize);

  //
  // |-------------------|---->
  // |      Stack        |    PeiStackSize
  // |-------------------|---->
  // |      Heap         |    PeiTemporayRamSize
  // |-------------------|---->  TempRamBase
  //
  // |-------------------|---->
  // |      Heap         |    PeiTemporayRamSize
  // |-------------------|---->
  // |      Stack        |    PeiStackSize
  // |-------------------|---->  PermanentMemoryBase
  //

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + PeiStackSize);

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + CopySize - PeiStackSize);
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  DebugAgentContext.HeapMigrateOffset = (UINTN)NewHeap - (UINTN)OldHeap;
  DebugAgentContext.StackMigrateOffset = (UINTN)NewStack - (UINTN)OldStack;

  OldStatus = SaveAndSetDebugTimerInterrupt (FALSE);

  //
  // Initialize Debug Agent to support source level debug in PEI phase after memory ready.
  // It will build HOB and fix up the pointer in IDT table.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, (VOID *) &DebugAgentContext, NULL);

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize - PeiStackSize);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, PeiStackSize);


  //
  // We need *not* fix the return address because currently,
  // The PeiCore is executed in flash.
  //

  //
  // Rebase IDT table in permanent memory
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtDescriptor.Base = IdtDescriptor.Base - (UINTN)OldStack + (UINTN)NewStack;

  AsmWriteIdtr (&IdtDescriptor);


  //
  // Program MTRR
  //

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediatly, also we need fixup the stack change caused by new call into
  // permenent memory.
  //
  SecSwitchStack (
    (UINT32) (UINTN) OldStack,
    (UINT32) (UINTN) NewStack
    );

  SaveAndSetDebugTimerInterrupt (OldStatus);

  return EFI_SUCCESS;
}

