/** @file
Semaphore mechanism to indicate to the BSP that an AP has exited SMM
after SMBASE relocation.

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"

X86_ASSEMBLY_PATCH_LABEL gPatchSmmRelocationOriginalAddressPtr32;
X86_ASSEMBLY_PATCH_LABEL gPatchRebasedFlagAddr32;

UINTN             mSmmRelocationOriginalAddress;
volatile BOOLEAN  *mRebasedFlag;

/**
AP Semaphore operation in 32-bit mode while BSP runs in 64-bit mode.
**/
VOID
SmmRelocationSemaphoreComplete32 (
  VOID
  );

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param[in] CpuIndex     The processor index.
  @param[in] RebasedFlag  A pointer to a flag that is set to TRUE
                          immediately after AP exits SMM.

**/
VOID
SemaphoreHook (
  IN UINTN             CpuIndex,
  IN volatile BOOLEAN  *RebasedFlag
  )
{
  SMRAM_SAVE_STATE_MAP  *CpuState;
  UINTN                 TempValue;

  mRebasedFlag       = RebasedFlag;
  PatchInstructionX86 (
    gPatchRebasedFlagAddr32,
    (UINT32)(UINTN)mRebasedFlag,
    4
    );

  CpuState = (SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);
  mSmmRelocationOriginalAddress = HookReturnFromSmm (
                                    CpuIndex,
                                    CpuState,
                                    (UINT64)(UINTN)&SmmRelocationSemaphoreComplete32,
                                    (UINT64)(UINTN)&SmmRelocationSemaphoreComplete
                                    );

  //
  // Use temp value to fix ICC complier warning
  //
  TempValue = (UINTN)&mSmmRelocationOriginalAddress;
  PatchInstructionX86 (
    gPatchSmmRelocationOriginalAddressPtr32,
    (UINT32)TempValue,
    4
    );
}
