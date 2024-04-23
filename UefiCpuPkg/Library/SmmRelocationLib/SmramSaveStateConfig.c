/** @file
  Config SMRAM Save State for SmmBases Relocation.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalSmmRelocationLib.h"
#include <Library/CpuLib.h>

/**
  Get the mode of the CPU at the time an SMI occurs

  @retval EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT   32 bit.
  @retval EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT   64 bit.

**/
UINT8
GetMmSaveStateRegisterLma (
  VOID
  )
{
  CPUID_VERSION_INFO_EAX      RegEax;
  CPUID_EXTENDED_CPU_SIG_EDX  RegEdx;
  UINTN                       FamilyId;
  UINTN                       ModelId;
  UINT32                      Eax;
  UINT8                       SmmSaveStateRegisterLma;

  //
  // Determine the mode of the CPU at the time an SMI occurs
  //   Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  //   Volume 3C, Section 34.4.1.1
  //
  RegEax.Uint32 = GetCpuFamilyModel ();
  FamilyId      = RegEax.Bits.FamilyId;
  ModelId       = RegEax.Bits.Model;
  if ((FamilyId == 0x06) || (FamilyId == 0x0f)) {
    ModelId = ModelId | RegEax.Bits.ExtendedModelId << 4;
  }

  RegEdx.Uint32 = 0;
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Eax, NULL, NULL, NULL);
  if (Eax >= CPUID_EXTENDED_CPU_SIG) {
    AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &(RegEdx.Uint32));
  }

  SmmSaveStateRegisterLma = (UINT8)EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT;
  if (RegEdx.Bits.LM) {
    SmmSaveStateRegisterLma = (UINT8)EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT;
  }

  if (FamilyId == 0x06) {
    if ((ModelId == 0x17) || (ModelId == 0x0f) || (ModelId == 0x1c)) {
      SmmSaveStateRegisterLma = (UINT8)EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT;
    }
  }

  return SmmSaveStateRegisterLma;
}

/**
  This function configures the SmBase on the currently executing CPU.

  @param[in]     SmBase             The SmBase on the currently executing CPU.

**/
VOID
EFIAPI
ConfigureSmBase (
  IN UINT64  SmBase
  )
{
  SMRAM_SAVE_STATE_MAP  *CpuState;

  CpuState = (SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);

  CpuState->x86.SMBASE = (UINT32)SmBase;
}

/**
  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.  The mode of code executed immediately after RSM must be
  detected, and the appropriate hook must be selected.  Always clear the auto
  HALT restart flag if it is set.

  @param[in,out] CpuState                 Pointer to SMRAM Save State Map for the
                                          currently executing CPU.
  @param[in]     NewInstructionPointer32  Instruction pointer to use if resuming to
                                          32-bit mode from 64-bit SMM.
  @param[in]     NewInstructionPointer    Instruction pointer to use if resuming to
                                          same mode as SMM.

  @retval The value of the original instruction pointer before it was hooked.

**/
UINT64
EFIAPI
HookReturnFromSmm (
  IN OUT SMRAM_SAVE_STATE_MAP  *CpuState,
  IN     UINT64                NewInstructionPointer32,
  IN     UINT64                NewInstructionPointer
  )
{
  UINT64  OriginalInstructionPointer;

  if (GetMmSaveStateRegisterLma () == EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT) {
    OriginalInstructionPointer = (UINT64)CpuState->x86._EIP;
    CpuState->x86._EIP         = (UINT32)NewInstructionPointer;

    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuState->x86.AutoHALTRestart & BIT0) != 0) {
      CpuState->x86.AutoHALTRestart &= ~BIT0;
    }
  } else {
    OriginalInstructionPointer = CpuState->x64._RIP;
    if ((CpuState->x64.IA32_EFER & LMA) == 0) {
      CpuState->x64._RIP = (UINT32)NewInstructionPointer32;
    } else {
      CpuState->x64._RIP = (UINT32)NewInstructionPointer;
    }

    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((CpuState->x64.AutoHALTRestart & BIT0) != 0) {
      CpuState->x64.AutoHALTRestart &= ~BIT0;
    }
  }

  return OriginalInstructionPointer;
}
