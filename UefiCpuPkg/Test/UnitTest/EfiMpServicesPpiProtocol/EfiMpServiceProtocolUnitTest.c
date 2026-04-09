/** @file
  PEI Module to test EfiMpServiceProtocol.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include "EfiMpServicesUnitTestCommom.h"

#define UNIT_TEST_NAME     "EfiMpServiceProtocol Unit Test"
#define UNIT_TEST_VERSION  "0.1"

/**
  Get EFI_MP_SERVICES_PROTOCOL pointer.

  @param[out] MpServices    Pointer to the buffer where EFI_MP_SERVICES_PROTOCOL is stored

  @retval EFI_SUCCESS       EFI_MP_SERVICES_PROTOCOL interface is returned
  @retval EFI_NOT_FOUND     EFI_MP_SERVICES_PROTOCOL interface is not found
**/
EFI_STATUS
MpServicesUnitTestGetMpServices (
  OUT MP_SERVICES  *MpServices
  )
{
  return gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpServices->Protocol);
}

/**
  Retrieve the number of logical processor in the platform and the number of those logical processors that
  are enabled on this boot.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[out] NumberOfProcessors  Pointer to the total number of logical processors in the system, including
                                  the BSP and disabled APs.
  @param[out] NumberOfEnabledProcessors Pointer to the number of processors in the system that are enabled.

  @retval EFI_SUCCESS       Retrieve the number of logical processor successfully
  @retval Others            Retrieve the number of logical processor unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestGetNumberOfProcessors (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *NumberOfProcessors,
  OUT UINTN       *NumberOfEnabledProcessors
  )
{
  return MpServices.Protocol->GetNumberOfProcessors (MpServices.Protocol, NumberOfProcessors, NumberOfEnabledProcessors);
}

/**
  Get detailed information on the requested logical processor.

  @param[in]  MpServices          MP_SERVICES structure.
  @param[in]  ProcessorNumber     The handle number of the processor.
  @param[out] ProcessorInfoBuffer Pointer to the buffer where the processor information is stored.

  @retval EFI_SUCCESS       Get information on the requested logical processor successfully
  @retval Others            Get information on the requested logical processor unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestGetProcessorInfo (
  IN MP_SERVICES                 MpServices,
  IN UINTN                       ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  )
{
  return MpServices.Protocol->GetProcessorInfo (MpServices.Protocol, ProcessorNumber, ProcessorInfoBuffer);
}

/**
  Execute a caller provided function on all enabled APs.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  SingleThread  If TRUE, then all the enabled APs execute the function specified by Procedure
                            one by one, in ascending order of processor handle number.
                            If FALSE, then all the enabled APs execute the function specified by Procedure
                            simultaneously.
  @param[in]  TimeoutInMicroSeconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.

  @retval EFI_SUCCESS       Execute a caller provided function on all enabled APs successfully
  @retval Others            Execute a caller provided function on all enabled APs unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupAllAPs (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN BOOLEAN           SingleThread,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  )
{
  return MpServices.Protocol->StartupAllAPs (MpServices.Protocol, Procedure, SingleThread, NULL, TimeoutInMicroSeconds, ProcedureArgument, NULL);
}

/**
  Caller gets one enabled AP to execute a caller-provided function.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled APs of the system.
  @param[in]  ProcessorNumber       The handle number of the AP.
  @param[in]  TimeoutInMicroSeconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all APs.


  @retval EFI_SUCCESS       Caller gets one enabled AP to execute a caller-provided function successfully
  @retval Others            Caller gets one enabled AP to execute a caller-provided function unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupThisAP (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN UINTN             ProcessorNumber,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  )
{
  return MpServices.Protocol->StartupThisAP (MpServices.Protocol, Procedure, ProcessorNumber, NULL, TimeoutInMicroSeconds, ProcedureArgument, NULL);
}

/**
  Switch the requested AP to be the BSP from that point onward.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  ProcessorNumber The handle number of AP that is to become the new BSP.
  @param[in]  EnableOldBSP  If TRUE, the old BSP will be listed as an enabled AP. Otherwise, it will be disabled.

  @retval EFI_SUCCESS       Switch the requested AP to be the BSP successfully
  @retval Others            Switch the requested AP to be the BSP unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestSwitchBSP (
  IN MP_SERVICES  MpServices,
  IN UINTN        ProcessorNumber,
  IN BOOLEAN      EnableOldBSP
  )
{
  return MpServices.Protocol->SwitchBSP (MpServices.Protocol, ProcessorNumber, EnableOldBSP);
}

/**
  Caller enables or disables an AP from this point onward.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  ProcessorNumber The handle number of the AP.
  @param[in]  EnableAP      Specifies the new state for the processor for enabled, FALSE for disabled.
  @param[in]  HealthFlag    If not NULL, a pointer to a value that specifies the new health status of the AP.

  @retval EFI_SUCCESS       Caller enables or disables an AP successfully.
  @retval Others            Caller enables or disables an AP unsuccessfully.
**/
EFI_STATUS
MpServicesUnitTestEnableDisableAP (
  IN MP_SERVICES  MpServices,
  IN UINTN        ProcessorNumber,
  IN BOOLEAN      EnableAP,
  IN UINT32       *HealthFlag
  )
{
  return MpServices.Protocol->EnableDisableAP (MpServices.Protocol, ProcessorNumber, EnableAP, HealthFlag);
}

/**
  Get the handle number for the calling processor.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[out] ProcessorNumber The handle number for the calling processor.

  @retval EFI_SUCCESS       Get the handle number for the calling processor successfully.
  @retval Others            Get the handle number for the calling processor unsuccessfully.
**/
EFI_STATUS
MpServicesUnitTestWhoAmI (
  IN MP_SERVICES  MpServices,
  OUT UINTN       *ProcessorNumber
  )
{
  return MpServices.Protocol->WhoAmI (MpServices.Protocol, ProcessorNumber);
}

/**
  Initialize the unit test framework, suite and unit tests for the EfiMpServiceProtocol and run the unit tests.

  @retval EFI_SUCCESS       Initialize the unit test framework, suite, unit tests and run the unit tests successfully.
  @retval Others            Initialize the unit test framework, suite, unit tests or run the unit tests unsuccessfully.

**/
EFI_STATUS
EFIAPI
EfiMpServiceProtocolUnitTest (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  MP_SERVICE_UT_CONTEXT       Context;

  Framework                = NULL;
  Context.MpServices.Ppi   = NULL;
  Context.CommonBuffer     = NULL;
  Context.DisabledApNumber = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Create test suite and unit tests for both EdkiiPeiMpServices2Ppi and EfiMpServiceProtocol.
  //
  Status = AddCommonTestCase (Framework, &Context);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in AddCommonTestCase. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard DXE driver or UEFI application entry point for unit test execution from DXE or UEFI Shell.
  Initialize the unit test framework, suite, and unit tests for the EfiMpServiceProtocol and run the unit test.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiMpServiceProtocolUnitTest ();
}
