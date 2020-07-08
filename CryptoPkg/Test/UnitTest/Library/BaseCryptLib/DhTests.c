/** @file
  Application for Diffie-Hellman Primitives Validation.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

VOID    *mDh1;
VOID    *mDh2;

UNIT_TEST_STATUS
EFIAPI
TestVerifyDhPreReq (
  UNIT_TEST_CONTEXT           Context
  )
{
  mDh1 = DhNew ();
  if (mDh1 == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  mDh2 = DhNew ();
  if (mDh2 == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyDhCleanUp (
  UNIT_TEST_CONTEXT           Context
  )
{
  if (mDh1 != NULL) {
    DhFree (mDh1);
    mDh1 = NULL;
  }
  if (mDh2 != NULL) {
    DhFree (mDh2);
    mDh2 = NULL;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyDhGenerateKey (
  UNIT_TEST_CONTEXT           Context
  )
{
  UINT8   Prime[64];
  UINT8   PublicKey1[64];
  UINTN   PublicKey1Length;
  UINT8   PublicKey2[64];
  UINTN   PublicKey2Length;
  UINT8   Key1[64];
  UINTN   Key1Length;
  UINT8   Key2[64];
  UINTN   Key2Length;
  BOOLEAN Status;

  //
  // Initialize Key Length
  //
  PublicKey1Length = sizeof (PublicKey1);
  PublicKey2Length = sizeof (PublicKey2);
  Key1Length       = sizeof (Key1);
  Key2Length       = sizeof (Key2);

  Status = DhGenerateParameter (mDh1, 2, 64, Prime);
  UT_ASSERT_TRUE (Status);

  Status = DhSetParameter (mDh2, 2, 64, Prime);
  UT_ASSERT_TRUE (Status);

  Status = DhGenerateKey (mDh1, PublicKey1, &PublicKey1Length);
  UT_ASSERT_TRUE (Status);

  Status = DhGenerateKey (mDh2, PublicKey2, &PublicKey2Length);
  UT_ASSERT_TRUE (Status);

  Status = DhComputeKey (mDh1, PublicKey2, PublicKey2Length, Key1, &Key1Length);
  UT_ASSERT_TRUE (Status);

  Status = DhComputeKey (mDh2, PublicKey1, PublicKey1Length, Key2, &Key2Length);
  UT_ASSERT_TRUE (Status);

  UT_ASSERT_EQUAL (Key1Length, Key2Length);

  UT_ASSERT_MEM_EQUAL (Key1, Key2, Key1Length);

  return UNIT_TEST_PASSED;
}

TEST_DESC mDhTest[] = {
    //
    // -----Description--------------------------------Class---------------------Function----------------Pre-----------------Post------------Context
    //
    {"TestVerifyDhGenerateKey()",        "CryptoPkg.BaseCryptLib.Dh",   TestVerifyDhGenerateKey,  TestVerifyDhPreReq, TestVerifyDhCleanUp, NULL},
};

UINTN mDhTestNum = ARRAY_SIZE(mDhTest);
