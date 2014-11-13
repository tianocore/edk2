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
UINTN gPollInterval = 100; // 100 microseconds

MP_SYSTEM_DATA mMpSystemData;

VOID *mCommonStack = 0;
VOID *mTopOfApCommonStack = 0;
VOID *mApStackStart = 0;

EFI_MP_SERVICES_PROTOCOL  mMpServicesTemplate = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  NULL, // StartupAllAPs,
  StartupThisAP,
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
  Set the Application Processors state.

  @param   CpuData    The pointer to CPU_DATA_BLOCK of specified AP
  @param   State      The AP status

**/
VOID
SetApState (
  IN  CPU_DATA_BLOCK   *CpuData,
  IN  CPU_STATE        State
  )
{
  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  CpuData->State = State;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Set the Application Processor prepare to run a function specified
  by Params.

  @param CpuData           the pointer to CPU_DATA_BLOCK of specified AP
  @param Procedure         A pointer to the function to be run on enabled APs of the system
  @param ProcedureArgument Pointer to the optional parameter of the assigned function

**/
VOID
SetApProcedure (
  IN   CPU_DATA_BLOCK        *CpuData,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *ProcedureArgument
  )
{
  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }

  CpuData->Parameter  = ProcedureArgument;
  CpuData->Procedure  = Procedure;
  ReleaseSpinLock (&CpuData->CpuDataLock);
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
  Searches for the next blocking AP.

  Search for the next AP that is put in blocking state by single-threaded StartupAllAPs().

  @param  NextNumber           Pointer to the processor number of the next blocking AP.

  @retval EFI_SUCCESS          The next blocking AP has been found.
  @retval EFI_NOT_FOUND        No blocking AP exists.

**/
EFI_STATUS
GetNextBlockedNumber (
  OUT UINTN  *NextNumber
  )
{
  UINTN                 Number;
  CPU_STATE             CpuState;
  CPU_DATA_BLOCK        *CpuData;

  for (Number = 0; Number < mMpSystemData.NumberOfProcessors; Number++) {
    CpuData = &mMpSystemData.CpuDatas[Number];
    if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
      //
      // Skip BSP
      //
      continue;
    }

    CpuState = GetApState (CpuData);
    if (CpuState == CpuStateBlocked) {
      *NextNumber = Number;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
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
  This service lets the caller get one enabled AP to execute a caller-provided
  function. The caller can request the BSP to either wait for the completion
  of the AP or just proceed with the next task by using the EFI event mechanism.
  See EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() for more details on non-blocking
  execution support.  This service may only be called from the BSP.

  This function is used to dispatch one enabled AP to the function specified by
  Procedure passing in the argument specified by ProcedureArgument.  If WaitEvent
  is NULL, execution is in blocking mode. The BSP waits until the AP finishes or
  TimeoutInMicroSecondss expires. Otherwise, execution is in non-blocking mode.
  BSP proceeds to the next task without waiting for the AP. If a non-blocking mode
  is requested after the UEFI Event EFI_EVENT_GROUP_READY_TO_BOOT is signaled,
  then EFI_UNSUPPORTED must be returned.

  If the timeout specified by TimeoutInMicroseconds expires before the AP returns
  from Procedure, then execution of Procedure by the AP is terminated. The AP is
  available for subsequent calls to EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() and
  EFI_MP_SERVICES_PROTOCOL.StartupThisAP().

  @param[in]  This                    A pointer to the EFI_MP_SERVICES_PROTOCOL
                                      instance.
  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  ProcessorNumber         The handle number of the AP. The range is
                                      from 0 to the total number of logical
                                      processors minus 1. The total number of
                                      logical processors can be retrieved by
                                      EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until all APs finish
                                      or TimeoutInMicroseconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on all the enabled
                                      APs, and go on executing immediately. If
                                      all return from Procedure or TimeoutInMicroseconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      all APs return from Procedure, then Procedure
                                      on the failed APs is terminated. All enabled
                                      APs are available for next function assigned
                                      by EFI_MP_SERVICES_PROTOCOL.StartupAllAPs()
                                      or EFI_MP_SERVICES_PROTOCOL.StartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] Finished                If NULL, this parameter is ignored.  In
                                      blocking mode, this parameter is ignored.
                                      In non-blocking mode, if AP returns from
                                      Procedure before the timeout expires, its
                                      content is set to TRUE. Otherwise, the
                                      value is set to FALSE. The caller can
                                      determine if the AP returned from Procedure
                                      by evaluating this value.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before
                                  the timeout expires.
  @retval EFI_SUCCESS             In non-blocking mode, the function has been
                                  dispatched to specified AP.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  the specified AP has finished.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupThisAP (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN                   *Finished               OPTIONAL
  )
{
  CPU_DATA_BLOCK        *CpuData;
  EFI_STATUS            Status;

  CpuData = NULL;

  if (Finished != NULL) {
    *Finished = FALSE;
  }

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= mMpSystemData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
  if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT) ||
      !TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
    return EFI_INVALID_PARAMETER;
  }

  if (GetApState (CpuData) != CpuStateIdle) {
    return EFI_NOT_READY;
  }

  SetApState (CpuData, CpuStateReady);

  SetApProcedure (CpuData, Procedure, ProcedureArgument);

  CpuData->Timeout = TimeoutInMicroseconds;
  CpuData->WaitEvent = WaitEvent;
  CpuData->TimeoutActive = !!(TimeoutInMicroseconds);
  CpuData->Finished = Finished;

  if (WaitEvent != NULL) {
    //
    // Non Blocking
    //
    Status = gBS->SetTimer (
                    CpuData->CheckThisAPEvent,
                    TimerPeriodic,
                    EFI_TIMER_PERIOD_MICROSECONDS (100)
                    );
    return Status;
  }

  //
  // Blocking
  //
  while (TRUE) {
    if (GetApState (CpuData) == CpuStateFinished) {
      SetApState (CpuData, CpuStateIdle);
      break;
    }

    if (CpuData->TimeoutActive && CpuData->Timeout < 0) {
      ResetProcessorToIdleState (CpuData);
      return EFI_TIMEOUT;
    }

    gBS->Stall (gPollInterval);
    CpuData->Timeout -= gPollInterval;
  }

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
  Terminate AP's task and set it to idle state.

  This function terminates AP's task due to timeout by sending INIT-SIPI,
  and sends it to idle state.

  @param CpuData           the pointer to CPU_DATA_BLOCK of specified AP

**/
VOID
ResetProcessorToIdleState (
  IN CPU_DATA_BLOCK  *CpuData
  )
{
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
  Checks AP' status periodically.

  This function is triggerred by timer perodically to check the
  state of AP forStartupThisAP() executed in non-blocking mode.

  @param  Event    Event triggered.
  @param  Context  Parameter passed with the event.

**/
VOID
EFIAPI
CheckThisAPStatus (
  IN  EFI_EVENT        Event,
  IN  VOID             *Context
  )
{
  CPU_DATA_BLOCK  *CpuData;
  CPU_STATE       CpuState;

  CpuData = (CPU_DATA_BLOCK *) Context;
  if (CpuData->TimeoutActive) {
    CpuData->Timeout -= gPollInterval;
  }

  CpuState = GetApState (CpuData);

  if (CpuState == CpuStateFinished) {
    if (CpuData->Finished) {
      *CpuData->Finished = TRUE;
    }
    SetApState (CpuData, CpuStateIdle);
    goto out;
  }

  if (CpuData->TimeoutActive && CpuData->Timeout < 0) {
    if (CpuState != CpuStateIdle &&
        CpuData->Finished) {
      *CpuData->Finished = FALSE;
    }
    ResetProcessorToIdleState (CpuData);
    goto out;
  }

  return;

out:
  gBS->SetTimer (CpuData->CheckThisAPEvent, TimerCancel, 0);
  if (CpuData->WaitEvent) {
    gBS->SignalEvent (CpuData->WaitEvent);
    CpuData->WaitEvent = NULL;
  }
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
  UINTN          ProcessorNumber;
  CPU_DATA_BLOCK *CpuData;
  EFI_STATUS     Status;

  ZeroMem (&mMpSystemData, sizeof (MP_SYSTEM_DATA));

  mMpSystemData.NumberOfProcessors = 1;
  mMpSystemData.NumberOfEnabledProcessors = 1;

  mMpSystemData.CpuDatas = AllocateZeroPool (sizeof (CPU_DATA_BLOCK) * gMaxLogicalProcessorNumber);
  ASSERT(mMpSystemData.CpuDatas != NULL);

  for (ProcessorNumber = 0; ProcessorNumber < gMaxLogicalProcessorNumber; ProcessorNumber++) {
    CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    CheckThisAPStatus,
                    (VOID *) CpuData,
                    &CpuData->CheckThisAPEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

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
