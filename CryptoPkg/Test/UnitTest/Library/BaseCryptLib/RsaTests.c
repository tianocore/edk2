/** @file
  Application for RSA Primitives Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define  RSA_MODULUS_LENGTH  512

//
// RSA PKCS#1 Validation Data from OpenSSL "Fips_rsa_selftest.c"
//

//
// Public Modulus of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaN[] = {
  0xBB, 0xF8, 0x2F, 0x09, 0x06, 0x82, 0xCE, 0x9C, 0x23, 0x38, 0xAC, 0x2B, 0x9D, 0xA8, 0x71, 0xF7,
  0x36, 0x8D, 0x07, 0xEE, 0xD4, 0x10, 0x43, 0xA4, 0x40, 0xD6, 0xB6, 0xF0, 0x74, 0x54, 0xF5, 0x1F,
  0xB8, 0xDF, 0xBA, 0xAF, 0x03, 0x5C, 0x02, 0xAB, 0x61, 0xEA, 0x48, 0xCE, 0xEB, 0x6F, 0xCD, 0x48,
  0x76, 0xED, 0x52, 0x0D, 0x60, 0xE1, 0xEC, 0x46, 0x19, 0x71, 0x9D, 0x8A, 0x5B, 0x8B, 0x80, 0x7F,
  0xAF, 0xB8, 0xE0, 0xA3, 0xDF, 0xC7, 0x37, 0x72, 0x3E, 0xE6, 0xB4, 0xB7, 0xD9, 0x3A, 0x25, 0x84,
  0xEE, 0x6A, 0x64, 0x9D, 0x06, 0x09, 0x53, 0x74, 0x88, 0x34, 0xB2, 0x45, 0x45, 0x98, 0x39, 0x4E,
  0xE0, 0xAA, 0xB1, 0x2D, 0x7B, 0x61, 0xA5, 0x1F, 0x52, 0x7A, 0x9A, 0x41, 0xF6, 0xC1, 0x68, 0x7F,
  0xE2, 0x53, 0x72, 0x98, 0xCA, 0x2A, 0x8F, 0x59, 0x46, 0xF8, 0xE5, 0xFD, 0x09, 0x1D, 0xBD, 0xCB
  };

//
// Public Exponent of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaE[] = { 0x11 };

//
// Private Exponent of RSA Key
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaD[] = {
  0xA5, 0xDA, 0xFC, 0x53, 0x41, 0xFA, 0xF2, 0x89, 0xC4, 0xB9, 0x88, 0xDB, 0x30, 0xC1, 0xCD, 0xF8,
  0x3F, 0x31, 0x25, 0x1E, 0x06, 0x68, 0xB4, 0x27, 0x84, 0x81, 0x38, 0x01, 0x57, 0x96, 0x41, 0xB2,
  0x94, 0x10, 0xB3, 0xC7, 0x99, 0x8D, 0x6B, 0xC4, 0x65, 0x74, 0x5E, 0x5C, 0x39, 0x26, 0x69, 0xD6,
  0x87, 0x0D, 0xA2, 0xC0, 0x82, 0xA9, 0x39, 0xE3, 0x7F, 0xDC, 0xB8, 0x2E, 0xC9, 0x3E, 0xDA, 0xC9,
  0x7F, 0xF3, 0xAD, 0x59, 0x50, 0xAC, 0xCF, 0xBC, 0x11, 0x1C, 0x76, 0xF1, 0xA9, 0x52, 0x94, 0x44,
  0xE5, 0x6A, 0xAF, 0x68, 0xC5, 0x6C, 0x09, 0x2C, 0xD3, 0x8D, 0xC3, 0xBE, 0xF5, 0xD2, 0x0A, 0x93,
  0x99, 0x26, 0xED, 0x4F, 0x74, 0xA1, 0x3E, 0xDD, 0xFB, 0xE1, 0xA1, 0xCE, 0xCC, 0x48, 0x94, 0xAF,
  0x94, 0x28, 0xC2, 0xB7, 0xB8, 0x88, 0x3F, 0xE4, 0x46, 0x3A, 0x4B, 0xC8, 0x5B, 0x1C, 0xB3, 0xC1
  };

//
// Known Answer Test (KAT) Data for RSA PKCS#1 Signing
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 RsaSignData[] = "OpenSSL FIPS 140-2 Public Key RSA KAT";

//
// Known Signature for the above message, under SHA-1 Digest
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 RsaPkcs1Signature[] = {
  0x71, 0xEE, 0x1A, 0xC0, 0xFE, 0x01, 0x93, 0x54, 0x79, 0x5C, 0xF2, 0x4C, 0x4A, 0xFD, 0x1A, 0x05,
  0x8F, 0x64, 0xB1, 0x6D, 0x61, 0x33, 0x8D, 0x9B, 0xE7, 0xFD, 0x60, 0xA3, 0x83, 0xB5, 0xA3, 0x51,
  0x55, 0x77, 0x90, 0xCF, 0xDC, 0x22, 0x37, 0x8E, 0xD0, 0xE1, 0xAE, 0x09, 0xE3, 0x3D, 0x1E, 0xF8,
  0x80, 0xD1, 0x8B, 0xC2, 0xEC, 0x0A, 0xD7, 0x6B, 0x88, 0x8B, 0x8B, 0xA1, 0x20, 0x22, 0xBE, 0x59,
  0x5B, 0xE0, 0x23, 0x24, 0xA1, 0x49, 0x30, 0xBA, 0xA9, 0x9E, 0xE8, 0xB1, 0x8A, 0x62, 0x16, 0xBF,
  0x4E, 0xCA, 0x2E, 0x4E, 0xBC, 0x29, 0xA8, 0x67, 0x13, 0xB7, 0x9F, 0x1D, 0x04, 0x44, 0xE5, 0x5F,
  0x35, 0x07, 0x11, 0xBC, 0xED, 0x19, 0x37, 0x21, 0xCF, 0x23, 0x48, 0x1F, 0x72, 0x05, 0xDE, 0xE6,
  0xE8, 0x7F, 0x33, 0x8A, 0x76, 0x4B, 0x2F, 0x95, 0xDF, 0xF1, 0x5F, 0x84, 0x80, 0xD9, 0x46, 0xB4
  };

//
// Default public key 0x10001 = 65537
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 DefaultPublicKey[] = {
  0x01, 0x00, 0x01
};

VOID     *mRsa;

UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaPreReq (
  UNIT_TEST_CONTEXT           Context
  )
{
  mRsa = RsaNew ();

  if (mRsa == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyRsaCleanUp (
  UNIT_TEST_CONTEXT           Context
  )
{
  if (mRsa != NULL) {
    RsaFree (mRsa);
    mRsa = NULL;
  }
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaSetGetKeyComponents (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  BOOLEAN  Status;
  UINTN    KeySize;
  UINT8    *KeyBuffer;

  //
  // Set/Get RSA Key Components
  //

  //
  // Set/Get RSA Key N
  //
  Status = RsaSetKey (mRsa, RsaKeyN, RsaN, sizeof (RsaN));
  UT_ASSERT_TRUE (Status);

  KeySize = 0;
  Status = RsaGetKey (mRsa, RsaKeyN, NULL, &KeySize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (KeySize, sizeof (RsaN));

  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (mRsa, RsaKeyN, KeyBuffer, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, sizeof (RsaN));

  UT_ASSERT_MEM_EQUAL (KeyBuffer, RsaN, KeySize);

  FreePool (KeyBuffer);

  //
  // Set/Get RSA Key E
  //
  Status = RsaSetKey (mRsa, RsaKeyE, RsaE, sizeof (RsaE));
  UT_ASSERT_TRUE (Status);

  KeySize = 0;
  Status = RsaGetKey (mRsa, RsaKeyE, NULL, &KeySize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (KeySize, sizeof (RsaE));

  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (mRsa, RsaKeyE, KeyBuffer, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, sizeof (RsaE));

  UT_ASSERT_MEM_EQUAL (KeyBuffer, RsaE, KeySize);

  FreePool (KeyBuffer);

  //
  // Clear/Get RSA Key Components
  //

  //
  // Clear/Get RSA Key N
  //
  Status = RsaSetKey (mRsa, RsaKeyN, NULL, 0);
  UT_ASSERT_TRUE (Status);

  KeySize = 1;
  Status = RsaGetKey (mRsa, RsaKeyN, NULL, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, 0);

  //
  // Clear/Get RSA Key E
  //
  Status = RsaSetKey (mRsa, RsaKeyE, NULL, 0);
  UT_ASSERT_TRUE (Status);

  KeySize = 1;
  Status = RsaGetKey (mRsa, RsaKeyE, NULL, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, 0);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaGenerateKeyComponents (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  BOOLEAN  Status;
  UINTN    KeySize;
  UINT8    *KeyBuffer;

  //
  // Generate RSA Key Components
  //

  Status = RsaGenerateKey (mRsa, RSA_MODULUS_LENGTH, NULL, 0);
  UT_ASSERT_TRUE (Status);

  KeySize = RSA_MODULUS_LENGTH / 8;
  KeyBuffer = AllocatePool (KeySize);
  Status = RsaGetKey (mRsa, RsaKeyE, KeyBuffer, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, 3);
  UT_ASSERT_MEM_EQUAL (KeyBuffer, DefaultPublicKey, 3);

  KeySize = RSA_MODULUS_LENGTH / 8;
  Status = RsaGetKey (mRsa, RsaKeyN, KeyBuffer, &KeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (KeySize, RSA_MODULUS_LENGTH / 8);

  Status = RsaCheckKey (mRsa);
  UT_ASSERT_TRUE (Status);

  //
  // Check invalid RSA key components
  //
  Status = RsaSetKey (mRsa, RsaKeyN, RsaN, sizeof (RsaN));
  UT_ASSERT_TRUE (Status);

  Status = RsaCheckKey (mRsa);
  UT_ASSERT_FALSE (Status);

  Status = RsaSetKey (mRsa, RsaKeyN, KeyBuffer, KeySize);
  UT_ASSERT_TRUE (Status);

  Status = RsaCheckKey (mRsa);
  UT_ASSERT_TRUE (Status);

  Status = RsaSetKey (mRsa, RsaKeyE, RsaE, sizeof (RsaE));
  UT_ASSERT_TRUE (Status);

  Status = RsaCheckKey (mRsa);
  UT_ASSERT_FALSE (Status);

  FreePool (KeyBuffer);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaPkcs1SignVerify (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT8    HashValue[SHA1_DIGEST_SIZE];
  UINTN    HashSize;
  UINT8    *Signature;
  UINTN    SigSize;
  BOOLEAN  Status;

  //
  // SHA-1 Digest Message for PKCS#1 Signature
  //
  HashSize = SHA1_DIGEST_SIZE;
  ZeroMem (HashValue, HashSize);

  Status  = Sha1HashAll (RsaSignData, AsciiStrLen (RsaSignData), HashValue);
  UT_ASSERT_TRUE (Status);

  //
  // Sign RSA PKCS#1-encoded Signature
  //

  Status = RsaSetKey (mRsa, RsaKeyN, RsaN, sizeof (RsaN));
  UT_ASSERT_TRUE (Status);

  Status = RsaSetKey (mRsa, RsaKeyE, RsaE, sizeof (RsaE));
  UT_ASSERT_TRUE (Status);

  Status = RsaSetKey (mRsa, RsaKeyD, RsaD, sizeof (RsaD));
  UT_ASSERT_TRUE (Status);

  SigSize = 0;
  Status  = RsaPkcs1Sign (mRsa, HashValue, HashSize, NULL, &SigSize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_NOT_EQUAL (SigSize, 0);

  Signature = AllocatePool (SigSize);
  Status  = RsaPkcs1Sign (mRsa, HashValue, HashSize, Signature, &SigSize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, sizeof (RsaPkcs1Signature));

  UT_ASSERT_MEM_EQUAL (Signature, RsaPkcs1Signature, SigSize);

  //
  // Verify RSA PKCS#1-encoded Signature
  //
  Status = RsaPkcs1Verify (mRsa, HashValue, HashSize, Signature, SigSize);
  UT_ASSERT_TRUE (Status);

  FreePool(Signature);

  return UNIT_TEST_PASSED;
}

TEST_DESC mRsaTest[] = {
    //
    // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
    //
    {"TestVerifyRsaSetGetKeyComponents()",       "CryptoPkg.BaseCryptLib.Rsa",   TestVerifyRsaSetGetKeyComponents,       TestVerifyRsaPreReq, TestVerifyRsaCleanUp, NULL},
    {"TestVerifyRsaGenerateKeyComponents()",     "CryptoPkg.BaseCryptLib.Rsa",   TestVerifyRsaGenerateKeyComponents,     TestVerifyRsaPreReq, TestVerifyRsaCleanUp, NULL},
    {"TestVerifyRsaPkcs1SignVerify()",           "CryptoPkg.BaseCryptLib.Rsa",   TestVerifyRsaPkcs1SignVerify,           TestVerifyRsaPreReq, TestVerifyRsaCleanUp, NULL},
};

UINTN mRsaTestNum = ARRAY_SIZE(mRsaTest);
