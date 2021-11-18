/** @file
  EDKII_PEI_MP_SERVICES2_PPI Implementation code.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuMpPei.h"

/**
  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  This function is used to retrieve the following information:
    - The number of logical processors that are present in the system.
    - The number of enabled logical processors in the system at the instant
      this call is made.

  Because MP Service Ppi provides services to enable and disable processors
  dynamically, the number of enabled logical processors may vary during the
  course of a boot session.

  If this service is called from an AP, then EFI_DEVICE_ERROR is returned.
  If NumberOfProcessors or NumberOfEnabledProcessors is NULL, then
  EFI_INVALID_PARAMETER is returned. Otherwise, the total number of processors
  is returned in NumberOfProcessors, the number of currently enabled processor
  is returned in NumberOfEnabledProcessors, and EFI_SUCCESS is returned.

  @param[in]  This                Pointer to this instance of the PPI.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors
                                  Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL.
                                  NumberOfEnabledProcessors is NULL.
**/
EFI_STATUS
EFIAPI
EdkiiPeiGetNumberOfProcessors (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  OUT UINTN                       *NumberOfProcessors,
  OUT UINTN                       *NumberOfEnabledProcessors
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

  @param[in]  This                Pointer to this instance of the PPI.
  @param[in]  ProcessorNumber     Pointer to the total number of logical processors in
                                  the system, including the BSP and disabled APs.
  @param[out] ProcessorInfoBuffer Number of processors in the system that are enabled.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
**/
EFI_STATUS
EFIAPI
EdkiiPeiGetProcessorInfo (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  UINTN                       ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION   *ProcessorInfoBuffer
  )
{
  return MpInitLibGetProcessorInfo (ProcessorNumber, ProcessorInfoBuffer, NULL);
}

/**
  This service executes a caller provided function on all enabled APs. APs can
  run either simultaneously or one at a time in sequence. This service supports
  both blocking requests only. This service may only
  be called from the BSP.

  This function is used to dispatch all the enabled APs to the function specified
  by Procedure.  If any enabled AP is busy, then EFI_NOT_READY is returned
  immediately and Procedure is not started on any AP.

  If SingleThread is TRUE, all the enabled APs execute the function specified by
  Procedure one by one, in ascending order of processor handle number. Otherwise,
  all the enabled APs execute the function specified by Procedure simultaneously.

  If the timeout specified by TimeoutInMicroSeconds expires before all APs return
  from Procedure, then Procedure on the failed APs is terminated. All enabled APs
  are always available for further calls to EDKII_PEI_MP_SERVICES2_PPI.StartupAllAPs()
  and EDKII_PEI_MP_SERVICES2_PPI.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the EDKII_PEI_MP_SERVICES2_PPI.StartupAllAPs()
  to make sure that the nature of the code that is executed on the BSP and the
  dispatched APs is well controlled. The MP Services Ppi does not guarantee
  that the Procedure function is MP-safe. Hence, the tasks that can be run in
  parallel are limited to certain independent tasks and well-controlled exclusive
  code. PEI services and Ppis may not be called by APs unless otherwise
  specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroSeconds expires.

  @param[in] This                 A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] SingleThread         If TRUE, then all the enabled APs execute the function
                                  specified by Procedure one by one, in ascending order
                                  of processor handle number. If FALSE, then all the
                                  enabled APs execute the function specified by Procedure
                                  simultaneously.
  @param[in] TimeoutInMicroSeconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EDKII_PEI_MP_SERVICES2_PPI.StartupAllAPs()
                                  or EDKII_PEI_MP_SERVICES2_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before the
                                  timeout expired.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_STARTED         No enabled APs exist in the system.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before all
                                  enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
EdkiiPeiStartupAllAPs (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  EFI_AP_PROCEDURE            Procedure,
  IN  BOOLEAN                     SingleThread,
  IN  UINTN                       TimeoutInMicroSeconds,
  IN  VOID                        *ProcedureArgument      OPTIONAL
  )
{
  return MpInitLibStartupAllAPs (
           Procedure,
           SingleThread,
           NULL,
           TimeoutInMicroSeconds,
           ProcedureArgument,
           NULL
           );
}

/**
  This service lets the caller get one enabled AP to execute a caller-provided
  function. The caller can request the BSP to wait for the completion
  of the AP. This service may only be called from the BSP.

  This function is used to dispatch one enabled AP to the function specified by
  Procedure passing in the argument specified by ProcedureArgument.
  The execution is in blocking mode. The BSP waits until the AP finishes or
  TimeoutInMicroSecondss expires.

  If the timeout specified by TimeoutInMicroseconds expires before the AP returns
  from Procedure, then execution of Procedure by the AP is terminated. The AP is
  available for subsequent calls to EDKII_PEI_MP_SERVICES2_PPI.StartupAllAPs() and
  EDKII_PEI_MP_SERVICES2_PPI.StartupThisAP().

  @param[in] This                 A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EDKII_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
  @param[in] TimeoutInMicroseconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EDKII_PEI_MP_SERVICES2_PPI.StartupAllAPs()
                                  or EDKII_PEI_MP_SERVICES2_PPI.StartupThisAP(). If the
                                  timeout expires in blocking mode, BSP returns
                                  EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS             In blocking mode, specified AP finished before the
                                  timeout expires.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before the
                                  specified AP has finished.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
EdkiiPeiStartupThisAP (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  EFI_AP_PROCEDURE            Procedure,
  IN  UINTN                       ProcessorNumber,
  IN  UINTN                       TimeoutInMicroseconds,
  IN  VOID                        *ProcedureArgument      OPTIONAL
  )
{
  return MpInitLibStartupThisAP (
           Procedure,
           ProcessorNumber,
           NULL,
           TimeoutInMicroseconds,
           ProcedureArgument,
           NULL
           );
}

/**
  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes.   This call can only be performed
  by the current BSP.

  This service switches the requested AP to be the BSP from that point onward.
  This service changes the BSP for all purposes. The new BSP can take over the
  execution of the old BSP and continue seamlessly from where the old one left
  off.

  If the BSP cannot be switched prior to the return from this service, then
  EFI_UNSUPPORTED must be returned.

  @param[in] This                 A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EDKII_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
  @param[in] EnableOldBSP         If TRUE, then the old BSP will be listed as an enabled
                                  AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS             BSP successfully switched.
  @retval EFI_UNSUPPORTED         Switching the BSP cannot be completed prior to this
                                  service returning.
  @retval EFI_UNSUPPORTED         Switching the BSP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the current BSP or a disabled
                                  AP.
  @retval EFI_NOT_READY           The specified AP is busy.
**/
EFI_STATUS
EFIAPI
EdkiiPeiSwitchBSP (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  UINTN                       ProcessorNumber,
  IN  BOOLEAN                     EnableOldBSP
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
  that is compatible with an MP operating system.

  If the enable or disable AP operation cannot be completed prior to the return
  from this service, then EFI_UNSUPPORTED must be returned.

  @param[in] This                 A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EDKII_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
  @param[in] EnableAP             Specifies the new state for the processor for enabled,
                                  FALSE for disabled.
  @param[in] HealthFlag           If not NULL, a pointer to a value that specifies the
                                  new health status of the AP. This flag corresponds to
                                  StatusFlag defined in EDKII_PEI_MP_SERVICES2_PPI.GetProcessorInfo().
                                  Only the PROCESSOR_HEALTH_STATUS_BIT is used. All other
                                  bits are ignored. If it is NULL, this parameter is
                                  ignored.

  @retval EFI_SUCCESS             The specified AP was enabled or disabled successfully.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP cannot be completed prior
                                  to this service returning.
  @retval EFI_UNSUPPORTED         Enabling or disabling an AP is not supported.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_NOT_FOUND           Processor with the handle specified by ProcessorNumber
                                  does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber specifies the BSP.
**/
EFI_STATUS
EFIAPI
EdkiiPeiEnableDisableAP (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  UINTN                       ProcessorNumber,
  IN  BOOLEAN                     EnableAP,
  IN  UINT32                      *HealthFlag OPTIONAL
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
  with EDKII_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  This                A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EDKII_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
EFI_STATUS
EFIAPI
EdkiiPeiWhoAmI (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  OUT UINTN                       *ProcessorNumber
  )
{
  return MpInitLibWhoAmI (ProcessorNumber);
}

/**
  This service executes a caller provided function on all enabled CPUs. CPUs can
  run either simultaneously or one at a time in sequence. This service may only
  be called from the BSP.

  @param[in] This                 A pointer to the EDKII_PEI_MP_SERVICES2_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] TimeoutInMicroSeconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity.  If the timeout expires in blocking
                                  mode, BSP returns EFI_TIMEOUT.
  @param[in] ProcedureArgument    The parameter passed into Procedure for all CPUs.

  @retval EFI_SUCCESS             In blocking mode, all APs have finished before the
                                  timeout expired.
  @retval EFI_DEVICE_ERROR        Caller processor is AP.
  @retval EFI_NOT_READY           Any enabled APs are busy.
  @retval EFI_TIMEOUT             In blocking mode, the timeout expired before all
                                  enabled APs have finished.
  @retval EFI_INVALID_PARAMETER   Procedure is NULL.
**/
EFI_STATUS
EFIAPI
EdkiiPeiStartupAllCPUs (
  IN  EDKII_PEI_MP_SERVICES2_PPI  *This,
  IN  EFI_AP_PROCEDURE            Procedure,
  IN  UINTN                       TimeoutInMicroSeconds,
  IN  VOID                        *ProcedureArgument      OPTIONAL
  )
{
  return MpInitLibStartupAllCPUs (
           Procedure,
           TimeoutInMicroSeconds,
           ProcedureArgument
           );
}

//
// CPU MP2 PPI to be installed
//
EDKII_PEI_MP_SERVICES2_PPI  mMpServices2Ppi = {
  EdkiiPeiGetNumberOfProcessors,
  EdkiiPeiGetProcessorInfo,
  EdkiiPeiStartupAllAPs,
  EdkiiPeiStartupThisAP,
  EdkiiPeiSwitchBSP,
  EdkiiPeiEnableDisableAP,
  EdkiiPeiWhoAmI,
  EdkiiPeiStartupAllCPUs
};
