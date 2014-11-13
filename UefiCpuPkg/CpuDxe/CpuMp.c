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

MP_SYSTEM_DATA mMpSystemData;

VOID *mCommonStack = 0;
VOID *mTopOfApCommonStack = 0;
VOID *mApStackStart = 0;

EFI_MP_SERVICES_PROTOCOL  mMpServicesTemplate = {
  NULL, // GetNumberOfProcessors,
  NULL, // GetProcessorInfo,
  NULL, // StartupAllAPs,
  NULL, // StartupThisAP,
  NULL, // SwitchBSP,
  NULL, // EnableDisableAP,
  NULL  // WhoAmI
};

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
  VOID* TopOfApStack;

  FillInProcessorInformation (FALSE, mMpSystemData.NumberOfProcessors);
  TopOfApStack  = (UINT8*)mApStackStart + gApStackSize;
  mApStackStart = TopOfApStack;

  mMpSystemData.NumberOfProcessors++;

  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)ProcessorToIdleState,
    NULL,
    NULL,
    TopOfApStack);
}

/**
  This function is called by all processors (both BSP and AP) once and collects MP related data.

  @param Bsp             TRUE if the CPU is BSP
  @param ProcessorNumber The specific processor number

  @retval EFI_SUCCESS    Data for the processor collected and filled in

**/
EFI_STATUS
FillInProcessorInformation (
  IN     BOOLEAN              Bsp,
  IN     UINTN                ProcessorNumber
  )
{
  CPU_DATA_BLOCK  *CpuData;
  UINT32          ProcessorId;

  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
  ProcessorId  = GetApicId ();
  CpuData->Info.ProcessorId  = ProcessorId;
  CpuData->Info.StatusFlag   = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT;
  if (Bsp) {
    CpuData->Info.StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }
  CpuData->Info.Location.Package = ProcessorId;
  CpuData->Info.Location.Core    = 0;
  CpuData->Info.Location.Thread  = 0;
  CpuData->State = Bsp ? CpuStateBuzy : CpuStateIdle;

  CpuData->Procedure        = NULL;
  CpuData->Parameter        = NULL;
  InitializeSpinLock (&CpuData->CpuDataLock);

  return EFI_SUCCESS;
}

/**
  Prepare the System Data.

  @retval EFI_SUCCESS     the System Data finished initilization.

**/
EFI_STATUS
InitMpSystemData (
  VOID
  )
{
  ZeroMem (&mMpSystemData, sizeof (MP_SYSTEM_DATA));

  mMpSystemData.NumberOfProcessors = 1;
  mMpSystemData.NumberOfEnabledProcessors = 1;

  mMpSystemData.CpuDatas = AllocateZeroPool (sizeof (CPU_DATA_BLOCK) * gMaxLogicalProcessorNumber);
  ASSERT(mMpSystemData.CpuDatas != NULL);

  //
  // BSP
  //
  FillInProcessorInformation (TRUE, 0);

  return EFI_SUCCESS;
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

  InitMpSystemData ();

  if (mMpSystemData.NumberOfProcessors == 1) {
    FreePages (mCommonStack, EFI_SIZE_TO_PAGES (gMaxLogicalProcessorNumber * gApStackSize));
    return;
  }

  if (mMpSystemData.NumberOfProcessors < gMaxLogicalProcessorNumber) {
    FreePages (mApStackStart, EFI_SIZE_TO_PAGES (
                                (gMaxLogicalProcessorNumber - mMpSystemData.NumberOfProcessors) *
                                gApStackSize));
  }
}
