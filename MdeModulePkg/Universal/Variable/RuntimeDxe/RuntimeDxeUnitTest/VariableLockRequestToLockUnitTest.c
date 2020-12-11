/** @file
  This is a host-based unit test for the VariableLockRequestToLock shim.

  Copyright (c) Microsoft Corporation.
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
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <Library/VariablePolicyLib.h>
#include <Library/VariablePolicyHelperLib.h>

#include <Protocol/VariableLock.h>

#define UNIT_TEST_NAME        "VarPol/VarLock Shim Unit Test"
#define UNIT_TEST_VERSION     "1.0"

///=== CODE UNDER TEST ===========================================================================

EFI_STATUS
EFIAPI
VariableLockRequestToLock (
  IN CONST EDKII_VARIABLE_LOCK_PROTOCOL  *This,
  IN       CHAR16                        *VariableName,
  IN       EFI_GUID                      *VendorGuid
  );

///=== TEST DATA ==================================================================================

//
// Test GUID 1 {F955BA2D-4A2C-480C-BFD1-3CC522610592}
//
EFI_GUID  mTestGuid1 = {
  0xf955ba2d, 0x4a2c, 0x480c, {0xbf, 0xd1, 0x3c, 0xc5, 0x22, 0x61, 0x5, 0x92}
};

//
// Test GUID 2 {2DEA799E-5E73-43B9-870E-C945CE82AF3A}
//
EFI_GUID  mTestGuid2 = {
  0x2dea799e, 0x5e73, 0x43b9, {0x87, 0xe, 0xc9, 0x45, 0xce, 0x82, 0xaf, 0x3a}
};

//
// Test GUID 3 {698A2BFD-A616-482D-B88C-7100BD6682A9}
//
EFI_GUID  mTestGuid3 = {
  0x698a2bfd, 0xa616, 0x482d, {0xb8, 0x8c, 0x71, 0x0, 0xbd, 0x66, 0x82, 0xa9}
};

#define TEST_VAR_1_NAME              L"TestVar1"
#define TEST_VAR_2_NAME              L"TestVar2"
#define TEST_VAR_3_NAME              L"TestVar3"

#define TEST_POLICY_ATTRIBUTES_NULL  0
#define TEST_POLICY_MIN_SIZE_NULL    0
#define TEST_POLICY_MAX_SIZE_NULL    MAX_UINT32

#define TEST_POLICY_MIN_SIZE_10      10
#define TEST_POLICY_MAX_SIZE_200     200

///=== HELPER FUNCTIONS ===========================================================================

/**
  Mocked version of GetVariable, for testing.

  @param  VariableName
  @param  VendorGuid
  @param  Attributes
  @param  DataSize
  @param  Data
**/
EFI_STATUS
EFIAPI
StubGetVariableNull (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes,  OPTIONAL
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data         OPTIONAL
  )
{
  UINT32      MockedAttr;
  UINTN       MockedDataSize;
  VOID        *MockedData;
  EFI_STATUS  MockedReturn;

  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);
  check_expected_ptr (DataSize);

  MockedAttr     = (UINT32)mock();
  MockedDataSize = (UINTN)mock();
  MockedData     = (VOID*)(UINTN)mock();
  MockedReturn   = (EFI_STATUS)mock();

  if (Attributes != NULL) {
    *Attributes = MockedAttr;
  }
  if (Data != NULL && !EFI_ERROR (MockedReturn)) {
    CopyMem (Data, MockedData, MockedDataSize);
  }

  *DataSize = MockedDataSize;

  return MockedReturn;
}

//
// Anything you think might be helpful that isn't a test itself.
//

/**
  This is a common setup function that will ensure the library is always
  initialized with the stubbed GetVariable.

  Not used by all test cases, but by most.

  @param[in]  Context  Unit test case context
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
LibInitMocked (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return EFI_ERROR (InitVariablePolicyLib (StubGetVariableNull)) ? UNIT_TEST_ERROR_PREREQUISITE_NOT_MET : UNIT_TEST_PASSED;
}

/**
  Common cleanup function to make sure that the library is always de-initialized
  prior to the next test case.

  @param[in]  Context  Unit test case context
**/
STATIC
VOID
EFIAPI
LibCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  DeinitVariablePolicyLib();
}

///=== TEST CASES =================================================================================

///===== SHIM SUITE ===========================================================

/**
  Test Case that locks a single variable using the Variable Lock Protocol.
  The call is expected to succeed.

  @param[in]  Context  Unit test case context
**/
UNIT_TEST_STATUS
EFIAPI
LockingWithoutAnyPoliciesShouldSucceed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks the same variable twice using the Variable Lock Protocol.
  Both calls are expected to succeed.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
LockingTwiceShouldSucceed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using the Variable Policy Protocol then locks
  the same variable using the Variable Lock Protocol.
  Both calls are expected to succeed.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
LockingALockedVariableShouldSucceed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS             Status;
  VARIABLE_POLICY_ENTRY  *NewEntry;

  //
  // Create a variable policy that locks the variable.
  //
  Status = CreateBasicVariablePolicy (
             &mTestGuid1,
             TEST_VAR_1_NAME,
             TEST_POLICY_MIN_SIZE_NULL,
             TEST_POLICY_MAX_SIZE_200,
             TEST_POLICY_ATTRIBUTES_NULL,
             TEST_POLICY_ATTRIBUTES_NULL,
             VARIABLE_POLICY_TYPE_LOCK_NOW,
             &NewEntry
             );
  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Register the new policy.
  //
  Status = RegisterVariablePolicy (NewEntry);

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using the Variable Policy Protocol with a
  policy other than LOCK_NOW then attempts to lock the same variable using the
  Variable Lock Protocol.  The call to Variable Policy is expected to succeed
  and the call to Variable Lock is expected to fail.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
LockingAnUnlockedVariableShouldFail (
  IN UNIT_TEST_CONTEXT      Context
  )
{
  EFI_STATUS                Status;
  VARIABLE_POLICY_ENTRY     *NewEntry;

  // Create a variable policy that locks the variable.
  Status = CreateVarStateVariablePolicy (&mTestGuid1,
                                         TEST_VAR_1_NAME,
                                         TEST_POLICY_MIN_SIZE_NULL,
                                         TEST_POLICY_MAX_SIZE_200,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         &mTestGuid2,
                                         1,
                                         TEST_VAR_2_NAME,
                                         &NewEntry);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Register the new policy.
  Status = RegisterVariablePolicy (NewEntry);

 // Configure the stub to not care about parameters. We're testing errors.
  expect_any_always( StubGetVariableNull, VariableName );
  expect_any_always( StubGetVariableNull, VendorGuid );
  expect_any_always( StubGetVariableNull, DataSize );

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  will_return( StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL );    // Attributes
  will_return( StubGetVariableNull, 0 );                              // Size
  will_return( StubGetVariableNull, NULL );                           // DataPtr
  will_return( StubGetVariableNull, EFI_NOT_FOUND);                   // Status

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using the Variable Policy Protocol with a
  policy other than LOCK_NOW, but is currently locked.  Then attempts to lock
  the same variable using the Variable Lock Protocol.  The call to Variable
  Policy is expected to succeed and the call to Variable Lock also expected to
  succeed.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
LockingALockedVariableWithMatchingDataShouldSucceed (
  IN UNIT_TEST_CONTEXT      Context
  )
{
  EFI_STATUS                Status;
  VARIABLE_POLICY_ENTRY     *NewEntry;
  UINT8                     Data;

  // Create a variable policy that locks the variable.
  Status = CreateVarStateVariablePolicy (&mTestGuid1,
                                         TEST_VAR_1_NAME,
                                         TEST_POLICY_MIN_SIZE_NULL,
                                         TEST_POLICY_MAX_SIZE_200,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         &mTestGuid2,
                                         1,
                                         TEST_VAR_2_NAME,
                                         &NewEntry);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Register the new policy.
  Status = RegisterVariablePolicy (NewEntry);

 // Configure the stub to not care about parameters. We're testing errors.
  expect_any_always( StubGetVariableNull, VariableName );
  expect_any_always( StubGetVariableNull, VendorGuid );
  expect_any_always( StubGetVariableNull, DataSize );

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  Data = 1;
  will_return( StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL );    // Attributes
  will_return( StubGetVariableNull, sizeof (Data) );                  // Size
  will_return( StubGetVariableNull, &Data );                          // DataPtr
  will_return( StubGetVariableNull, EFI_SUCCESS);                     // Status

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_TRUE (!EFI_ERROR (Status));

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using the Variable Policy Protocol with a
  policy other than LOCK_NOW, but variable data does not match.  Then attempts
  to lock the same variable using the Variable Lock Protocol.  The call to
  Variable Policy is expected to succeed and the call to Variable Lock is
  expected to fail.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
LockingALockedVariableWithNonMatchingDataShouldFail (
  IN UNIT_TEST_CONTEXT      Context
  )
{
  EFI_STATUS                Status;
  VARIABLE_POLICY_ENTRY     *NewEntry;
  UINT8                     Data;

  // Create a variable policy that locks the variable.
  Status = CreateVarStateVariablePolicy (&mTestGuid1,
                                         TEST_VAR_1_NAME,
                                         TEST_POLICY_MIN_SIZE_NULL,
                                         TEST_POLICY_MAX_SIZE_200,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         &mTestGuid2,
                                         1,
                                         TEST_VAR_2_NAME,
                                         &NewEntry);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Register the new policy.
  Status = RegisterVariablePolicy (NewEntry);

 // Configure the stub to not care about parameters. We're testing errors.
  expect_any_always( StubGetVariableNull, VariableName );
  expect_any_always( StubGetVariableNull, VendorGuid );
  expect_any_always( StubGetVariableNull, DataSize );

  // With a policy, make sure that writes still work, since the variable doesn't exist.
  Data = 2;
  will_return( StubGetVariableNull, TEST_POLICY_ATTRIBUTES_NULL );    // Attributes
  will_return( StubGetVariableNull, sizeof (Data) );                  // Size
  will_return( StubGetVariableNull, &Data );                          // DataPtr
  will_return( StubGetVariableNull, EFI_SUCCESS);                     // Status

  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using Variable Lock Protocol Policy Protocol
  then and then attempts to lock the same variable using the Variable Policy
  Protocol.  The call to Variable Lock is expected to succeed and the call to
  Variable Policy is expected to fail.

  @param[in]  Context  Unit test case context
  **/
UNIT_TEST_STATUS
EFIAPI
SettingPolicyForALockedVariableShouldFail (
  IN UNIT_TEST_CONTEXT      Context
  )
{
  EFI_STATUS                Status;
  VARIABLE_POLICY_ENTRY     *NewEntry;

  // Lock the variable.
  Status = VariableLockRequestToLock (NULL, TEST_VAR_1_NAME, &mTestGuid1);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Create a variable policy that locks the variable.
  Status = CreateVarStateVariablePolicy (&mTestGuid1,
                                         TEST_VAR_1_NAME,
                                         TEST_POLICY_MIN_SIZE_NULL,
                                         TEST_POLICY_MAX_SIZE_200,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         TEST_POLICY_ATTRIBUTES_NULL,
                                         &mTestGuid2,
                                         1,
                                         TEST_VAR_2_NAME,
                                         &NewEntry);
  UT_ASSERT_NOT_EFI_ERROR (Status);

  // Register the new policy.
  Status = RegisterVariablePolicy (NewEntry);
  UT_ASSERT_TRUE (EFI_ERROR (Status));

  FreePool (NewEntry);

  return UNIT_TEST_PASSED;
}

/**
  Main entry point to this unit test application.

  Sets up and runs the test suites.
**/
VOID
EFIAPI
UnitTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      ShimTests;

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

  //
  // Add all test suites and tests.
  //
  Status = CreateUnitTestSuite (
             &ShimTests, Framework,
             "Variable Lock Shim Tests", "VarPolicy.VarLockShim", NULL, NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ShimTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (
    ShimTests,
    "Locking a variable with no matching policies should always work", "EmptyPolicies",
    LockingWithoutAnyPoliciesShouldSucceed, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Locking a variable twice should always work", "DoubleLock",
    LockingTwiceShouldSucceed, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Locking a variable that's already locked by another policy should work", "LockAfterPolicy",
    LockingALockedVariableShouldSucceed, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Locking a variable that already has an unlocked policy should fail", "LockAfterUnlockedPolicy",
    LockingAnUnlockedVariableShouldFail, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Locking a variable that already has an locked policy should succeed", "LockAfterLockedPolicyMatchingData",
    LockingALockedVariableWithMatchingDataShouldSucceed, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Locking a variable that already has an locked policy with matching data should succeed", "LockAfterLockedPolicyNonMatchingData",
    LockingALockedVariableWithNonMatchingDataShouldFail, LibInitMocked, LibCleanup, NULL
    );
  AddTestCase (
    ShimTests,
    "Adding a policy for a variable that has previously been locked should always fail", "SetPolicyAfterLock",
    SettingPolicyForALockedVariableShouldFail, LibInitMocked, LibCleanup, NULL
    );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework != NULL) {
    FreeUnitTestFramework (Framework);
  }

  return;
}

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define Main main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
Main (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  UnitTestMain ();
  return 0;
}
