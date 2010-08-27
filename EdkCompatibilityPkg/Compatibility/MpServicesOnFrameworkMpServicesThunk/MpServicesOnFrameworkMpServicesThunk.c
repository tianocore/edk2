/** @file
Produces PI MP Services Protocol on top of Framework MP Services Protocol.

Intel's Framework MP Services Protocol is replaced by EFI_MP_SERVICES_PROTOCOL in PI 1.1.
This module produces PI MP Services Protocol on top of Framework MP Services Protocol.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include "MpServicesOnFrameworkMpServicesThunk.h"

EFI_HANDLE                          mHandle = NULL;
MP_SYSTEM_DATA                      mMPSystemData;
EFI_PHYSICAL_ADDRESS                mStartupVector;
MP_CPU_EXCHANGE_INFO                *mExchangeInfo;
BOOLEAN                             mStopCheckAPsStatus = FALSE;
UINTN                               mNumberOfProcessors;
EFI_GENERIC_MEMORY_TEST_PROTOCOL    *mGenMemoryTest;

FRAMEWORK_EFI_MP_SERVICES_PROTOCOL  *mFrameworkMpService;
EFI_MP_SERVICES_PROTOCOL            mMpService = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  StartupAllAPs,
  StartupThisAP,
  SwitchBSP,
  EnableDisableAP,
  WhoAmI
};


/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  @param  This                       A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  NumberOfProcessors         Pointer to the total number of logical processors in the system,
                                     including the BSP and disabled APs.
  @param  NumberOfEnabledProcessors  Pointer to the number of enabled logical processors that exist
                                     in system, including the BSP.

  @retval EFI_SUCCESS	             Number of logical processors and enabled logical processors retrieved..
  @retval EFI_DEVICE_ERROR           Caller processor is AP.
  @retval EFI_INVALID_PARAMETER      NumberOfProcessors is NULL
  @retval EFI_INVALID_PARAMETER      NumberOfEnabledProcessors is NULL

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL     *This,
  OUT UINTN                        *NumberOfProcessors,
  OUT UINTN                        *NumberOfEnabledProcessors
  )
{
  EFI_STATUS      Status;
  UINTN           CallerNumber;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != GetBspNumber ()) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter NumberOfProcessors
  //
  if (NumberOfProcessors == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check parameter NumberOfEnabledProcessors
  //
  if (NumberOfEnabledProcessors == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = mFrameworkMpService->GetGeneralMPInfo (
                                  mFrameworkMpService,
                                  NumberOfProcessors,
                                  NULL,
                                  NumberOfEnabledProcessors,
                                  NULL,
                                  NULL
                                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  ProcessorInfoBuffer   A pointer to the buffer where information for the requested processor is deposited.

  @retval EFI_SUCCESS           Processor information successfully returned.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_INVALID_PARAMETER ProcessorInfoBuffer is NULL
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.

**/
EFI_STATUS
EFIAPI
GetProcessorInfo (
  IN       EFI_MP_SERVICES_PROTOCOL     *This,
  IN       UINTN                        ProcessorNumber,
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  )
{
  EFI_STATUS                 Status;
  UINTN                      CallerNumber;
  UINTN                      BufferSize;
  EFI_MP_PROC_CONTEXT        ProcessorContextBuffer;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != GetBspNumber ()) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter ProcessorInfoBuffer
  //
  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mNumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof (EFI_MP_PROC_CONTEXT);
  Status = mFrameworkMpService->GetProcessorContext (
                                  mFrameworkMpService,
                                  ProcessorNumber,
                                  &BufferSize,
                                  &ProcessorContextBuffer
                                  );
  ASSERT_EFI_ERROR (Status);

  ProcessorInfoBuffer->ProcessorId = (UINT64) ProcessorContextBuffer.ApicID;

  //
  // Get Status Flag of specified processor
  //
  ProcessorInfoBuffer->StatusFlag = 0;

  if (ProcessorContextBuffer.Enabled) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_ENABLED_BIT;
  }

  if (ProcessorContextBuffer.Designation == EfiCpuBSP) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  if (ProcessorContextBuffer.Health.Flags.Uint32 == 0) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_HEALTH_STATUS_BIT;
  }

  ProcessorInfoBuffer->Location.Package = (UINT32) ProcessorContextBuffer.PackageNumber;
  ProcessorInfoBuffer->Location.Core    = (UINT32) ProcessorContextBuffer.NumberOfCores;
  ProcessorInfoBuffer->Location.Thread  = (UINT32) ProcessorContextBuffer.NumberOfThreads;

  return EFI_SUCCESS;
}

/**
  Implementation of StartupAllAPs() service of MP Services Protocol.

  This service lets the caller get all enabled APs to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on enabled APs of the system.
  @param  SingleThread          Indicates whether to execute the function simultaneously or one by one..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroSeconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  FailedCpuList         The list of processor numbers that fail to finish the function before
                                TimeoutInMicrosecsond expires.

  @retval EFI_SUCCESS           In blocking mode, all APs have finished before the timeout expired.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to all enabled APs.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_STARTED       No enabled AP exists in the system.
  @retval EFI_NOT_READY         Any enabled AP is busy.
  @retval EFI_TIMEOUT           In blocking mode, The timeout expired before all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupAllAPs (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  BOOLEAN                    SingleThread,
  IN  EFI_EVENT                  WaitEvent             OPTIONAL,
  IN  UINTN                      TimeoutInMicroSeconds,
  IN  VOID                       *ProcedureArgument    OPTIONAL,
  OUT UINTN                      **FailedCpuList       OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINTN           ProcessorNumber;
  CPU_DATA_BLOCK  *CpuData;
  BOOLEAN         Blocking;
  UINTN           BspNumber;

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
  }

  //
  // Check whether caller processor is BSP
  //
  BspNumber = GetBspNumber ();
  WhoAmI (This, &ProcessorNumber);
  if (ProcessorNumber != BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter Procedure
  //
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Temporarily suppress CheckAPsStatus()
  //
  mStopCheckAPsStatus = TRUE;

  //
  // Check whether all enabled APs are idle.
  // If any enabled AP is not idle, return EFI_NOT_READY.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    mMPSystemData.CpuList[ProcessorNumber] = FALSE;
    if (ProcessorNumber != BspNumber) {
      if (CpuData->State != CpuStateDisabled) {
        if (CpuData->State != CpuStateIdle) {
          mStopCheckAPsStatus = FALSE;
          return EFI_NOT_READY;
        } else {
          //
          // Mark this processor as responsible for current calling.
          //
          mMPSystemData.CpuList[ProcessorNumber] = TRUE;
        }
      }
    }
  }

  mMPSystemData.FinishCount = 0;
  mMPSystemData.StartCount  = 0;
  Blocking                  = FALSE;
  //
  // Go through all enabled APs to wakeup them for Procedure.
  // If in Single Thread mode, then only one AP is woken up, and others are waiting.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];
    //
    // Check whether this processor is responsible for current calling.
    //
    if (mMPSystemData.CpuList[ProcessorNumber]) {

      mMPSystemData.StartCount++;

      AcquireSpinLock (&CpuData->CpuDataLock);
      CpuData->State = CpuStateReady;
      ReleaseSpinLock (&CpuData->CpuDataLock);

      if (!Blocking) {
        WakeUpAp (
          ProcessorNumber,
          Procedure,
          ProcedureArgument
          );
      }

      if (SingleThread) {
        Blocking = TRUE;
      }
    }
  }

  //
  // If no enabled AP exists, return EFI_NOT_STARTED.
  //
  if (mMPSystemData.StartCount == 0) {
    mStopCheckAPsStatus = FALSE;
    return EFI_NOT_STARTED;
  }

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  mMPSystemData.Procedure      = Procedure;
  mMPSystemData.ProcArguments  = ProcedureArgument;
  mMPSystemData.SingleThread   = SingleThread;
  mMPSystemData.FailedCpuList  = FailedCpuList;
  mMPSystemData.ExpectedTime   = CalculateTimeout (TimeoutInMicroSeconds, &mMPSystemData.CurrentTime);
  mMPSystemData.WaitEvent      = WaitEvent;

  //
  // Allow CheckAPsStatus()
  //
  mStopCheckAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    return EFI_SUCCESS;
  }

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks APs'state until all APs finish or TimeoutInMicrosecsond expires.
  //
  do {
    Status = CheckAllAPs ();
  } while (Status == EFI_NOT_READY);

  return Status;
}

/**
  Implementation of StartupThisAP() service of MP Services Protocol.

  This service lets the caller get one enabled AP to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on the designated AP.
  @param  ProcessorNumber       The handle number of AP..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroseconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  Finished              Indicates whether AP has finished assigned function.
                                In blocking mode, it is ignored.

  @retval EFI_SUCCESS           In blocking mode, specified AP has finished before the timeout expires.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to specified AP.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_TIMEOUT           In blocking mode, the timeout expires before specified AP has finished.
  @retval EFI_NOT_READY         Specified AP is busy.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupThisAP (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  UINTN                      ProcessorNumber,
  IN  EFI_EVENT                  WaitEvent OPTIONAL,
  IN  UINTN                      TimeoutInMicroseconds,
  IN  VOID                       *ProcedureArgument OPTIONAL,
  OUT BOOLEAN                    *Finished OPTIONAL
  )
{
  CPU_DATA_BLOCK  *CpuData;
  UINTN           CallerNumber;
  EFI_STATUS      Status;
  UINTN           BspNumber;

  if (Finished != NULL) {
    *Finished = TRUE;
  }

  //
  // Check whether caller processor is BSP
  //
  BspNumber = GetBspNumber ();
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mNumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check parameter Procedure
  //
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  // Temporarily suppress CheckAPsStatus()
  //
  mStopCheckAPsStatus = TRUE;

  //
  // Check whether specified AP is disabled
  //
  if (CpuData->State == CpuStateDisabled) {
    mStopCheckAPsStatus = FALSE;
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (CpuData->State != CpuStateIdle) {
    mStopCheckAPsStatus = FALSE;
    return EFI_NOT_READY;
  }

  //
  // Wakeup specified AP for Procedure.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateReady;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  WakeUpAp (
    ProcessorNumber,
    Procedure,
    ProcedureArgument
    );

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  CpuData->WaitEvent      = WaitEvent;
  CpuData->Finished       = Finished;
  CpuData->ExpectedTime   = CalculateTimeout (TimeoutInMicroseconds, &CpuData->CurrentTime);

  //
  // Allow CheckAPsStatus()
  //
  mStopCheckAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    return EFI_SUCCESS;
  }

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks AP's state until it finishes or TimeoutInMicrosecsond expires.
  //
  do {
    Status = CheckThisAP (ProcessorNumber);
  } while (Status == EFI_NOT_READY);

  return Status;
}

/**
  Implementation of SwitchBSP() service of MP Services Protocol.

  This service switches the requested AP to be the BSP from that point onward.
  This service may only be called from the current BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  EnableOldBSP          Whether to enable or disable the original BSP.

  @retval EFI_SUCCESS           BSP successfully switched.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_NOT_READY         Specified AP is busy.

**/
EFI_STATUS
EFIAPI
SwitchBSP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             EnableOldBSP
  )
{
  EFI_STATUS            Status;
  CPU_DATA_BLOCK        *CpuData;
  UINTN                 CallerNumber;
  UINTN                 BspNumber;
  UINTN                 ApicBase;
  UINT32                CurrentTimerValue;
  UINT32                CurrentTimerRegister;
  UINT32                CurrentTimerDivide;
  UINT64                CurrentTscValue;
  BOOLEAN               OldInterruptState;

  //
  // Check whether caller processor is BSP
  //
  BspNumber = GetBspNumber ();
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mNumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  // Check whether specified AP is disabled
  //
  if (CpuData->State == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (CpuData->State != CpuStateIdle) {
    return EFI_NOT_READY;
  }

  //
  // Save and disable interrupt.
  //
  OldInterruptState = SaveAndDisableInterrupts ();
	
  //
  // Record the current local APIC timer setting of BSP
  //
  ApicBase = (UINTN)AsmMsrBitFieldRead64 (MSR_IA32_APIC_BASE, 12, 35) << 12;
  CurrentTimerValue     = MmioRead32 (ApicBase + APIC_REGISTER_TIMER_COUNT);
  CurrentTimerRegister  = MmioRead32 (ApicBase + APIC_REGISTER_LVT_TIMER);
  CurrentTimerDivide    = MmioRead32 (ApicBase + APIC_REGISTER_TIMER_DIVIDE);
  //
  // Set mask bit (BIT 16) of LVT Timer Register to disable its interrupt
  //
  MmioBitFieldWrite32 (ApicBase + APIC_REGISTER_LVT_TIMER, 16, 16, 1);

  //
  // Record the current TSC value
  //
  CurrentTscValue = AsmReadTsc ();
  
  Status = mFrameworkMpService->SwitchBSP (
                                  mFrameworkMpService,
                                  ProcessorNumber,
                                  EnableOldBSP
                                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Restore TSC value
  //
  AsmWriteMsr64 (MSR_IA32_TIME_STAMP_COUNTER, CurrentTscValue);

  //
  // Restore local APIC timer setting to new BSP
  //
  MmioWrite32 (ApicBase + APIC_REGISTER_TIMER_DIVIDE, CurrentTimerDivide);
  MmioWrite32 (ApicBase + APIC_REGISTER_TIMER_INIT_COUNT, CurrentTimerValue);
  MmioWrite32 (ApicBase + APIC_REGISTER_LVT_TIMER, CurrentTimerRegister);

  //
  // Restore interrupt state.
  //
  SetInterruptState (OldInterruptState);
  
  ChangeCpuState (BspNumber, EnableOldBSP);

  return EFI_SUCCESS;
}

/**
  Implementation of EnableDisableAP() service of MP Services Protocol.

  This service lets the caller enable or disable an AP.
  This service may only be called from the BSP.

  @param  This                   A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber        The handle number of processor.
  @param  EnableAP               Indicates whether the newstate of the AP is enabled or disabled.
  @param  HealthFlag             Indicates new health state of the AP..

  @retval EFI_SUCCESS            AP successfully enabled or disabled.
  @retval EFI_DEVICE_ERROR       Caller processor is AP.
  @retval EFI_NOT_FOUND          Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETERS ProcessorNumber specifies the BSP.

**/
EFI_STATUS
EFIAPI
EnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             EnableAP,
  IN  UINT32                              *HealthFlag OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINTN           CallerNumber;
  EFI_MP_HEALTH   HealthState;
  EFI_MP_HEALTH   *HealthStatePointer;
  UINTN           BspNumber;

  //
  // Check whether caller processor is BSP
  //
  BspNumber = GetBspNumber ();
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mNumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  if (HealthFlag == NULL) {
    HealthStatePointer = NULL;
  } else {
    if ((*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT) == 0) {
      HealthState.Flags.Uint32 = 1;
    } else {
      HealthState.Flags.Uint32 = 0;
    }
    HealthState.TestStatus = 0;

    HealthStatePointer = &HealthState;
  }

  Status = mFrameworkMpService->EnableDisableAP (
                                  mFrameworkMpService,
                                  ProcessorNumber,
                                  EnableAP,
                                  HealthStatePointer
                                  );
  ASSERT_EFI_ERROR (Status);

  ChangeCpuState (ProcessorNumber, EnableAP);

  return EFI_SUCCESS;
}

/**
  Implementation of WhoAmI() service of MP Services Protocol.

  This service lets the caller processor get its handle number.
  This service may be called from the BSP and APs.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       Pointer to the handle number of AP.

  @retval EFI_SUCCESS           Processor number successfully returned.
  @retval EFI_INVALID_PARAMETER ProcessorNumber is NULL

**/
EFI_STATUS
EFIAPI
WhoAmI (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  OUT UINTN                               *ProcessorNumber
  )
{
  EFI_STATUS  Status;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = mFrameworkMpService->WhoAmI (
                                  mFrameworkMpService,
                                  ProcessorNumber
                                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Checks APs' status periodically.

  This function is triggerred by timer perodically to check the
  state of APs for StartupAllAPs() and StartupThisAP() executed
  in non-blocking mode.

  @param  Event    Event triggered.
  @param  Context  Parameter passed with the event.

**/
VOID
EFIAPI
CheckAPsStatus (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  UINTN           ProcessorNumber;
  CPU_DATA_BLOCK  *CpuData;
  EFI_STATUS      Status;

  //
  // If CheckAPsStatus() is stopped, then return immediately.
  //
  if (mStopCheckAPsStatus) {
    return;
  }

  //
  // First, check whether pending StartupAllAPs() exists.
  //
  if (mMPSystemData.WaitEvent != NULL) {

    Status = CheckAllAPs ();
    //
    // If all APs finish for StartupAllAPs(), signal the WaitEvent for it..
    //
    if (Status != EFI_NOT_READY) {
      Status = gBS->SignalEvent (mMPSystemData.WaitEvent);
      mMPSystemData.WaitEvent = NULL;
    }
  }

  //
  // Second, check whether pending StartupThisAPs() callings exist.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    if (CpuData->WaitEvent == NULL) {
      continue;
    }

    Status = CheckThisAP (ProcessorNumber);

    if (Status != EFI_NOT_READY) {
      gBS->SignalEvent (CpuData->WaitEvent);
      CpuData->WaitEvent = NULL;
    }
  }
  return ;
}

/**
  Checks status of all APs.

  This function checks whether all APs have finished task assigned by StartupAllAPs(),
  and whether timeout expires.

  @retval EFI_SUCCESS           All APs have finished task assigned by StartupAllAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         APs have not finished task and timeout has not expired.

**/
EFI_STATUS
CheckAllAPs (
  VOID
  )
{
  UINTN           ProcessorNumber;
  UINTN           NextProcessorNumber;
  UINTN           ListIndex;
  EFI_STATUS      Status;
  CPU_STATE       CpuState;
  CPU_DATA_BLOCK  *CpuData;

  NextProcessorNumber = 0;

  //
  // Go through all APs that are responsible for the StartupAllAPs().
  //
  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {
    if (!mMPSystemData.CpuList[ProcessorNumber]) {
      continue;
    }

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    //
    //  Check the CPU state of AP. If it is CpuStateFinished, then the AP has finished its task.
    //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
    //  value of state after setting the it to CpuStateFinished, so BSP can safely make use of its value.
    //
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuState = CpuData->State;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    if (CpuState == CpuStateFinished) {
      mMPSystemData.FinishCount++;
      mMPSystemData.CpuList[ProcessorNumber] = FALSE;

      AcquireSpinLock (&CpuData->CpuDataLock);
      CpuData->State = CpuStateIdle;
      ReleaseSpinLock (&CpuData->CpuDataLock);

      //
      // If in Single Thread mode, then search for the next waiting AP for execution.
      //
      if (mMPSystemData.SingleThread) {
        Status = GetNextWaitingProcessorNumber (&NextProcessorNumber);

        if (!EFI_ERROR (Status)) {
          WakeUpAp (
            NextProcessorNumber,
            mMPSystemData.Procedure,
            mMPSystemData.ProcArguments
            );
        }
      }
    }
  }

  //
  // If all APs finish, return EFI_SUCCESS.
  //
  if (mMPSystemData.FinishCount == mMPSystemData.StartCount) {
    return EFI_SUCCESS;
  }

  //
  // If timeout expires, report timeout.
  //
  if (CheckTimeout (&mMPSystemData.CurrentTime, &mMPSystemData.TotalTime, mMPSystemData.ExpectedTime)) {
    //
    // If FailedCpuList is not NULL, record all failed APs in it.
    //
    if (mMPSystemData.FailedCpuList != NULL) {
      *mMPSystemData.FailedCpuList = AllocatePool ((mMPSystemData.StartCount - mMPSystemData.FinishCount + 1) * sizeof(UINTN));
      ASSERT (*mMPSystemData.FailedCpuList != NULL);
    }
    ListIndex = 0;

    for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {
      //
      // Check whether this processor is responsible for StartupAllAPs().
      //
      if (mMPSystemData.CpuList[ProcessorNumber]) {
        //
        // Reset failed APs to idle state
        //
        ResetProcessorToIdleState (ProcessorNumber);
        mMPSystemData.CpuList[ProcessorNumber] = FALSE;
        if (mMPSystemData.FailedCpuList != NULL) {
          (*mMPSystemData.FailedCpuList)[ListIndex++] = ProcessorNumber;
        }
      }
    }
    if (mMPSystemData.FailedCpuList != NULL) {
      (*mMPSystemData.FailedCpuList)[ListIndex] = END_OF_CPU_LIST;
    }
    return EFI_TIMEOUT;
  }
  return EFI_NOT_READY;
}

/**
  Checks status of specified AP.

  This function checks whether specified AP has finished task assigned by StartupThisAP(),
  and whether timeout expires.

  @param  ProcessorNumber       The handle number of processor.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.

**/
EFI_STATUS
CheckThisAP (
  UINTN  ProcessorNumber
  )
{
  CPU_DATA_BLOCK  *CpuData;
  CPU_STATE       CpuState;

  ASSERT (ProcessorNumber < mNumberOfProcessors);
  ASSERT (ProcessorNumber < MAX_CPU_NUMBER);

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  //  Check the CPU state of AP. If it is CpuStateFinished, then the AP has finished its task.
  //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
  //  value of state after setting the it to CpuStateFinished, so BSP can safely make use of its value.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuState = CpuData->State;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  //
  // If the APs finishes for StartupThisAP(), return EFI_SUCCESS.
  //
  if (CpuState == CpuStateFinished) {

    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateIdle;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    if (CpuData->Finished != NULL) {
      *(CpuData->Finished) = TRUE;
    }
    return EFI_SUCCESS;
  } else {
    //
    // If timeout expires for StartupThisAP(), report timeout.
    //
    if (CheckTimeout (&CpuData->CurrentTime, &CpuData->TotalTime, CpuData->ExpectedTime)) {

      if (CpuData->Finished != NULL) {
        *(CpuData->Finished) = FALSE;
      }
      //
      // Reset failed AP to idle state
      //
      ResetProcessorToIdleState (ProcessorNumber);

      return EFI_TIMEOUT;
    }
  }
  return EFI_NOT_READY;
}

/**
  Calculate timeout value and return the current performance counter value.

  Calculate the number of performance counter ticks required for a timeout.
  If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  as infinity.

  @param  TimeoutInMicroseconds   Timeout value in microseconds.
  @param  CurrentTime             Returns the current value of the performance counter.

  @return Expected timestamp counter for timeout.
          If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
          as infinity.

**/
UINT64
CalculateTimeout (
  IN  UINTN   TimeoutInMicroseconds,
  OUT UINT64  *CurrentTime
  )
{
  //
  // Read the current value of the performance counter
  //
  *CurrentTime = GetPerformanceCounter ();

  //
  // If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  // as infinity.
  //
  if (TimeoutInMicroseconds == 0) {
    return 0;
  }

  //
  // GetPerformanceCounterProperties () returns the timestamp counter's frequency
  // in Hz. So multiply the return value with TimeoutInMicroseconds and then divide
  // it by 1,000,000, to get the number of ticks for the timeout value.
  //
  return DivU64x32 (
           MultU64x64 (
             GetPerformanceCounterProperties (NULL, NULL),
             TimeoutInMicroseconds
             ),
           1000000
           );
}

/**
  Checks whether timeout expires.

  Check whether the number of ellapsed performance counter ticks required for a timeout condition
  has been reached.  If Timeout is zero, which means infinity, return value is always FALSE.

  @param  PreviousTime         On input, the value of the performance counter when it was last read.
                               On output, the current value of the performance counter
  @param  TotalTime            The total amount of ellapsed time in performance counter ticks.
  @param  Timeout              The number of performance counter ticks required to reach a timeout condition.

  @retval TRUE                 A timeout condition has been reached.
  @retval FALSE                A timeout condition has not been reached.

**/
BOOLEAN
CheckTimeout (
  IN OUT UINT64  *PreviousTime,
  IN     UINT64  *TotalTime,
  IN     UINT64  Timeout
  )
{
  UINT64  Start;
  UINT64  End;
  UINT64  CurrentTime;
  INT64   Delta;
  INT64   Cycle;

  if (Timeout == 0) {
    return FALSE;
  }
  GetPerformanceCounterProperties (&Start, &End);
  Cycle = End - Start;
  if (Cycle < 0) {
    Cycle = -Cycle;
  }
  Cycle++;
  CurrentTime = GetPerformanceCounter();
  Delta = (INT64) (CurrentTime - *PreviousTime);
  if (Start > End) {
    Delta = -Delta;
  }
  if (Delta < 0) {
    Delta += Cycle;
  }
  *TotalTime += Delta;
  *PreviousTime = CurrentTime;
  if (*TotalTime > Timeout) {
    return TRUE;
  }
  return FALSE;
}

/**
  Searches for the next waiting AP.

  Search for the next AP that is put in waiting state by single-threaded StartupAllAPs().

  @param  NextProcessorNumber  Pointer to the processor number of the next waiting AP.

  @retval EFI_SUCCESS          The next waiting AP has been found.
  @retval EFI_NOT_FOUND        No waiting AP exists.

**/
EFI_STATUS
GetNextWaitingProcessorNumber (
  OUT UINTN                               *NextProcessorNumber
  )
{
  UINTN           ProcessorNumber;

  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {

    if (mMPSystemData.CpuList[ProcessorNumber]) {
      *NextProcessorNumber = ProcessorNumber;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Wrapper function for all procedures assigned to AP.

  Wrapper function for all procedures assigned to AP via MP service protocol.
  It controls states of AP and invokes assigned precedure.

**/
VOID
ApProcWrapper (
  VOID
  )
{
  EFI_AP_PROCEDURE      Procedure;
  VOID                  *Parameter;
  UINTN                 ProcessorNumber;
  CPU_DATA_BLOCK        *CpuData;

  //
  // Program virtual wire mode for AP, since it will be lost after AP wake up
  //
  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();

  //
  // Initialize Debug Agent to support source level debug on AP code.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_AP, NULL, NULL);

  WhoAmI (&mMpService, &ProcessorNumber);
  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateBusy;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  //
  // Now let us check it out.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  Procedure = CpuData->Procedure;
  Parameter = CpuData->Parameter;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  if (Procedure != NULL) {

    Procedure (Parameter);

    //
    // if BSP is switched to AP, it continue execute from here, but it carries register state
    // of the old AP, so need to reload CpuData (might be stored in a register after compiler
    // optimization) to make sure it points to the right data
    //
    WhoAmI (&mMpService, &ProcessorNumber);
    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->Procedure = NULL;
    ReleaseSpinLock (&CpuData->CpuDataLock);
  }

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateFinished;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Function to wake up a specified AP and assign procedure to it.

  @param  ProcessorNumber  Handle number of the specified processor.
  @param  Procedure        Procedure to assign.
  @param  ProcArguments    Argument for Procedure.

**/
VOID
WakeUpAp (
  IN   UINTN                 ProcessorNumber,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *ProcArguments
  )
{
  EFI_STATUS                   Status;
  CPU_DATA_BLOCK               *CpuData;
  EFI_PROCESSOR_INFORMATION    ProcessorInfoBuffer;

  ASSERT (ProcessorNumber < mNumberOfProcessors);
  ASSERT (ProcessorNumber < MAX_CPU_NUMBER);

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->Parameter  = ProcArguments;
  CpuData->Procedure  = Procedure;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  Status = GetProcessorInfo (
             &mMpService,
             ProcessorNumber,
             &ProcessorInfoBuffer
             );
  ASSERT_EFI_ERROR (Status);

  mExchangeInfo->ApFunction = (VOID *) (UINTN) ApProcWrapper;
  mExchangeInfo->ProcessorNumber[ProcessorInfoBuffer.ProcessorId] = (UINT32) ProcessorNumber;
  SendInitSipiSipi (
    (UINT32) ProcessorInfoBuffer.ProcessorId,
    (UINT32) (UINTN) mStartupVector
    );
}

/**
  Terminate AP's task and set it to idle state.

  This function terminates AP's task due to timeout by sending INIT-SIPI,
  and sends it to idle state.

  @param  ProcessorNumber  Handle number of the specified processor.

**/
VOID
ResetProcessorToIdleState (
  UINTN      ProcessorNumber
  )
{
  EFI_STATUS                   Status;
  CPU_DATA_BLOCK               *CpuData;
  EFI_PROCESSOR_INFORMATION    ProcessorInfoBuffer;

  Status = GetProcessorInfo (
             &mMpService,
             ProcessorNumber,
             &ProcessorInfoBuffer
             );
  ASSERT_EFI_ERROR (Status);

  mExchangeInfo->ApFunction = NULL;
  mExchangeInfo->ProcessorNumber[ProcessorInfoBuffer.ProcessorId] = (UINT32) ProcessorNumber;
  SendInitSipiSipi (
    (UINT32) ProcessorInfoBuffer.ProcessorId,
    (UINT32) (UINTN) mStartupVector
    );

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateIdle;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Worker function of EnableDisableAP ()

  Worker function of EnableDisableAP (). Changes state of specified processor.

  @param  ProcessorNumber Processor number of specified AP.
  @param  NewState        Desired state of the specified AP.

  @retval EFI_SUCCESS     AP's state successfully changed.

**/
EFI_STATUS
ChangeCpuState (
  IN     UINTN                      ProcessorNumber,
  IN     BOOLEAN                    NewState
  )
{
  CPU_DATA_BLOCK  *CpuData;

  ASSERT (ProcessorNumber < mNumberOfProcessors);
  ASSERT (ProcessorNumber < MAX_CPU_NUMBER);

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  if (!NewState) {
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateDisabled;
    ReleaseSpinLock (&CpuData->CpuDataLock);
  } else {
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateIdle;
    ReleaseSpinLock (&CpuData->CpuDataLock);
  }

  return EFI_SUCCESS;
}

/**
  Test memory region of EfiGcdMemoryTypeReserved.

  @param  Length           The length of memory region to test.

  @retval EFI_SUCCESS      The memory region passes test.
  @retval EFI_NOT_FOUND    The memory region is not reserved memory.
  @retval EFI_DEVICE_ERROR The memory fails on test.

**/
EFI_STATUS
TestReservedMemory (
  UINTN  Length
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;
  EFI_PHYSICAL_ADDRESS             Address;
  UINTN                            LengthCovered;
  UINTN                            RemainingLength;

  //
  // Walk through the memory descriptors covering the memory range.
  //
  Address         = mStartupVector;
  RemainingLength = Length;
  while (Address < mStartupVector + Length) {
    Status = gDS->GetMemorySpaceDescriptor(
                    Address,
                    &Descriptor
                    );
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
      return EFI_NOT_FOUND;
    }
    //
    // Calculated the length of the intersected range.
    //
    LengthCovered = (UINTN) (Descriptor.BaseAddress + Descriptor.Length - Address);
    if (LengthCovered > RemainingLength) {
      LengthCovered = RemainingLength;
    }

    Status = mGenMemoryTest->CompatibleRangeTest (
                               mGenMemoryTest,
                               Address,
                               LengthCovered
                               );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Address         += LengthCovered;
    RemainingLength -= LengthCovered;
  }

  return EFI_SUCCESS;
}

/**
  Allocates startup vector for APs.

  This function allocates Startup vector for APs.

  @param  Size  The size of startup vector.

**/
VOID
AllocateStartupVector (
  UINTN   Size
  )
{
  EFI_STATUS                            Status;

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **) &mGenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    mGenMemoryTest = NULL;
  }

  for (mStartupVector = 0x7F000; mStartupVector >= 0x2000; mStartupVector -= EFI_PAGE_SIZE) {
    if (mGenMemoryTest != NULL) {
      //
      // Test memory if it is EfiGcdMemoryTypeReserved.
      //
      Status = TestReservedMemory (EFI_SIZE_TO_PAGES (Size) * EFI_PAGE_SIZE);
      if (Status == EFI_DEVICE_ERROR) {
        continue;
      }
    }

    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiBootServicesCode,
                    EFI_SIZE_TO_PAGES (Size),
                    &mStartupVector
                    );

    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  Prepares Startup Vector for APs.

  This function prepares Startup Vector for APs.

**/
VOID
PrepareAPStartupVector (
  VOID
  )
{
  MP_ASSEMBLY_ADDRESS_MAP   AddressMap;
  IA32_DESCRIPTOR           GdtrForBSP;
  IA32_DESCRIPTOR           IdtrForBSP;
  EFI_PHYSICAL_ADDRESS      GdtForAP;
  EFI_PHYSICAL_ADDRESS      IdtForAP;
  EFI_STATUS                Status;

  //
  // Get the address map of startup code for AP,
  // including code size, and offset of long jump instructions to redirect.
  //
  AsmGetAddressMap (&AddressMap);

  //
  // Allocate a 4K-aligned region under 1M for startup vector for AP.
  // The region contains AP startup code and exchange data between BSP and AP.
  //
  AllocateStartupVector (AddressMap.Size + sizeof (MP_CPU_EXCHANGE_INFO));

  //
  // Copy AP startup code to startup vector, and then redirect the long jump
  // instructions for mode switching.
  //
  CopyMem ((VOID *) (UINTN) mStartupVector, AddressMap.RendezvousFunnelAddress, AddressMap.Size);
  *(UINT32 *) (UINTN) (mStartupVector + AddressMap.FlatJumpOffset + 3) = (UINT32) (mStartupVector + AddressMap.PModeEntryOffset);
  //
  // For IA32 mode, LongJumpOffset is filled with zero. If non-zero, then we are in X64 mode, so further redirect for long mode switch.
  //
  if (AddressMap.LongJumpOffset != 0) {
    *(UINT32 *) (UINTN) (mStartupVector + AddressMap.LongJumpOffset + 2) = (UINT32) (mStartupVector + AddressMap.LModeEntryOffset);
  }

  //
  // Get the start address of exchange data between BSP and AP.
  //
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (mStartupVector + AddressMap.Size);

  ZeroMem ((VOID *) mExchangeInfo, sizeof (MP_CPU_EXCHANGE_INFO));

  mExchangeInfo->StackStart  = AllocatePages (EFI_SIZE_TO_PAGES (mNumberOfProcessors * AP_STACK_SIZE));
  mExchangeInfo->StackSize  = AP_STACK_SIZE;

  AsmReadGdtr (&GdtrForBSP);
  AsmReadIdtr (&IdtrForBSP);

  //
  // Allocate memory under 4G to hold GDT for APs
  //
  GdtForAP = 0xffffffff;
  Status   = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiBootServicesData,
                    EFI_SIZE_TO_PAGES ((GdtrForBSP.Limit + 1) + (IdtrForBSP.Limit + 1)),
                    &GdtForAP
                    );
  ASSERT_EFI_ERROR (Status);

  IdtForAP = (UINTN) GdtForAP + GdtrForBSP.Limit + 1;

  CopyMem ((VOID *) (UINTN) GdtForAP, (VOID *) GdtrForBSP.Base, GdtrForBSP.Limit + 1);
  CopyMem ((VOID *) (UINTN) IdtForAP, (VOID *) IdtrForBSP.Base, IdtrForBSP.Limit + 1);

  mExchangeInfo->GdtrProfile.Base  = (UINTN) GdtForAP;
  mExchangeInfo->GdtrProfile.Limit = GdtrForBSP.Limit;
  mExchangeInfo->IdtrProfile.Base  = (UINTN) IdtForAP;
  mExchangeInfo->IdtrProfile.Limit = IdtrForBSP.Limit;

  mExchangeInfo->BufferStart = (UINT32) mStartupVector;
  mExchangeInfo->Cr3         = (UINT32) (AsmReadCr3 ());
}

/**
  Prepares memory region for processor configuration.

  This function prepares memory region for processor configuration.

**/
VOID
PrepareMemoryForConfiguration (
  VOID
  )
{
  UINTN                Index;

  //
  // Initialize Spin Locks for system
  //
  InitializeSpinLock (&mMPSystemData.APSerializeLock);
  for (Index = 0; Index < MAX_CPU_NUMBER; Index++) {
    InitializeSpinLock (&mMPSystemData.CpuData[Index].CpuDataLock);
  }

  PrepareAPStartupVector ();
}

/**
  Gets the processor number of BSP.

  @return  The processor number of BSP.

**/
UINTN
GetBspNumber (
  VOID
  )
{
  UINTN                      ProcessorNumber;
  EFI_MP_PROC_CONTEXT        ProcessorContextBuffer;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;

  BufferSize = sizeof (EFI_MP_PROC_CONTEXT);

  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {
    Status = mFrameworkMpService->GetProcessorContext (
                                    mFrameworkMpService,
                                    ProcessorNumber,
                                    &BufferSize,
                                    &ProcessorContextBuffer
                                    );
    ASSERT_EFI_ERROR (Status);

    if (ProcessorContextBuffer.Designation == EfiCpuBSP) {
      break;
    }
  }
  ASSERT (ProcessorNumber < mNumberOfProcessors);

  return ProcessorNumber;
}

/**
  Entrypoint of MP Services Protocol thunk driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
InitializeMpServicesProtocol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  //
  // Locates Framework version MP Services Protocol
  //
  Status = gBS->LocateProtocol (
                  &gFrameworkEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **) &mFrameworkMpService
                  );
  ASSERT_EFI_ERROR (Status);

  Status = mFrameworkMpService->GetGeneralMPInfo (
                                  mFrameworkMpService,
                                  &mNumberOfProcessors,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL
                                  );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mNumberOfProcessors < MAX_CPU_NUMBER);

  PrepareMemoryForConfiguration ();

  //
  // Create timer event to check AP state for non-blocking execution.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckAPsStatus,
                  NULL,
                  &mMPSystemData.CheckAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Now install the MP services protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiMpServiceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpService
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Launch the timer event to check AP state.
  //
  Status = gBS->SetTimer (
                  mMPSystemData.CheckAPsEvent,
                  TimerPeriodic,
                  100000
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
