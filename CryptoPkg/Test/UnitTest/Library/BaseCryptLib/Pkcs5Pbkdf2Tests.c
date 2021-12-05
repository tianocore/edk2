/** @file
  Application for PKCS#5 PBKDF2 Function Validation.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// PBKDF2 HMAC-SHA1 Test Vector from RFC6070
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Password    = "password"; // Input Password
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        PassLen      = 8;          // Length of Input Password
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Salt        = "salt";     // Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        SaltLen      = 4;          // Length of Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  Count        = 2;          // InterationCount
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  KeyLen       = 20;         // Length of derived key
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKey[] = {
  // Expected output key
  0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c, 0xcd, 0x1e, 0xd9, 0x2a, 0xce, 0x1d, 0x41, 0xf0,
  0xd8, 0xde, 0x89, 0x57
};

UNIT_TEST_STATUS
EFIAPI
TestVerifyPkcs5Pbkdf2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    *OutKey;

  OutKey = AllocatePool (KeyLen);

  //
  // Verify PKCS#5 PBKDF2 Key Derivation Function
  //
  Status = Pkcs5HashPassword (
             PassLen,
             Password,
             SaltLen,
             (CONST UINT8 *)Salt,
             Count,
             SHA1_DIGEST_SIZE,
             KeyLen,
             OutKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Check the output key with the expected key result
  //
  UT_ASSERT_MEM_EQUAL (OutKey, DerivedKey, KeyLen);

  //
  // Release Resources
  //
  FreePool (OutKey);

  return EFI_SUCCESS;
}

TEST_DESC  mPkcs5Test[] = {
  //
  // -----Description------------------------------Class----------------------Function-----------------Pre---Post--Context
  //
  { "TestVerifyPkcs5Pbkdf2()", "CryptoPkg.BaseCryptLib.Pkcs5", TestVerifyPkcs5Pbkdf2, NULL, NULL, NULL },
};

UINTN  mPkcs5TestNum = ARRAY_SIZE (mPkcs5Test);
