/** @file
Semaphore mechanism to indicate to the BSP that an AP has exited SMM
after SMBASE relocation.

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCpuDxeSmm.h"

UINTN             mSmmRelocationOriginalAddress;
volatile BOOLEAN  *mRebasedFlag;

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

  mRebasedFlag = RebasedFlag;

  CpuState = (SMRAM_SAVE_STATE_MAP *)(UINTN)(SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET);
  mSmmRelocationOriginalAddress = (UINTN)HookReturnFromSmm (
                                           CpuIndex,
                                           CpuState,
                                           (UINT64)(UINTN)&SmmRelocationSemaphoreComplete,
                                           (UINT64)(UINTN)&SmmRelocationSemaphoreComplete
                                           );
}
