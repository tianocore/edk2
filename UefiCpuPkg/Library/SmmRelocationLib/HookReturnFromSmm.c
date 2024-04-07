/** @file
  Hook return code from Smm.

  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalSmmRelocationLib.h"

/**
  Hook the code executed immediately after an RSM instruction on the currently
  executing CPU.  The mode of code executed immediately after RSM must be
  detected, and the appropriate hook must be selected.  Always clear the auto
  HALT restart flag if it is set.

  @param[in] CpuIndex                 The processor index for the currently
                                      executing CPU.
  @param[in] CpuState                 Pointer to SMRAM Save State Map for the
                                      currently executing CPU.
  @param[in] NewInstructionPointer32  Instruction pointer to use if resuming to
                                      32-bit mode from 64-bit SMM.
  @param[in] NewInstructionPointer    Instruction pointer to use if resuming to
                                      same mode as SMM.

  @retval The value of the original instruction pointer before it was hooked.

**/
UINT64
EFIAPI
HookReturnFromSmm (
  IN UINTN              CpuIndex,
  SMRAM_SAVE_STATE_MAP  *CpuState,
  UINT64                NewInstructionPointer32,
  UINT64                NewInstructionPointer
  )
{
  UINT64  OriginalInstructionPointer;

  if (mSmmSaveStateRegisterLma == EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT) {
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
