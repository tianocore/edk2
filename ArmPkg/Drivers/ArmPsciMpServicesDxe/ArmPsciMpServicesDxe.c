/** @file
  Construct MP Services Protocol.

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

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Protocol/LoadedImage.h>

#include "MpServicesInternal.h"

#define POLL_INTERVAL_US  50000

STATIC CPU_MP_DATA  mCpuMpData;
STATIC BOOLEAN      mNonBlockingModeAllowed;
UINT64              *gApStacksBase;
UINT64              *gProcessorIDs;
CONST UINT64        gApStackSize = AP_STACK_SIZE;
VOID                *gTtbr0;
UINTN               gTcr;
UINTN               gMair;

STATIC
BOOLEAN
IsCurrentProcessorBSP (
  VOID
  );

/** Turns on the specified core using PSCI and executes the user-supplied
    function that's been configured via a previous call to SetApProcedure.

    @param ProcessorIndex The index of the core to turn on.

    @retval EFI_SUCCESS      Success.
    @retval EFI_DEVICE_ERROR The processor could not be turned on.

**/
STATIC
EFI_STATUS
EFIAPI
DispatchCpu (
  IN UINTN  ProcessorIndex
  )
{
  ARM_SMC_ARGS  Args;
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;

  mCpuMpData.CpuData[ProcessorIndex].State = CpuStateBusy;

  /* Turn the AP on */
  if (sizeof (Args.Arg0) == sizeof (UINT32)) {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_ON_AARCH32;
  } else {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_ON_AARCH64;
  }

  Args.Arg1 = gProcessorIDs[ProcessorIndex];
  Args.Arg2 = (UINTN)ApEntryPoint;

  ArmCallSmc (&Args);

  if (Args.Arg0 == ARM_SMC_PSCI_RET_ALREADY_ON) {
    Status = EFI_NOT_READY;
  } else if (Args.Arg0 != ARM_SMC_PSCI_RET_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "PSCI_CPU_ON call failed: %d\n", Args.Arg0));
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/** Returns whether the specified processor is the BSP.

  @param[in] ProcessorIndex The index the processor to check.

  @return TRUE if the processor is the BSP, FALSE otherwise.
**/
STATIC
BOOLEAN
IsProcessorBSP (
  UINTN  ProcessorIndex
  )
{
  EFI_PROCESSOR_INFORMATION  *CpuInfo;

  CpuInfo = &mCpuMpData.CpuData[ProcessorIndex].Info;

  return (CpuInfo->StatusFlag & PROCESSOR_AS_BSP_BIT) != 0;
}

/** Get the Application Processors state.

  @param[in]  CpuData    The pointer to CPU_AP_DATA of specified AP.

  @return The AP status.
**/
CPU_STATE
GetApState (
  IN  CPU_AP_DATA  *CpuData
  )
{
  return CpuData->State;
}

/** Configures the processor context with the user-supplied procedure and
    argument.

   @param CpuData           The processor context.
   @param Procedure         The user-supplied procedure.
   @param ProcedureArgument The user-supplied procedure argument.

**/
STATIC
VOID
SetApProcedure (
  IN   CPU_AP_DATA       *CpuData,
  IN   EFI_AP_PROCEDURE  Procedure,
  IN   VOID              *ProcedureArgument
  )
{
  ASSERT (CpuData != NULL);
  ASSERT (Procedure != NULL);

  CpuData->Parameter = ProcedureArgument;
  CpuData->Procedure = Procedure;
}

/** Returns the index of the next processor that is blocked.

   @param[out] NextNumber The index of the next blocked processor.

   @retval EFI_SUCCESS   Successfully found the next blocked processor.
   @retval EFI_NOT_FOUND There are no blocked processors.

**/
STATIC
EFI_STATUS
GetNextBlockedNumber (
  OUT UINTN  *NextNumber
  )
{
  UINTN        Index;
  CPU_STATE    State;
  CPU_AP_DATA  *CpuData;

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    CpuData = &mCpuMpData.CpuData[Index];
    if (IsProcessorBSP (Index)) {
      // Skip BSP
      continue;
    }

    State = CpuData->State;

    if (State == CpuStateBlocked) {
      *NextNumber = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Stalls the BSP for the minimum of POLL_INTERVAL_US and Timeout.

   @param[in]  Timeout    The time limit in microseconds remaining for
                          APs to return from Procedure.

   @retval     StallTime  Time of execution stall.
**/
STATIC
UINTN
CalculateAndStallInterval (
  IN UINTN  Timeout
  )
{
  UINTN  StallTime;

  if ((Timeout < POLL_INTERVAL_US) && (Timeout != 0)) {
    StallTime = Timeout;
  } else {
    StallTime = POLL_INTERVAL_US;
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

  @param[in]  This                        A pointer to the
                                          EFI_MP_SERVICES_PROTOCOL instance.
  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including
                                          the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled
                                          logical processors that exist in the
                                          system, including the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
  @retval EFI_INVALID_PARAMETER   NumberOfEnabledProcessors is NULL.

**/
STATIC
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

  if (!IsCurrentProcessorBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  *NumberOfProcessors        = mCpuMpData.NumberOfProcessors;
  *NumberOfEnabledProcessors = mCpuMpData.NumberOfEnabledProcessors;
  return EFI_SUCCESS;
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  This service retrieves detailed MP-related information about any processor
  on the platform. Note the following:
    - The processor information may change during the course of a boot session.
    - The information presented here is entirely MP related.

  Information regarding the number of caches and their sizes, frequency of
  operation, slot numbers is all considered platform-related information and is
  not provided by this service.

  @param[in]  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL
                                    instance.
  @param[in]  ProcessorIndex        The index of the processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information
                                    for the requested processor is deposited.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.

**/
STATIC
EFI_STATUS
EFIAPI
GetProcessorInfo (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  UINTN                      ProcessorIndex,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  )
{
  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsCurrentProcessorBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  ProcessorIndex &= ~CPU_V2_EXTENDED_TOPOLOGY;

  if (ProcessorIndex >= mCpuMpData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CopyMem (
    ProcessorInfoBuffer,
    &mCpuMpData.CpuData[ProcessorIndex].Info,
    sizeof (EFI_PROCESSOR_INFORMATION)
    );
  return EFI_SUCCESS;
}

/**
  This service executes a caller provided function on all enabled APs. APs can
  run either simultaneously or one at a time in sequence. This service supports
  both blocking and non-blocking requests. The non-blocking requests use EFI
  events so the BSP can detect when the APs have finished. This service may only
  be called from the BSP.

  This function is used to dispatch all the enabled APs to the function
  specified by Procedure.  If any enabled AP is busy, then EFI_NOT_READY is
  returned immediately and Procedure is not started on any AP.

  If SingleThread is TRUE, all the enabled APs execute the function specified by
  Procedure one by one, in ascending order of processor handle number.
  Otherwise, all the enabled APs execute the function specified by Procedure
  simultaneously.

  If WaitEvent is NULL, execution is in blocking mode. The BSP waits until all
  APs finish or TimeoutInMicroseconds expires. Otherwise, execution is in
  non-blocking mode, and the BSP returns from this service without waiting for
  APs. If a non-blocking mode is requested after the UEFI Event
  EFI_EVENT_GROUP_READY_TO_BOOT is signaled, then EFI_UNSUPPORTED must be
  returned.

  If the timeout specified by TimeoutInMicroseconds expires before all APs
  return from Procedure, then Procedure on the failed APs is terminated.
  All enabled APs are always available for further calls to
  EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() and
  EFI_MP_SERVICES_PROTOCOL.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the
  EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() to make sure that the nature of the
  code that is executed on the BSP and the dispatched APs is well controlled.
  The MP Services Protocol does not guarantee that the Procedure function is
  MP-safe. Hence, the tasks that can be run in parallel are limited to certain
  independent tasks and well-controlled exclusive code. EFI services and
  protocols may not be called by APs unless otherwise specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroseconds expires.

  In non-blocking execution mode, BSP is freed to return to the caller and then
  proceed to the next task without having to wait for APs. The following
  sequence needs to occur in a non-blocking execution mode:

    -# The caller that intends to use this MP Services Protocol in non-blocking
       mode creates WaitEvent by calling the EFI CreateEvent() service.  The
       caller invokes EFI_MP_SERVICES_PROTOCOL.StartupAllAPs(). If the parameter
       WaitEvent is not NULL, then StartupAllAPs() executes in non-blocking
       mode. It requests the function specified by Procedure to be started on
       all the enabled APs, and releases the BSP to continue with other tasks.
    -# The caller can use the CheckEvent() and WaitForEvent() services to check
       the state of the WaitEvent created in step 1.
    -# When the APs complete their task or TimeoutInMicroSecondss expires, the
       MP Service signals WaitEvent by calling the EFI SignalEvent() function.
       If FailedCpuList is not NULL, its content is available when WaitEvent is
       signaled. If all APs returned from Procedure prior to the timeout, then
       FailedCpuList is set to NULL. If not all APs return from Procedure before
       the timeout, then FailedCpuList is filled in with the list of the failed
       APs. The buffer is allocated by MP Service Protocol using AllocatePool().
       It is the caller's responsibility to free the buffer with FreePool()
       service.
    -# This invocation of SignalEvent() function informs the caller that invoked
       EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() that either all the APs
       completed the specified task or a timeout occurred. The contents of
       FailedCpuList can be examined to determine which APs did not complete the
       specified task prior to the timeout.

  @param[in]  This                    A pointer to the EFI_MP_SERVICES_PROTOCOL
                                      instance.
  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  SingleThread            If TRUE, then all the enabled APs execute
                                      the function specified by Procedure one by
                                      one, in ascending order of processor
                                      handle number.  If FALSE, then all the
                                      enabled APs execute the function specified
                                      by Procedure simultaneously.
  @param[in]  WaitEvent               The event created by the caller with
                                      CreateEvent() service.  If it is NULL,
                                      then execute in blocking mode. BSP waits
                                      until all APs finish or
                                      TimeoutInMicroseconds expires.  If it's
                                      not NULL, then execute in non-blocking
                                      mode. BSP requests the function specified
                                      by Procedure to be started on all the
                                      enabled APs, and go on executing
                                      immediately. If all return from Procedure,
                                      or TimeoutInMicroseconds expires, this
                                      event is signaled. The BSP can use the
                                      CheckEvent() or WaitForEvent()
                                      services to check the state of event. Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds
                                      for APs to return from Procedure, either
                                      for blocking or non-blocking mode. Zero
                                      means infinity.  If the timeout expires
                                      before all APs return from Procedure, then
                                      Procedure on the failed APs is terminated.
                                      All enabled APs are available for next
                                      function assigned by
                                      EFI_MP_SERVICES_PROTOCOL.StartupAllAPs()
                                      or EFI_MP_SERVICES_PROTOCOL.StartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.
  @param[out] FailedCpuList           If NULL, this parameter is ignored.
                                      Otherwise, if all APs finish successfully,
                                      then its content is set to NULL. If not
                                      all APs finish before timeout expires,
                                      then its content is set to address of the
                                      buffer holding handle numbers of the
                                      failed APs.
                                      The buffer is allocated by MP Service
                                      Protocol, and it's the caller's
                                      responsibility to free the buffer with
                                      FreePool() service.
                                      In blocking mode, it is ready for
                                      consumption when the call returns. In
                                      non-blocking mode, it is ready when
                                      WaitEvent is signaled. The list of failed
                                      CPU is terminated by  END_OF_CPU_LIST.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before
                                  the timeout expired.
  @retval EFI_SUCCESS             In non-blocking mode, function has been
                                  dispatched to all enabled APs.
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
STATIC
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
  EFI_STATUS  Status;

  if (!IsCurrentProcessorBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if ((mCpuMpData.NumberOfProcessors == 1) || (mCpuMpData.NumberOfEnabledProcessors == 1)) {
    return EFI_NOT_STARTED;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((WaitEvent != NULL) && !mNonBlockingModeAllowed) {
    return EFI_UNSUPPORTED;
  }

  if (FailedCpuList != NULL) {
    mCpuMpData.FailedList = AllocateZeroPool (
                              (mCpuMpData.NumberOfProcessors + 1) *
                              sizeof (UINTN)
                              );
    if (mCpuMpData.FailedList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    SetMemN (
      mCpuMpData.FailedList,
      (mCpuMpData.NumberOfProcessors + 1) *
      sizeof (UINTN),
      END_OF_CPU_LIST
      );
    mCpuMpData.FailedListIndex = 0;
    *FailedCpuList             = mCpuMpData.FailedList;
  }

  StartupAllAPsPrepareState (SingleThread);

  // If any enabled APs are busy (ignoring the BSP), return EFI_NOT_READY
  if (mCpuMpData.StartCount != (mCpuMpData.NumberOfEnabledProcessors - 1)) {
    return EFI_NOT_READY;
  }

  if (WaitEvent != NULL) {
    Status = StartupAllAPsWithWaitEvent (
               Procedure,
               ProcedureArgument,
               WaitEvent,
               TimeoutInMicroseconds,
               SingleThread,
               FailedCpuList
               );

    if (EFI_ERROR (Status) && (FailedCpuList != NULL)) {
      if (mCpuMpData.FailedListIndex == 0) {
        FreePool (*FailedCpuList);
        *FailedCpuList = NULL;
      }
    }
  } else {
    Status = StartupAllAPsNoWaitEvent (
               Procedure,
               ProcedureArgument,
               TimeoutInMicroseconds,
               SingleThread,
               FailedCpuList
               );

    if (FailedCpuList != NULL) {
      if (mCpuMpData.FailedListIndex == 0) {
        FreePool (*FailedCpuList);
        *FailedCpuList = NULL;
      }
    }
  }

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
STATIC
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
  EFI_STATUS   Status;
  UINTN        Timeout;
  CPU_AP_DATA  *CpuData;

  if (!IsCurrentProcessorBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProcessorNumber >= mCpuMpData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CpuData = &mCpuMpData.CpuData[ProcessorNumber];

  if (IsProcessorBSP (ProcessorNumber)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsProcessorEnabled (ProcessorNumber)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((GetApState (CpuData) != CpuStateIdle) &&
      (GetApState (CpuData) != CpuStateFinished))
  {
    return EFI_NOT_READY;
  }

  if ((WaitEvent != NULL) && !mNonBlockingModeAllowed) {
    return EFI_UNSUPPORTED;
  }

  Timeout = TimeoutInMicroseconds;

  CpuData->Timeout       = TimeoutInMicroseconds;
  CpuData->TimeTaken     = 0;
  CpuData->TimeoutActive = (BOOLEAN)(TimeoutInMicroseconds != 0);

  SetApProcedure (
    CpuData,
    Procedure,
    ProcedureArgument
    );

  Status = DispatchCpu (ProcessorNumber);
  if (EFI_ERROR (Status)) {
    CpuData->State = CpuStateIdle;
    return EFI_NOT_READY;
  }

  if (WaitEvent != NULL) {
    // Non Blocking
    if (Finished != NULL) {
      CpuData->SingleApFinished = Finished;
      *Finished                 = FALSE;
    }

    CpuData->WaitEvent = WaitEvent;
    Status             = gBS->SetTimer (
                                CpuData->CheckThisAPEvent,
                                TimerPeriodic,
                                POLL_INTERVAL_US
                                );

    return EFI_SUCCESS;
  }

  // Blocking
  while (TRUE) {
    if (GetApState (CpuData) == CpuStateFinished) {
      CpuData->State = CpuStateIdle;
      break;
    }

    if ((TimeoutInMicroseconds != 0) && (Timeout == 0)) {
      return EFI_TIMEOUT;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
  }

  return EFI_SUCCESS;
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be
  performed by the current BSP.

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
STATIC
EFI_STATUS
EFIAPI
SwitchBSP (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  )
{
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
STATIC
EFI_STATUS
EFIAPI
EnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  )
{
  UINTN        StatusFlag;
  CPU_AP_DATA  *CpuData;

  StatusFlag = mCpuMpData.CpuData[ProcessorNumber].Info.StatusFlag;
  CpuData    = &mCpuMpData.CpuData[ProcessorNumber];

  if (!IsCurrentProcessorBSP ()) {
    return EFI_DEVICE_ERROR;
  }

  if (ProcessorNumber >= mCpuMpData.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  if (IsProcessorBSP (ProcessorNumber)) {
    return EFI_INVALID_PARAMETER;
  }

  if (GetApState (CpuData) != CpuStateIdle) {
    return EFI_UNSUPPORTED;
  }

  if (EnableAP) {
    if (!IsProcessorEnabled (ProcessorNumber)) {
      mCpuMpData.NumberOfEnabledProcessors++;
    }

    StatusFlag |= PROCESSOR_ENABLED_BIT;
  } else {
    if (IsProcessorEnabled (ProcessorNumber) && !IsProcessorBSP (ProcessorNumber)) {
      mCpuMpData.NumberOfEnabledProcessors--;
    }

    StatusFlag &= ~PROCESSOR_ENABLED_BIT;
  }

  if ((HealthFlag != NULL) && !IsProcessorBSP (ProcessorNumber)) {
    StatusFlag &= ~PROCESSOR_HEALTH_STATUS_BIT;
    StatusFlag |= (*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT);
  }

  mCpuMpData.CpuData[ProcessorNumber].Info.StatusFlag = StatusFlag;
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
  @param[out] ProcessorNumber  The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
STATIC
EFI_STATUS
EFIAPI
WhoAmI (
  IN EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  UINTN   Index;
  UINT64  ProcessorId;

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ProcessorId = GET_MPIDR_AFFINITY_BITS (ArmReadMpidr ());
  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    if (ProcessorId == gProcessorIDs[Index]) {
      *ProcessorNumber = Index;
      break;
    }
  }

  return EFI_SUCCESS;
}

STATIC EFI_MP_SERVICES_PROTOCOL  mMpServicesProtocol = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  StartupAllAPs,
  StartupThisAP,
  SwitchBSP,
  EnableDisableAP,
  WhoAmI
};

/** Adds the specified processor the list of failed processors.

   @param ProcessorIndex The processor index to add.
   @param ApState        Processor state.

**/
STATIC
VOID
AddProcessorToFailedList (
  UINTN      ProcessorIndex,
  CPU_STATE  ApState
  )
{
  UINTN    Index;
  BOOLEAN  Found;

  Found = FALSE;

  if ((mCpuMpData.FailedList == NULL) ||
      (ApState == CpuStateIdle) ||
      (ApState == CpuStateFinished) ||
      IsProcessorBSP (ProcessorIndex))
  {
    return;
  }

  // If we are retrying make sure we don't double count
  for (Index = 0; Index < mCpuMpData.FailedListIndex; Index++) {
    if (mCpuMpData.FailedList[Index] == ProcessorIndex) {
      Found = TRUE;
      break;
    }
  }

  /* If the CPU isn't already in the FailedList, add it */
  if (!Found) {
    mCpuMpData.FailedList[mCpuMpData.FailedListIndex++] = ProcessorIndex;
  }
}

/** Handles the StartupAllAPs case where the timeout has occurred.

**/
STATIC
VOID
ProcessStartupAllAPsTimeout (
  VOID
  )
{
  CPU_AP_DATA  *CpuData;
  UINTN        Index;

  if (mCpuMpData.FailedList == NULL) {
    return;
  }

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    CpuData = &mCpuMpData.CpuData[Index];
    if (IsProcessorBSP (Index)) {
      // Skip BSP
      continue;
    }

    if (!IsProcessorEnabled (Index)) {
      // Skip Disabled processors
      continue;
    }

    CpuData = &mCpuMpData.CpuData[Index];
    AddProcessorToFailedList (Index, GetApState (CpuData));
  }
}

/** Updates the status of the APs.

   @param[in] ProcessorIndex The index of the AP to update.
**/
STATIC
VOID
UpdateApStatus (
  IN UINTN  ProcessorIndex
  )
{
  EFI_STATUS   Status;
  CPU_AP_DATA  *CpuData;
  CPU_AP_DATA  *NextCpuData;
  CPU_STATE    State;
  UINTN        NextNumber;

  CpuData = &mCpuMpData.CpuData[ProcessorIndex];

  if (IsProcessorBSP (ProcessorIndex)) {
    // Skip BSP
    return;
  }

  if (!IsProcessorEnabled (ProcessorIndex)) {
    // Skip Disabled processors
    return;
  }

  State = GetApState (CpuData);

  switch (State) {
    case CpuStateFinished:
      if (mCpuMpData.SingleThread) {
        Status = GetNextBlockedNumber (&NextNumber);
        if (!EFI_ERROR (Status)) {
          NextCpuData = &mCpuMpData.CpuData[NextNumber];

          NextCpuData->State = CpuStateReady;

          SetApProcedure (
            NextCpuData,
            mCpuMpData.Procedure,
            mCpuMpData.ProcedureArgument
            );

          Status = DispatchCpu (NextNumber);
          if (!EFI_ERROR (Status)) {
            mCpuMpData.StartCount++;
          } else {
            AddProcessorToFailedList (NextNumber, NextCpuData->State);
          }
        }
      }

      CpuData->State = CpuStateIdle;
      mCpuMpData.FinishCount++;
      break;

    default:
      break;
  }
}

/**
  If a timeout is specified in StartupAllAps(), a timer is set, which invokes
  this procedure periodically to check whether all APs have finished.

  @param[in] Event   The WaitEvent the user supplied.
  @param[in] Context The event context.
**/
STATIC
VOID
EFIAPI
CheckAllAPsStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  mCpuMpData.AllTimeTaken += POLL_INTERVAL_US;

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    UpdateApStatus (Index);
  }

  if (mCpuMpData.AllTimeoutActive && (mCpuMpData.AllTimeTaken > mCpuMpData.AllTimeout)) {
    ProcessStartupAllAPsTimeout ();

    // Force terminal exit
    mCpuMpData.FinishCount = mCpuMpData.StartCount;
  }

  if (mCpuMpData.FinishCount != mCpuMpData.StartCount) {
    return;
  }

  gBS->SetTimer (
         mCpuMpData.CheckAllAPsEvent,
         TimerCancel,
         0
         );

  if (mCpuMpData.FailedListIndex == 0) {
    if (mCpuMpData.FailedList != NULL) {
      // Since we don't have the original `FailedCpuList`
      // pointer here to set to NULL, don't free the
      // memory.
    }
  }

  Status = gBS->SignalEvent (mCpuMpData.AllWaitEvent);
  ASSERT_EFI_ERROR (Status);
  mCpuMpData.AllWaitEvent = NULL;
}

/** Invoked periodically via a timer to check the state of the processor.

   @param Event   The event supplied by the timer expiration.
   @param Context The processor context.

**/
STATIC
VOID
EFIAPI
CheckThisAPStatus (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS   Status;
  CPU_AP_DATA  *CpuData;
  CPU_STATE    State;

  CpuData = Context;

  CpuData->TimeTaken += POLL_INTERVAL_US;

  State = GetApState (CpuData);

  if (State == CpuStateFinished) {
    Status = gBS->SetTimer (CpuData->CheckThisAPEvent, TimerCancel, 0);
    ASSERT_EFI_ERROR (Status);

    if (CpuData->SingleApFinished != NULL) {
      *(CpuData->SingleApFinished) = TRUE;
    }

    if (CpuData->WaitEvent != NULL) {
      Status = gBS->SignalEvent (CpuData->WaitEvent);
      ASSERT_EFI_ERROR (Status);
    }

    CpuData->State = CpuStateIdle;
  }

  if (CpuData->TimeoutActive && (CpuData->TimeTaken > CpuData->Timeout)) {
    Status = gBS->SetTimer (CpuData->CheckThisAPEvent, TimerCancel, 0);
    if (CpuData->WaitEvent != NULL) {
      Status = gBS->SignalEvent (CpuData->WaitEvent);
      ASSERT_EFI_ERROR (Status);
      CpuData->WaitEvent = NULL;
    }
  }
}

/**
  This function is called by all processors (both BSP and AP) once and collects
  MP related data.

  @param BSP            TRUE if the processor is the BSP.
  @param Mpidr          The MPIDR for the specified processor. This should be
                        the full MPIDR and not only the affinity bits.
  @param ProcessorIndex The index of the processor.

  @return EFI_SUCCESS if the data for the processor collected and filled in.

**/
STATIC
EFI_STATUS
FillInProcessorInformation (
  IN BOOLEAN  BSP,
  IN UINTN    Mpidr,
  IN UINTN    ProcessorIndex
  )
{
  EFI_PROCESSOR_INFORMATION  *CpuInfo;

  CpuInfo = &mCpuMpData.CpuData[ProcessorIndex].Info;

  CpuInfo->ProcessorId = GET_MPIDR_AFFINITY_BITS (Mpidr);
  CpuInfo->StatusFlag  = PROCESSOR_ENABLED_BIT | PROCESSOR_HEALTH_STATUS_BIT;

  if (BSP) {
    CpuInfo->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  if ((Mpidr & MPIDR_MT_BIT) > 0) {
    CpuInfo->Location.Package = GET_MPIDR_AFF2 (Mpidr);
    CpuInfo->Location.Core    = GET_MPIDR_AFF1 (Mpidr);
    CpuInfo->Location.Thread  = GET_MPIDR_AFF0 (Mpidr);

    CpuInfo->ExtendedInformation.Location2.Package = GET_MPIDR_AFF3 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Die     = GET_MPIDR_AFF2 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Core    = GET_MPIDR_AFF1 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Thread  = GET_MPIDR_AFF0 (Mpidr);
  } else {
    CpuInfo->Location.Package = GET_MPIDR_AFF1 (Mpidr);
    CpuInfo->Location.Core    = GET_MPIDR_AFF0 (Mpidr);
    CpuInfo->Location.Thread  = 0;

    CpuInfo->ExtendedInformation.Location2.Package = GET_MPIDR_AFF2 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Die     = GET_MPIDR_AFF1 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Core    = GET_MPIDR_AFF0 (Mpidr);
    CpuInfo->ExtendedInformation.Location2.Thread  = 0;
  }

  mCpuMpData.CpuData[ProcessorIndex].State = BSP ? CpuStateBusy : CpuStateIdle;

  mCpuMpData.CpuData[ProcessorIndex].Procedure = NULL;
  mCpuMpData.CpuData[ProcessorIndex].Parameter = NULL;

  return EFI_SUCCESS;
}

/** Initializes the MP Services system data

   @param NumberOfProcessors The number of processors, both BSP and AP.
   @param CoreInfo           CPU information gathered earlier during boot.

**/
STATIC
EFI_STATUS
MpServicesInitialize (
  IN   UINTN                NumberOfProcessors,
  IN   CONST ARM_CORE_INFO  *CoreInfo
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_EVENT   ReadyToBootEvent;
  BOOLEAN     IsBsp;

  //
  // Clear the data structure area first.
  //
  ZeroMem (&mCpuMpData, sizeof (CPU_MP_DATA));
  //
  // First BSP fills and inits all known values, including its own records.
  //
  mCpuMpData.NumberOfProcessors        = NumberOfProcessors;
  mCpuMpData.NumberOfEnabledProcessors = NumberOfProcessors;

  mCpuMpData.CpuData = AllocateZeroPool (
                         mCpuMpData.NumberOfProcessors * sizeof (CPU_AP_DATA)
                         );

  if (mCpuMpData.CpuData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  /* Allocate one extra for the sentinel entry at the end */
  gProcessorIDs = AllocateZeroPool ((mCpuMpData.NumberOfProcessors + 1) * sizeof (UINT64));
  ASSERT (gProcessorIDs != NULL);

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckAllAPsStatus,
                  NULL,
                  &mCpuMpData.CheckAllAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  gApStacksBase = AllocatePages (
                    EFI_SIZE_TO_PAGES (
                      mCpuMpData.NumberOfProcessors *
                      gApStackSize
                      )
                    );
  ASSERT (gApStacksBase != NULL);

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    if (GET_MPIDR_AFFINITY_BITS (ArmReadMpidr ()) == CoreInfo[Index].Mpidr) {
      IsBsp = TRUE;
    } else {
      IsBsp = FALSE;
    }

    FillInProcessorInformation (IsBsp, CoreInfo[Index].Mpidr, Index);

    gProcessorIDs[Index] = mCpuMpData.CpuData[Index].Info.ProcessorId;

    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    CheckThisAPStatus,
                    (VOID *)&mCpuMpData.CpuData[Index],
                    &mCpuMpData.CpuData[Index].CheckThisAPEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  gProcessorIDs[Index] = MAX_UINT64;

  gTcr   = ArmGetTCR ();
  gMair  = ArmGetMAIR ();
  gTtbr0 = ArmGetTTBR0BaseAddress ();

  //
  // The global pointer variables as well as the gProcessorIDs array contents
  // are accessed by the other cores so we must clean them to the PoC
  //
  WriteBackDataCacheRange (&gProcessorIDs, sizeof (UINT64 *));
  WriteBackDataCacheRange (&gApStacksBase, sizeof (UINT64 *));

  WriteBackDataCacheRange (
    gProcessorIDs,
    (mCpuMpData.NumberOfProcessors + 1) * sizeof (UINT64)
    );

  mNonBlockingModeAllowed = TRUE;

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             ReadyToBootSignaled,
             NULL,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Event notification function called when the EFI_EVENT_GROUP_READY_TO_BOOT is
  signaled. After this point, non-blocking mode is no longer allowed.

  @param  Event     Event whose notification function is being invoked.
  @param  Context   The pointer to the notification function's context,
                    which is implementation-dependent.

**/
STATIC
VOID
EFIAPI
ReadyToBootSignaled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  mNonBlockingModeAllowed = FALSE;
}

/** Initialize multi-processor support.

  @param ImageHandle  Image handle.
  @param SystemTable  System table.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
EFIAPI
ArmPsciMpServicesDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 Handle;
  UINTN                      MaxCpus;
  EFI_LOADED_IMAGE_PROTOCOL  *Image;
  EFI_HOB_GENERIC_HEADER     *Hob;
  VOID                       *HobData;
  UINTN                      HobDataSize;
  CONST ARM_CORE_INFO        *CoreInfo;

  MaxCpus = 1;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&Image
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Parts of the code in this driver may be executed by other cores running
  // with the MMU off so we need to ensure that everything is clean to the
  // point of coherency (PoC)
  //
  WriteBackDataCacheRange (Image->ImageBase, Image->ImageSize);

  Hob = GetFirstGuidHob (&gArmMpCoreInfoGuid);
  if (Hob != NULL) {
    HobData     = GET_GUID_HOB_DATA (Hob);
    HobDataSize = GET_GUID_HOB_DATA_SIZE (Hob);
    CoreInfo    = (ARM_CORE_INFO *)HobData;
    MaxCpus     = HobDataSize / sizeof (ARM_CORE_INFO);
  }

  if (MaxCpus == 1) {
    DEBUG ((DEBUG_WARN, "Trying to use EFI_MP_SERVICES_PROTOCOL on a UP system"));
    // We are not MP so nothing to do
    return EFI_NOT_FOUND;
  }

  Status = MpServicesInitialize (MaxCpus, CoreInfo);
  if (Status != EFI_SUCCESS) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Now install the MP services protocol.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiMpServiceProtocolGuid,
                  &mMpServicesProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** AP exception handler.

  @param InterruptType The AArch64 CPU exception type.
  @param SystemContext System context.

**/
STATIC
VOID
EFIAPI
ApExceptionHandler (
  IN CONST EFI_EXCEPTION_TYPE  InterruptType,
  IN CONST EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  ARM_SMC_ARGS  Args;
  UINT64        Mpidr;
  UINTN         Index;
  UINTN         ProcessorIndex;

  Mpidr = GET_MPIDR_AFFINITY_BITS (ArmReadMpidr ());

  Index          = 0;
  ProcessorIndex = MAX_UINT64;

  do {
    if (gProcessorIDs[Index] == Mpidr) {
      ProcessorIndex = Index;
      break;
    }

    Index++;
  } while (gProcessorIDs[Index] != MAX_UINT64);

  if (ProcessorIndex != MAX_UINT64) {
    mCpuMpData.CpuData[ProcessorIndex].State = CpuStateFinished;
    ArmDataMemoryBarrier ();
  }

  Args.Arg0 = ARM_SMC_ID_PSCI_CPU_OFF;
  ArmCallSmc (&Args);

  /* Should never be reached */
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/** C entry-point for the AP.
    This function gets called from the assembly function ApEntryPoint.

**/
VOID
ApProcedure (
  VOID
  )
{
  ARM_SMC_ARGS      Args;
  EFI_AP_PROCEDURE  UserApProcedure;
  VOID              *UserApParameter;
  UINTN             ProcessorIndex;

  ProcessorIndex = 0;

  WhoAmI (&mMpServicesProtocol, &ProcessorIndex);

  /* Fetch the user-supplied procedure and parameter to execute */
  UserApProcedure = mCpuMpData.CpuData[ProcessorIndex].Procedure;
  UserApParameter = mCpuMpData.CpuData[ProcessorIndex].Parameter;

  InitializeCpuExceptionHandlers (NULL);
  RegisterCpuInterruptHandler (EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS, ApExceptionHandler);
  RegisterCpuInterruptHandler (EXCEPT_AARCH64_IRQ, ApExceptionHandler);
  RegisterCpuInterruptHandler (EXCEPT_AARCH64_FIQ, ApExceptionHandler);
  RegisterCpuInterruptHandler (EXCEPT_AARCH64_SERROR, ApExceptionHandler);

  UserApProcedure (UserApParameter);

  mCpuMpData.CpuData[ProcessorIndex].State = CpuStateFinished;

  ArmDataMemoryBarrier ();

  /* Since we're finished with this AP, turn it off */
  Args.Arg0 = ARM_SMC_ID_PSCI_CPU_OFF;
  ArmCallSmc (&Args);

  /* Should never be reached */
  ASSERT (FALSE);
  CpuDeadLoop ();
}

/** Returns whether the processor executing this function is the BSP.

    @return Whether the current processor is the BSP.
**/
STATIC
BOOLEAN
IsCurrentProcessorBSP (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       ProcessorIndex;

  Status = WhoAmI (&mMpServicesProtocol, &ProcessorIndex);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  return IsProcessorBSP (ProcessorIndex);
}

/** Returns whether the specified processor is enabled.

   @param[in] ProcessorIndex The index of the processor to check.

   @return TRUE if the processor is enabled, FALSE otherwise.
**/
STATIC
BOOLEAN
IsProcessorEnabled (
  UINTN  ProcessorIndex
  )
{
  EFI_PROCESSOR_INFORMATION  *CpuInfo;

  CpuInfo = &mCpuMpData.CpuData[ProcessorIndex].Info;

  return (CpuInfo->StatusFlag & PROCESSOR_ENABLED_BIT) != 0;
}

/** Sets up the state for the StartupAllAPs function.

   @param SingleThread Whether the APs will execute sequentially.

**/
STATIC
VOID
StartupAllAPsPrepareState (
  IN BOOLEAN  SingleThread
  )
{
  UINTN        Index;
  CPU_STATE    APInitialState;
  CPU_AP_DATA  *CpuData;

  mCpuMpData.FinishCount  = 0;
  mCpuMpData.StartCount   = 0;
  mCpuMpData.SingleThread = SingleThread;

  APInitialState = CpuStateReady;

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    CpuData = &mCpuMpData.CpuData[Index];

    //
    // Get APs prepared, and put failing APs into FailedCpuList.
    // If "SingleThread", only 1 AP will put into ready state, other AP will be
    // put into ready state 1 by 1, until the previous 1 finished its task.
    // If not "SingleThread", all APs are put into ready state from the
    // beginning
    //

    if (IsProcessorBSP (Index)) {
      // Skip BSP
      continue;
    }

    if (!IsProcessorEnabled (Index)) {
      // Skip Disabled processors
      if (mCpuMpData.FailedList != NULL) {
        mCpuMpData.FailedList[mCpuMpData.FailedListIndex++] = Index;
      }

      continue;
    }

    // If any APs finished after timing out, reset state to Idle
    if (GetApState (CpuData) == CpuStateFinished) {
      CpuData->State = CpuStateIdle;
    }

    if (GetApState (CpuData) != CpuStateIdle) {
      // Skip busy processors
      if (mCpuMpData.FailedList != NULL) {
        mCpuMpData.FailedList[mCpuMpData.FailedListIndex++] = Index;
      }
    }

    CpuData->State = APInitialState;

    mCpuMpData.StartCount++;
    if (SingleThread) {
      APInitialState = CpuStateBlocked;
    }
  }
}

/** Handles execution of StartupAllAPs when a WaitEvent has been specified.

  @param Procedure         The user-supplied procedure.
  @param ProcedureArgument The user-supplied procedure argument.
  @param WaitEvent         The wait event to be signaled when the work is
                           complete or a timeout has occurred.
  @param TimeoutInMicroseconds The timeout for the work to be completed. Zero
                               indicates an infinite timeout.
  @param SingleThread          Whether the APs will execute sequentially.
  @param FailedCpuList         User-supplied pointer for list of failed CPUs.

   @return EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
StartupAllAPsWithWaitEvent (
  IN EFI_AP_PROCEDURE  Procedure,
  IN VOID              *ProcedureArgument,
  IN EFI_EVENT         WaitEvent,
  IN UINTN             TimeoutInMicroseconds,
  IN BOOLEAN           SingleThread,
  IN UINTN             **FailedCpuList
  )
{
  EFI_STATUS   Status;
  UINTN        Index;
  CPU_AP_DATA  *CpuData;

  for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
    CpuData = &mCpuMpData.CpuData[Index];
    if (IsProcessorBSP (Index)) {
      // Skip BSP
      continue;
    }

    if (!IsProcessorEnabled (Index)) {
      // Skip Disabled processors
      continue;
    }

    if (GetApState (CpuData) == CpuStateReady) {
      SetApProcedure (CpuData, Procedure, ProcedureArgument);
      if ((mCpuMpData.StartCount == 0) || !SingleThread) {
        Status = DispatchCpu (Index);
        if (EFI_ERROR (Status)) {
          AddProcessorToFailedList (Index, CpuData->State);
          break;
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    return EFI_NOT_READY;
  }

  //
  // Save data into private data structure, and create timer to poll AP state
  // before exiting
  //
  mCpuMpData.Procedure         = Procedure;
  mCpuMpData.ProcedureArgument = ProcedureArgument;
  mCpuMpData.AllWaitEvent      = WaitEvent;
  mCpuMpData.AllTimeout        = TimeoutInMicroseconds;
  mCpuMpData.AllTimeTaken      = 0;
  mCpuMpData.AllTimeoutActive  = (BOOLEAN)(TimeoutInMicroseconds != 0);
  Status                       = gBS->SetTimer (
                                        mCpuMpData.CheckAllAPsEvent,
                                        TimerPeriodic,
                                        POLL_INTERVAL_US
                                        );

  return Status;
}

/** Handles execution of StartupAllAPs when no wait event has been specified.

  @param Procedure             The user-supplied procedure.
  @param ProcedureArgument     The user-supplied procedure argument.
  @param TimeoutInMicroseconds The timeout for the work to be completed. Zero
                                indicates an infinite timeout.
  @param SingleThread          Whether the APs will execute sequentially.
  @param FailedCpuList         User-supplied pointer for list of failed CPUs.

  @return EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
StartupAllAPsNoWaitEvent (
  IN EFI_AP_PROCEDURE  Procedure,
  IN VOID              *ProcedureArgument,
  IN UINTN             TimeoutInMicroseconds,
  IN BOOLEAN           SingleThread,
  IN UINTN             **FailedCpuList
  )
{
  EFI_STATUS   Status;
  UINTN        Index;
  UINTN        NextIndex;
  UINTN        Timeout;
  CPU_AP_DATA  *CpuData;
  BOOLEAN      DispatchError;

  Timeout       = TimeoutInMicroseconds;
  DispatchError = FALSE;

  while (TRUE) {
    for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
      CpuData = &mCpuMpData.CpuData[Index];
      if (IsProcessorBSP (Index)) {
        // Skip BSP
        continue;
      }

      if (!IsProcessorEnabled (Index)) {
        // Skip Disabled processors
        continue;
      }

      switch (GetApState (CpuData)) {
        case CpuStateReady:
          SetApProcedure (CpuData, Procedure, ProcedureArgument);
          Status = DispatchCpu (Index);
          if (EFI_ERROR (Status)) {
            AddProcessorToFailedList (Index, CpuData->State);
            CpuData->State = CpuStateIdle;
            mCpuMpData.StartCount--;
            DispatchError = TRUE;

            if (SingleThread) {
              // Dispatch the next available AP
              Status = GetNextBlockedNumber (&NextIndex);
              if (!EFI_ERROR (Status)) {
                mCpuMpData.CpuData[NextIndex].State = CpuStateReady;
              }
            }
          }

          break;

        case CpuStateFinished:
          mCpuMpData.FinishCount++;
          if (SingleThread) {
            Status = GetNextBlockedNumber (&NextIndex);
            if (!EFI_ERROR (Status)) {
              mCpuMpData.CpuData[NextIndex].State = CpuStateReady;
            }
          }

          CpuData->State = CpuStateIdle;
          break;

        default:
          break;
      }
    }

    if (mCpuMpData.FinishCount == mCpuMpData.StartCount) {
      Status = EFI_SUCCESS;
      break;
    }

    if ((TimeoutInMicroseconds != 0) && (Timeout == 0)) {
      Status = EFI_TIMEOUT;
      break;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
  }

  if (Status == EFI_TIMEOUT) {
    // Add any remaining CPUs to the FailedCpuList
    if (FailedCpuList != NULL) {
      for (Index = 0; Index < mCpuMpData.NumberOfProcessors; Index++) {
        AddProcessorToFailedList (Index, mCpuMpData.CpuData[Index].State);
      }
    }
  }

  if (DispatchError) {
    Status = EFI_NOT_READY;
  }

  return Status;
}
