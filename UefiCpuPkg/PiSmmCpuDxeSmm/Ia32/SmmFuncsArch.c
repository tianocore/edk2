/** @file
  SMM CPU misc functions for Ia32 arch specific.
  
Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

extern UINT64 gTaskGateDescriptor;

EFI_PHYSICAL_ADDRESS                mGdtBuffer;
UINTN                               mGdtBufferSize;

/**
  Initialize IDT for SMM Stack Guard.

**/
VOID
EFIAPI
InitializeIDTSmmStackGuard (
  VOID
  )
{
  IA32_IDT_GATE_DESCRIPTOR  *IdtGate;

  //
  // If SMM Stack Guard feature is enabled, the Page Fault Exception entry in IDT
  // is a Task Gate Descriptor so that when a Page Fault Exception occurs,
  // the processors can use a known good stack in case stack is ran out.
  //
  IdtGate = (IA32_IDT_GATE_DESCRIPTOR *)gcSmiIdtr.Base;
  IdtGate += EXCEPT_IA32_PAGE_FAULT;
  IdtGate->Uint64 = gTaskGateDescriptor;
}

/**
  Initialize Gdt for all processors.
  
  @param[in]   Cr3          CR3 value.
  @param[out]  GdtStepSize  The step size for GDT table.

  @return GdtBase for processor 0.
          GdtBase for processor X is: GdtBase + (GdtStepSize * X)
**/
VOID *
InitGdt (
  IN  UINTN  Cr3,
  OUT UINTN  *GdtStepSize
  )
{
  UINTN                     Index;
  IA32_SEGMENT_DESCRIPTOR   *GdtDescriptor;
  UINTN                     TssBase;
  UINTN                     GdtTssTableSize;
  UINT8                     *GdtTssTables;
  UINTN                     GdtTableStepSize;

  if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
    //
    // For IA32 SMM, if SMM Stack Guard feature is enabled, we use 2 TSS.
    // in this case, we allocate separate GDT/TSS for each CPUs to avoid TSS load contention
    // on each SMI entry.
    //

    //
    // Enlarge GDT to contain 2 TSS descriptors
    //
    gcSmiGdtr.Limit += (UINT16)(2 * sizeof (IA32_SEGMENT_DESCRIPTOR));

    GdtTssTableSize = (gcSmiGdtr.Limit + 1 + TSS_SIZE * 2 + 7) & ~7; // 8 bytes aligned
    mGdtBufferSize = GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    GdtTssTables = (UINT8*)AllocateCodePages (EFI_SIZE_TO_PAGES (mGdtBufferSize));
    ASSERT (GdtTssTables != NULL);
    mGdtBuffer = (UINTN)GdtTssTables;
    GdtTableStepSize = GdtTssTableSize;

    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID*)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1 + TSS_SIZE * 2);
      //
      // Fixup TSS descriptors
      //
      TssBase = (UINTN)(GdtTssTables + GdtTableStepSize * Index + gcSmiGdtr.Limit + 1);
      GdtDescriptor = (IA32_SEGMENT_DESCRIPTOR *)(TssBase) - 2;
      GdtDescriptor->Bits.BaseLow = (UINT16)TssBase;
      GdtDescriptor->Bits.BaseMid = (UINT8)(TssBase >> 16);
      GdtDescriptor->Bits.BaseHigh = (UINT8)(TssBase >> 24);

      TssBase += TSS_SIZE;
      GdtDescriptor++;
      GdtDescriptor->Bits.BaseLow = (UINT16)TssBase;
      GdtDescriptor->Bits.BaseMid = (UINT8)(TssBase >> 16);
      GdtDescriptor->Bits.BaseHigh = (UINT8)(TssBase >> 24);
      //
      // Fixup TSS segments
      //
      // ESP as known good stack
      //
      *(UINTN *)(TssBase + TSS_IA32_ESP_OFFSET) =  mSmmStackArrayBase + EFI_PAGE_SIZE + Index * mSmmStackSize;
      *(UINT32 *)(TssBase + TSS_IA32_CR3_OFFSET) = Cr3;
    }
  } else {
    //
    // Just use original table, AllocatePage and copy them here to make sure GDTs are covered in page memory.
    //
    GdtTssTableSize = gcSmiGdtr.Limit + 1;
    mGdtBufferSize = GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    GdtTssTables = (UINT8*)AllocateCodePages (EFI_SIZE_TO_PAGES (mGdtBufferSize));
    ASSERT (GdtTssTables != NULL);
    mGdtBuffer = (UINTN)GdtTssTables;
    GdtTableStepSize = GdtTssTableSize;

    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID*)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1);
    }
  }

  *GdtStepSize = GdtTableStepSize;
  return GdtTssTables;
}

/**
  Transfer AP to safe hlt-loop after it finished restore CPU features on S3 patch.

  @param[in] ApHltLoopCode          The address of the safe hlt-loop function.
  @param[in] TopOfStack             A pointer to the new stack to use for the ApHltLoopCode.
  @param[in] NumberToFinishAddress  Address of Semaphore of APs finish count.

**/
VOID
TransferApToSafeState (
  IN UINTN  ApHltLoopCode,
  IN UINTN  TopOfStack,
  IN UINTN  NumberToFinishAddress
  )
{
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)ApHltLoopCode,
    (VOID *)NumberToFinishAddress,
    NULL,
    (VOID *)TopOfStack
    );
  //
  // It should never reach here
  //
  ASSERT (FALSE);
}
