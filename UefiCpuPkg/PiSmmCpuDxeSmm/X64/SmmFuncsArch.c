/** @file
  SMM CPU misc functions for x64 arch specific.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"

EFI_PHYSICAL_ADDRESS                mGdtBuffer;
UINTN                               mGdtBufferSize;

extern BOOLEAN mCetSupported;
extern UINTN mSmmShadowStackSize;

X86_ASSEMBLY_PATCH_LABEL mPatchCetPl0Ssp;
X86_ASSEMBLY_PATCH_LABEL mPatchCetInterruptSsp;
X86_ASSEMBLY_PATCH_LABEL mPatchCetInterruptSspTable;
UINT32 mCetPl0Ssp;
UINT32 mCetInterruptSsp;
UINT32 mCetInterruptSspTable;

UINTN  mSmmInterruptSspTables;

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
  // If SMM Stack Guard feature is enabled, set the IST field of
  // the interrupt gate for Page Fault Exception to be 1
  //
  IdtGate = (IA32_IDT_GATE_DESCRIPTOR *)gcSmiIdtr.Base;
  IdtGate += EXCEPT_IA32_PAGE_FAULT;
  IdtGate->Bits.Reserved_0 = 1;
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

  //
  // For X64 SMM, we allocate separate GDT/TSS for each CPUs to avoid TSS load contention
  // on each SMI entry.
  //
  GdtTssTableSize = (gcSmiGdtr.Limit + 1 + TSS_SIZE + 7) & ~7; // 8 bytes aligned
  mGdtBufferSize = GdtTssTableSize * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus;
  GdtTssTables = (UINT8*)AllocateCodePages (EFI_SIZE_TO_PAGES (mGdtBufferSize));
  ASSERT (GdtTssTables != NULL);
  mGdtBuffer = (UINTN)GdtTssTables;
  GdtTableStepSize = GdtTssTableSize;

  for (Index = 0; Index < gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus; Index++) {
    CopyMem (GdtTssTables + GdtTableStepSize * Index, (VOID*)(UINTN)gcSmiGdtr.Base, gcSmiGdtr.Limit + 1 + TSS_SIZE);

    //
    // Fixup TSS descriptors
    //
    TssBase = (UINTN)(GdtTssTables + GdtTableStepSize * Index + gcSmiGdtr.Limit + 1);
    GdtDescriptor = (IA32_SEGMENT_DESCRIPTOR *)(TssBase) - 2;
    GdtDescriptor->Bits.BaseLow = (UINT16)(UINTN)TssBase;
    GdtDescriptor->Bits.BaseMid = (UINT8)((UINTN)TssBase >> 16);
    GdtDescriptor->Bits.BaseHigh = (UINT8)((UINTN)TssBase >> 24);

    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      //
      // Setup top of known good stack as IST1 for each processor.
      //
      *(UINTN *)(TssBase + TSS_X64_IST1_OFFSET) = (mSmmStackArrayBase + EFI_PAGE_SIZE + Index * (mSmmStackSize + mSmmShadowStackSize));
    }
  }

  *GdtStepSize = GdtTableStepSize;
  return GdtTssTables;
}

/**
  Get Protected mode code segment from current GDT table.

  @return  Protected mode code segment value.
**/
UINT16
GetProtectedModeCS (
  VOID
  )
{
  IA32_DESCRIPTOR          GdtrDesc;
  IA32_SEGMENT_DESCRIPTOR  *GdtEntry;
  UINTN                    GdtEntryCount;
  UINT16                   Index;

  AsmReadGdtr (&GdtrDesc);
  GdtEntryCount = (GdtrDesc.Limit + 1) / sizeof (IA32_SEGMENT_DESCRIPTOR);
  GdtEntry = (IA32_SEGMENT_DESCRIPTOR *) GdtrDesc.Base;
  for (Index = 0; Index < GdtEntryCount; Index++) {
    if (GdtEntry->Bits.L == 0) {
      if (GdtEntry->Bits.Type > 8 && GdtEntry->Bits.DB == 1) {
        break;
      }
    }
    GdtEntry++;
  }
  ASSERT (Index != GdtEntryCount);
  return Index * 8;
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
  AsmDisablePaging64 (
    GetProtectedModeCS (),
    (UINT32)ApHltLoopCode,
    (UINT32)NumberToFinishAddress,
    0,
    (UINT32)TopOfStack
    );
  //
  // It should never reach here
  //
  ASSERT (FALSE);
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
  UINTN       SmmShadowStackSize;
  UINT64      *InterruptSspTable;
  UINT32      InterruptSsp;

  if ((PcdGet32 (PcdControlFlowEnforcementPropertyMask) != 0) && mCetSupported) {
    SmmShadowStackSize = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (PcdGet32 (PcdCpuSmmShadowStackSize)));
    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      SmmShadowStackSize += EFI_PAGES_TO_SIZE (2);
    }
    mCetPl0Ssp = (UINT32)((UINTN)ShadowStack + SmmShadowStackSize - sizeof(UINT64));
    PatchInstructionX86 (mPatchCetPl0Ssp, mCetPl0Ssp, 4);
    DEBUG ((DEBUG_INFO, "mCetPl0Ssp - 0x%x\n", mCetPl0Ssp));
    DEBUG ((DEBUG_INFO, "ShadowStack - 0x%x\n", ShadowStack));
    DEBUG ((DEBUG_INFO, "  SmmShadowStackSize - 0x%x\n", SmmShadowStackSize));

    if (FeaturePcdGet (PcdCpuSmmStackGuard)) {
      if (mSmmInterruptSspTables == 0) {
        mSmmInterruptSspTables = (UINTN)AllocateZeroPool(sizeof(UINT64) * 8 * gSmmCpuPrivate->SmmCoreEntryContext.NumberOfCpus);
        ASSERT (mSmmInterruptSspTables != 0);
        DEBUG ((DEBUG_INFO, "mSmmInterruptSspTables - 0x%x\n", mSmmInterruptSspTables));
      }

      //
      // The highest address on the stack (0xFF8) is a save-previous-ssp token pointing to a location that is 40 bytes away - 0xFD0.
      // The supervisor shadow stack token is just above it at address 0xFF0. This is where the interrupt SSP table points.
      // So when an interrupt of exception occurs, we can use SAVESSP/RESTORESSP/CLEARSSBUSY for the supervisor shadow stack,
      // due to the reason the RETF in SMM exception handler cannot clear the BUSY flag with same CPL.
      // (only IRET or RETF with different CPL can clear BUSY flag)
      // Please refer to UefiCpuPkg/Library/CpuExceptionHandlerLib/X64 for the full stack frame at runtime.
      //
      InterruptSsp = (UINT32)((UINTN)ShadowStack + EFI_PAGES_TO_SIZE(1) - sizeof(UINT64));
      *(UINT64 *)(UINTN)InterruptSsp = (InterruptSsp - sizeof(UINT64) * 4) | 0x2;
      mCetInterruptSsp = InterruptSsp - sizeof(UINT64);

      mCetInterruptSspTable = (UINT32)(UINTN)(mSmmInterruptSspTables + sizeof(UINT64) * 8 * CpuIndex);
      InterruptSspTable = (UINT64 *)(UINTN)mCetInterruptSspTable;
      InterruptSspTable[1] = mCetInterruptSsp;
      PatchInstructionX86 (mPatchCetInterruptSsp, mCetInterruptSsp, 4);
      PatchInstructionX86 (mPatchCetInterruptSspTable, mCetInterruptSspTable, 4);
      DEBUG ((DEBUG_INFO, "mCetInterruptSsp - 0x%x\n", mCetInterruptSsp));
      DEBUG ((DEBUG_INFO, "mCetInterruptSspTable - 0x%x\n", mCetInterruptSspTable));
    }
  }
}

