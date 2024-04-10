/** @file
  Config SMRAM Save State for SmmBases Relocation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalSmmRelocationLib.h"
#include <Register/Amd/SmramSaveStateMap.h>

#define EFER_ADDRESS  0XC0000080ul

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
  UINT8   SmmSaveStateRegisterLma;
  UINT32  LMAValue;

  SmmSaveStateRegisterLma = (UINT8)EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT;

  LMAValue = (UINT32)AsmReadMsr64 (EFER_ADDRESS) & LMA;
  if (LMAValue) {
    SmmSaveStateRegisterLma = (UINT8)EFI_MM_SAVE_STATE_REGISTER_LMA_64BIT;
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
  AMD_SMRAM_SAVE_STATE_MAP  *AmdCpuState;

  AmdCpuState = (AMD_SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);

  AmdCpuState->x64.SMBASE = (UINT32)SmBase;
}

/**
  This function updates the SMRAM save state on the currently executing CPU
  to resume execution at a specific address after an RSM instruction.  This
  function must evaluate the SMRAM save state to determine the execution mode
  the RSM instruction resumes and update the resume execution address with
  either NewInstructionPointer32 or NewInstructionPoint.  The auto HALT restart
  flag in the SMRAM save state must always be cleared.  This function returns
  the value of the instruction pointer from the SMRAM save state that was
  replaced.  If this function returns 0, then the SMRAM save state was not
  modified.

  This function is called during the very first SMI on each CPU after
  SmmCpuFeaturesInitializeProcessor() to set a flag in normal execution mode
  to signal that the SMBASE of each CPU has been updated before the default
  SMBASE address is used for the first SMI to the next CPU.

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
  UINT64                    OriginalInstructionPointer;
  AMD_SMRAM_SAVE_STATE_MAP  *AmdCpuState;

  AmdCpuState = (AMD_SMRAM_SAVE_STATE_MAP *)CpuState;

  if (GetMmSaveStateRegisterLma () == EFI_MM_SAVE_STATE_REGISTER_LMA_32BIT) {
    OriginalInstructionPointer = (UINT64)AmdCpuState->x86._EIP;
    AmdCpuState->x86._EIP      = (UINT32)NewInstructionPointer;
    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((AmdCpuState->x86.AutoHALTRestart & BIT0) != 0) {
      AmdCpuState->x86.AutoHALTRestart &= ~BIT0;
    }
  } else {
    OriginalInstructionPointer = AmdCpuState->x64._RIP;
    if ((AmdCpuState->x64.EFER & LMA) == 0) {
      AmdCpuState->x64._RIP = (UINT32)NewInstructionPointer32;
    } else {
      AmdCpuState->x64._RIP = (UINT32)NewInstructionPointer;
    }

    //
    // Clear the auto HALT restart flag so the RSM instruction returns
    // program control to the instruction following the HLT instruction.
    //
    if ((AmdCpuState->x64.AutoHALTRestart & BIT0) != 0) {
      AmdCpuState->x64.AutoHALTRestart &= ~BIT0;
    }
  }

  return OriginalInstructionPointer;
}
