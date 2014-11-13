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
  GetNumberOfProcessors,
  GetProcessorInfo,
  NULL, // StartupAllAPs,
  NULL, // StartupThisAP,
  NULL, // SwitchBSP,
  EnableDisableAP,
  WhoAmI
};

/**
  Check whether caller processor is BSP.

  @retval  TRUE       the caller is BSP
  @retval  FALSE      the caller is AP

**/
BOOLEAN
IsBSP (
  VOID
  )
{
  UINTN           CpuIndex;
  CPU_DATA_BLOCK  *CpuData;

  CpuData = NULL;

  WhoAmI (&mMpServicesTemplate, &CpuIndex);
  CpuData = &mMpSystemData.CpuDatas[CpuIndex];

  return CpuData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT ? TRUE : FALSE;
}

/**
  Get the Application Processors state.

  @param   CpuData    the pointer to CPU_DATA_BLOCK of specified AP

  @retval  CPU_STATE  the AP status

**/
CPU_STATE
GetApState (
  IN  CPU_DATA_BLOCK  *CpuData
  )
{
  CPU_STATE State;

  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  State = CpuData->State;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  return State;
}

/**
  Check the Application Processors Status whether contains the Flags.

  @param     CpuData  the pointer to CPU_DATA_BLOCK of specified AP
  @param     Flags    the StatusFlag describing in EFI_PROCESSOR_INFORMATION

  @retval    TRUE     the AP status includes the StatusFlag
  @retval    FALSE    the AP status excludes the StatusFlag

**/
BOOLEAN
TestCpuStatusFlag (
  IN  CPU_DATA_BLOCK  *CpuData,
  IN  UINT32          Flags
  )
{
  UINT32 Ret;

  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  Ret = CpuData->Info.StatusFlag & Flags;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  return !!(Ret);
}

/**
  Bitwise-Or of the Application Processors Status with the Flags.

  @param     CpuData  the pointer to CPU_DATA_BLOCK of specified AP
  @param     Flags    the StatusFlag describing in EFI_PROCESSOR_INFORMATION

**/
VOID
CpuStatusFlagOr (
  IN  CPU_DATA_BLOCK  *CpuData,
  IN  UINT32          Flags
  )
{
  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  CpuData->Info.StatusFlag |= Flags;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Bitwise-AndNot of the Application Processors Status with the Flags.

  @param     CpuData  the pointer to CPU_DATA_BLOCK of specified AP
  @param     Flags    the StatusFlag describing in EFI_PROCESSOR_INFORMATION

**/
VOID
CpuStatusFlagAndNot (
  IN  CPU_DATA_BLOCK  *CpuData,
  IN  UINT32          Flags
  )
{
  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  CpuData->Info.StatusFlag &= ~Flags;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  This function is used to retrieve the following information:
    - The number of logical processors that are present in the system.
    - The number of enabled logical processors in the system at the instant
      this call is made.

  Because MP Service Protocol provides services to enable and disable processors
  dynamically, the number of enabled logical processors may vary during the
  course of a boot session.

  If this service is called from an AP, then EFI_DEVICE_ERROR is returned.
  If NumberOfProcessors or NumberOfEnabledProcessors is NULL, then
  EFI_INVALID_PARAMETER is returned. Otherwise, the total number of processors
  is returned in NumberOfProcessors, the number of currently enabled processor
  is returned in NumberOfEnabledProcessors, and EFI_SUCCESS is returned.

  @param[in]  This                        A pointer to the EFI_MP_SERVICES_PROTOCOL
                                          instance.
  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
  @retval EFI_INVALID_PARAMETER   NumberOfEnabledProcessors is NULL.

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                     *NumberOfProcessors,
  OUT UINTN                     *NumberOfEnabledProcessors
  )
{
  if ((NumberOfProcessors == NULL) || (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  *NumberOfProcessors        = mMpSystemData.NumberOfProcessors;
  *NumberOfEnabledProcessors = mMpSystemData.NumberOfEnabledProcessors;
  return EFI_SUCCESS;
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  This service retrieves detailed MP-related information about any processor
  on the platform. Note the following:
    - The processor information may change during the course of a boot session.
    - The information presented here is entirely MP related.

  Information regarding the number of caches and their sizes, frequency of operation,
  slot numbers is all considered platform-related information and is not provided
  by this service.

  @param[in]  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL
                                    instance.
  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.

**/
EFI_STATUS
EFIAPI
GetProcessorInfo (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  )
{
  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= mMpSystemData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CopyMem (ProcessorInfoBuffer, &mMpSystemData.CpuDatas[ProcessorNumber], sizeof (EFI_PROCESSOR_INFORMATION));
  return EFI_SUCCESS;
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  This service allows the caller enable or disable an AP from this point onward.
  The caller can optionally specify the health status of the AP by Health. If
  an AP is being disabled, then the state of the disabled AP is implementation
  dependent. If an AP is enabled, then the implementation must guarantee that a
  complete initialization sequence is performed on the AP, so the AP is in a state
  that is compatible with an MP operating system. This service may not be supported
  after the UEFI Event EFI_EVENT_GROUP_READY_TO_BOOT is signaled.

  If the enable or disable AP operation cannot be completed prior to the return
  from this service, then EFI_UNSUPPORTED must be returned.

  @param[in] This              A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().
  @param[in] EnableAP          Specifies the new state for the processor for
                               enabled, FALSE for disabled.
  @param[in] HealthFlag        If not NULL, a pointer to a value that specifies
                               the new health status of the AP. This flag
                               corresponds to StatusFlag defined in
                               EFI_MP_SERVICES_PROTOCOL.GetProcessorInfo(). Only
                               the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                               bits are ignored.  If it is NULL, this parameter
                               is ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed
                                  prior to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.

**/
EFI_STATUS
EFIAPI
EnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  CPU_DATA_BLOCK *CpuData;

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= mMpSystemData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
  if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
    return EFI_INVALID_PARAMETER;
  }

  if (GetApState (CpuData) != CpuStateIdle) {
    return EFI_UNSUPPORTED;
  }

  if (EnableAP) {
    if (!(TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT))) {
      mMpSystemData.NumberOfEnabledProcessors++;
    }
    CpuStatusFlagOr (CpuData, PROCESSOR_ENABLED_BIT);
  } else {
    if (TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
      mMpSystemData.NumberOfEnabledProcessors--;
    }
    CpuStatusFlagAndNot (CpuData, PROCESSOR_ENABLED_BIT);
  }

  if (HealthFlag != NULL) {
    CpuStatusFlagAndNot (CpuData, (UINT32)~PROCESSOR_HEALTH_STATUS_BIT);
    CpuStatusFlagOr (CpuData, (*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT));
  }

  return EFI_SUCCESS;
}

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  This             A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param[out] ProcessorNumber  The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
EFI_STATUS
EFIAPI
WhoAmI (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  UINTN   Index;
  UINT32  ProcessorId;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ProcessorId = GetApicId ();
  for (Index = 0; Index < mMpSystemData.NumberOfProcessors; Index++) {
    if (mMpSystemData.CpuDatas[Index].Info.ProcessorId == ProcessorId) {
      break;
    }
  }

  *ProcessorNumber = Index;
  return EFI_SUCCESS;
}

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
