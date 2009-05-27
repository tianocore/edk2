/** @file
  Main SEC phase code.  Transitions to PEI.

  Copyright (c) 2008 - 2009, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/PcdLib.h>

#include "SecMain.h"

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

STATIC TEMPORARY_RAM_SUPPORT_PPI mTempRamSupportPpi = {
  (TEMPORARY_RAM_MIGRATION) TemporaryRamMigration
};

STATIC EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamSupportPpiGuid,
    &mTempRamSupportPpi
  },
};


VOID
InitializeIdtPtr (
  IN VOID* IdtPtr
  )
{
  IA32_DESCRIPTOR             IdtDescriptor;

  IdtDescriptor.Base  = (UINTN)IdtPtr;
  IdtDescriptor.Limit = (UINT16) 0;
  AsmWriteIdtr (&IdtDescriptor);
}

VOID
EFIAPI
SecCoreStartupWithStack (
  IN VOID       *BootFirmwareVolumePtr,
  IN VOID       *SecCoreEntryPoint,
  IN VOID       *PeiCoreEntryPoint,
  IN VOID       *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  UINT8                       *BottomOfTempRam;
  UINT8                       *TopOfTempRam;
  UINTN                       SizeOfTempRam;
  VOID                        *IdtPtr;

  DEBUG ((EFI_D_ERROR,
    "SecCoreStartupWithStack(0x%x, 0x%x, 0x%x, 0x%x)\n",
    (UINT32)(UINTN)BootFirmwareVolumePtr,
    (UINT32)(UINTN)SecCoreEntryPoint,
    (UINT32)(UINTN)PeiCoreEntryPoint,
    (UINT32)(UINTN)TopOfCurrentStack));

  
  BottomOfTempRam = (UINT8*)(UINTN) INITIAL_TOP_OF_STACK;
  SizeOfTempRam = (UINTN) SIZE_64KB;
  TopOfTempRam = BottomOfTempRam + SizeOfTempRam;

  //
  // |-------------|
  // | SecCoreData | 4k
  // |-------------|
  // |    Heap     | 28k
  // |-------------|
  // |   Stack     | 32k
  // |-------------| <---- INITIAL_TOP_OF_STACK
  //

  //
  // Bind this information into the SEC hand-off state
  //
  SecCoreData = (EFI_SEC_PEI_HAND_OFF*)((UINTN) TopOfTempRam - SIZE_4KB);
  SecCoreData->DataSize = sizeof(EFI_SEC_PEI_HAND_OFF);

  SecCoreData->BootFirmwareVolumeBase = (VOID*)(UINTN) PcdGet32 (PcdOvmfFlashFvRecoveryBase);
  SecCoreData->BootFirmwareVolumeSize = PcdGet32 (PcdOvmfFlashFvRecoverySize);

  SecCoreData->TemporaryRamBase       = (VOID*) BottomOfTempRam;
  SecCoreData->TemporaryRamSize       = SizeOfTempRam;

  SecCoreData->PeiTemporaryRamSize    = 28 * SIZE_1KB;
  SecCoreData->PeiTemporaryRamBase    = (VOID*)((UINTN)SecCoreData - SecCoreData->PeiTemporaryRamSize);

  SecCoreData->StackBase              = SecCoreData->TemporaryRamBase;
  SecCoreData->StackSize              = (UINTN)SecCoreData->PeiTemporaryRamBase - (UINTN)SecCoreData->TemporaryRamBase;

  //
  // Initialize the IDT Pointer, since IA32 & X64 architectures
  // use it to store the PEI Services pointer.
  //
  IdtPtr = (VOID*)((UINT8*)SecCoreData + sizeof (*SecCoreData) + sizeof (UINTN));
  IdtPtr = ALIGN_POINTER(IdtPtr, 16);
  InitializeIdtPtr (IdtPtr);

  //
  // Transfer control to the PEI Core
  //
  PeiSwitchStacks (
    (SWITCH_STACK_ENTRY_POINT) (UINTN) PeiCoreEntryPoint,
    SecCoreData,
    (VOID *) (UINTN) ((EFI_PEI_PPI_DESCRIPTOR *) &mPrivateDispatchTable),
    NULL,
    TopOfCurrentStack,
    (VOID *)((UINTN)SecCoreData->StackBase + SecCoreData->StackSize)
    );

  //
  // If we get here, then the PEI Core returned.  This is an error
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  DEBUG ((EFI_D_ERROR, "TemporaryRamMigration(0x%x, 0x%x, 0x%x)\n", (UINTN)TemporaryMemoryBase, (UINTN)PermanentMemoryBase, CopySize));

  //
  // Migrate the whole temporary memory to permenent memory.
  // 
  CopyMem((VOID*)(UINTN)PermanentMemoryBase, (VOID*)(UINTN)TemporaryMemoryBase, CopySize);

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediatly, also we need fixup the stack change caused by new call into 
  // permenent memory.
  // 
  SecSwitchStack (
    (UINTN) TemporaryMemoryBase,
    (UINTN) PermanentMemoryBase,
    CopySize
    );

  return EFI_SUCCESS;
}

