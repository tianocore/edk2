/** @file
  PEI Module to test APIs defined in EdkiiPeiMpServices2Ppi.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include "EfiMpServicesUnitTestCommom.h"

#define UNIT_TEST_NAME     "EdkiiPeiMpServices2Ppi Unit Test"
#define UNIT_TEST_VERSION  "0.1"

/**
  Get EDKII_PEI_MP_SERVICES2_PPI pointer.

  @param[out] MpServices    Pointer to the buffer where EDKII_PEI_MP_SERVICES2_PPI is stored.

  @retval EFI_SUCCESS       EDKII_PEI_MP_SERVICES2_PPI interface is returned
  @retval EFI_NOT_FOUND     EDKII_PEI_MP_SERVICES2_PPI interface is not found
**/
EFI_STATUS
MpServicesUnitTestGetMpServices (
  OUT MP_SERVICES  *MpServices
  )
{
  return PeiServicesLocatePpi (&gEdkiiPeiMpServices2PpiGuid, 0, NULL, (VOID **)&MpServices->Ppi);
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
  return MpServices.Ppi->GetNumberOfProcessors (MpServices.Ppi, NumberOfProcessors, NumberOfEnabledProcessors);
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
  return MpServices.Ppi->GetProcessorInfo (MpServices.Ppi, ProcessorNumber, ProcessorInfoBuffer);
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
  return MpServices.Ppi->StartupAllAPs (MpServices.Ppi, Procedure, SingleThread, TimeoutInMicroSeconds, ProcedureArgument);
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
  return MpServices.Ppi->StartupThisAP (MpServices.Ppi, Procedure, ProcessorNumber, TimeoutInMicroSeconds, ProcedureArgument);
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
  return MpServices.Ppi->SwitchBSP (MpServices.Ppi, ProcessorNumber, EnableOldBSP);
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
  return MpServices.Ppi->EnableDisableAP (MpServices.Ppi, ProcessorNumber, EnableAP, HealthFlag);
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
  return MpServices.Ppi->WhoAmI (MpServices.Ppi, ProcessorNumber);
}

/**
  Execute a caller provided function on all enabled CPUs.

  @param[in]  MpServices    MP_SERVICES structure.
  @param[in]  Procedure     Pointer to the function to be run on enabled CPUs of the system.
  @param[in]  TimeoutInMicroSeconds Indicates the time limit in microseconds for APs to return from Procedure,
                                    for blocking mode only. Zero means infinity.
  @param[in]  ProcedureArgument     The parameter passed into Procedure for all enabled CPUs.

  @retval EFI_SUCCESS       Execute a caller provided function on all enabled CPUs successfully
  @retval Others            Execute a caller provided function on all enabled CPUs unsuccessfully
**/
EFI_STATUS
MpServicesUnitTestStartupAllCPUs (
  IN MP_SERVICES       MpServices,
  IN EFI_AP_PROCEDURE  Procedure,
  IN UINTN             TimeoutInMicroSeconds,
  IN VOID              *ProcedureArgument
  )
{
  return MpServices.Ppi->StartupAllCPUs (MpServices.Ppi, Procedure, TimeoutInMicroSeconds, ProcedureArgument);
}

/**
  Infinite loop procedure to be run on specified AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
ApInfiniteLoopProcedure (
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorNumber;
  volatile BOOLEAN       InfiniteLoop;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  Status = MpServicesUnitTestWhoAmI (LocalContext->MpServices, &ProcessorNumber);
  ASSERT_EFI_ERROR (Status);

  if (ProcessorNumber == LocalContext->BspNumber) {
    InfiniteLoop = FALSE;
  } else {
    InfiniteLoop = TRUE;
  }

  while (InfiniteLoop) {
  }
}

/**
  Procedure to run MP service StartupAllCPUs on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceStartupAllCPUsOnAp (
  IN OUT VOID  *Buffer
  )
{
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestStartupAllCPUs (
                                            LocalContext->MpServices,
                                            (EFI_AP_PROCEDURE)EmptyProcedure,
                                            0,
                                            NULL
                                            );
}

/**
  Unit test of PEI MP service StartupAllCPU.
  All CPUs should execute the Procedure.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllCPUs1 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  SetMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer), 0xFF);
  Status = MpServicesUnitTestStartupAllCPUs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)StoreCpuNumbers,
             0,
             (VOID *)LocalContext
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors; ProcessorIndex++) {
    UT_ASSERT_TRUE (LocalContext->CommonBuffer[ProcessorIndex] == ProcessorIndex);
  }

  return UNIT_TEST_PASSED;
}

/**
  Unit test of PEI MP service StartupAllCPU.
  When this service is called from an AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllCPUs2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    LocalContext->ApNumber = ApNumber;
    Status                 = MpServicesUnitTestStartupThisAP (
                               LocalContext->MpServices,
                               (EFI_AP_PROCEDURE)RunMpServiceStartupAllCPUsOnAp,
                               ApNumber,
                               0,
                               (VOID *)LocalContext
                               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_STATUS_EQUAL (LocalContext->ApProcedureReturnStatus, EFI_DEVICE_ERROR);
    }
  }

  return UNIT_TEST_PASSED;
}

/**
  Unit test of PEI MP service StartupAllCPU.
  When called with all CPUs timeout, the return status should be EFI_TIMEOUT.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllCPUs3 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS             Status;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  Status = MpServicesUnitTestStartupAllCPUs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)ApInfiniteLoopProcedure,
             RUN_PROCEDURE_TIMEOUT_VALUE,
             (VOID *)LocalContext
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_TIMEOUT);

  return UNIT_TEST_PASSED;
}

/**
  Create test suite and unit tests only for EdkiiPeiMpServices2Ppi.

  @param[in]  Framework     A pointer to the framework that is being persisted.
  @param[in]  Context       A pointer to the private data buffer.

  @retval     EFI_SUCCESS   Create test suite and unit tests successfully.
  @retval     Others        Create test suite and unit tests unsuccessfully.
**/
EFI_STATUS
AddTestCaseOnlyForEdkiiPeiMpServices2Ppi (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN  MP_SERVICE_UT_CONTEXT       *Context
  )
{
  EFI_STATUS              Status;
  UNIT_TEST_SUITE_HANDLE  MpServiceStartupAllCPUsTestSuite;

  MpServiceStartupAllCPUsTestSuite = NULL;

  //
  // Test StartupAllCPUs function
  //
  Status = CreateUnitTestSuite (&MpServiceStartupAllCPUsTestSuite, Framework, "Execute a caller provided function on all enabled CPUs", "MpServices.StartupAllCPUs", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceStartupAllCPUs Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceStartupAllCPUsTestSuite, "Test StartupAllCPUs 1", "TestStartupAllCPUs1", TestStartupAllCPUs1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllCPUsTestSuite, "Test StartupAllCPUs 2", "TestStartupAllCPUs2", TestStartupAllCPUs2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllCPUsTestSuite, "Test StartupAllCPUs 3", "TestStartupAllCPUs3", TestStartupAllCPUs3, InitUTContext, CheckUTContext, Context);

  return EFI_SUCCESS;
}

/**
  Standard PEIM entry point for unit test execution from PEI.
  Initialize the unit test framework, suite, and unit tests for the EdkiiPeiMpServices2Ppi and run the unit test.

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Pointer to PEI Services table.

**/
EFI_STATUS
EFIAPI
PeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
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
  // Create test suite and unit tests only for EdkiiPeiMpServices2Ppi.
  //
  Status = AddTestCaseOnlyForEdkiiPeiMpServices2Ppi (Framework, &Context);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in AddTestCaseOnlyForEdkiiPeiMpServices2Ppi. Status = %r\n", Status));
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
