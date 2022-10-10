/** @file
  Common code to test EdkiiPeiMpServices2Ppi and EfiMpServiceProtocol.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EfiMpServicesUnitTestCommom.h"

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
  )
{
  EFI_STATUS                 Status;
  UINTN                      NumberOfProcessors;
  UINTN                      NumberOfEnabledProcessors;
  UINTN                      NumberOfDisabledAPs;
  UINTN                      IndexOfDisabledAPs;
  UINTN                      BspNumber;
  UINTN                      ProcessorNumber;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  if (LocalContext->MpServices.Ppi != NULL) {
    return UNIT_TEST_PASSED;
  }

  Status = MpServicesUnitTestGetMpServices (&LocalContext->MpServices);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = MpServicesUnitTestWhoAmI (LocalContext->MpServices, &BspNumber);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  DEBUG ((DEBUG_INFO, "%a: BspNumber = 0x%x\n", __FUNCTION__, BspNumber));

  Status = MpServicesUnitTestGetNumberOfProcessors (
             LocalContext->MpServices,
             &NumberOfProcessors,
             &NumberOfEnabledProcessors
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  DEBUG ((
    DEBUG_INFO,
    "%a: NumberOfProcessors = 0x%x, NumberOfEnabledProcessors = 0x%x\n",
    __FUNCTION__,
    NumberOfProcessors,
    NumberOfEnabledProcessors
    ));

  LocalContext->BspNumber                 = BspNumber;
  LocalContext->NumberOfProcessors        = NumberOfProcessors;
  LocalContext->NumberOfEnabledProcessors = NumberOfEnabledProcessors;

  LocalContext->CommonBuffer = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfProcessors * sizeof (*LocalContext->CommonBuffer)));
  UT_ASSERT_NOT_NULL (LocalContext->CommonBuffer);

  NumberOfDisabledAPs = NumberOfProcessors - NumberOfEnabledProcessors;
  if ((NumberOfDisabledAPs > 0) && (LocalContext->DisabledApNumber == NULL)) {
    LocalContext->DisabledApNumber = AllocatePages (EFI_SIZE_TO_PAGES (NumberOfDisabledAPs * sizeof (*LocalContext->DisabledApNumber)));
    UT_ASSERT_NOT_NULL (LocalContext->DisabledApNumber);
    ZeroMem (LocalContext->DisabledApNumber, NumberOfDisabledAPs * sizeof (*LocalContext->DisabledApNumber));

    for (ProcessorNumber = 0, IndexOfDisabledAPs = 0; ProcessorNumber < LocalContext->NumberOfProcessors; ProcessorNumber++) {
      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 ProcessorNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      if (!(ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)) {
        //
        // Save ProcessorNumber of disabled AP.
        //
        LocalContext->DisabledApNumber[IndexOfDisabledAPs] = ProcessorNumber;
        IndexOfDisabledAPs++;

        DEBUG ((DEBUG_INFO, "%a: AP(0x%x) is disabled and temporarily enable it.\n", __FUNCTION__, ProcessorNumber));
        Status = MpServicesUnitTestEnableDisableAP (
                   LocalContext->MpServices,
                   ProcessorNumber,
                   TRUE,
                   NULL
                   );
        UT_ASSERT_NOT_EFI_ERROR (Status);
      }
    }

    UT_ASSERT_TRUE (IndexOfDisabledAPs == NumberOfDisabledAPs);
  }

  return UNIT_TEST_PASSED;
}

/**
  Cleanup routine for Unit test function.
  If any processor is disabled unexpectedly then reenable it.

  @param[in]  Context   Context pointer for this test.
**/
VOID
EFIAPI
CheckUTContext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS                 Status;
  UINTN                      NumberOfProcessors;
  UINTN                      NumberOfEnabledProcessors;
  UINTN                      BspNumber;
  UINTN                      ProcessorNumber;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;
  ASSERT (LocalContext->MpServices.Ppi != NULL);

  Status = MpServicesUnitTestWhoAmI (LocalContext->MpServices, &BspNumber);
  ASSERT_EFI_ERROR (Status);

  if (BspNumber != LocalContext->BspNumber) {
    LocalContext->BspNumber = BspNumber;
    DEBUG ((DEBUG_INFO, "%a: New BspNumber = 0x%x\n", __FUNCTION__, BspNumber));
  }

  ASSERT (BspNumber == LocalContext->BspNumber);

  Status = MpServicesUnitTestGetNumberOfProcessors (
             LocalContext->MpServices,
             &NumberOfProcessors,
             &NumberOfEnabledProcessors
             );
  ASSERT_EFI_ERROR (Status);

  if (NumberOfProcessors != LocalContext->NumberOfProcessors) {
    LocalContext->NumberOfProcessors = NumberOfProcessors;
    DEBUG ((DEBUG_INFO, "%a: New NumberOfProcessors = 0x%x\n", __FUNCTION__, NumberOfProcessors));
  }

  if (NumberOfEnabledProcessors != LocalContext->NumberOfProcessors) {
    DEBUG ((DEBUG_INFO, "%a: New NumberOfEnabledProcessors = 0x%x\n", __FUNCTION__, NumberOfEnabledProcessors));

    for (ProcessorNumber = 0; ProcessorNumber < LocalContext->NumberOfProcessors; ProcessorNumber++) {
      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 ProcessorNumber,
                 &ProcessorInfoBuffer
                 );
      ASSERT_EFI_ERROR (Status);

      if (!(ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)) {
        DEBUG ((DEBUG_INFO, "%a: AP(0x%x) is disabled unexpectedly and reenable it.\n", __FUNCTION__, ProcessorNumber));
        Status = MpServicesUnitTestEnableDisableAP (
                   LocalContext->MpServices,
                   ProcessorNumber,
                   TRUE,
                   NULL
                   );
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  Cleanup routine for Unit test function.
  It will be called by the last "AddTestCase" to restore AP state and free pointer.

  @param[in]  Context   Context pointer for this test.
**/
VOID
EFIAPI
FreeUTContext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS             Status;
  UINTN                  NumberOfDisabledAPs;
  UINTN                  IndexOfDisabledAPs;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  CheckUTContext (Context);

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;
  ASSERT (LocalContext->MpServices.Ppi != NULL);

  if (LocalContext->DisabledApNumber != NULL) {
    NumberOfDisabledAPs = LocalContext->NumberOfProcessors - LocalContext->NumberOfEnabledProcessors;
    for (IndexOfDisabledAPs = 0; IndexOfDisabledAPs < NumberOfDisabledAPs; IndexOfDisabledAPs++) {
      DEBUG ((
        DEBUG_INFO,
        "%a: Disable AP(0x%x) to restore its state.\n",
        __FUNCTION__,
        LocalContext->DisabledApNumber[IndexOfDisabledAPs]
        ));

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 LocalContext->DisabledApNumber[IndexOfDisabledAPs],
                 FALSE,
                 NULL
                 );
      ASSERT_EFI_ERROR (Status);
    }

    FreePages (LocalContext->DisabledApNumber, EFI_SIZE_TO_PAGES (NumberOfDisabledAPs * sizeof (*LocalContext->DisabledApNumber)));
  }

  if (LocalContext->CommonBuffer != NULL) {
    FreePages (LocalContext->CommonBuffer, EFI_SIZE_TO_PAGES (LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer)));
  }
}

/**
  Produce to store ProcessorNumber in the corresponding location of CommonBuffer.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
StoreCpuNumbers (
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  Status = MpServicesUnitTestWhoAmI (LocalContext->MpServices, &ProcessorNumber);
  ASSERT_EFI_ERROR (Status);

  //
  // The layout of CommonBuffer (E.g. BspNumber = 2 and NumberOfProcessors = 6)
  // Index  00    01    02    03    04    05
  // Value  00    01    02    03    04    05
  //
  if (ProcessorNumber < LocalContext->NumberOfProcessors) {
    LocalContext->CommonBuffer[ProcessorNumber] = ProcessorNumber;
  }
}

/**
  Produce to store the ProcessorNumber of AP execution order in CommonBuffer.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
StoreAPsExecutionOrder (
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorNumber;
  UINTN                  *ApCounter;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  Status = MpServicesUnitTestWhoAmI (LocalContext->MpServices, &ProcessorNumber);
  ASSERT_EFI_ERROR (Status);

  //
  // The layout of CommonBuffer (E.g. BspNumber = 2 and NumberOfProcessors = 6)
  // Index  00    01    02    03    04    05
  // Value  00    01    03    04    05  ApCounter(5)
  //
  ApCounter                              = &(LocalContext->CommonBuffer[LocalContext->NumberOfProcessors - 1]);
  LocalContext->CommonBuffer[*ApCounter] = ProcessorNumber;
  (*ApCounter)++;
}

/**
  Infinite loop procedure to be run on specified CPU.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
InfiniteLoopProcedure (
  IN OUT VOID  *Buffer
  )
{
  volatile BOOLEAN  InfiniteLoop;

  InfiniteLoop = TRUE;

  while (InfiniteLoop) {
  }
}

/**
  Empty procedure to be run on specified CPU.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
EmptyProcedure (
  IN OUT VOID  *Buffer
  )
{
}

/**
  Procedure to run MP service GetNumberOfProcessors on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceGetNumberOfProcessorsOnAp (
  IN OUT VOID  *Buffer
  )
{
  UINTN                  NumberOfProcessors;
  UINTN                  NumberOfEnabledProcessors;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestGetNumberOfProcessors (
                                            LocalContext->MpServices,
                                            &NumberOfProcessors,
                                            &NumberOfEnabledProcessors
                                            );
}

/**
  Procedure to run MP service GetProcessorInfo on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceGetProcessorInfoOnAp (
  IN OUT VOID  *Buffer
  )
{
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestGetProcessorInfo (
                                            LocalContext->MpServices,
                                            LocalContext->ApNumber,
                                            &ProcessorInfoBuffer
                                            );
}

/**
  Procedure to run MP service EnableDisableAP on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceEnableDisableAPOnAp (
  IN OUT VOID  *Buffer
  )
{
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestEnableDisableAP (
                                            LocalContext->MpServices,
                                            LocalContext->ApNumber,
                                            FALSE,
                                            NULL
                                            );
}

/**
  Procedure to run MP service StartupThisAP on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceStartupThisAPOnAp (
  IN OUT VOID  *Buffer
  )
{
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestStartupThisAP (
                                            LocalContext->MpServices,
                                            (EFI_AP_PROCEDURE)EmptyProcedure,
                                            LocalContext->ApNumber,
                                            0,
                                            NULL
                                            );
}

/**
  Procedure to run MP service StartupAllAPs on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceStartupAllAPsOnAp (
  IN OUT VOID  *Buffer
  )
{
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestStartupAllAPs (
                                            LocalContext->MpServices,
                                            (EFI_AP_PROCEDURE)EmptyProcedure,
                                            FALSE,
                                            0,
                                            NULL
                                            );
}

/**
  Procedure to run MP service SwitchBSP on AP.

  @param[in,out] Buffer   The pointer to private data buffer.
**/
VOID
RunMpServiceSwitchBSPOnAp (
  IN OUT VOID  *Buffer
  )
{
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Buffer;

  LocalContext->ApProcedureReturnStatus = MpServicesUnitTestSwitchBSP (
                                            LocalContext->MpServices,
                                            LocalContext->ApNumber,
                                            TRUE
                                            );
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorNumber;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  Status = MpServicesUnitTestWhoAmI (
             LocalContext->MpServices,
             &ProcessorNumber
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (ProcessorNumber < LocalContext->NumberOfProcessors);

  SetMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer), 0xFF);
  LocalContext->CommonBuffer[ProcessorNumber] = ProcessorNumber;

  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)StoreCpuNumbers,
             FALSE,
             0,
             (VOID *)LocalContext
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // The layout of CommonBuffer (E.g. BspNumber = 2 and NumberOfProcessors = 6)
  // Index  00    01    02    03    04    05
  // Value  00    01    02    03    04    05
  //
  for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors; ProcessorIndex++) {
    UT_ASSERT_TRUE (LocalContext->CommonBuffer[ProcessorIndex] == ProcessorIndex);
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  NumberOfProcessors;
  UINTN                  NumberOfEnabledProcessors;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  Status = MpServicesUnitTestGetNumberOfProcessors (
             LocalContext->MpServices,
             &NumberOfProcessors,
             &NumberOfEnabledProcessors
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (NumberOfProcessors > 0 && NumberOfProcessors >= NumberOfEnabledProcessors);

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceGetNumberOfProcessorsOnAp,
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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  UINTN                  NumberOfProcessors;
  UINTN                  NumberOfEnabledProcessors;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               FALSE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestGetNumberOfProcessors (
                 LocalContext->MpServices,
                 &NumberOfProcessors,
                 &NumberOfEnabledProcessors
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (NumberOfProcessors == LocalContext->NumberOfProcessors);

      if (ApNumber < LocalContext->BspNumber) {
        UT_ASSERT_TRUE (NumberOfEnabledProcessors == LocalContext->NumberOfProcessors - (ApNumber + 1));
      } else {
        UT_ASSERT_TRUE (NumberOfEnabledProcessors == LocalContext->NumberOfProcessors - ApNumber);
      }
    }
  }

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               TRUE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestGetNumberOfProcessors (
                 LocalContext->MpServices,
                 &NumberOfProcessors,
                 &NumberOfEnabledProcessors
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (NumberOfProcessors == LocalContext->NumberOfProcessors);

      if (ApNumber < LocalContext->BspNumber) {
        UT_ASSERT_TRUE (NumberOfEnabledProcessors == ApNumber + 2);
      } else {
        UT_ASSERT_TRUE (NumberOfEnabledProcessors == ApNumber + 1);
      }
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS                 Status;
  UINTN                      ProcessorNumber;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ProcessorNumber = 0; ProcessorNumber <= LocalContext->NumberOfProcessors; ProcessorNumber++) {
    Status = MpServicesUnitTestGetProcessorInfo (
               LocalContext->MpServices,
               ProcessorNumber,
               &ProcessorInfoBuffer
               );

    if (ProcessorNumber == LocalContext->NumberOfProcessors) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE ((ProcessorInfoBuffer.StatusFlag & (UINT32) ~(PROCESSOR_AS_BSP_BIT|PROCESSOR_ENABLED_BIT|PROCESSOR_HEALTH_STATUS_BIT)) == 0);

      if (ProcessorNumber == LocalContext->BspNumber) {
        UT_ASSERT_TRUE ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) && (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT));
      } else {
        UT_ASSERT_TRUE (!(ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT));
      }
    }
  }

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceGetProcessorInfoOnAp,
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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber <= LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               FALSE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else if (ApNumber == LocalContext->NumberOfProcessors) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestStartupThisAP (
                 LocalContext->MpServices,
                 (EFI_AP_PROCEDURE)EmptyProcedure,
                 ApNumber,
                 0,
                 NULL
                 );
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 ApNumber,
                 TRUE,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestStartupThisAP (
                 LocalContext->MpServices,
                 (EFI_AP_PROCEDURE)EmptyProcedure,
                 ApNumber,
                 0,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceEnableDisableAPOnAp,
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
  )
{
  EFI_STATUS                 Status;
  UINTN                      ApNumber;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  UINT32                     OldHealthFlag;
  UINT32                     NewHealthFlag;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestGetProcessorInfo (
               LocalContext->MpServices,
               ApNumber,
               &ProcessorInfoBuffer
               );
    UT_ASSERT_NOT_EFI_ERROR (Status);

    OldHealthFlag = ProcessorInfoBuffer.StatusFlag & PROCESSOR_HEALTH_STATUS_BIT;
    NewHealthFlag = OldHealthFlag ^ PROCESSOR_HEALTH_STATUS_BIT;
    Status        = MpServicesUnitTestEnableDisableAP (
                      LocalContext->MpServices,
                      ApNumber,
                      TRUE,
                      &NewHealthFlag
                      );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 ApNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE ((ProcessorInfoBuffer.StatusFlag & PROCESSOR_HEALTH_STATUS_BIT) == NewHealthFlag);

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 ApNumber,
                 TRUE,
                 &OldHealthFlag
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber <= LocalContext->NumberOfProcessors; ApNumber++) {
    SetMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer), 0xFF);
    Status = MpServicesUnitTestStartupThisAP (
               LocalContext->MpServices,
               (EFI_AP_PROCEDURE)StoreCpuNumbers,
               ApNumber,
               0,
               (VOID *)LocalContext
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else if (ApNumber == LocalContext->NumberOfProcessors) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors; ProcessorIndex++) {
        UT_ASSERT_TRUE (
          ((ProcessorIndex == ApNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == ProcessorIndex)) ||
          ((ProcessorIndex != ApNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == (UINTN) ~0))
          );
      }
    }
  }

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceStartupThisAPOnAp,
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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestStartupThisAP (
               LocalContext->MpServices,
               (EFI_AP_PROCEDURE)InfiniteLoopProcedure,
               ApNumber,
               RUN_PROCEDURE_TIMEOUT_VALUE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_TIMEOUT);
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               FALSE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestStartupThisAP (
                 LocalContext->MpServices,
                 (EFI_AP_PROCEDURE)EmptyProcedure,
                 ApNumber,
                 0,
                 NULL
                 );
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 ApNumber,
                 TRUE,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestStartupThisAP (
                 LocalContext->MpServices,
                 (EFI_AP_PROCEDURE)EmptyProcedure,
                 ApNumber,
                 0,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  SetMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer), 0xFF);
  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)StoreCpuNumbers,
             FALSE,
             0,
             (VOID *)LocalContext
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors; ProcessorIndex++) {
    UT_ASSERT_TRUE (
      ((ProcessorIndex == LocalContext->BspNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == (UINTN) ~0)) ||
      ((ProcessorIndex != LocalContext->BspNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == ProcessorIndex))
      );
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  ZeroMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer));
  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)StoreAPsExecutionOrder,
             TRUE,
             0,
             (VOID *)LocalContext
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // The layout of CommonBuffer (E.g. BspNumber = 2 and NumberOfProcessors = 6)
  // Index  00    01    02    03    04    05
  // Value  00    01    03    04    05  ApCounter(5)
  //
  for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors - 2; ProcessorIndex++) {
    UT_ASSERT_TRUE (LocalContext->CommonBuffer[ProcessorIndex] < LocalContext->CommonBuffer[ProcessorIndex + 1]);
  }

  UT_ASSERT_EQUAL (LocalContext->CommonBuffer[LocalContext->NumberOfProcessors - 1], LocalContext->NumberOfProcessors - 1);

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceStartupAllAPsOnAp,
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
  )
{
  EFI_STATUS             Status;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)InfiniteLoopProcedure,
             TRUE,
             RUN_PROCEDURE_TIMEOUT_VALUE,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_TIMEOUT);

  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)InfiniteLoopProcedure,
             FALSE,
             RUN_PROCEDURE_TIMEOUT_VALUE,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_TIMEOUT);

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  ApNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               FALSE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  Status = MpServicesUnitTestStartupAllAPs (
             LocalContext->MpServices,
             (EFI_AP_PROCEDURE)EmptyProcedure,
             FALSE,
             0,
             NULL
             );
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_STARTED);

  for (ApNumber = 0; ApNumber < LocalContext->NumberOfProcessors; ApNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               ApNumber,
               TRUE,
               NULL
               );

    if (ApNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS             Status;
  UINTN                  NewBspNumber;
  UINTN                  ProcessorIndex;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (NewBspNumber = 0; NewBspNumber <= LocalContext->NumberOfProcessors; NewBspNumber++) {
    Status = MpServicesUnitTestSwitchBSP (
               LocalContext->MpServices,
               NewBspNumber,
               TRUE
               );

    if (NewBspNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else if (NewBspNumber == LocalContext->NumberOfProcessors) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      SetMem (LocalContext->CommonBuffer, LocalContext->NumberOfProcessors * sizeof (*LocalContext->CommonBuffer), 0xFF);
      Status = MpServicesUnitTestStartupAllAPs (
                 LocalContext->MpServices,
                 (EFI_AP_PROCEDURE)StoreCpuNumbers,
                 FALSE,
                 0,
                 (VOID *)LocalContext
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      for (ProcessorIndex = 0; ProcessorIndex < LocalContext->NumberOfProcessors; ProcessorIndex++) {
        UT_ASSERT_TRUE (
          ((ProcessorIndex == NewBspNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == (UINTN) ~0)) ||
          ((ProcessorIndex != NewBspNumber) && (LocalContext->CommonBuffer[ProcessorIndex] == ProcessorIndex))
          );
      }

      Status = MpServicesUnitTestSwitchBSP (
                 LocalContext->MpServices,
                 LocalContext->BspNumber,
                 TRUE
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
                               (EFI_AP_PROCEDURE)RunMpServiceSwitchBSPOnAp,
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
  )
{
  EFI_STATUS             Status;
  UINTN                  NewBspNumber;
  MP_SERVICE_UT_CONTEXT  *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (NewBspNumber = 0; NewBspNumber < LocalContext->NumberOfProcessors; NewBspNumber++) {
    Status = MpServicesUnitTestEnableDisableAP (
               LocalContext->MpServices,
               NewBspNumber,
               FALSE,
               NULL
               );

    if (NewBspNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestSwitchBSP (
                 LocalContext->MpServices,
                 NewBspNumber,
                 TRUE
                 );
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 NewBspNumber,
                 TRUE,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS                 Status;
  UINTN                      NewBspNumber;
  EFI_PROCESSOR_INFORMATION  ProcessorInfoBuffer;
  MP_SERVICE_UT_CONTEXT      *LocalContext;

  LocalContext = (MP_SERVICE_UT_CONTEXT *)Context;

  for (NewBspNumber = 0; NewBspNumber < LocalContext->NumberOfProcessors; NewBspNumber++) {
    Status = MpServicesUnitTestSwitchBSP (
               LocalContext->MpServices,
               NewBspNumber,
               FALSE
               );

    if (NewBspNumber == LocalContext->BspNumber) {
      UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);
    } else {
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 NewBspNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (
        (ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) &&
        (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)
        );

      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 LocalContext->BspNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (
        !(ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) &&
        !(ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)
        );

      Status = MpServicesUnitTestEnableDisableAP (
                 LocalContext->MpServices,
                 LocalContext->BspNumber,
                 TRUE,
                 NULL
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestSwitchBSP (
                 LocalContext->MpServices,
                 LocalContext->BspNumber,
                 TRUE
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);

      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 LocalContext->BspNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (
        (ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) &&
        (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)
        );

      Status = MpServicesUnitTestGetProcessorInfo (
                 LocalContext->MpServices,
                 NewBspNumber,
                 &ProcessorInfoBuffer
                 );
      UT_ASSERT_NOT_EFI_ERROR (Status);
      UT_ASSERT_TRUE (
        !(ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT) &&
        (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT)
        );
    }
  }

  return UNIT_TEST_PASSED;
}

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
  )
{
  EFI_STATUS              Status;
  UNIT_TEST_SUITE_HANDLE  MpServiceWhoAmITestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceGetNumberOfProcessorsTestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceGetProcessorInfoTestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceEnableDisableAPTestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceStartupThisAPTestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceStartupAllAPsTestSuite;
  UNIT_TEST_SUITE_HANDLE  MpServiceSwitchBSPTestSuite;

  MpServiceWhoAmITestSuite                = NULL;
  MpServiceGetNumberOfProcessorsTestSuite = NULL;
  MpServiceGetProcessorInfoTestSuite      = NULL;
  MpServiceEnableDisableAPTestSuite       = NULL;
  MpServiceStartupThisAPTestSuite         = NULL;
  MpServiceStartupAllAPsTestSuite         = NULL;
  MpServiceSwitchBSPTestSuite             = NULL;

  //
  // Test WhoAmI function
  //
  Status = CreateUnitTestSuite (&MpServiceWhoAmITestSuite, Framework, "Identify the currently executing processor", "MpServices.WhoAmI", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceWhoAmI Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceWhoAmITestSuite, "Test WhoAmI 1", "TestWhoAmI1", TestWhoAmI1, InitUTContext, CheckUTContext, Context);

  //
  // Test GetNumberOfProcessors function
  //
  Status = CreateUnitTestSuite (&MpServiceGetNumberOfProcessorsTestSuite, Framework, "Retrieve the number of logical processor", "MpServices.GetNumberOfProcessors", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceGetNumberOfProcessors Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceGetNumberOfProcessorsTestSuite, "Test GetNumberOfProcessors 1", "TestGetNumberOfProcessors1", TestGetNumberOfProcessors1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceGetNumberOfProcessorsTestSuite, "Test GetNumberOfProcessors 2", "TestGetNumberOfProcessors2", TestGetNumberOfProcessors2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceGetNumberOfProcessorsTestSuite, "Test GetNumberOfProcessors 3", "TestGetNumberOfProcessors3", TestGetNumberOfProcessors3, InitUTContext, CheckUTContext, Context);

  //
  // Test GetProcessorInfo function
  //
  Status = CreateUnitTestSuite (&MpServiceGetProcessorInfoTestSuite, Framework, "Get detailed information on the requested logical processor", "MpServices.GetProcessorInfo", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceGetProcessorInfo Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceGetProcessorInfoTestSuite, "Test GetProcessorInfo 1", "TestGetProcessorInfo1", TestGetProcessorInfo1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceGetProcessorInfoTestSuite, "Test GetProcessorInfo 2", "TestGetProcessorInfo2", TestGetProcessorInfo2, InitUTContext, CheckUTContext, Context);

  //
  // Test EnableDisableAP function
  //
  Status = CreateUnitTestSuite (&MpServiceEnableDisableAPTestSuite, Framework, "Caller enables or disables an AP from this point onward", "MpServices.EnableDisableAP", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceEnableDisableAP Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceEnableDisableAPTestSuite, "Test EnableDisableAP 1", "TestEnableDisableAP1", TestEnableDisableAP1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceEnableDisableAPTestSuite, "Test EnableDisableAP 2", "TestEnableDisableAP2", TestEnableDisableAP2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceEnableDisableAPTestSuite, "Test EnableDisableAP 3", "TestEnableDisableAP3", TestEnableDisableAP3, InitUTContext, CheckUTContext, Context);

  //
  // Test StartupThisAP function
  //
  Status = CreateUnitTestSuite (&MpServiceStartupThisAPTestSuite, Framework, "Get the requested AP to execute a caller-provided function", "MpServices.StartupThisAP", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceStartupThisAP Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceStartupThisAPTestSuite, "Test StartupThisAP 1", "TestStartupThisAP1", TestStartupThisAP1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupThisAPTestSuite, "Test StartupThisAP 2", "TestStartupThisAP2", TestStartupThisAP2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupThisAPTestSuite, "Test StartupThisAP 3", "TestStartupThisAP3", TestStartupThisAP3, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupThisAPTestSuite, "Test StartupThisAP 4", "TestStartupThisAP4", TestStartupThisAP4, InitUTContext, CheckUTContext, Context);

  //
  // Test StartupAllAPs function
  //
  Status = CreateUnitTestSuite (&MpServiceStartupAllAPsTestSuite, Framework, "Execute a caller provided function on all enabled APs", "MpServices.StartupAllAPs", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceStartupAllAPs Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceStartupAllAPsTestSuite, "Test StartupAllAPs 1", "TestStartupAllAPs1", TestStartupAllAPs1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllAPsTestSuite, "Test StartupAllAPs 2", "TestStartupAllAPs2", TestStartupAllAPs2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllAPsTestSuite, "Test StartupAllAPs 3", "TestStartupAllAPs3", TestStartupAllAPs3, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllAPsTestSuite, "Test StartupAllAPs 4", "TestStartupAllAPs4", TestStartupAllAPs4, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceStartupAllAPsTestSuite, "Test StartupAllAPs 5", "TestStartupAllAPs5", TestStartupAllAPs5, InitUTContext, CheckUTContext, Context);

  //
  // Test SwitchBSP function
  //
  Status = CreateUnitTestSuite (&MpServiceSwitchBSPTestSuite, Framework, "Switch the requested AP to be the BSP from that point onward", "MpServices.SwitchBSP", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MpServiceSwitchBSP Test Suite\n"));
    return Status;
  }

  AddTestCase (MpServiceSwitchBSPTestSuite, "Test SwitchBSP 1", "TestSwitchBSP1", TestSwitchBSP1, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceSwitchBSPTestSuite, "Test SwitchBSP 2", "TestSwitchBSP2", TestSwitchBSP2, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceSwitchBSPTestSuite, "Test SwitchBSP 3", "TestSwitchBSP3", TestSwitchBSP3, InitUTContext, CheckUTContext, Context);
  AddTestCase (MpServiceSwitchBSPTestSuite, "Test SwitchBSP 4", "TestSwitchBSP4", TestSwitchBSP4, InitUTContext, FreeUTContext, Context);

  return EFI_SUCCESS;
}
