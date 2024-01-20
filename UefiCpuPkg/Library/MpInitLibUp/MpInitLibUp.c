/** @file
  Multiple-Processor initialization Library for uniprocessor platforms.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Ppi/SecPlatformInformation.h>
#include <Protocol/MpService.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

/**
  MP Initialize Library initialization.

  This service will allocate AP reset vector and wakeup all APs to do APs
  initialization.

  This service must be invoked before all other MP Initialize Library
  service are invoked.

  @retval  EFI_SUCCESS           MP initialization succeeds.
  @retval  Others                MP initialization fails.

**/
EFI_STATUS
EFIAPI
MpInitLibInitialize (
  VOID
  )
{
  //
  // Enable the local APIC for Virtual Wire Mode.
  //
  ProgramVirtualWireMode ();

  return EFI_SUCCESS;
}

/**
  Retrieves the number of logical processor in the platform and the number of
  those logical processors that are enabled on this boot. This service may only
  be called from the BSP.

  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL and NumberOfEnabledProcessors
                                  is NULL.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibGetNumberOfProcessors (
  OUT UINTN  *NumberOfProcessors        OPTIONAL,
  OUT UINTN  *NumberOfEnabledProcessors OPTIONAL
  )
{
  *NumberOfProcessors        = 1;
  *NumberOfEnabledProcessors = 1;
  return EFI_SUCCESS;
}

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param[in]  ProcessorNumber       The handle number of processor.
                                    Lower 24 bits contains the actual processor number.
                                    BIT24 indicates if the EXTENDED_PROCESSOR_INFORMATION will be retrived.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.
  @param[out] HealthData            Return processor health data.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibGetProcessorInfo (
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer,
  OUT EFI_HEALTH_FLAGS           *HealthData  OPTIONAL
  )
{
  EFI_HOB_GUID_TYPE                    *GuidHob;
  EFI_SEC_PLATFORM_INFORMATION_RECORD  *SecPlatformInformation;

  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Lower 24 bits contains the actual processor number.
  //
  if ((ProcessorNumber & (BIT24 - 1)) != 0) {
    return EFI_NOT_FOUND;
  }

  ZeroMem (ProcessorInfoBuffer, sizeof (*ProcessorInfoBuffer));
  ProcessorInfoBuffer->StatusFlag = PROCESSOR_AS_BSP_BIT  |
                                    PROCESSOR_ENABLED_BIT |
                                    PROCESSOR_HEALTH_STATUS_BIT;

  if (HealthData != NULL) {
    GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
    if (GuidHob != NULL) {
      SecPlatformInformation = GET_GUID_HOB_DATA (GuidHob);
      HealthData->Uint32     = SecPlatformInformation->IA32HealthFlags.Uint32;
    } else {
      DEBUG ((DEBUG_INFO, "Does not find any HOB stored CPU BIST information!\n"));
      HealthData->Uint32 = 0;
    }
  }

  return EFI_SUCCESS;
}

/**
  This service executes a caller provided function on all enabled APs.

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
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on all the enabled
                                      APs, and go on executing immediately. If
                                      all return from Procedure, or TimeoutInMicroSeconds
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
                                      by MpInitLibStartupAllAPs() or
                                      MPInitLibStartupThisAP().
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
                                      The buffer is allocated by MP Initialization
                                      library, and it's the caller's responsibility to
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
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupAllAPs (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  BOOLEAN           SingleThread,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT UINTN             **FailedCpuList         OPTIONAL
  )
{
  return EFI_NOT_STARTED;
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function.

  @param[in]  Procedure               A pointer to the function to be run on the
                                      designated AP of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  ProcessorNumber         The handle number of the AP. The range is
                                      from 0 to the total number of logical
                                      processors minus 1. The total number of
                                      logical processors can be retrieved by
                                      MpInitLibGetNumberOfProcessors().
  @param[in]  WaitEvent               The event created by the caller with CreateEvent()
                                      service.  If it is NULL, then execute in
                                      blocking mode. BSP waits until this AP finish
                                      or TimeoutInMicroSeconds expires.  If it's
                                      not NULL, then execute in non-blocking mode.
                                      BSP requests the function specified by
                                      Procedure to be started on this AP,
                                      and go on executing immediately. If this AP
                                      return from Procedure or TimeoutInMicroSeconds
                                      expires, this event is signaled. The BSP
                                      can use the CheckEvent() or WaitForEvent()
                                      services to check the state of event.  Type
                                      EFI_EVENT is defined in CreateEvent() in
                                      the Unified Extensible Firmware Interface
                                      Specification.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      this AP to finish this Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity.  If the timeout expires before
                                      this AP returns from Procedure, then Procedure
                                      on the AP is terminated. The
                                      AP is available for next function assigned
                                      by MpInitLibStartupAllAPs() or
                                      MpInitLibStartupThisAP().
                                      If the timeout expires in blocking mode,
                                      BSP returns EFI_TIMEOUT.  If the timeout
                                      expires in non-blocking mode, WaitEvent
                                      is signaled with SignalEvent().
  @param[in]  ProcedureArgument       The parameter passed into Procedure on the
                                      specified AP.
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
  @retval EFI_UNSUPPORTED         WaitEvent is not NULL if non-blocking mode is not
                                  supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before
                                  the specified AP has finished.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupThisAP (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             ProcessorNumber,
  IN  EFI_EVENT         WaitEvent               OPTIONAL,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN           *Finished               OPTIONAL
  )
{
  return EFI_INVALID_PARAMETER;
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. This call can only be performed
  by the current BSP.

  @param[in] ProcessorNumber   The handle number of AP that is to become the new
                               BSP. The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
  @param[in] EnableOldBSP      If TRUE, then the old BSP will be listed as an
                               enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to
                                  this service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or
                                  a disabled AP.
  @retval EFI_NOT_READY           The specified AP is busy.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibSwitchBSP (
  IN UINTN    ProcessorNumber,
  IN BOOLEAN  EnableOldBSP
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This service lets the caller enable or disable an AP from this point onward.
  This service may only be called from the BSP.

  @param[in] ProcessorNumber   The handle number of AP.
                               The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().
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
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibEnableDisableAP (
  IN  UINTN    ProcessorNumber,
  IN  BOOLEAN  EnableAP,
  IN  UINT32   *HealthFlag OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  @param[out] ProcessorNumber  Pointer to the handle number of AP.
                               The range is from 0 to the total number of
                               logical processors minus 1. The total number of
                               logical processors can be retrieved by
                               MpInitLibGetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
MpInitLibWhoAmI (
  OUT UINTN  *ProcessorNumber
  )
{
  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ProcessorNumber = 0;
  return EFI_SUCCESS;
}

/**
  This service executes a caller provided function on all enabled CPUs.

  @param[in]  Procedure               A pointer to the function to be run on
                                      enabled APs of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  TimeoutInMicroseconds   Indicates the time limit in microseconds for
                                      APs to return from Procedure, either for
                                      blocking or non-blocking mode. Zero means
                                      infinity. TimeoutInMicroseconds is ignored
                                      for BSP.
  @param[in]  ProcedureArgument       The parameter passed into Procedure for
                                      all APs.

  @retval EFI_SUCCESS             CPU have finished the procedure.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.

**/
EFI_STATUS
EFIAPI
MpInitLibStartupAllCPUs (
  IN  EFI_AP_PROCEDURE  Procedure,
  IN  UINTN             TimeoutInMicroseconds,
  IN  VOID              *ProcedureArgument      OPTIONAL
  )
{
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Procedure (ProcedureArgument);

  return EFI_SUCCESS;
}
