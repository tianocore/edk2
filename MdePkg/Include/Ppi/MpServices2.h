/** @file
  This file declares UEFI PI Multi-processor 2 PPI.
  This PPI is installed by some platform or chipset-specific PEIM that abstracts
  handling multiprocessor support.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is introduced in PI Version 1.8.

**/

#ifndef __EFI_PEI_MP_SERVICES2_PPI_H__
#define __EFI_PEI_MP_SERVICES2_PPI_H__

#include <Ppi/MpServices.h>

#define EFI_PEI_MP_SERVICES2_PPI_GUID \
  { \
    0x5cb9cb3d, 0x31a4, 0x480c, { 0x94, 0x98, 0x29, 0xd2, 0x69, 0xba, 0xcf, 0xba} \
  }

typedef struct _EFI_PEI_MP_SERVICES2_PPI EFI_PEI_MP_SERVICES2_PPI;

/**
  Get the number of CPU's.

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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_GET_NUMBER_OF_PROCESSORS)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  OUT UINTN                        *NumberOfProcessors,
  OUT UINTN                        *NumberOfEnabledProcessors
  );

/**
  Get information on a specific CPU.

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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_GET_PROCESSOR_INFO)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  UINTN                        ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  );

/**
  Activate all of the application proessors.

  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
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
                                  function assigned by EFI_PEI_MP_SERVICES2_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES2_PPI.StartupThisAP(). If the
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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_STARTUP_ALL_APS)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  EFI_AP_PROCEDURE             Procedure,
  IN  BOOLEAN                      SingleThread,
  IN  UINTN                        TimeoutInMicroSeconds,
  IN  VOID                         *ProcedureArgument      OPTIONAL
  );

/**
  Activate a specific application processor.

  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
  @param[in] Procedure            A pointer to the function to be run on enabled APs of
                                  the system.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
  @param[in] TimeoutInMicroSeconds
                                  Indicates the time limit in microseconds for APs to
                                  return from Procedure, for blocking mode only. Zero
                                  means infinity. If the timeout expires before all APs
                                  return from Procedure, then Procedure on the failed APs
                                  is terminated. All enabled APs are available for next
                                  function assigned by EFI_PEI_MP_SERVICES2_PPI.StartupAllAPs()
                                  or EFI_PEI_MP_SERVICES2_PPI.StartupThisAP(). If the
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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_STARTUP_THIS_AP)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  EFI_AP_PROCEDURE             Procedure,
  IN  UINTN                        ProcessorNumber,
  IN  UINTN                        TimeoutInMicroseconds,
  IN  VOID                         *ProcedureArgument      OPTIONAL
  );

/**
  Switch the boot strap processor.

  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_SWITCH_BSP)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  UINTN                        ProcessorNumber,
  IN  BOOLEAN                      EnableOldBSP
  );

/**
  Enable or disable an application processor.

  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
  @param[in] ProcessorNumber      The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().
  @param[in] EnableAP             Specifies the new state for the processor for enabled,
                                  FALSE for disabled.
  @param[in] HealthFlag           If not NULL, a pointer to a value that specifies the
                                  new health status of the AP. This flag corresponds to
                                  StatusFlag defined in EFI_PEI_MP_SERVICES2_PPI.GetProcessorInfo().
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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_ENABLEDISABLEAP)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  UINTN                        ProcessorNumber,
  IN  BOOLEAN                      EnableAP,
  IN  UINT32                       *HealthFlag      OPTIONAL
  );

/**
  Identify the currently executing processor.

  @param[in]  This                A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES2_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_WHOAMI)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  OUT UINTN                        *ProcessorNumber
  );

/**
  Activate all of the application proessors.

  @param[in] This                 A pointer to the EFI_PEI_MP_SERVICES2_PPI instance.
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
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_MP_SERVICES2_STARTUP_ALL_CPUS)(
  IN  EFI_PEI_MP_SERVICES2_PPI     *This,
  IN  EFI_AP_PROCEDURE             Procedure,
  IN  UINTN                        TimeoutInMicroSeconds,
  IN  VOID                         *ProcedureArgument      OPTIONAL
  );

struct _EFI_PEI_MP_SERVICES2_PPI {
  EFI_PEI_MP_SERVICES2_GET_NUMBER_OF_PROCESSORS    GetNumberOfProcessors;
  EFI_PEI_MP_SERVICES2_GET_PROCESSOR_INFO          GetProcessorInfo;
  EFI_PEI_MP_SERVICES2_STARTUP_ALL_APS             StartupAllAPs;
  EFI_PEI_MP_SERVICES2_STARTUP_THIS_AP             StartupThisAP;
  EFI_PEI_MP_SERVICES2_SWITCH_BSP                  SwitchBSP;
  EFI_PEI_MP_SERVICES2_ENABLEDISABLEAP             EnableDisableAP;
  EFI_PEI_MP_SERVICES2_WHOAMI                      WhoAmI;
  EFI_PEI_MP_SERVICES2_STARTUP_ALL_CPUS            StartupAllCPUs;
};

extern EFI_GUID  gEfiPeiMpServices2PpiGuid;

//
// The EDK II PEI MP Services 2 PPI has been replaced by the PEI MP Services 2
// PPI in the MdePkg. The following definitions are only present for backwards
// compatibility and will be removed in the future.
//
#define EDKII_PEI_MP_SERVICES2_PPI_GUID  EFI_PEI_MP_SERVICES2_PPI_GUID

typedef EFI_PEI_MP_SERVICES2_PPI EDKII_PEI_MP_SERVICES2_PPI;

#endif
