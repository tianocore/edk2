/** @file
  Definitions to install Multiple Processor PPI.

  Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_MP_PEI_H_
#define _CPU_MP_PEI_H_

#include <PiPei.h>

#include <Ppi/MpServices.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MpServices2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/MpInitLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

extern EFI_PEI_PPI_DESCRIPTOR   mPeiCpuMpPpiDesc;

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

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
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
PeiGetNumberOfProcessors (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  OUT UINTN                     *NumberOfProcessors,
  OUT UINTN                     *NumberOfEnabledProcessors
  );

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

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
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
PeiGetProcessorInfo (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI    *This,
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  );

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
  are always available for further calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  and EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If FailedCpuList is not NULL, its
  content points to the list of processor handle numbers in which Procedure was
  terminated.

  Note: It is the responsibility of the consumer of the EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
  to make sure that the nature of the code that is executed on the BSP and the
  dispatched APs is well controlled. The MP Services Ppi does not guarantee
  that the Procedure function is MP-safe. Hence, the tasks that can be run in
  parallel are limited to certain independent tasks and well-controlled exclusive
  code. PEI services and Ppis may not be called by APs unless otherwise
  specified.

  In blocking execution mode, BSP waits until all APs finish or
  TimeoutInMicroSeconds expires.

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
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
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
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
PeiStartupAllAPs (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  BOOLEAN                   SingleThread,
  IN  UINTN                     TimeoutInMicroSeconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL
  );

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
  available for subsequent calls to EFI_PEI_MP_SERVICES_PPI.StartupAllAPs() and
  EFI_PEI_MP_SERVICES_PPI.StartupThisAP().

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] TimeoutInMicroseconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES_PPI.StartupThisAP(). If the
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
PeiStartupThisAP (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL
  );

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

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
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
PeiSwitchBSP (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  IN  UINTN                    ProcessorNumber,
  IN  BOOLEAN                  EnableOldBSP
  );

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

  @param[in] PeiServices          An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().
  @param[in] EnableAP             Specifies the new state for the processor for enabled,
                                  FALSE for disabled.
  @param[in] HealthFlag           If not NULL, a pointer to a value that specifies the
                                  new health status of the AP. This flag corresponds to
                                  StatusFlag defined in EFI_PEI_MP_SERVICES_PPI.GetProcessorInfo().
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
PeiEnableDisableAP (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI   *This,
  IN  UINTN                     ProcessorNumber,
  IN  BOOLEAN                   EnableAP,
  IN  UINT32                    *HealthFlag OPTIONAL
  );

/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
EFI_STATUS
EFIAPI
PeiWhoAmI (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  OUT UINTN                    *ProcessorNumber
  );

/**
  Collects BIST data from PPI.

  This function collects BIST data from Sec Platform Information2 PPI
  or SEC Platform Information PPI.

  @param PeiServices         Pointer to PEI Services Table

**/
VOID
CollectBistDataFromPpi (
  IN CONST EFI_PEI_SERVICES             **PeiServices
  );

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param  PeiServices                The pointer to the PEI Services Table.
  @param  StructureSize              The pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small. The current buffer size needed to
                                     hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2 (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2 *PlatformInformationRecord2
  );

/**
  Migrates the Global Descriptor Table (GDT) to permanent memory.

  @retval   EFI_SUCCESS           The GDT was migrated successfully.
  @retval   EFI_OUT_OF_RESOURCES  The GDT could not be migrated due to lack of available memory.

**/
EFI_STATUS
MigrateGdt (
  VOID
  );

/**
  Initializes MP and exceptions handlers.

  @param  PeiServices                The pointer to the PEI Services Table.

  @retval EFI_SUCCESS     MP was successfully initialized.
  @retval others          Error occurred in MP initialization.

**/
EFI_STATUS
InitializeCpuMpWorker (
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

/**
  Enable/setup stack guard for each processor if PcdCpuStackGuard is set to TRUE.

  Doing this in the memory-discovered callback is to make sure the Stack Guard
  feature to cover as most PEI code as possible.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  There's error in MP initialization.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

extern EFI_PEI_NOTIFY_DESCRIPTOR  mPostMemNotifyList[];

#endif
