/** @file
  Common header file for EdkiiPeiMpServices2Ppi and EfiMpServiceProtocol unit test.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EFI_MP_SERVICES_UNIT_TEST_COMMOM_H_
#define EFI_MP_SERVICES_UNIT_TEST_COMMOM_H_

#include <PiPei.h>
#include <Ppi/MpServices2.h>
#include <Protocol/MpService.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#define RUN_PROCEDURE_TIMEOUT_VALUE  100000  // microseconds

typedef union {
  EDKII_PEI_MP_SERVICES2_PPI    *Ppi;
  EFI_MP_SERVICES_PROTOCOL      *Protocol;
} MP_SERVICES;

typedef struct {
  MP_SERVICES    MpServices;
  UINTN          BspNumber;
  UINTN          ApNumber;
  UINTN          NumberOfProcessors;
  UINTN          NumberOfEnabledProcessors;
  UINTN          *CommonBuffer;
  EFI_STATUS     ApProcedureReturnStatus;
  UINTN          *DisabledApNumber;
} MP_SERVICE_UT_CONTEXT;

/**
  Get EFI_MP_SERVICES_PROTOCOL pointer.

  @param[out] MpServices    Pointer to the buffer where EFI_MP_SERVICES_PROTOCOL is stored

  @retval EFI_SUCCESS       EFI_MP_SERVICES_PROTOCOL interface is returned
  @retval EFI_NOT_FOUND     EFI_MP_SERVICES_PROTOCOL interface is not found
**/
EFI_STATUS
MpServicesUnitTestGetMpServices (
  OUT MP_SERVICES  *MpServices
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Empty procedure to be run on specified CPU.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
EmptyProcedure (
  IN OUT VOID  *Buffer
  );

/**
  Produce to store ProcessorNumber in CommonBuffer and be run on specified CPU.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
StoreCpuNumbers (
  IN OUT VOID  *Buffer
  );

/**
  Prep routine for Unit test function.
  To save the ProcessorNumber of disabled AP and temporarily enable it.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             Prep routine runs successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  Prep routine runs unsuccessful.
**/
UNIT_TEST_STATUS
EFIAPI
InitUTContext (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Cleanup routine for Unit test function.
  If any processor is disabled unexpectedly then reenable it.

  @param[in]  Context   Context pointer for this test.
**/
VOID
EFIAPI
CheckUTContext (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Cleanup routine for Unit test function.
  It will be called by the last "AddTestCase" to restore AP state and free pointer.

  @param[in]  Context   Context pointer for this test.
**/
VOID
EFIAPI
FreeUTContext (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service WhoAmI.
  The range of ProcessorNumber should be from 0 to NumberOfCPUs minus 1.
  The ProcessorNumbers of all CPUs are unique.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestWhoAmI1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service GetNumberOfProcessors.
  NumberOfProcessors should be greater that 0 and not less than NumberOfEnabledProcessors.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestGetNumberOfProcessors1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service GetNumberOfProcessors.
  When this service is called from an AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestGetNumberOfProcessors2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service GetNumberOfProcessors.
  Call EnableDisableAP() to change the number of enabled AP.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestGetNumberOfProcessors3 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service GetProcessorInfo.
  When all the parameters are valid, all reserved bits of StatusFlag in ProcessorInfoBuffer should be set to zero.
  When all the parameters are valid, the StatusFlag should not have an invalid value (The BSP can never be in the disabled state.).
  When called with nonexistent processor handle, the return status should be EFI_NOT_FOUND.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestGetProcessorInfo1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service GetProcessorInfo.
  When this service is called from an AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestGetProcessorInfo2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service EnableDisableAP.
  When called with BSP number, the return status should be EFI_INVALID_PARAMETER.
  When called with a nonexistent processor handle, the return status should be EFI_NOT_FOUND.
  The AP should be really Enable/Disabled.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestEnableDisableAP1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service EnableDisableAP.
  When run this procedure on AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestEnableDisableAP2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service EnableDisableAP.
  When run this procedure on AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestEnableDisableAP3 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupThisAP.
  When called to startup a BSP, the return status should be EFI_INVALID_PARAMETER.
  When called with a nonexistent processor handle, the return status should be EFI_NOT_FOUND.
  The requested AP should execute the Procedure when called by StartupThisAP.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupThisAP1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupThisAP.
  When this service is called from an AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupThisAP2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupThisAP.
  When timeout expired before the requested AP has finished, the return status should be EFI_TIMEOUT.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupThisAP3 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupThisAP.
  When called with disabled AP, the return status should be EFI_INVALID_PARAMETER.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupThisAP4 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupAllAPs.
  All APs should execute the Procedure when called by StartupAllAPs.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllAPs1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupAllAPs.
  When called in single thread, the return status should be EFI_SUCCESS and AP executes in ascending order
  of processor handle number.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllAPs2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupAllAPs.
  When this service is called from an AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllAPs3 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupAllAPs.
  When called with all AP timeout, the return status should be EFI_TIMEOUT.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllAPs4 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service StartupAllAPs.
  When called with the empty Procedure on all disabled APs, the return status should be EFI_NOT_STARTED.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestStartupAllAPs5 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service SwitchBSP.
  When switch current BSP to be BSP, the return status should be EFI_INVALID_PARAMETER.
  When switch nonexistent processor to be BSP, the return status should be EFI_NOT_FOUND.
  After switch BSP, all APs(includes new AP) should execute the Procedure when called by StartupAllAP.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSwitchBSP1 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service SwitchBSP.
  When run this procedure on AP, the return status should be EFI_DEVICE_ERROR.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSwitchBSP2 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service SwitchBSP.
  When switch a disabled AP to be BSP, the return status should be EFI_INVALID_PARAMETER.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSwitchBSP3 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Unit test of MP service SwitchBSP.
  When SwitchBSP and EnableOldBSP is TRUE, the new BSP should be in the enabled state and the old BSP should
  be in the enabled state.
  When SwitchBSP and EnableOldBSP is False, the new BSP should be in the enabled state and the old BSP should
  be in the disabled state.

  @param[in]  Context   Context pointer for this test.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestSwitchBSP4 (
  IN UNIT_TEST_CONTEXT  Context
  );

/**
  Create test suite and unit tests for both EdkiiPeiMpServices2Ppi and EfiMpServiceProtocol.

  @param[in]  Framework     A pointer to the framework that is being persisted.
  @param[in]  Context       A pointer to the private data buffer.

  @retval     EFI_SUCCESS   Create test suite and unit tests successfully.
  @retval     Others        Create test suite and unit tests unsuccessfully.
**/
EFI_STATUS
AddCommonTestCase (
  IN  UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN  MP_SERVICE_UT_CONTEXT       *Context
  );

#endif
