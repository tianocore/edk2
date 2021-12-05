/** @file
  CPU DXE Module to produce CPU MP Protocol.

  Copyright (c) 2008 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuDxe.h"
#include "CpuMp.h"

EFI_HANDLE  mMpServiceHandle    = NULL;
UINTN       mNumberOfProcessors = 1;

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

  return MpInitLibGetNumberOfProcessors (
           NumberOfProcessors,
           NumberOfEnabledProcessors
           );
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
  return MpInitLibGetProcessorInfo (ProcessorNumber, ProcessorInfoBuffer, NULL);
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
    -# When the APs complete their task or TimeoutInMicroSeconds expires, the MP
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
  return MpInitLibStartupAllAPs (
           Procedure,
           SingleThread,
           WaitEvent,
           TimeoutInMicroseconds,
           ProcedureArgument,
           FailedCpuList
           );
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
  TimeoutInMicroSeconds expires. Otherwise, execution is in non-blocking mode.
  BSP proceeds to the next task without waiting for the AP. If a non-blocking mode
  is requested after the UEFI Event EFI_EVENT_GROUP_READY_TO_BOOT is signaled,
  then EFI_UNSUPPORTED must be returned.

  If the timeout specified by TimeoutInMicroseconds expires before the AP returns
  from Procedure, then execution of Procedure by the AP is terminated. The AP is
  available for subsequent calls to EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() and
  EFI_MP_SERVICES_PROTOCOL.StartupThisAP().

  @param[in]  This                    A pointer to the EFI_MP_SERVICES_PROTOCOL
                                      instance.
  @param[in]  Procedure               A pointer to the function to be run on the
                                      designated AP of the system. See type
                                      EFI_AP_PROCEDURE.
  @param[in]  ProcessorNumber         The handle number of the AP. The range is
                                      from 0 to the total number of logical
                                      processors minus 1. The total number of
                                      logical processors can be retrieved by
                                      EFI_MP_SERVICES_PROTOCOL.GetNumberOfProcessors().
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
                                      by EFI_MP_SERVICES_PROTOCOL.StartupAllAPs()
                                      or EFI_MP_SERVICES_PROTOCOL.StartupThisAP().
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
  return MpInitLibStartupThisAP (
           Procedure,
           ProcessorNumber,
           WaitEvent,
           TimeoutInMicroseconds,
           ProcedureArgument,
           Finished
           );
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
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
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
  return MpInitLibSwitchBSP (ProcessorNumber, EnableOldBSP);
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
  @param[in] ProcessorNumber   The handle number of AP.
                               The range is from 0 to the total number of
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
  return MpInitLibEnableDisableAP (ProcessorNumber, EnableAP, HealthFlag);
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
  @param[out] ProcessorNumber  Pointer to the handle number of AP.
                               The range is from 0 to the total number of
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
  return MpInitLibWhoAmI (ProcessorNumber);
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
  EFI_PROCESSOR_INFORMATION             ProcessorInfo;
  EFI_HEALTH_FLAGS                      BistData;
  UINTN                                 CpuInstanceNumber;

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
    NumberOfData            = SecPlatformInformation2->NumberOfCpus;
    CpuInstance             = SecPlatformInformation2->CpuInstance;
  } else {
    //
    // Otherwise, get gEfiSecPlatformInformationPpiGuid Guided HOB
    //
    GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
    if (GuidHob != NULL) {
      SecPlatformInformation = GET_GUID_HOB_DATA (GuidHob);
      NumberOfData           = 1;
      //
      // SEC Platform Information only includes BSP's BIST information
      // does not have BSP's APIC ID
      //
      BspCpuInstance.CpuLocation                       = GetApicId ();
      BspCpuInstance.InfoRecord.IA32HealthFlags.Uint32 = SecPlatformInformation->IA32HealthFlags.Uint32;
      CpuInstance                                      = &BspCpuInstance;
    } else {
      DEBUG ((DEBUG_INFO, "Does not find any HOB stored CPU BIST information!\n"));
      //
      // Does not find any HOB stored BIST information
      //
      return;
    }
  }

  for (ProcessorNumber = 0; ProcessorNumber < mNumberOfProcessors; ProcessorNumber++) {
    MpInitLibGetProcessorInfo (ProcessorNumber, &ProcessorInfo, &BistData);
    for (CpuInstanceNumber = 0; CpuInstanceNumber < NumberOfData; CpuInstanceNumber++) {
      if (ProcessorInfo.ProcessorId == CpuInstance[CpuInstanceNumber].CpuLocation) {
        //
        // Update CPU health status for MP Services Protocol according to BIST data.
        //
        BistData = CpuInstance[CpuInstanceNumber].InfoRecord.IA32HealthFlags;
      }
    }

    if (BistData.Uint32 != 0) {
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

/**
  Get GDT register value.

  This function is mainly for AP purpose because AP may have different GDT
  table than BSP.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
GetGdtr (
  IN OUT VOID  *Buffer
  )
{
  AsmReadGdtr ((IA32_DESCRIPTOR *)Buffer);
}

/**
  Initializes CPU exceptions handlers for the sake of stack switch requirement.

  This function is a wrapper of InitializeCpuExceptionHandlersEx. It's mainly
  for the sake of AP's init because of EFI_AP_PROCEDURE API requirement.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
InitializeExceptionStackSwitchHandlers (
  IN OUT VOID  *Buffer
  )
{
  CPU_EXCEPTION_INIT_DATA  *EssData;
  IA32_DESCRIPTOR          Idtr;
  EFI_STATUS               Status;

  EssData = Buffer;
  //
  // We don't plan to replace IDT table with a new one, but we should not assume
  // the AP's IDT is the same as BSP's IDT either.
  //
  AsmReadIdtr (&Idtr);
  EssData->Ia32.IdtTable     = (VOID *)Idtr.Base;
  EssData->Ia32.IdtTableSize = Idtr.Limit + 1;
  Status                     = InitializeCpuExceptionHandlersEx (NULL, EssData);
  ASSERT_EFI_ERROR (Status);
}

/**
  Initializes MP exceptions handlers for the sake of stack switch requirement.

  This function will allocate required resources required to setup stack switch
  and pass them through CPU_EXCEPTION_INIT_DATA to each logic processor.

**/
VOID
InitializeMpExceptionStackSwitchHandlers (
  VOID
  )
{
  UINTN                    Index;
  UINTN                    Bsp;
  UINTN                    ExceptionNumber;
  UINTN                    OldGdtSize;
  UINTN                    NewGdtSize;
  UINTN                    NewStackSize;
  IA32_DESCRIPTOR          Gdtr;
  CPU_EXCEPTION_INIT_DATA  EssData;
  UINT8                    *GdtBuffer;
  UINT8                    *StackTop;

  ExceptionNumber = FixedPcdGetSize (PcdCpuStackSwitchExceptionList);
  NewStackSize    = FixedPcdGet32 (PcdCpuKnownGoodStackSize) * ExceptionNumber;

  StackTop = AllocateRuntimeZeroPool (NewStackSize * mNumberOfProcessors);
  ASSERT (StackTop != NULL);
  StackTop += NewStackSize  * mNumberOfProcessors;

  //
  // The default exception handlers must have been initialized. Let's just skip
  // it in this method.
  //
  EssData.Ia32.Revision            = CPU_EXCEPTION_INIT_DATA_REV;
  EssData.Ia32.InitDefaultHandlers = FALSE;

  EssData.Ia32.StackSwitchExceptions      = FixedPcdGetPtr (PcdCpuStackSwitchExceptionList);
  EssData.Ia32.StackSwitchExceptionNumber = ExceptionNumber;
  EssData.Ia32.KnownGoodStackSize         = FixedPcdGet32 (PcdCpuKnownGoodStackSize);

  //
  // Initialize Gdtr to suppress incorrect compiler/analyzer warnings.
  //
  Gdtr.Base  = 0;
  Gdtr.Limit = 0;
  MpInitLibWhoAmI (&Bsp);
  for (Index = 0; Index < mNumberOfProcessors; ++Index) {
    //
    // To support stack switch, we need to re-construct GDT but not IDT.
    //
    if (Index == Bsp) {
      GetGdtr (&Gdtr);
    } else {
      //
      // AP might have different size of GDT from BSP.
      //
      MpInitLibStartupThisAP (GetGdtr, Index, NULL, 0, (VOID *)&Gdtr, NULL);
    }

    //
    // X64 needs only one TSS of current task working for all exceptions
    // because of its IST feature. IA32 needs one TSS for each exception
    // in addition to current task. Since AP is not supposed to allocate
    // memory, we have to do it in BSP. To simplify the code, we allocate
    // memory for IA32 case to cover both IA32 and X64 exception stack
    // switch.
    //
    // Layout of memory to allocate for each processor:
    //    --------------------------------
    //    |            Alignment         |  (just in case)
    //    --------------------------------
    //    |                              |
    //    |        Original GDT          |
    //    |                              |
    //    --------------------------------
    //    |    Current task descriptor   |
    //    --------------------------------
    //    |                              |
    //    |  Exception task descriptors  |  X ExceptionNumber
    //    |                              |
    //    --------------------------------
    //    |  Current task-state segment  |
    //    --------------------------------
    //    |                              |
    //    | Exception task-state segment |  X ExceptionNumber
    //    |                              |
    //    --------------------------------
    //
    OldGdtSize                        = Gdtr.Limit + 1;
    EssData.Ia32.ExceptionTssDescSize = sizeof (IA32_TSS_DESCRIPTOR) *
                                        (ExceptionNumber + 1);
    EssData.Ia32.ExceptionTssSize = sizeof (IA32_TASK_STATE_SEGMENT) *
                                    (ExceptionNumber + 1);
    NewGdtSize = sizeof (IA32_TSS_DESCRIPTOR) +
                 OldGdtSize +
                 EssData.Ia32.ExceptionTssDescSize +
                 EssData.Ia32.ExceptionTssSize;

    GdtBuffer = AllocateRuntimeZeroPool (NewGdtSize);
    ASSERT (GdtBuffer != NULL);

    //
    // Make sure GDT table alignment
    //
    EssData.Ia32.GdtTable     = ALIGN_POINTER (GdtBuffer, sizeof (IA32_TSS_DESCRIPTOR));
    NewGdtSize               -= ((UINT8 *)EssData.Ia32.GdtTable - GdtBuffer);
    EssData.Ia32.GdtTableSize = NewGdtSize;

    EssData.Ia32.ExceptionTssDesc = ((UINT8 *)EssData.Ia32.GdtTable + OldGdtSize);
    EssData.Ia32.ExceptionTss     = ((UINT8 *)EssData.Ia32.GdtTable + OldGdtSize +
                                     EssData.Ia32.ExceptionTssDescSize);

    EssData.Ia32.KnownGoodStackTop = (UINTN)StackTop;
    DEBUG ((
      DEBUG_INFO,
      "Exception stack top[cpu%lu]: 0x%lX\n",
      (UINT64)(UINTN)Index,
      (UINT64)(UINTN)StackTop
      ));

    if (Index == Bsp) {
      InitializeExceptionStackSwitchHandlers (&EssData);
    } else {
      MpInitLibStartupThisAP (
        InitializeExceptionStackSwitchHandlers,
        Index,
        NULL,
        0,
        (VOID *)&EssData,
        NULL
        );
    }

    StackTop -= NewStackSize;
  }
}

/**
  Initializes MP exceptions handlers for special features, such as Heap Guard
  and Stack Guard.
**/
VOID
InitializeMpExceptionHandlers (
  VOID
  )
{
  //
  // Enable non-stop mode for #PF triggered by Heap Guard or NULL Pointer
  // Detection.
  //
  if (HEAP_GUARD_NONSTOP_MODE || NULL_DETECTION_NONSTOP_MODE) {
    RegisterCpuInterruptHandler (EXCEPT_IA32_DEBUG, DebugExceptionHandler);
    RegisterCpuInterruptHandler (EXCEPT_IA32_PAGE_FAULT, PageFaultExceptionHandler);
  }

  //
  // Setup stack switch for Stack Guard feature.
  //
  if (PcdGetBool (PcdCpuStackGuard)) {
    InitializeMpExceptionStackSwitchHandlers ();
  }
}

/**
  Initialize Multi-processor support.

**/
VOID
InitializeMpSupport (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       NumberOfProcessors;
  UINTN       NumberOfEnabledProcessors;

  //
  // Wakeup APs to do initialization
  //
  Status = MpInitLibInitialize ();
  ASSERT_EFI_ERROR (Status);

  MpInitLibGetNumberOfProcessors (&NumberOfProcessors, &NumberOfEnabledProcessors);
  mNumberOfProcessors = NumberOfProcessors;
  DEBUG ((DEBUG_INFO, "Detect CPU count: %d\n", mNumberOfProcessors));

  //
  // Initialize special exception handlers for each logic processor.
  //
  InitializeMpExceptionHandlers ();

  //
  // Update CPU healthy information from Guided HOB
  //
  CollectBistDataFromHob ();

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mMpServiceHandle,
                  &gEfiMpServiceProtocolGuid,
                  &mMpServicesTemplate,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}
