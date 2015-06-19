/** @file
  CPU DXE Module.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
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
EFI_HANDLE     mMpServiceHandle       = NULL;
EFI_EVENT      mExitBootServicesEvent = (EFI_EVENT)NULL;

VOID *mCommonStack = 0;
VOID *mTopOfApCommonStack = 0;
VOID *mApStackStart = 0;

volatile BOOLEAN mAPsAlreadyInitFinished = FALSE;
volatile BOOLEAN mStopCheckAllAPsStatus = TRUE;

EFI_MP_SERVICES_PROTOCOL  mMpServicesTemplate = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  StartupAllAPs,
  StartupThisAP,
  SwitchBSP,
  EnableDisableAP,
  WhoAmI
};

/**
   Get Mp Service Lock.

  @param   CpuData    the pointer to CPU_DATA_BLOCK of specified processor

**/
VOID
GetMpSpinLock (
  IN  CPU_DATA_BLOCK  *CpuData
  )
{
  while (!AcquireSpinLockOrFail (&CpuData->CpuDataLock)) {
    CpuPause ();
  }
  CpuData->LockSelf = GetApicId ();
}

/**
   Release Mp Service Lock.

  @param   CpuData    the pointer to CPU_DATA_BLOCK of specified processor

**/
VOID
ReleaseMpSpinLock (
  IN  CPU_DATA_BLOCK  *CpuData
  )
{
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

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

  GetMpSpinLock (CpuData);
  State = CpuData->State;
  ReleaseMpSpinLock (CpuData);

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
  GetMpSpinLock (CpuData);
  CpuData->State = State;
  ReleaseMpSpinLock (CpuData);
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
  GetMpSpinLock (CpuData);
  CpuData->Parameter  = ProcedureArgument;
  CpuData->Procedure  = Procedure;
  ReleaseMpSpinLock (CpuData);
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

  GetMpSpinLock (CpuData);
  Ret = CpuData->Info.StatusFlag & Flags;
  ReleaseMpSpinLock (CpuData);

  return (BOOLEAN) (Ret != 0);
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
  GetMpSpinLock (CpuData);
  CpuData->Info.StatusFlag |= Flags;
  ReleaseMpSpinLock (CpuData);
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
  GetMpSpinLock (CpuData);
  CpuData->Info.StatusFlag &= ~Flags;
  ReleaseMpSpinLock (CpuData);
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
  Check if the APs state are finished, and update them to idle state
  by StartupAllAPs().

**/
VOID
CheckAndUpdateAllAPsToIdleState (
  VOID
  )
{
  UINTN                 ProcessorNumber;
  UINTN                 NextNumber;
  CPU_DATA_BLOCK        *CpuData;
  EFI_STATUS            Status;
  CPU_STATE             CpuState;

  for (ProcessorNumber = 0; ProcessorNumber < mMpSystemData.NumberOfProcessors; ProcessorNumber++) {
    CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
    if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
      //
      // Skip BSP
      //
      continue;
    }

    if (!TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
      //
      // Skip Disabled processors
      //
      continue;
    }

    CpuState = GetApState (CpuData);
    if (CpuState == CpuStateFinished) {
      mMpSystemData.FinishCount++;
      if (mMpSystemData.SingleThread) {
        Status = GetNextBlockedNumber (&NextNumber);
        if (!EFI_ERROR (Status)) {
          SetApState (&mMpSystemData.CpuDatas[NextNumber], CpuStateReady);
          SetApProcedure (&mMpSystemData.CpuDatas[NextNumber],
                          mMpSystemData.Procedure,
                          mMpSystemData.ProcedureArgument);
          //
          // If this AP previous state is blocked, we should
          // wake up this AP by sent a SIPI. and avoid
          // re-involve the sleeping state. we must call
          // SetApProcedure() first.
          //
          ResetProcessorToIdleState (&mMpSystemData.CpuDatas[NextNumber]);
        }
      }
      SetApState (CpuData, CpuStateIdle);
    }
  }
}

/**
  If the timeout expires before all APs returns from Procedure,
  we should forcibly terminate the executing AP and fill FailedList back
  by StartupAllAPs().

**/
VOID
ResetAllFailedAPs (
  VOID
  )
{
  CPU_DATA_BLOCK        *CpuData;
  UINTN                 Number;
  CPU_STATE             CpuState;

  if (mMpSystemData.FailedList != NULL) {
     *mMpSystemData.FailedList = AllocatePool ((mMpSystemData.StartCount - mMpSystemData.FinishCount + 1) * sizeof(UINTN));
     ASSERT (*mMpSystemData.FailedList != NULL);
  }

  for (Number = 0; Number < mMpSystemData.NumberOfProcessors; Number++) {
    CpuData = &mMpSystemData.CpuDatas[Number];
    if (TestCpuStatusFlag (CpuData,  PROCESSOR_AS_BSP_BIT)) {
      //
      // Skip BSP
      //
      continue;
    }

    if (!TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
      //
      // Skip Disabled processors
      //
      continue;
    }

    CpuState = GetApState (CpuData);
    if (CpuState != CpuStateIdle &&
        CpuState != CpuStateSleeping) {
      if (mMpSystemData.FailedList != NULL) {
        (*mMpSystemData.FailedList)[mMpSystemData.FailedListIndex++] = Number;
      }
      ResetProcessorToIdleState (CpuData);
    }
  }

  if (mMpSystemData.FailedList != NULL) {
    (*mMpSystemData.FailedList)[mMpSystemData.FailedListIndex] = END_OF_CPU_LIST;
  }
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
  This service executes a caller provided function on all enabled APs. APs can
  run either simultaneously or one at a time in sequence. This service supports
  both blocking and non-blocking requests. The non-blocking requests use EFI
  events so the BSP can detect when the APs have finished. This service may only
  be called from the BSP.

  This function is used to dispatch all the enabled APs to the function specified
  by Procedure.  If any enabled AP is busy, then EFI_NOT_READY is returned
  immediately and Procedure is not started on any AP.

  If SingleThread is TRUE, all the enabled APs execute the function specified by
  Procedure one by one, in ascending order of processor handle number. Otherwise,
  all the enabled APs execute the function specified by Procedure simultaneously.

  If WaitEvent is NULL, execution is in blocking mode. The BSP waits until all
  APs finish or TimeoutInMicroseconds expires. Otherwise, execution is in non-blocking
  mode, and the BSP returns from this service without waiting for APs. If a
  non-blocking mode is requested after the UEFI Event EFI_EVENT_GROUP_READY_TO_BOOT
  is signaled, then EFI_UNSUPPORTED must be returned.

  If the timeout specified by TimeoutInMicroseconds expires before all APs return
  from Procedure, then Procedure on the failed APs is terminated. All enabled APs
  are always available for further calls to EFI_MP_SERVICES_PROTOCOL.StartupAllAPs()
  and EFI_MP_SERVICES_PROTOCOL.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the EFI_MP_SERVICES_PROTOCOL.StartupAllAPs()
  to make sure that the nature of the code that is executed on the BSP and the
  dispatched APs is well controlled. The MP Services Protocol does not guarantee
  that the Procedure function is MP-safe. Hence, the tasks that can be run in
  parallel are limited to certain independent tasks and well-controlled exclusive
  code. EFI services and protocols may not be called by APs unless otherwise
  specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroseconds expires.

  In non-blocking execution mode, BSP is freed to return to the caller and then
  proceed to the next task without having to wait for APs. The following
  sequence needs to occur in a non-blocking execution mode:

    -# The caller that intends to use this MP Services Protocol in non-blocking
       mode creates WaitEvent by calling the EFI CreateEvent() service.  The caller
       invokes EFI_MP_SERVICES_PROTOCOL.StartupAllAPs(). If the parameter WaitEvent
       is not NULL, then StartupAllAPs() executes in non-blocking mode. It requests
       the function specified by Procedure to be started on all the enabled APs,
       and releases the BSP to continue with other tasks.
    -# The caller can use the CheckEvent() and WaitForEvent() services to check
       the state of the WaitEvent created in step 1.
    -# When the APs complete their task or TimeoutInMicroSecondss expires, the MP
       Service signals WaitEvent by calling the EFI SignalEvent() function. If
       FailedCpuList is not NULL, its content is available when WaitEvent is
       signaled. If all APs returned from Procedure prior to the timeout, then
       FailedCpuList is set to NULL. If not all APs return from Procedure before
       the timeout, then FailedCpuList is filled in with the list of the failed
       APs. The buffer is allocated by MP Service Protocol using AllocatePool().
       It is the caller's responsibility to free the buffer with FreePool() service.
    -# This invocation of SignalEvent() function informs the caller that invoked
       EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() that either all the APs completed
       the specified task or a timeout occurred. The contents of FailedCpuList
       can be examined to determine which APs did not complete the specified task
       prior to the timeout.

  @param[in]  This                    A pointer to the EFI_MP_SERVICES_PROTOCOL
                                      instance.
  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  SingleThread            If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor handle
                                      number.  If FALSE, then all the enabled APs
                                      execute the function specified by Procedure
                                      simultaneously.
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until all APs finish
                                      or TimeoutInMicroseconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on all the enabled
                                      APs, and go on executing immediately. If
                                      all return from Procedure, or TimeoutInMicroseconds
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
  @param[out] FailedCpuList           If NULL, this parameter is ignored. Otherwise,
                                      if all APs finish successfully, then its
                                      content is set to NULL. If not all APs
                                      finish before timeout expires, then its
                                      content is set to address of the buffer
                                      holding handle numbers of the failed APs.
                                      The buffer is allocated by MP Service Protocol,
                                      and it's the caller's responsibility to
                                      free the buffer with FreePool() service.
                                      In blocking mode, it is ready for consumption
                                      when the call returns. In non-blocking mode,
                                      it is ready when WaitEvent is signaled.  The
                                      list of failed CPU is terminated by
                                      END_OF_CPU_LIST.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been dispatched
                                  to all enabled APs.
  @retval EFI_UNSUPPORTED         A non-blocking mode request was made after the
                                  UEFI event EFI_EVENT_GROUP_READY_TO_BOOT was
                                  signaled.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupAllAPs (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                   SingleThread,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT UINTN                     **FailedCpuList         OPTIONAL
  )
{
  EFI_STATUS            Status;
  CPU_DATA_BLOCK        *CpuData;
  UINTN                 Number;
  CPU_STATE             APInitialState;
  CPU_STATE             CpuState;

  CpuData = NULL;

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
  }

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (mMpSystemData.NumberOfProcessors == 1) {
    return EFI_NOT_STARTED;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // temporarily stop checkAllAPsStatus for avoid resource dead-lock.
  //
  mStopCheckAllAPsStatus = TRUE;

  for (Number = 0; Number < mMpSystemData.NumberOfProcessors; Number++) {
    CpuData = &mMpSystemData.CpuDatas[Number];
    if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
      //
      // Skip BSP
      //
      continue;
    }

    if (!TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
      //
      // Skip Disabled processors
      //
      continue;
    }

    CpuState = GetApState (CpuData);
    if (CpuState != CpuStateIdle &&
        CpuState != CpuStateSleeping) {
      return EFI_NOT_READY;
    }
  }

  mMpSystemData.Procedure         = Procedure;
  mMpSystemData.ProcedureArgument = ProcedureArgument;
  mMpSystemData.WaitEvent         = WaitEvent;
  mMpSystemData.Timeout           = TimeoutInMicroseconds;
  mMpSystemData.TimeoutActive     = (BOOLEAN) (TimeoutInMicroseconds != 0);
  mMpSystemData.FinishCount       = 0;
  mMpSystemData.StartCount        = 0;
  mMpSystemData.SingleThread      = SingleThread;
  mMpSystemData.FailedList        = FailedCpuList;
  mMpSystemData.FailedListIndex   = 0;
  APInitialState                  = CpuStateReady;

  for (Number = 0; Number < mMpSystemData.NumberOfProcessors; Number++) {
    CpuData = &mMpSystemData.CpuDatas[Number];
    if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
      //
      // Skip BSP
      //
      continue;
    }

    if (!TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
      //
      // Skip Disabled processors
      //
      continue;
    }

    //
    // Get APs prepared, and put failing APs into FailedCpuList
    // if "SingleThread", only 1 AP will put to ready state, other AP will be put to ready
    // state 1 by 1, until the previous 1 finished its task
    // if not "SingleThread", all APs are put to ready state from the beginning
    //
    CpuState = GetApState (CpuData);
    if (CpuState == CpuStateIdle ||
        CpuState == CpuStateSleeping) {
      mMpSystemData.StartCount++;

      SetApState (CpuData, APInitialState);

      if (APInitialState == CpuStateReady) {
        SetApProcedure (CpuData, Procedure, ProcedureArgument);
        //
        // If this AP previous state is Sleeping, we should
        // wake up this AP by sent a SIPI. and avoid
        // re-involve the sleeping state. we must call
        // SetApProcedure() first.
        //
        if (CpuState == CpuStateSleeping) {
          ResetProcessorToIdleState (CpuData);
        }
      }

      if (SingleThread) {
        APInitialState = CpuStateBlocked;
      }
    }
  }

  mStopCheckAllAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    //
    // non blocking
    //
    return EFI_SUCCESS;
  }

  //
  // Blocking temporarily stop CheckAllAPsStatus()
  //
  mStopCheckAllAPsStatus = TRUE;

  while (TRUE) {
    CheckAndUpdateAllAPsToIdleState ();
    if (mMpSystemData.FinishCount == mMpSystemData.StartCount) {
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // task timeout
    //
    if (mMpSystemData.TimeoutActive && mMpSystemData.Timeout < 0) {
      ResetAllFailedAPs();
      Status = EFI_TIMEOUT;
      goto Done;
    }

    gBS->Stall (gPollInterval);
    mMpSystemData.Timeout -= gPollInterval;
  }

Done:

  return Status;
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
  CPU_STATE             CpuState;

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

  //
  // temporarily stop checkAllAPsStatus for avoid resource dead-lock.
  //
  mStopCheckAllAPsStatus = TRUE;

  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
  if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT) ||
      !TestCpuStatusFlag (CpuData, PROCESSOR_ENABLED_BIT)) {
    return EFI_INVALID_PARAMETER;
  }

  CpuState = GetApState (CpuData);
  if (CpuState != CpuStateIdle &&
      CpuState != CpuStateSleeping) {
    return EFI_NOT_READY;
  }

  SetApState (CpuData, CpuStateReady);

  SetApProcedure (CpuData, Procedure, ProcedureArgument);
  //
  // If this AP previous state is Sleeping, we should
  // wake up this AP by sent a SIPI. and avoid
  // re-involve the sleeping state. we must call
  // SetApProcedure() first.
  //
  if (CpuState == CpuStateSleeping) {
    ResetProcessorToIdleState (CpuData);
  }

  CpuData->Timeout = TimeoutInMicroseconds;
  CpuData->WaitEvent = WaitEvent;
  CpuData->TimeoutActive = (BOOLEAN) (TimeoutInMicroseconds != 0);
  CpuData->Finished = Finished;

  mStopCheckAllAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    //
    // Non Blocking
    //
    return EFI_SUCCESS;
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
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be performed
  by the current BSP.

  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. The new BSP can take over the
  execution of the old BSP and continue seamlessly from where the old one left
  off. This service may not be supported after the UEFI Event EFI_EVENT_GROUP_READY_TO_BOOT
  is signaled.

  If the BSP cannot be switched prior to the return from this service, then
  EFI_UNSUPPORTED must be returned.

  @param[in] This              A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to
                                  this service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_SUCCESS             The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or
                                  a disabled AP.
  @retval EFI_NOT_READY           The specified AP is busy.

**/
EFI_STATUS
EFIAPI
SwitchBSP (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
   //
   // Current always return unsupported.
   //
   return EFI_UNSUPPORTED;
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
  BOOLEAN        TempStopCheckState;
  CPU_STATE      CpuState;

  CpuData = NULL;
  TempStopCheckState = FALSE;

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= mMpSystemData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // temporarily stop checkAllAPsStatus for initialize parameters.
  //
  if (!mStopCheckAllAPsStatus) {
    mStopCheckAllAPsStatus = TRUE;
    TempStopCheckState = TRUE;
  }

  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
  if (TestCpuStatusFlag (CpuData, PROCESSOR_AS_BSP_BIT)) {
    return EFI_INVALID_PARAMETER;
  }

  CpuState = GetApState (CpuData);
  if (CpuState != CpuStateIdle &&
      CpuState != CpuStateSleeping) {
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

  if (TempStopCheckState) {
    mStopCheckAllAPsStatus = FALSE;
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
  ResetApStackless ((UINT32)CpuData->Info.ProcessorId);
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
  UINTN                 ProcessorNumber;
  CPU_DATA_BLOCK        *CpuData;
  EFI_AP_PROCEDURE      Procedure;
  volatile VOID         *ProcedureArgument;

  AsmApDoneWithCommonStack ();

  while (!mAPsAlreadyInitFinished) {
    CpuPause ();
  }

  WhoAmI (&mMpServicesTemplate, &ProcessorNumber);
  CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];

  //
  // Avoid forcibly reset AP caused the AP got lock not release.
  //
  if (CpuData->LockSelf == (INTN) GetApicId ()) {
    ReleaseSpinLock (&CpuData->CpuDataLock);
  }

  //
  // Avoid forcibly reset AP caused the timeout AP State is not
  // updated.
  //
  GetMpSpinLock (CpuData);
  if (CpuData->State == CpuStateBusy) {
    CpuData->Procedure = NULL;
  }
  CpuData->State = CpuStateIdle;
  ReleaseMpSpinLock (CpuData);

  while (TRUE) {
    GetMpSpinLock (CpuData);
    ProcedureArgument = CpuData->Parameter;
    Procedure = CpuData->Procedure;
    ReleaseMpSpinLock (CpuData);

    if (Procedure != NULL) {
      SetApState (CpuData, CpuStateBusy);

      Procedure ((VOID*) ProcedureArgument);

      GetMpSpinLock (CpuData);
      CpuData->Procedure = NULL;
      CpuData->State = CpuStateFinished;
      ReleaseMpSpinLock (CpuData);
    } else {
      //
      // if no procedure to execution, we simply put AP
      // into sleeping state, and waiting BSP sent SIPI.
      //
      GetMpSpinLock (CpuData);
      if (CpuData->State == CpuStateIdle) {
          CpuData->State = CpuStateSleeping;
      }
      ReleaseMpSpinLock (CpuData);
    }

    if (GetApState (CpuData) == CpuStateSleeping) {
      CpuSleep ();
    }

    CpuPause ();
  }

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
  CpuData->TimeoutActive = FALSE;
  gBS->SignalEvent (CpuData->WaitEvent);
  CpuData->WaitEvent = NULL;
}

/**
  Checks APs' status periodically.

  This function is triggerred by timer perodically to check the
  state of APs for StartupAllAPs() executed in non-blocking mode.

  @param  Event    Event triggered.
  @param  Context  Parameter passed with the event.

**/
VOID
EFIAPI
CheckAllAPsStatus (
  IN  EFI_EVENT        Event,
  IN  VOID             *Context
  )
{
  CPU_DATA_BLOCK *CpuData;
  UINTN          Number;
  EFI_STATUS     Status;

  if (mMpSystemData.TimeoutActive) {
    mMpSystemData.Timeout -= gPollInterval;
  }

  if (mStopCheckAllAPsStatus) {
    return;
  }

  //
  // avoid next timer enter.
  //
  Status = gBS->SetTimer (
                  mMpSystemData.CheckAllAPsEvent,
                  TimerCancel,
                  0
                  );
  ASSERT_EFI_ERROR (Status);

  if (mMpSystemData.WaitEvent != NULL) {
    CheckAndUpdateAllAPsToIdleState ();
    //
    // task timeout
    //
    if (mMpSystemData.TimeoutActive && mMpSystemData.Timeout < 0) {
      ResetAllFailedAPs();
      //
      // force exit
      //
      mMpSystemData.FinishCount = mMpSystemData.StartCount;
    }

    if (mMpSystemData.FinishCount != mMpSystemData.StartCount) {
      goto EXIT;
    }

    mMpSystemData.TimeoutActive = FALSE;
    gBS->SignalEvent (mMpSystemData.WaitEvent);
    mMpSystemData.WaitEvent = NULL;
    mStopCheckAllAPsStatus = TRUE;

    goto EXIT;
  }

  //
  // check each AP status for StartupThisAP
  //
  for (Number = 0; Number < mMpSystemData.NumberOfProcessors; Number++) {
    CpuData = &mMpSystemData.CpuDatas[Number];
    if (CpuData->WaitEvent) {
      CheckThisAPStatus (NULL, (VOID *)CpuData);
    }
  }

EXIT:
  Status = gBS->SetTimer (
                  mMpSystemData.CheckAllAPsEvent,
                  TimerPeriodic,
                  EFI_TIMER_PERIOD_MICROSECONDS (100)
                  );
  ASSERT_EFI_ERROR (Status);
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
  VOID*           TopOfApStack;
  UINTN           ProcessorNumber;

  if (!mAPsAlreadyInitFinished) {
    FillInProcessorInformation (FALSE, mMpSystemData.NumberOfProcessors);
    TopOfApStack  = (UINT8*)mApStackStart + gApStackSize;
    mApStackStart = TopOfApStack;

    //
    // Store the Stack address, when reset the AP, We can found the original address.
    //
    mMpSystemData.CpuDatas[mMpSystemData.NumberOfProcessors].TopOfStack = TopOfApStack;
    mMpSystemData.NumberOfProcessors++;
    mMpSystemData.NumberOfEnabledProcessors++;
  } else {
    WhoAmI (&mMpServicesTemplate, &ProcessorNumber);
    //
    // Get the original stack address.
    //
    TopOfApStack = mMpSystemData.CpuDatas[ProcessorNumber].TopOfStack;
  }

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
  CpuData->State = Bsp ? CpuStateBusy : CpuStateIdle;

  CpuData->Procedure        = NULL;
  CpuData->Parameter        = NULL;
  InitializeSpinLock (&CpuData->CpuDataLock);
  CpuData->LockSelf         = -1;

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
  EFI_STATUS     Status;

  ZeroMem (&mMpSystemData, sizeof (MP_SYSTEM_DATA));

  mMpSystemData.NumberOfProcessors = 1;
  mMpSystemData.NumberOfEnabledProcessors = 1;

  mMpSystemData.CpuDatas = AllocateZeroPool (sizeof (CPU_DATA_BLOCK) * gMaxLogicalProcessorNumber);
  ASSERT(mMpSystemData.CpuDatas != NULL);

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckAllAPsStatus,
                  NULL,
                  &mMpSystemData.CheckAllAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Set timer to check all APs status.
  //
  Status = gBS->SetTimer (
                  mMpSystemData.CheckAllAPsEvent,
                  TimerPeriodic,
                  EFI_TIMER_PERIOD_MICROSECONDS (100)
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // BSP
  //
  FillInProcessorInformation (TRUE, 0);

  return EFI_SUCCESS;
}

/**
  Collects BIST data from HOB.

  This function collects BIST data from HOB built from Sec Platform Information
  PPI or SEC Platform Information2 PPI.

**/
VOID
CollectBistDataFromHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE                     *GuidHob;
  EFI_SEC_PLATFORM_INFORMATION_RECORD2  *SecPlatformInformation2;
  EFI_SEC_PLATFORM_INFORMATION_RECORD   *SecPlatformInformation;
  UINTN                                 NumberOfData;
  EFI_SEC_PLATFORM_INFORMATION_CPU      *CpuInstance;
  EFI_SEC_PLATFORM_INFORMATION_CPU      BspCpuInstance;
  UINTN                                 ProcessorNumber;
  UINT32                                InitialLocalApicId;
  CPU_DATA_BLOCK                        *CpuData;

  SecPlatformInformation2 = NULL;
  SecPlatformInformation  = NULL;

  //
  // Get gEfiSecPlatformInformation2PpiGuid Guided HOB firstly
  //
  GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformation2PpiGuid);
  if (GuidHob != NULL) {
    //
    // Sec Platform Information2 PPI includes BSP/APs' BIST information
    //
    SecPlatformInformation2 = GET_GUID_HOB_DATA (GuidHob);
    NumberOfData = SecPlatformInformation2->NumberOfCpus;
    CpuInstance  = SecPlatformInformation2->CpuInstance;
  } else {
    //
    // Otherwise, get gEfiSecPlatformInformationPpiGuid Guided HOB
    //
    GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
    if (GuidHob != NULL) {
      SecPlatformInformation = GET_GUID_HOB_DATA (GuidHob);
      NumberOfData = 1;
      //
      // SEC Platform Information only includes BSP's BIST information
      // does not have BSP's APIC ID
      //
      BspCpuInstance.CpuLocation = GetApicId ();
      BspCpuInstance.InfoRecord.IA32HealthFlags.Uint32  = SecPlatformInformation->IA32HealthFlags.Uint32;
      CpuInstance = &BspCpuInstance;
    } else {
      DEBUG ((EFI_D_INFO, "Does not find any HOB stored CPU BIST information!\n"));
      //
      // Does not find any HOB stored BIST information
      //
      return;
    }
  }

  while ((NumberOfData--) > 0) {
    for (ProcessorNumber = 0; ProcessorNumber < mMpSystemData.NumberOfProcessors; ProcessorNumber++) {
      CpuData = &mMpSystemData.CpuDatas[ProcessorNumber];
      InitialLocalApicId = (UINT32) CpuData->Info.ProcessorId;
      if (InitialLocalApicId == CpuInstance[NumberOfData].CpuLocation) {
        //
        // Update CPU health status for MP Services Protocol according to BIST data.
        //
        if (CpuInstance[NumberOfData].InfoRecord.IA32HealthFlags.Uint32 != 0) {
          CpuData->Info.StatusFlag &= ~PROCESSOR_HEALTH_STATUS_BIT;
          //
          // Report Status Code that self test is failed
          //
          REPORT_STATUS_CODE (
            EFI_ERROR_CODE | EFI_ERROR_MAJOR,
            (EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_SELF_TEST)
            );
        }
      }
    }
  }
}

/**
  Callback function for ExitBootServices.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
ExitBootServicesCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  //
  // Avoid APs access invalid buff datas which allocated by BootServices,
  // so we send INIT IPI to APs to let them wait for SIPI state.
  //
  SendInitIpiAllExcludingSelf ();
}

/**
  Initialize Multi-processor support.

**/
VOID
InitializeMpSupport (
  VOID
  )
{
  EFI_STATUS Status;

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

  PrepareAPStartupCode ();

  StartApsStackless ();

  DEBUG ((DEBUG_INFO, "Detect CPU count: %d\n", mMpSystemData.NumberOfProcessors));
  if (mMpSystemData.NumberOfProcessors == 1) {
    FreeApStartupCode ();
    FreePages (mCommonStack, EFI_SIZE_TO_PAGES (gMaxLogicalProcessorNumber * gApStackSize));
    return;
  }

  mMpSystemData.CpuDatas = ReallocatePool (
                             sizeof (CPU_DATA_BLOCK) * gMaxLogicalProcessorNumber,
                             sizeof (CPU_DATA_BLOCK) * mMpSystemData.NumberOfProcessors,
                             mMpSystemData.CpuDatas);

  mAPsAlreadyInitFinished = TRUE;

  //
  // Update CPU healthy information from Guided HOB
  //
  CollectBistDataFromHob ();

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMpServiceHandle,
                  &gEfiMpServiceProtocolGuid,  &mMpServicesTemplate,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  if (mMpSystemData.NumberOfProcessors < gMaxLogicalProcessorNumber) {
    FreePages (mApStackStart, EFI_SIZE_TO_PAGES (
                                (gMaxLogicalProcessorNumber - mMpSystemData.NumberOfProcessors) *
                                gApStackSize));
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  ExitBootServicesCallback,
                  NULL,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);
}
