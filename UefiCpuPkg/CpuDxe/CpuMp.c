/** @file
  CPU DXE Module.

  Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"
#include "CpuMp.h"

UINTN gMaxLogicalProcessorNumber;
UINTN gApStackSize;

VOID *mCommonStack = 0;
VOID *mTopOfApCommonStack = 0;
VOID *mApStackStart = 0;

volatile UINTN mNumberOfProcessors;

/**
  Application Processors do loop routine
  after switch to its own stack.

  @param  Context1    A pointer to the context to pass into the function.
  @param  Context2    A pointer to the context to pass into the function.

**/
VOID
ProcessorToIdleState (
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2   OPTIONAL
  )
{
  DEBUG ((DEBUG_INFO, "Ap apicid is %d\n", GetApicId ()));

  AsmApDoneWithCommonStack ();

  CpuSleep ();
  CpuDeadLoop ();
}

/**
  Application Processor C code entry point.

**/
VOID
EFIAPI
ApEntryPointInC (
  VOID
  )
{
  mNumberOfProcessors++;
  mApStackStart = (UINT8*)mApStackStart + gApStackSize;

  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)ProcessorToIdleState,
    NULL,
    NULL,
    mApStackStart);
}


/**
  Initialize Multi-processor support.

**/
VOID
InitializeMpSupport (
  VOID
  )
{
  gMaxLogicalProcessorNumber = (UINTN) PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  if (gMaxLogicalProcessorNumber < 1) {
    DEBUG ((DEBUG_ERROR, "Setting PcdCpuMaxLogicalProcessorNumber should be more than zero.\n"));
    return;
  }

  if (gMaxLogicalProcessorNumber == 1) {
    return;
  }

  gApStackSize = (UINTN) PcdGet32 (PcdCpuApStackSize);
  ASSERT ((gApStackSize & (SIZE_4KB - 1)) == 0);

  mApStackStart = AllocatePages (EFI_SIZE_TO_PAGES (gMaxLogicalProcessorNumber * gApStackSize));
  ASSERT (mApStackStart != NULL);

  //
  // the first buffer of stack size used for common stack, when the amount of AP
  // more than 1, we should never free the common stack which maybe used for AP reset.
  //
  mCommonStack = mApStackStart;
  mTopOfApCommonStack = (UINT8*) mApStackStart + gApStackSize;
  mApStackStart = mTopOfApCommonStack;

  mNumberOfProcessors = 1;

  if (mNumberOfProcessors == 1) {
    FreePages (mCommonStack, EFI_SIZE_TO_PAGES (gMaxLogicalProcessorNumber * gApStackSize));
    return;
  }

  if (mNumberOfProcessors < gMaxLogicalProcessorNumber) {
    FreePages (mApStackStart, EFI_SIZE_TO_PAGES ((gMaxLogicalProcessorNumber - mNumberOfProcessors) *
                                                 gApStackSize));
  }
}
