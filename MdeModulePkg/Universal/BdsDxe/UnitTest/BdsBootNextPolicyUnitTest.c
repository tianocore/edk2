/** @file
  Host tests for BDS BootNext preservation policy.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#include "BootNextPolicy.h"

#define UNIT_TEST_APP_NAME     "BDS BootNext Policy Unit Test"
#define UNIT_TEST_APP_VERSION  "1.0"

STATIC
UNIT_TEST_STATUS
EFIAPI
NormalBootConsumesBootNext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (
    BdsShouldPreserveCapsuleBootNext (
      TRUE,
      TRUE,
      BOOT_WITH_FULL_CONFIGURATION
      )
    );

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
S4ResumePreservesCapsuleBootNext (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_TRUE (
    BdsShouldPreserveCapsuleBootNext (
      TRUE,
      TRUE,
      BOOT_ON_S4_RESUME
      )
    );

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
UnrelatedBootNextIsNotPreserved (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_FALSE (
    BdsShouldPreserveCapsuleBootNext (
      TRUE,
      FALSE,
      BOOT_WITH_FULL_CONFIGURATION
      )
    );

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
  UNIT_TEST_SUITE_HANDLE      Suite;

  Framework = NULL;
  Suite     = NULL;
  Status    = InitUnitTestFramework (
                &Framework,
                UNIT_TEST_APP_NAME,
                gEfiCallerBaseName,
                UNIT_TEST_APP_VERSION
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CreateUnitTestSuite (
             &Suite,
             Framework,
             "Capsule BootNext policy",
             "Bds.BootNextPolicy",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return Status;
  }

  AddTestCase (Suite, "Normal boot consumes BootNext", "NormalBootNext", NormalBootConsumesBootNext, NULL, NULL, NULL);
  AddTestCase (Suite, "S4 preserves capsule BootNext", "S4BootNext", S4ResumePreservesCapsuleBootNext, NULL, NULL, NULL);
  AddTestCase (Suite, "Unrelated BootNext is not preserved", "UnrelatedBootNext", UnrelatedBootNextIsNotPreserved, NULL, NULL, NULL);

  Status = RunAllTestSuites (Framework);
  FreeUnitTestFramework (Framework);
  return Status;
}

#define BdsBootNextPolicyUnitTestMain  main

INT32
BdsBootNextPolicyUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return UnitTestingEntry ();
}
