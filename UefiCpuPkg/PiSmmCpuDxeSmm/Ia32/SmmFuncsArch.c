/** @file
  SMM CPU misc functions for Ia32 arch specific.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

extern UINT64  gTaskGateDescriptor;

EFI_PHYSICAL_ADDRESS  mGdtBuffer;
UINTN                 mGdtBufferSize;

extern BOOLEAN  mCetSupported;

X86_ASSEMBLY_PATCH_LABEL  mPatchCetPl0Ssp;
X86_ASSEMBLY_PATCH_LABEL  mPatchCetInterruptSsp;
UINT32                    mCetPl0Ssp;
UINT32                    mCetInterruptSsp;

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
  IdtGate         = (IA32_IDT_GATE_DESCRIPTOR *)gcSmiIdtr.Base;
  IdtGate        += EXCEPT_IA32_PAGE_FAULT;
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
  UINTN                    Index;
  IA32_SEGMENT_DESCRIPTOR  *GdtDescriptor;
  UINTN                    TssBase;
  UINTN                    GdtTssTableSize;
  UINT8                    *GdtTssTables;
  UINTN                    GdtTableStepSize;
  UINTN                    InterruptShadowStack;

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

    GdtTssTableSize = (gcSmiGdtr.Limit + 1 + TSS_SIZE + EXCEPTION_TSS_SIZE + 7) & ~7; // 8 bytes aligned
    mGdtBufferSize  = GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    //
    // IA32 Stack Guard need use task switch to switch stack that need
    // write GDT and TSS, so AllocateCodePages() could not be used here
    // as code pages will be set to RO.
    //
    GdtTssTables = (UINT8 *)AllocatePages (EFI_SIZE_TO_PAGES (mGdtBufferSize));
    ASSERT (GdtTssTables != NULL);
    mGdtBuffer       = (UINTN)GdtTssTables;
    GdtTableStepSize = GdtTssTableSize;

    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID *)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1 + TSS_SIZE + EXCEPTION_TSS_SIZE);
      //
      // Fixup TSS descriptors
      //
      TssBase                      = (UINTN)(GdtTssTables + GdtTableStepSize * Index + gcSmiGdtr.Limit + 1);
      GdtDescriptor                = (IA32_SEGMENT_DESCRIPTOR *)(TssBase) - 2;
      GdtDescriptor->Bits.BaseLow  = (UINT16)TssBase;
      GdtDescriptor->Bits.BaseMid  = (UINT8)(TssBase >> 16);
      GdtDescriptor->Bits.BaseHigh = (UINT8)(TssBase >> 24);

      TssBase += TSS_SIZE;
      GdtDescriptor++;
      GdtDescriptor->Bits.BaseLow  = (UINT16)TssBase;
      GdtDescriptor->Bits.BaseMid  = (UINT8)(TssBase >> 16);
      GdtDescriptor->Bits.BaseHigh = (UINT8)(TssBase >> 24);
      //
      // Fixup TSS segments
      //
      // ESP as known good stack
      //
      *(UINTN *)(TssBase + TSS_IA32_ESP_OFFSET)  =  mSmmStackArrayBase + EFI_PAGE_SIZE + Index * mSmmStackSize;
      *(UINT32 *)(TssBase + TSS_IA32_CR3_OFFSET) = Cr3;

      //
      // Setup ShadowStack for stack switch
      //
      if ((PcdGet32 (PcdControlFlowEnforcementPropertyMask) != 0) && mCetSupported) {
        InterruptShadowStack                       = (UINTN)(mSmmStackArrayBase + mSmmStackSize + EFI_PAGES_TO_SIZE (1) - sizeof (UINT64) + (mSmmStackSize + mSmmShadowStackSize) * Index);
        *(UINT32 *)(TssBase + TSS_IA32_SSP_OFFSET) = (UINT32)InterruptShadowStack;
      }
    }
  } else {
    //
    // Just use original table, AllocatePage and copy them here to make sure GDTs are covered in page memory.
    //
    GdtTssTableSize = gcSmiGdtr.Limit + 1;
    mGdtBufferSize  = GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
    GdtTssTables    = (UINT8 *)AllocateCodePages (EFI_SIZE_TO_PAGES (mGdtBufferSize));
    ASSERT (GdtTssTables != NULL);
    mGdtBuffer       = (UINTN)GdtTssTables;
    GdtTableStepSize = GdtTssTableSize;

    for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
      CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID *)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1);
    }
  }

  *GdtStepSize = GdtTableStepSize;
  return GdtTssTables;
}

/**
  Initialize the shadow stack related data structure.

  @param CpuIndex     The index of CPU.
  @param ShadowStack  The bottom of the shadow stack for this CPU.
**/
VOID
InitShadowStack (
  IN UINTN  CpuIndex,
  IN VOID   *ShadowStack
  )
{
  UINTN  SmmShadowStackSize;

  if ((PcdGet32 (PcdControlFlowEnforcementPropertyMask) != 0) && mCetSupported) {
    SmmShadowStackSize = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmShadowStackSize)));
    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      SmmShadowStackSize += EFI_PAGES_TO_SIZE (2);
    }

    mCetPl0Ssp = (UINT32)((UINTN)ShadowStack + SmmShadowStackSize - sizeof (UINT64));
    PatchInstructionX86 (mPatchCetPl0Ssp, mCetPl0Ssp, 4);
    DEBUG ((DEBUG_INFO, "mCetPl0Ssp - 0x%x\n", mCetPl0Ssp));
    DEBUG ((DEBUG_INFO, "ShadowStack - 0x%x\n", ShadowStack));
    DEBUG ((DEBUG_INFO, "  SmmShadowStackSize - 0x%x\n", SmmShadowStackSize));

    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      mCetInterruptSsp = (UINT32)((UINTN)ShadowStack + EFI_PAGES_TO_SIZE (1) - sizeof (UINT64));
      PatchInstructionX86 (mPatchCetInterruptSsp, mCetInterruptSsp, 4);
      DEBUG ((DEBUG_INFO, "mCetInterruptSsp - 0x%x\n", mCetInterruptSsp));
    }
  }
}
