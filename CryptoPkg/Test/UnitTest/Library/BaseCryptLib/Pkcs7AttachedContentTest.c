/** @file
  Application for Pkcs7 Attached Content Validation.

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

#include "Pkcs7AttachedContentSignatures.h"

/**
  TestPkcs7Attached()

  Pass the API a pkcs7 with content data and contents return should not be null.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 contents return not be null.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestPkcs7Attached (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );

  UT_ASSERT_EQUAL (Status, TRUE);

  // AttachedData != NULL
  if (AttachedData == NULL) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestPkcs7Attached()

/**
  TestPkcs7Detached()

  Pass the API a pkcs7 detached (without content data) and contents return should be null.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 detached contents return be null.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestPkcs7Detached (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7DetachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7DetachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, TRUE);

  // Check no AttachedData
  if ((AttachedData != NULL) || (AttachedDataSize != 0)) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestPkcs7Detached()

/**
  TestVerifyPkcs7ContentData()

  Pass the API a pkcs7 with content data and verify contents return.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 contents return were correct.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestVerifyPkcs7ContentData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, TRUE);

  // Compare AttachedData
  if (CompareMem (AttachedData, &AttachedContents, sizeof (AttachedContents)) != 0) {
    UT_ASSERT_TRUE (FALSE);
  }

  return UNIT_TEST_PASSED;
}// TestVerifyPkcs7ContentData()

/**
  TestWithInvalidpkcs7()

  Pass the API a Invalid pkcs7 and ensure that they don't pass.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required pkcs7 were found.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestWithInvalidPkcs7 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7FailTest);

  Status = Pkcs7GetAttachedContent (
             Pck7FailTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  return UNIT_TEST_PASSED;
}// TestWithInvalidPkcs7()

/**
  TestWithInvalidParameters()

  Pass the API a Invalid Parameters and checks function error handling.

  @param[in]  Framework - Unit-test framework handle.
  @param[in]  Context   - Optional context pointer for this test.

  @retval UNIT_TEST_PASSED            - The required API handling invalid parameters good.
  @retval UNIT_TEST_ERROR_TEST_FAILED - Something failed, check the debug output.
**/
static
UNIT_TEST_STATUS
EFIAPI
TestWithInvalidParameters (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN       SignedDataSize;
  UINT8       *AttachedData;
  UINTN       AttachedDataSize;
  EFI_STATUS  Status;

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             NULL,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = (UINTN)(2147483647U + 1U); // INT MAX + 1

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             NULL,
             &AttachedDataSize
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  SignedDataSize = sizeof (Pck7AttachedTest);

  Status = Pkcs7GetAttachedContent (
             Pck7AttachedTest,
             SignedDataSize,
             (VOID **)&AttachedData,
             NULL
             );
  UT_ASSERT_EQUAL (Status, FALSE);

  return UNIT_TEST_PASSED;
}// TestWithInvalidParameters()

TEST_DESC  mPkcs7ContentTest[] = {
  //
  // -----Description--------------------------------Class----------------------------Function------------------------------Pre---Post--Context
  //
  { "TestPkcs7Attached()",          "CryptoPkg.BaseCryptLib.Pkcs7Content",               TestPkcs7Attached,          NULL, NULL, NULL },
  { "TestPkcs7Detached()",          "CryptoPkg.BaseCryptLib.TestPkcs7Detached",          TestPkcs7Detached,          NULL, NULL, NULL },
  { "TestVerifyPkcs7ContentData()", "CryptoPkg.BaseCryptLib.TestVerifyPkcs7ContentData", TestVerifyPkcs7ContentData, NULL, NULL, NULL },
  { "TestWithInvalidPkcs7()",       "CryptoPkg.BaseCryptLib.TestWithInvalidPkcs7",       TestWithInvalidPkcs7,       NULL, NULL, NULL },
  { "TestWithInvalidParameters()",  "CryptoPkg.BaseCryptLib.TestWithInvalidParameters",  TestWithInvalidParameters,  NULL, NULL, NULL },
};

UINTN  mPkcs7ContentTestNum = ARRAY_SIZE (mPkcs7ContentTest);

TEST_DESC  mPkcs7ContentTestMbedTls[] = {
  //
  // -----Description--------------------------------Class----------------------------Function------------------------------Pre---Post--Context
  //
  { "TestWithInvalidPkcs7()",      "CryptoPkg.BaseCryptLib.TestWithInvalidPkcs7",      TestWithInvalidPkcs7,      NULL, NULL, NULL },
  { "TestWithInvalidParameters()", "CryptoPkg.BaseCryptLib.TestWithInvalidParameters", TestWithInvalidParameters, NULL, NULL, NULL },
};

UINTN  mPkcs7ContentTestMbedTlsNum = ARRAY_SIZE (mPkcs7ContentTestMbedTls);
