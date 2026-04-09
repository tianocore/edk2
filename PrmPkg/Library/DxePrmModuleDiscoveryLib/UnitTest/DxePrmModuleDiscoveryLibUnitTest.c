/** @file

  Unit tests for the PRM Module Discovery Library.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrmModuleDiscoveryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UnitTestLib.h>

#include "../PrmModuleDiscovery.h"

#define UNIT_TEST_NAME     "PRM Module Discovery Library Unit Test"
#define UNIT_TEST_VERSION  "0.1"

/// === TEST CASES =================================================================================

/// ===== CREATE NEW PRM MODULE IMAGE CONTEXT LIST ENTRY TESTS SUITE ==================================================

/**
  Verifies that the buffer returned can be deallocated.

  @param[in]  Context             [Optional] An optional context parameter.
                                  Not used in this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
PrmModuleImageContextListEntryShouldDeallocate (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *ListEntry;

  ListEntry = CreateNewPrmModuleImageContextListEntry ();

  UT_ASSERT_NOT_NULL (ListEntry);
  if (ListEntry != NULL) {
    FreePool (ListEntry);
  }

  return UNIT_TEST_PASSED;
}

/**
  Verifies that the list entry signature is set to the appropriate value.

  @param[in]  Context             [Optional] An optional context parameter.
                                  Not used in this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
PrmModuleImageContextListEntrySignatureShouldBeValid (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *ListEntry;

  ListEntry = CreateNewPrmModuleImageContextListEntry ();

  UT_ASSERT_TRUE (ListEntry->Signature == PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY_SIGNATURE);

  if (ListEntry != NULL) {
    FreePool (ListEntry);
  }

  return UNIT_TEST_PASSED;
}

/**
  Verifies that the Context buffer in the list entry is initialized to zero.

  @param[in]  Context             [Optional] An optional context parameter.
                                  Not used in this unit test.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped..

**/
UNIT_TEST_STATUS
EFIAPI
PrmModuleImageContextListEntryImageContextShouldBeZeroed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  PRM_MODULE_IMAGE_CONTEXT_LIST_ENTRY  *ListEntry;
  PRM_MODULE_IMAGE_CONTEXT             ImageContext;

  ListEntry = CreateNewPrmModuleImageContextListEntry ();

  ZeroMem (&ImageContext, sizeof (ImageContext));
  UT_ASSERT_MEM_EQUAL (&ListEntry->Context, &ImageContext, sizeof (ImageContext));

  if (ListEntry != NULL) {
    FreePool (ListEntry);
  }

  return UNIT_TEST_PASSED;
}

/// === TEST ENGINE ================================================================================

/**
  Entry point for the PRM Context Buffer Library unit tests.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
int
main (
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      CreateNewPrmModuleImageContextListEntryTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  Status =  CreateUnitTestSuite (
              &CreateNewPrmModuleImageContextListEntryTests,
              Framework,
              "Create New PRM Module Image Context List Entry Tests",
              "PrmModuleDiscoveryLib.CreateNewPrmModuleImageContextListEntry",
              NULL,
              NULL
              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for PrmModuleDiscoveryLib.CreateNewPrmModuleImageContextListEntry\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (
    CreateNewPrmModuleImageContextListEntryTests,
    "",
    "PrmModuleDiscoveryLib.CreateNewPrmModuleImageContextListEntry.ListEntryShouldDeallocate",
    PrmModuleImageContextListEntryShouldDeallocate,
    NULL,
    NULL,
    NULL
    );

  AddTestCase (
    CreateNewPrmModuleImageContextListEntryTests,
    "",
    "PrmModuleDiscoveryLib.CreateNewPrmModuleImageContextListEntry.ListEntrySignatureShouldBeValid",
    PrmModuleImageContextListEntrySignatureShouldBeValid,
    NULL,
    NULL,
    NULL
    );

  AddTestCase (
    CreateNewPrmModuleImageContextListEntryTests,
    "",
    "PrmModuleDiscoveryLib.CreateNewPrmModuleImageContextListEntry.ListEntryImageContextShouldBeZeroed",
    PrmModuleImageContextListEntryImageContextShouldBeZeroed,
    NULL,
    NULL,
    NULL
    );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}
