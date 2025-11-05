/** @file
  Application for Pseudorandom Number Generator Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define  RANDOM_NUMBER_SIZE  256

CONST  UINT8  SeedString[] = "This is the random seed for PRNG verification.";

UINT8  PreviousRandomBuffer[RANDOM_NUMBER_SIZE] = { 0x0 };

UINT8  RandomBuffer[RANDOM_NUMBER_SIZE] = { 0x0 };

UNIT_TEST_STATUS
EFIAPI
TestVerifyPrngGeneration (
  UNIT_TEST_CONTEXT  Context
  )
{
  UINTN    Index;
  BOOLEAN  Status;

  Status = RandomSeed (SeedString, sizeof (SeedString));
  UT_ASSERT_TRUE (Status);

  for (Index = 0; Index < 10; Index++) {
    Status = RandomBytes (RandomBuffer, RANDOM_NUMBER_SIZE);
    UT_ASSERT_TRUE (Status);

    Status = (CompareMem (PreviousRandomBuffer, RandomBuffer, RANDOM_NUMBER_SIZE) == 0);
    UT_ASSERT_FALSE (Status);

    CopyMem (PreviousRandomBuffer, RandomBuffer, RANDOM_NUMBER_SIZE);
  }

  return UNIT_TEST_PASSED;
}

TEST_DESC  mPrngTest[] = {
  //
  // -----Description--------------------------------Class--------------------Function----------------Pre---Post--Context
  //
  { "TestVerifyPrngGeneration()", "CryptoPkg.BaseCryptLib.Prng", TestVerifyPrngGeneration, NULL, NULL, NULL },
};

UINTN  mPrngTestNum = ARRAY_SIZE (mPrngTest);
