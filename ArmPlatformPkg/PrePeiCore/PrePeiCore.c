/** @file
*  Main file supporting the transition to PEI Core in Normal World for Versatile Express
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/ArmLib.h>

#include <Ppi/ArmGlobalVariable.h>

#include "PrePeiCore.h"

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI   mTemporaryRamSupportPpi = { PrePeiCoreTemporaryRamSupport };
ARM_GLOBAL_VARIABLE_PPI             mGlobalVariablePpi = { PrePeiCoreGetGlobalVariableMemory };

EFI_PEI_PPI_DESCRIPTOR      gCommonPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmGlobalVariablePpiGuid,
    &mGlobalVariablePpi
  }
};

VOID
CreatePpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  EFI_PEI_PPI_DESCRIPTOR *PlatformPpiList;
  UINTN                   PlatformPpiListSize;
  UINTN                   ListBase;
  EFI_PEI_PPI_DESCRIPTOR *LastPpi;

  // Get the Platform PPIs
  PlatformPpiListSize = 0;
  ArmPlatformGetPlatformPpiList (&PlatformPpiListSize, &PlatformPpiList);

  // Copy the Common and Platform PPis in Temporrary Memory
  ListBase = PcdGet64 (PcdCPUCoresStackBase);
  CopyMem ((VOID*)ListBase, gCommonPpiTable, sizeof(gCommonPpiTable));
  CopyMem ((VOID*)(ListBase + sizeof(gCommonPpiTable)), PlatformPpiList, PlatformPpiListSize);

  // Set the Terminate flag on the last PPI entry
  LastPpi = (EFI_PEI_PPI_DESCRIPTOR*)ListBase + ((sizeof(gCommonPpiTable) + PlatformPpiListSize) / sizeof(EFI_PEI_PPI_DESCRIPTOR)) - 1;
  LastPpi->Flags |= EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;

  *PpiList     = (EFI_PEI_PPI_DESCRIPTOR*)ListBase;
  *PpiListSize = sizeof(gCommonPpiTable) + PlatformPpiListSize;
}

VOID
CEntryPoint (
  IN  UINTN                     MpId,
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  // Data Cache enabled on Primary core when MMU is enabled.
  ArmDisableDataCache ();
  // Invalidate Data cache
  ArmInvalidateDataCache ();
  // Invalidate instruction cache
  ArmInvalidateInstructionCache ();
  // Enable Instruction Caches on all cores.
  ArmEnableInstructionCache ();

  //
  // Note: Doesn't have to Enable CPU interface in non-secure world,
  // as Non-secure interface is already enabled in Secure world.
  //

  // Write VBAR - The Exception Vector table must be aligned to its requirement
  // Note: The AArch64 Vector table must be 2k-byte aligned - if this assertion fails ensure
  // 'Align=4K' is defined into your FDF for this module.
  ASSERT (((UINTN)PeiVectorTable & ARM_VECTOR_TABLE_ALIGNMENT) == 0);
  ArmWriteVBar ((UINTN)PeiVectorTable);

  //Note: The MMU will be enabled by MemoryPeim. Only the primary core will have the MMU on.

  // If not primary Jump to Secondary Main
  if (ArmPlatformIsPrimaryCore (MpId)) {
    // Initialize the Debug Agent for Source Level Debugging
    InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
    SaveAndSetDebugTimerInterrupt (TRUE);

    // Initialize the platform specific controllers
    ArmPlatformInitialize (MpId);

    // Goto primary Main.
    PrimaryMain (PeiCoreEntryPoint);
  } else {
    SecondaryMain (MpId);
  }

  // PEI Core should always load and never return
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
PrePeiCoreTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  VOID                             *OldHeap;
  VOID                             *NewHeap;
  VOID                             *OldStack;
  VOID                             *NewStack;

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  //
  // Migrate the temporary memory stack to permanent memory stack.
  //
  CopyMem (NewStack, OldStack, CopySize >> 1);

  //
  // Migrate the temporary memory heap to permanent memory heap.
  //
  CopyMem (NewHeap, OldHeap, CopySize >> 1);

  SecSwitchStack ((UINTN)NewStack - (UINTN)OldStack);

  return EFI_SUCCESS;
}

EFI_STATUS
PrePeiCoreGetGlobalVariableMemory (
  OUT EFI_PHYSICAL_ADDRESS    *GlobalVariableBase
  )
{
  ASSERT (GlobalVariableBase != NULL);

  *GlobalVariableBase = (UINTN)PcdGet64 (PcdCPUCoresStackBase) +
                        (UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize) -
                        (UINTN)PcdGet32 (PcdPeiGlobalVariableSize);

  return EFI_SUCCESS;
}

