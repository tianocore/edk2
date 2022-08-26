/** @file
  Unit tests for the SMBIOS SPD parsing functions.

  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <IndustryStandard/SmBios.h>
#include <Library/SmbiosType17SpdLib.h>

#include "SpdTestData.h"

#define UNIT_TEST_APP_NAME     "SMBIOS SPD Unit Test Application"
#define UNIT_TEST_APP_VERSION  "1.0"

typedef struct {
  const UINT8                  *TestInput;
  UINTN                        TestInputSize;
  const SMBIOS_TABLE_TYPE17    *ExpectedResult;
  EFI_STATUS                   ExpectedStatus;
} SPD_SMBIOS_TEST_CONTEXT;

// ------------------------------------------------ Input------------------Input Size----------------------Output------------------Result------
static SPD_SMBIOS_TEST_CONTEXT  mSizeTest1 = { Ddr4DimmTestData1, DDR4_SPD_LEN, &Ddr4DimmTestData1ExpectedResult, EFI_SUCCESS };
static SPD_SMBIOS_TEST_CONTEXT  mSizeTest2 = { Ddr4DimmTestData2, DDR4_SPD_LEN, &Ddr4DimmTestData2ExpectedResult, EFI_SUCCESS };

/**
 Unit test to verify functionality for DDR4 SPD data.

 @param Context  Unit test context

 @return UNIT_TEST_PASSED
**/
static
UNIT_TEST_STATUS
EFIAPI
SpdCheckTestDdr4 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS               Status;
  SPD_SMBIOS_TEST_CONTEXT  *TestParams;
  SMBIOS_TABLE_TYPE17      *Table;
  UINT8                    *SpdData;

  TestParams = (SPD_SMBIOS_TEST_CONTEXT *)Context;
  Table      = NULL;
  SpdData    = (UINT8 *)TestParams->TestInput;

  //
  // Test case for basic functionality.
  //
  Status = GetSmbiosType17FromSpdData (SpdData, TestParams->TestInputSize, &Table, 0);
  UT_ASSERT_EQUAL (Status, TestParams->ExpectedStatus);
  UT_ASSERT_NOT_NULL (Table);

  UT_ASSERT_EQUAL (Table->Hdr.Length, sizeof (SMBIOS_TABLE_TYPE17));
  UT_ASSERT_EQUAL (Table->TotalWidth, TestParams->ExpectedResult->TotalWidth);
  UT_ASSERT_EQUAL (Table->DataWidth, TestParams->ExpectedResult->DataWidth);
  UT_ASSERT_EQUAL (Table->Size, TestParams->ExpectedResult->Size);
  UT_ASSERT_EQUAL (Table->FormFactor, TestParams->ExpectedResult->FormFactor);
  UT_ASSERT_EQUAL (Table->MemoryType, TestParams->ExpectedResult->MemoryType);

  // In future, we should calculate the speed bin in the library and verify it here.
  //  UT_ASSERT_EQUAL (Table->Speed, TestParams->ExpectedResult->Speed);
  //  UT_ASSERT_EQUAL (Table->ConfiguredMemoryClockSpeed, TestParams->ExpectedResult->ConfiguredMemoryClockSpeed);

  UT_ASSERT_EQUAL (Table->MinimumVoltage, TestParams->ExpectedResult->MinimumVoltage);
  UT_ASSERT_EQUAL (Table->MaximumVoltage, TestParams->ExpectedResult->MaximumVoltage);
  UT_ASSERT_EQUAL (Table->ConfiguredVoltage, TestParams->ExpectedResult->ConfiguredVoltage);
  UT_ASSERT_EQUAL (Table->MemoryTechnology, TestParams->ExpectedResult->MemoryTechnology);
  UT_ASSERT_EQUAL (Table->ModuleManufacturerID, TestParams->ExpectedResult->ModuleManufacturerID);
  UT_ASSERT_EQUAL (Table->MemorySubsystemControllerManufacturerID, TestParams->ExpectedResult->MemorySubsystemControllerManufacturerID);
  UT_ASSERT_EQUAL (Table->NonVolatileSize, TestParams->ExpectedResult->NonVolatileSize);
  UT_ASSERT_EQUAL (Table->VolatileSize, TestParams->ExpectedResult->VolatileSize);
  UT_ASSERT_EQUAL (Table->CacheSize, TestParams->ExpectedResult->CacheSize);
  UT_ASSERT_EQUAL (Table->LogicalSize, TestParams->ExpectedResult->LogicalSize);
  UT_ASSERT_EQUAL (Table->ExtendedSpeed, TestParams->ExpectedResult->ExtendedSpeed);
  UT_ASSERT_EQUAL (Table->ExtendedConfiguredMemorySpeed, TestParams->ExpectedResult->ExtendedConfiguredMemorySpeed);

  FreePool (Table);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  SMBIOS SPD APIs of SmbiosType17SpdLib and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      SpdParseTests;

  Fw = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the SMBIOS SPD Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&SpdParseTests, Fw, "SMBIOS SPD Parsing Tests", "SpdParseTest", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SpdParseTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // --------------Suite-----------Description-------------------Class Name----------Function-------Pre---Post---Context-----
  AddTestCase (SpdParseTests, "SMBIOS SPD Test 1 - DDR4 DIMM", "SmbiosSpd.Test1", SpdCheckTestDdr4, NULL, NULL, &mSizeTest1);
  AddTestCase (SpdParseTests, "SMBIOS SPD Test 2 - DDR4 DIMM", "SmbiosSpd.Test2", SpdCheckTestDdr4, NULL, NULL, &mSizeTest2);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}

/**
  Standard UEFI entry point for target based unit test execution from UEFI Shell.

  @param ImageHandle Image handle.
  @param SystemTable System table.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
BaseLibUnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnitTestingEntry ();
}

/**
  Standard POSIX C entry point for host based unit test execution.

  @param argc Number of arguments
  @param argv Array of arguments

  @return 0 on success; non-zero on failure.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UnitTestingEntry ();
}
