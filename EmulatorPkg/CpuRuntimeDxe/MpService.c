/** @file
  Construct MP Services Protocol on top of the EMU Thread protocol.
  This code makes APs show up in the emulator. PcdEmuApCount is the
  number of APs the emulator should produce.

  The MP Services Protocol provides a generalized way of performing following tasks:
    - Retrieving information of multi-processor environment and MP-related status of
      specific processors.
    - Dispatching user-provided function to APs.
    - Maintain MP-related processor status.

  The MP Services Protocol must be produced on any system with more than one logical
  processor.

  The Protocol is available only during boot time.

  MP Services Protocol is hardware-independent. Most of the logic of this protocol
  is architecturally neutral. It abstracts the multi-processor environment and
  status of processors, and provides interfaces to retrieve information, maintain,
  and dispatch.

  MP Services Protocol may be consumed by ACPI module. The ACPI module may use this
  protocol to retrieve data that are needed for an MP platform and report them to OS.
  MP Services Protocol may also be used to program and configure processors, such
  as MTRR synchronization for memory space attributes setting in DXE Services.
  MP Services Protocol may be used by non-CPU DXE drivers to speed up platform boot
  by taking advantage of the processing capabilities of the APs, for example, using
  APs to help test system memory in parallel with other device initialization.
  Diagnostics applications may also use this protocol for multi-processor.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
Portitions Copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "CpuDriver.h"

MP_SYSTEM_DATA             gMPSystem;
EMU_THREAD_THUNK_PROTOCOL  *gThread = NULL;
EFI_EVENT                  gReadToBootEvent;
BOOLEAN                    gReadToBoot = FALSE;
UINTN                      gPollInterval;

BOOLEAN
IsBSP (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       ProcessorNumber;

  Status = CpuMpServicesWhoAmI (&mMpServicesTemplate, &ProcessorNumber);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return (gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0;
}

VOID
SetApProcedure (
  IN   PROCESSOR_DATA_BLOCK  *Processor,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *ProcedureArgument
  )
{
  gThread->MutexLock (Processor->ProcedureLock);
  Processor->Parameter = ProcedureArgument;
  Processor->Procedure = Procedure;
  gThread->MutexUnlock (Processor->ProcedureLock);
}

EFI_STATUS
GetNextBlockedNumber (
  OUT UINTN  *NextNumber
  )
{
  UINTN                 Number;
  PROCESSOR_STATE       ProcessorState;
  PROCESSOR_DATA_BLOCK  *Data;

  for (Number = 0; Number < gMPSystem.NumberOfProcessors; Number++) {
    Data = &gMPSystem.ProcessorData[Number];
    if ((Data->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
      // Skip BSP
      continue;
    }

    gThread->MutexLock (Data->StateLock);
    ProcessorState = Data->State;
    gThread->MutexUnlock (Data->StateLock);

    if (ProcessorState == CPU_STATE_BLOCKED) {
      *NextNumber = Number;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
 * Calculated and stalled the interval time by BSP to check whether
 * the APs have finished.
 *
 * @param[in]  Timeout    The time limit in microseconds for
 *                        APs to return from Procedure.
 *
 * @retval     StallTime  Time of execution stall.
**/
UINTN
CalculateAndStallInterval (
  IN UINTN  Timeout
  )
{
  UINTN  StallTime;

  if ((Timeout < gPollInterval) && (Timeout != 0)) {
    StallTime = Timeout;
  } else {
    StallTime = gPollInterval;
  }

  gBS->Stall (StallTime);

  return StallTime;
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
CpuMpServicesGetNumberOfProcessors (
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

  *NumberOfProcessors        = gMPSystem.NumberOfProcessors;
  *NumberOfEnabledProcessors = gMPSystem.NumberOfEnabledProcessors;
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
CpuMpServicesGetProcessorInfo (
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

  if (ProcessorNumber >= gMPSystem.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CopyMem (ProcessorInfoBuffer, &gMPSystem.ProcessorData[ProcessorNumber], sizeof (EFI_PROCESSOR_INFORMATION));
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
  @param[in]  TimeoutInMicrosecsond   Indicates the time limit in microseconds for
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
CpuMpServicesStartupAllAps (
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
  PROCESSOR_DATA_BLOCK  *ProcessorData;
  UINTN                 Number;
  UINTN                 NextNumber;
  PROCESSOR_STATE       APInitialState;
  PROCESSOR_STATE       ProcessorState;
  UINTN                 Timeout;

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (gMPSystem.NumberOfProcessors == 1) {
    return EFI_NOT_STARTED;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((WaitEvent != NULL)  && gReadToBoot) {
    return EFI_UNSUPPORTED;
  }

  for (Number = 0; Number < gMPSystem.NumberOfProcessors; Number++) {
    ProcessorData = &gMPSystem.ProcessorData[Number];
    if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
      // Skip BSP
      continue;
    }

    if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
      // Skip Disabled processors
      continue;
    }

    gThread->MutexLock (ProcessorData->StateLock);
    if (ProcessorData->State != CPU_STATE_IDLE) {
      gThread->MutexUnlock (ProcessorData->StateLock);
      return EFI_NOT_READY;
    }

    gThread->MutexUnlock (ProcessorData->StateLock);
  }

  if (FailedCpuList != NULL) {
    gMPSystem.FailedList = AllocatePool ((gMPSystem.NumberOfProcessors + 1) * sizeof (UINTN));
    if (gMPSystem.FailedList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    SetMemN (gMPSystem.FailedList, (gMPSystem.NumberOfProcessors + 1) * sizeof (UINTN), END_OF_CPU_LIST);
    gMPSystem.FailedListIndex = 0;
    *FailedCpuList            = gMPSystem.FailedList;
  }

  Timeout = TimeoutInMicroseconds;

  ProcessorData = NULL;

  gMPSystem.FinishCount  = 0;
  gMPSystem.StartCount   = 0;
  gMPSystem.SingleThread = SingleThread;
  APInitialState         = CPU_STATE_READY;

  for (Number = 0; Number < gMPSystem.NumberOfProcessors; Number++) {
    ProcessorData = &gMPSystem.ProcessorData[Number];

    if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
      // Skip BSP
      continue;
    }

    if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
      // Skip Disabled processors
      gMPSystem.FailedList[gMPSystem.FailedListIndex++] = Number;
      continue;
    }

    //
    // Get APs prepared, and put failing APs into FailedCpuList
    // if "SingleThread", only 1 AP will put to ready state, other AP will be put to ready
    // state 1 by 1, until the previous 1 finished its task
    // if not "SingleThread", all APs are put to ready state from the beginning
    //
    gThread->MutexLock (ProcessorData->StateLock);
    ASSERT (ProcessorData->State == CPU_STATE_IDLE);
    ProcessorData->State = APInitialState;
    gThread->MutexUnlock (ProcessorData->StateLock);

    gMPSystem.StartCount++;
    if (SingleThread) {
      APInitialState = CPU_STATE_BLOCKED;
    }
  }

  if (WaitEvent != NULL) {
    for (Number = 0; Number < gMPSystem.NumberOfProcessors; Number++) {
      ProcessorData = &gMPSystem.ProcessorData[Number];
      if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
        // Skip BSP
        continue;
      }

      if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
        // Skip Disabled processors
        continue;
      }

      gThread->MutexLock (ProcessorData->StateLock);
      ProcessorState = ProcessorData->State;
      gThread->MutexUnlock (ProcessorData->StateLock);

      if (ProcessorState == CPU_STATE_READY) {
        SetApProcedure (ProcessorData, Procedure, ProcedureArgument);
      }
    }

    //
    // Save data into private data structure, and create timer to poll AP state before exiting
    //
    gMPSystem.Procedure         = Procedure;
    gMPSystem.ProcedureArgument = ProcedureArgument;
    gMPSystem.WaitEvent         = WaitEvent;
    gMPSystem.Timeout           = TimeoutInMicroseconds;
    gMPSystem.TimeoutActive     = (BOOLEAN)(TimeoutInMicroseconds != 0);
    Status                      = gBS->SetTimer (
                                         gMPSystem.CheckAllAPsEvent,
                                         TimerPeriodic,
                                         gPollInterval
                                         );
    return Status;
  }

  while (TRUE) {
    for (Number = 0; Number < gMPSystem.NumberOfProcessors; Number++) {
      ProcessorData = &gMPSystem.ProcessorData[Number];
      if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
        // Skip BSP
        continue;
      }

      if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
        // Skip Disabled processors
        continue;
      }

      gThread->MutexLock (ProcessorData->StateLock);
      ProcessorState = ProcessorData->State;
      gThread->MutexUnlock (ProcessorData->StateLock);

      switch (ProcessorState) {
        case CPU_STATE_READY:
          SetApProcedure (ProcessorData, Procedure, ProcedureArgument);
          break;

        case CPU_STATE_FINISHED:
          gMPSystem.FinishCount++;
          if (SingleThread) {
            Status = GetNextBlockedNumber (&NextNumber);
            if (!EFI_ERROR (Status)) {
              gThread->MutexLock (gMPSystem.ProcessorData[NextNumber].StateLock);
              gMPSystem.ProcessorData[NextNumber].State = CPU_STATE_READY;
              gThread->MutexUnlock (gMPSystem.ProcessorData[NextNumber].StateLock);
            }
          }

          gThread->MutexLock (ProcessorData->StateLock);
          ProcessorData->State = CPU_STATE_IDLE;
          gThread->MutexUnlock (ProcessorData->StateLock);

          break;

        default:
          break;
      }
    }

    if (gMPSystem.FinishCount == gMPSystem.StartCount) {
      Status = EFI_SUCCESS;
      goto Done;
    }

    if ((TimeoutInMicroseconds != 0) && (Timeout == 0)) {
      Status = EFI_TIMEOUT;
      goto Done;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
  }

Done:
  if (FailedCpuList != NULL) {
    if (gMPSystem.FailedListIndex == 0) {
      FreePool (*FailedCpuList);
      *FailedCpuList = NULL;
    }
  }

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
  @param[in]  TimeoutInMicrosecsond   Indicates the time limit in microseconds for
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
CpuMpServicesStartupThisAP (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN                   *Finished               OPTIONAL
  )
{
  UINTN  Timeout;

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= gMPSystem.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  gThread->MutexLock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
  if (gMPSystem.ProcessorData[ProcessorNumber].State != CPU_STATE_IDLE) {
    gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
    return EFI_NOT_READY;
  }

  gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);

  if ((WaitEvent != NULL)  && gReadToBoot) {
    return EFI_UNSUPPORTED;
  }

  Timeout = TimeoutInMicroseconds;

  gMPSystem.StartCount  = 1;
  gMPSystem.FinishCount = 0;

  SetApProcedure (&gMPSystem.ProcessorData[ProcessorNumber], Procedure, ProcedureArgument);

  if (WaitEvent != NULL) {
    // Non Blocking
    gMPSystem.WaitEvent = WaitEvent;
    gBS->SetTimer (
           gMPSystem.ProcessorData[ProcessorNumber].CheckThisAPEvent,
           TimerPeriodic,
           gPollInterval
           );
    return EFI_SUCCESS;
  }

  // Blocking
  while (TRUE) {
    gThread->MutexLock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
    if (gMPSystem.ProcessorData[ProcessorNumber].State == CPU_STATE_FINISHED) {
      gMPSystem.ProcessorData[ProcessorNumber].State = CPU_STATE_IDLE;
      gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
      break;
    }

    gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);

    if ((TimeoutInMicroseconds != 0) && (Timeout == 0)) {
      return EFI_TIMEOUT;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
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
CpuMpServicesSwitchBSP (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
  UINTN  Index;

  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= gMPSystem.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < gMPSystem.NumberOfProcessors; Index++) {
    if ((gMPSystem.ProcessorData[Index].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
      break;
    }
  }

  ASSERT (Index != gMPSystem.NumberOfProcessors);

  gThread->MutexLock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
  if (gMPSystem.ProcessorData[ProcessorNumber].State != CPU_STATE_IDLE) {
    gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
    return EFI_NOT_READY;
  }

  gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);

  // Skip for now as we need switch a bunch of stack stuff around and it's complex
  // May not be worth it?
  return EFI_NOT_READY;
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
CpuMpServicesEnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  if (!IsBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= gMPSystem.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  gThread->MutexLock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
  if (gMPSystem.ProcessorData[ProcessorNumber].State != CPU_STATE_IDLE) {
    gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);
    return EFI_UNSUPPORTED;
  }

  gThread->MutexUnlock (gMPSystem.ProcessorData[ProcessorNumber].StateLock);

  if (EnableAP) {
    if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0 ) {
      gMPSystem.NumberOfEnabledProcessors++;
    }

    gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag |= PROCESSOR_ENABLED_BIT;
  } else {
    if ((gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag & PROCESSOR_ENABLED_BIT) == PROCESSOR_ENABLED_BIT ) {
      gMPSystem.NumberOfEnabledProcessors--;
    }

    gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag &= ~PROCESSOR_ENABLED_BIT;
  }

  if (HealthFlag != NULL) {
    gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag &= ~PROCESSOR_HEALTH_STATUS_BIT;
    gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag |= (*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT);
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

  @param[in] This              A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param[in] ProcessorNumber   The handle number of AP that is to become the new
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
CpuMpServicesWhoAmI (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  UINTN   Index;
  UINT64  ProcessorId;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ProcessorId = gThread->Self ();
  for (Index = 0; Index < gMPSystem.NumberOfProcessors; Index++) {
    if (gMPSystem.ProcessorData[Index].Info.ProcessorId == ProcessorId) {
      break;
    }
  }

  *ProcessorNumber = Index;
  return EFI_SUCCESS;
}

EFI_MP_SERVICES_PROTOCOL  mMpServicesTemplate = {
  CpuMpServicesGetNumberOfProcessors,
  CpuMpServicesGetProcessorInfo,
  CpuMpServicesStartupAllAps,
  CpuMpServicesStartupThisAP,
  CpuMpServicesSwitchBSP,
  CpuMpServicesEnableDisableAP,
  CpuMpServicesWhoAmI
};

/*++
  If timeout occurs in StartupAllAps(), a timer is set, which invokes this
  procedure periodically to check whether all APs have finished.


--*/
VOID
EFIAPI
CpuCheckAllAPsStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  UINTN                 ProcessorNumber;
  UINTN                 NextNumber;
  PROCESSOR_DATA_BLOCK  *ProcessorData;
  PROCESSOR_DATA_BLOCK  *NextData;
  EFI_STATUS            Status;
  PROCESSOR_STATE       ProcessorState;
  UINTN                 Cpu;
  BOOLEAN               Found;

  if (gMPSystem.TimeoutActive) {
    gMPSystem.Timeout -= CalculateAndStallInterval (gMPSystem.Timeout);
  }

  for (ProcessorNumber = 0; ProcessorNumber < gMPSystem.NumberOfProcessors; ProcessorNumber++) {
    ProcessorData = &gMPSystem.ProcessorData[ProcessorNumber];
    if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
      // Skip BSP
      continue;
    }

    if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
      // Skip Disabled processors
      continue;
    }

    // This is an Interrupt Service routine.
    // This can grab a lock that is held in a non-interrupt
    // context. Meaning deadlock. Which is a bad thing.
    // So, try lock it. If we can get it, cool, do our thing.
    // otherwise, just dump out & try again on the next iteration.
    Status = gThread->MutexTryLock (ProcessorData->StateLock);
    if (EFI_ERROR (Status)) {
      return;
    }

    ProcessorState = ProcessorData->State;
    gThread->MutexUnlock (ProcessorData->StateLock);

    switch (ProcessorState) {
      case CPU_STATE_FINISHED:
        if (gMPSystem.SingleThread) {
          Status = GetNextBlockedNumber (&NextNumber);
          if (!EFI_ERROR (Status)) {
            NextData = &gMPSystem.ProcessorData[NextNumber];

            gThread->MutexLock (NextData->StateLock);
            NextData->State = CPU_STATE_READY;
            gThread->MutexUnlock (NextData->StateLock);

            SetApProcedure (NextData, gMPSystem.Procedure, gMPSystem.ProcedureArgument);
          }
        }

        gThread->MutexLock (ProcessorData->StateLock);
        ProcessorData->State = CPU_STATE_IDLE;
        gThread->MutexUnlock (ProcessorData->StateLock);
        gMPSystem.FinishCount++;
        break;

      default:
        break;
    }
  }

  if (gMPSystem.TimeoutActive && (gMPSystem.Timeout == 0)) {
    //
    // Timeout
    //
    if (gMPSystem.FailedList != NULL) {
      for (ProcessorNumber = 0; ProcessorNumber < gMPSystem.NumberOfProcessors; ProcessorNumber++) {
        ProcessorData = &gMPSystem.ProcessorData[ProcessorNumber];
        if ((ProcessorData->Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
          // Skip BSP
          continue;
        }

        if ((ProcessorData->Info.StatusFlag & PROCESSOR_ENABLED_BIT) == 0) {
          // Skip Disabled processors
          continue;
        }

        // Mark the
        Status = gThread->MutexTryLock (ProcessorData->StateLock);
        if (EFI_ERROR (Status)) {
          return;
        }

        ProcessorState = ProcessorData->State;
        gThread->MutexUnlock (ProcessorData->StateLock);

        if (ProcessorState != CPU_STATE_IDLE) {
          // If we are retrying make sure we don't double count
          for (Cpu = 0, Found = FALSE; Cpu < gMPSystem.NumberOfProcessors; Cpu++) {
            if (gMPSystem.FailedList[Cpu] == END_OF_CPU_LIST) {
              break;
            }

            if (gMPSystem.FailedList[ProcessorNumber] == Cpu) {
              Found = TRUE;
              break;
            }
          }

          if (!Found) {
            gMPSystem.FailedList[gMPSystem.FailedListIndex++] = Cpu;
          }
        }
      }
    }

    // Force terminal exit
    gMPSystem.FinishCount = gMPSystem.StartCount;
  }

  if (gMPSystem.FinishCount != gMPSystem.StartCount) {
    return;
  }

  gBS->SetTimer (
         gMPSystem.CheckAllAPsEvent,
         TimerCancel,
         0
         );

  if (gMPSystem.FailedListIndex == 0) {
    if (gMPSystem.FailedList != NULL) {
      FreePool (gMPSystem.FailedList);
      gMPSystem.FailedList = NULL;
    }
  }

  Status = gBS->SignalEvent (gMPSystem.WaitEvent);

  return;
}

VOID
EFIAPI
CpuCheckThisAPStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS            Status;
  PROCESSOR_DATA_BLOCK  *ProcessorData;
  PROCESSOR_STATE       ProcessorState;

  ProcessorData = (PROCESSOR_DATA_BLOCK *)Context;

  //
  // This is an Interrupt Service routine.
  // that can grab a lock that is held in a non-interrupt
  // context. Meaning deadlock. Which is a badddd thing.
  // So, try lock it. If we can get it, cool, do our thing.
  // otherwise, just dump out & try again on the next iteration.
  //
  Status = gThread->MutexTryLock (ProcessorData->StateLock);
  if (EFI_ERROR (Status)) {
    return;
  }

  ProcessorState = ProcessorData->State;
  gThread->MutexUnlock (ProcessorData->StateLock);

  if (ProcessorState == CPU_STATE_FINISHED) {
    Status = gBS->SetTimer (ProcessorData->CheckThisAPEvent, TimerCancel, 0);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->SignalEvent (gMPSystem.WaitEvent);
    ASSERT_EFI_ERROR (Status);

    gThread->MutexLock (ProcessorData->StateLock);
    ProcessorData->State = CPU_STATE_IDLE;
    gThread->MutexUnlock (ProcessorData->StateLock);
  }

  return;
}

/*++
  This function is called by all processors (both BSP and AP) once and collects MP related data

  MPSystemData  - Pointer to the data structure containing MP related data
  BSP           - TRUE if the CPU is BSP

  EFI_SUCCESS   - Data for the processor collected and filled in

--*/
EFI_STATUS
FillInProcessorInformation (
  IN     BOOLEAN  BSP,
  IN     UINTN    ProcessorNumber
  )
{
  gMPSystem.ProcessorData[ProcessorNumber].Info.ProcessorId = gThread->Self ();
  gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag  = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT;
  if (BSP) {
    gMPSystem.ProcessorData[ProcessorNumber].Info.StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  gMPSystem.ProcessorData[ProcessorNumber].Info.Location.Package = (UINT32)ProcessorNumber;
  gMPSystem.ProcessorData[ProcessorNumber].Info.Location.Core    = 0;
  gMPSystem.ProcessorData[ProcessorNumber].Info.Location.Thread  = 0;
  gMPSystem.ProcessorData[ProcessorNumber].State                 = BSP ? CPU_STATE_BUSY : CPU_STATE_IDLE;

  gMPSystem.ProcessorData[ProcessorNumber].Procedure     = NULL;
  gMPSystem.ProcessorData[ProcessorNumber].Parameter     = NULL;
  gMPSystem.ProcessorData[ProcessorNumber].StateLock     = gThread->MutexInit ();
  gMPSystem.ProcessorData[ProcessorNumber].ProcedureLock = gThread->MutexInit ();

  return EFI_SUCCESS;
}

VOID *
EFIAPI
CpuDriverApIdolLoop (
  VOID  *Context
  )
{
  EFI_AP_PROCEDURE      Procedure;
  VOID                  *Parameter;
  UINTN                 ProcessorNumber;
  PROCESSOR_DATA_BLOCK  *ProcessorData;

  ProcessorNumber = (UINTN)Context;
  ProcessorData   = &gMPSystem.ProcessorData[ProcessorNumber];

  ProcessorData->Info.ProcessorId = gThread->Self ();

  while (TRUE) {
    //
    // Make a local copy on the stack to be extra safe
    //
    gThread->MutexLock (ProcessorData->ProcedureLock);
    Procedure = ProcessorData->Procedure;
    Parameter = ProcessorData->Parameter;
    gThread->MutexUnlock (ProcessorData->ProcedureLock);

    if (Procedure != NULL) {
      gThread->MutexLock (ProcessorData->StateLock);
      ProcessorData->State = CPU_STATE_BUSY;
      gThread->MutexUnlock (ProcessorData->StateLock);

      Procedure (Parameter);

      gThread->MutexLock (ProcessorData->ProcedureLock);
      ProcessorData->Procedure = NULL;
      gThread->MutexUnlock (ProcessorData->ProcedureLock);

      gThread->MutexLock (ProcessorData->StateLock);
      ProcessorData->State = CPU_STATE_FINISHED;
      gThread->MutexUnlock (ProcessorData->StateLock);
    }

    // Poll 5 times a seconds, 200ms
    // Don't want to burn too many system resources doing nothing.
    gEmuThunk->Sleep (200 * 1000);
  }

  return 0;
}

EFI_STATUS
InitializeMpSystemData (
  IN   UINTN  NumberOfProcessors
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  //
  // Clear the data structure area first.
  //
  ZeroMem (&gMPSystem, sizeof (MP_SYSTEM_DATA));

  //
  // First BSP fills and inits all known values, including it's own records.
  //
  gMPSystem.NumberOfProcessors        = NumberOfProcessors;
  gMPSystem.NumberOfEnabledProcessors = NumberOfProcessors;

  gMPSystem.ProcessorData = AllocateZeroPool (gMPSystem.NumberOfProcessors * sizeof (PROCESSOR_DATA_BLOCK));
  ASSERT (gMPSystem.ProcessorData != NULL);

  FillInProcessorInformation (TRUE, 0);

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CpuCheckAllAPsStatus,
                  NULL,
                  &gMPSystem.CheckAllAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < gMPSystem.NumberOfProcessors; Index++) {
    if ((gMPSystem.ProcessorData[Index].Info.StatusFlag & PROCESSOR_AS_BSP_BIT) == PROCESSOR_AS_BSP_BIT) {
      // Skip BSP
      continue;
    }

    FillInProcessorInformation (FALSE, Index);

    Status = gThread->CreateThread (
                        (VOID *)&gMPSystem.ProcessorData[Index].Info.ProcessorId,
                        NULL,
                        CpuDriverApIdolLoop,
                        (VOID *)Index
                        );

    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    CpuCheckThisAPStatus,
                    (VOID *)&gMPSystem.ProcessorData[Index],
                    &gMPSystem.ProcessorData[Index].CheckThisAPEvent
                    );
  }

  return EFI_SUCCESS;
}

/**
  Invoke a notification event

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
CpuReadToBootFunction (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  gReadToBoot = TRUE;
}

EFI_STATUS
CpuMpServicesInit (
  OUT UINTN  *MaxCpus
  )
{
  EFI_STATUS             Status;
  EFI_HANDLE             Handle;
  EMU_IO_THUNK_PROTOCOL  *IoThunk;

  *MaxCpus = 1; // BSP
  IoThunk  = GetIoThunkInstance (&gEmuThreadThunkProtocolGuid, 0);
  if (IoThunk != NULL) {
    Status = IoThunk->Open (IoThunk);
    if (!EFI_ERROR (Status)) {
      if (IoThunk->ConfigString != NULL) {
        *MaxCpus += StrDecimalToUintn (IoThunk->ConfigString);
        gThread   = IoThunk->Interface;
      }
    }
  }

  if (*MaxCpus == 1) {
    // We are not MP so nothing to do
    return EFI_SUCCESS;
  }

  gPollInterval = (UINTN)PcdGet64 (PcdEmuMpServicesPollingInterval);

  Status = InitializeMpSystemData (*MaxCpus);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiCreateEventReadyToBootEx (TPL_CALLBACK, CpuReadToBootFunction, NULL, &gReadToBootEvent);
  ASSERT_EFI_ERROR (Status);

  //
  // Now install the MP services protocol.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMpServiceProtocolGuid,
                  &mMpServicesTemplate,
                  NULL
                  );
  return Status;
}
