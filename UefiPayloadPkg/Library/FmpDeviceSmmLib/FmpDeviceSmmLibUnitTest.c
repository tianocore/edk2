/** @file
  Unit tests for SMMSTORE-backed firmware update policy.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include "FmpDeviceSmmUpdatePolicy.h"

#define UNIT_TEST_APP_NAME     "FmpDeviceSmmLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

STATIC
UNIT_TEST_STATUS
EFIAPI
PreservedStoreKeepsVariableServices (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (FmpDeviceShouldDisableVariableServices (TRUE));
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
UnprovenStoreDisablesVariableServices (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceShouldDisableVariableServices (FALSE));
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
OverlappingRangesAreDetected (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x1000, 0x2000, 0x2000, 0x1000));
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x2000, 0x1000, 0x1000, 0x2000));
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
AdjacentRangesDoNotOverlap (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (FmpDeviceFlashRangesOverlap (0x1000, 0x1000, 0x2000, 0x1000));
  UT_ASSERT_FALSE (FmpDeviceFlashRangesOverlap (0x2000, 0x1000, 0x1000, 0x1000));
  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
MalformedRangesFailClosed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (0x1000, 0, 0x2000, 0x1000));
  UT_ASSERT_TRUE (FmpDeviceFlashRangesOverlap (MAX_UINTN - 1, 2, 0x2000, 0x1000));
  return UNIT_TEST_PASSED;
}

STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      PolicyTests;

  Framework = NULL;
  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = InitUnitTestFramework (
             &Framework,
             UNIT_TEST_APP_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_APP_VERSION
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &PolicyTests,
             Framework,
             "SMMSTORE update policy",
             "FmpDeviceSmmLib.Policy",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  AddTestCase (PolicyTests, "Preserved store keeps variable services", "PreservedStore", PreservedStoreKeepsVariableServices, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Unproven store disables variable services", "UnprovenStore", UnprovenStoreDisablesVariableServices, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Overlapping ranges are detected", "Overlap", OverlappingRangesAreDetected, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Adjacent ranges do not overlap", "Adjacent", AdjacentRangesDoNotOverlap, NULL, NULL, NULL);
  AddTestCase (PolicyTests, "Malformed ranges fail closed", "Malformed", MalformedRangesFailClosed, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}

#define FmpDeviceSmmLibUnitTestMain  main

INT32
FmpDeviceSmmLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return EFI_ERROR (UnitTestingEntry ()) ? 1 : 0;
}
