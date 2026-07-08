/** @file
  Application for EdDSA Primitives Validation.

Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define ED448_KEY_SIZE  57
#define ED448_SIG_SIZE  114

//
// Ed448 Raw Private Key (57 bytes) extracted from DER format
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mEd448TestPrivateKey[ED448_KEY_SIZE] = {
  0x1d, 0x72, 0xe7, 0x9b, 0x46, 0x5f, 0xcb, 0xcc, 0x24, 0x07, 0x2e, 0x20,
  0x92, 0xec, 0xb6, 0xdd, 0x11, 0x68, 0x14, 0x0d, 0x84, 0x1f, 0xf9, 0x8f,
  0x13, 0xe5, 0x73, 0x78, 0xb0, 0x42, 0xb7, 0xe5, 0x90, 0x45, 0xcc, 0x15,
  0x07, 0x41, 0x66, 0x46, 0x6a, 0xef, 0xc9, 0x9e, 0x2d, 0xaa, 0x3f, 0x28,
  0xfd, 0xac, 0x7f, 0x23, 0xe2, 0x15, 0x60, 0xaf, 0x7b
};

//
// Ed448 Raw Public Key (57 bytes) - derived from the private key above
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mEd448TestPublicKey[ED448_KEY_SIZE] = {
  0x47, 0xba, 0x8f, 0x77, 0x45, 0x59, 0xee, 0x91, 0x90, 0x52, 0x96, 0x9f,
  0x59, 0xcd, 0xfa, 0xd5, 0x82, 0x00, 0xc1, 0x92, 0x5c, 0x90, 0x58, 0x79,
  0xaa, 0xbd, 0x74, 0xd2, 0x2c, 0x34, 0xe8, 0x1e, 0xce, 0xbb, 0x59, 0x4d,
  0xa9, 0x58, 0xaa, 0x61, 0x86, 0x1c, 0xa3, 0xd6, 0x32, 0x64, 0xda, 0x67,
  0xf7, 0x8b, 0x0e, 0xfa, 0x09, 0xa7, 0x22, 0xfb, 0x80
};

//
// Ed448 X509 Certificate (DER format) matching the test keys
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mEd448TestCert[] = {
  0x30, 0x82, 0x02, 0x3a, 0x30, 0x82, 0x01, 0xba, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x14, 0x3b, 0x44, 0xcc, 0x80, 0x92, 0xfe, 0xa8, 0x1a, 0x35,
  0x97, 0x53, 0x71, 0x2b, 0xe0, 0xf8, 0xed, 0xb5, 0x44, 0x1c, 0x7f, 0x30,
  0x05, 0x06, 0x03, 0x2b, 0x65, 0x71, 0x30, 0x6d, 0x31, 0x0b, 0x30, 0x09,
  0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x12, 0x30,
  0x10, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x09, 0x54, 0x65, 0x73, 0x74,
  0x53, 0x74, 0x61, 0x74, 0x65, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55,
  0x04, 0x07, 0x0c, 0x08, 0x54, 0x65, 0x73, 0x74, 0x43, 0x69, 0x74, 0x79,
  0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x07, 0x54,
  0x65, 0x73, 0x74, 0x4f, 0x72, 0x67, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03,
  0x55, 0x04, 0x0b, 0x0c, 0x08, 0x54, 0x65, 0x73, 0x74, 0x55, 0x6e, 0x69,
  0x74, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09,
  0x45, 0x44, 0x34, 0x34, 0x38, 0x54, 0x65, 0x73, 0x74, 0x30, 0x1e, 0x17,
  0x0d, 0x32, 0x36, 0x30, 0x36, 0x30, 0x32, 0x30, 0x30, 0x35, 0x39, 0x34,
  0x39, 0x5a, 0x17, 0x0d, 0x32, 0x37, 0x30, 0x36, 0x30, 0x32, 0x30, 0x30,
  0x35, 0x39, 0x34, 0x39, 0x5a, 0x30, 0x6d, 0x31, 0x0b, 0x30, 0x09, 0x06,
  0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x12, 0x30, 0x10,
  0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x09, 0x54, 0x65, 0x73, 0x74, 0x53,
  0x74, 0x61, 0x74, 0x65, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04,
  0x07, 0x0c, 0x08, 0x54, 0x65, 0x73, 0x74, 0x43, 0x69, 0x74, 0x79, 0x31,
  0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x07, 0x54, 0x65,
  0x73, 0x74, 0x4f, 0x72, 0x67, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55,
  0x04, 0x0b, 0x0c, 0x08, 0x54, 0x65, 0x73, 0x74, 0x55, 0x6e, 0x69, 0x74,
  0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x09, 0x45,
  0x44, 0x34, 0x34, 0x38, 0x54, 0x65, 0x73, 0x74, 0x30, 0x43, 0x30, 0x05,
  0x06, 0x03, 0x2b, 0x65, 0x71, 0x03, 0x3a, 0x00, 0x47, 0xba, 0x8f, 0x77,
  0x45, 0x59, 0xee, 0x91, 0x90, 0x52, 0x96, 0x9f, 0x59, 0xcd, 0xfa, 0xd5,
  0x82, 0x00, 0xc1, 0x92, 0x5c, 0x90, 0x58, 0x79, 0xaa, 0xbd, 0x74, 0xd2,
  0x2c, 0x34, 0xe8, 0x1e, 0xce, 0xbb, 0x59, 0x4d, 0xa9, 0x58, 0xaa, 0x61,
  0x86, 0x1c, 0xa3, 0xd6, 0x32, 0x64, 0xda, 0x67, 0xf7, 0x8b, 0x0e, 0xfa,
  0x09, 0xa7, 0x22, 0xfb, 0x80, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06,
  0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x89, 0xc9, 0x06, 0xf9,
  0x9e, 0xb7, 0xca, 0x87, 0x48, 0x22, 0xf1, 0x19, 0xcb, 0xf8, 0x7b, 0x3d,
  0x80, 0x22, 0xa0, 0xac, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04,
  0x18, 0x30, 0x16, 0x80, 0x14, 0x89, 0xc9, 0x06, 0xf9, 0x9e, 0xb7, 0xca,
  0x87, 0x48, 0x22, 0xf1, 0x19, 0xcb, 0xf8, 0x7b, 0x3d, 0x80, 0x22, 0xa0,
  0xac, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04,
  0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65,
  0x71, 0x03, 0x73, 0x00, 0x11, 0x40, 0x03, 0xee, 0xdc, 0xc6, 0x43, 0x45,
  0xf4, 0x31, 0x28, 0x89, 0x5f, 0x3d, 0x89, 0x8f, 0xd6, 0xa8, 0x0a, 0x07,
  0x0b, 0xf5, 0x5f, 0x43, 0xc0, 0xd6, 0xf0, 0xf7, 0xb1, 0x2a, 0xaa, 0x4f,
  0x93, 0xcc, 0x09, 0x6f, 0x19, 0xf6, 0xc5, 0x1c, 0x47, 0xc9, 0x0c, 0xeb,
  0x7f, 0xfe, 0xff, 0x37, 0x48, 0x1f, 0xb0, 0x45, 0x68, 0x6b, 0x92, 0xc3,
  0x80, 0x62, 0x3d, 0xa0, 0x71, 0x96, 0xdf, 0x88, 0x0d, 0x1c, 0x8d, 0x28,
  0xd6, 0xfb, 0xdf, 0x25, 0x3f, 0xfd, 0x78, 0xb2, 0xab, 0xc5, 0x39, 0x40,
  0x83, 0xe2, 0x5e, 0x1b, 0xd4, 0xa6, 0xf8, 0x4d, 0x11, 0xd2, 0xdb, 0x6c,
  0x5b, 0x86, 0x0f, 0x79, 0xc7, 0x44, 0x5b, 0xb3, 0xe0, 0xdb, 0x2f, 0xa9,
  0x83, 0x1f, 0x15, 0x3e, 0x6e, 0x54, 0x42, 0x5c, 0x35, 0x00
};

//
// PEM key data for Ed448 Private key matching the test key above
// This is an unencrypted PKCS#8 format PEM containing mEd448TestPrivateKey
// Format: -----BEGIN PRIVATE KEY----- (not ENCRYPTED)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  mEd448TestPemKey[] = {
  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x50,
  0x52, 0x49, 0x56, 0x41, 0x54, 0x45, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d,
  0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x45, 0x63, 0x43, 0x41, 0x51, 0x41, 0x77,
  0x42, 0x51, 0x59, 0x44, 0x4b, 0x32, 0x56, 0x78, 0x42, 0x44, 0x73, 0x45,
  0x4f, 0x52, 0x31, 0x79, 0x35, 0x35, 0x74, 0x47, 0x58, 0x38, 0x76, 0x4d,
  0x4a, 0x41, 0x63, 0x75, 0x49, 0x4a, 0x4c, 0x73, 0x74, 0x74, 0x30, 0x52,
  0x61, 0x42, 0x51, 0x4e, 0x68, 0x42, 0x2f, 0x35, 0x6a, 0x78, 0x50, 0x6c,
  0x63, 0x33, 0x69, 0x77, 0x51, 0x72, 0x66, 0x6c, 0x0a, 0x6b, 0x45, 0x58,
  0x4d, 0x46, 0x51, 0x64, 0x42, 0x5a, 0x6b, 0x5a, 0x71, 0x37, 0x38, 0x6d,
  0x65, 0x4c, 0x61, 0x6f, 0x2f, 0x4b, 0x50, 0x32, 0x73, 0x66, 0x79, 0x50,
  0x69, 0x46, 0x57, 0x43, 0x76, 0x65, 0x77, 0x3d, 0x3d, 0x0a, 0x2d, 0x2d,
  0x2d, 0x2d, 0x2d, 0x45, 0x4e, 0x44, 0x20, 0x50, 0x52, 0x49, 0x56, 0x41,
  0x54, 0x45, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
};

//
// Test message for signing and verification
//
CONST CHAR8  *mEdDsaTestMessage = "Test message for EdDSA signing and verification";

VOID  *EdDsaContext1;
VOID  *EdDsaContext2;

/**
  Prerequisite function for EdDSA tests.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  EdDsaContext1 = NULL;
  EdDsaContext2 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Cleanup function for EdDSA tests.

  @param[in]  Context  Unit test context.
**/
VOID
EFIAPI
TestVerifyEdDsaCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (EdDsaContext1 != NULL) {
    EdDsaFree (EdDsaContext1);
    EdDsaContext1 = NULL;
  }

  if (EdDsaContext2 != NULL) {
    EdDsaFree (EdDsaContext2);
    EdDsaContext2 = NULL;
  }
}

/**
  Validate UEFI-OpenSSL EdDSA Context Creation and Destruction.

  This test validates:
  - EdDsaNewByNid creates a valid context for supported curves (Ed448)
  - EdDsaNewByNid returns NULL for unsupported/invalid NIDs
  - EdDsaFree properly releases context resources
  - EdDsaFree handles NULL context gracefully

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaNew (
  UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Test EdDsaNewByNid with Ed448 (supported curve)
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  //
  // Verify the context can be used (set a key to confirm it's valid)
  //
  BOOLEAN  Status;

  Status = EdDsaSetPrivKey (
             EdDsaContext1,
             (UINT8 *)mEd448TestPrivateKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaFree releases the context properly
  //
  EdDsaFree (EdDsaContext1);
  EdDsaContext1 = NULL;

  //
  // Test EdDsaNewByNid with CRYPTO_NID_NULL (invalid NID)
  //
  EdDsaContext2 = EdDsaNewByNid (CRYPTO_NID_NULL);
  UT_ASSERT_EQUAL ((UINTN)EdDsaContext2, (UINTN)NULL);

  //
  // Test EdDsaNewByNid with unsupported curve NID
  // (using an EC curve NID that's not EdDSA)
  //
  EdDsaContext2 = EdDsaNewByNid (CRYPTO_NID_SECP256R1);
  UT_ASSERT_EQUAL ((UINTN)EdDsaContext2, (UINTN)NULL);

  //
  // Test EdDsaFree with NULL context (should not crash)
  //
  EdDsaFree (NULL);

  //
  // Test creating multiple contexts simultaneously
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  EdDsaContext2 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext2);

  //
  // Verify both contexts are independent
  //
  UT_ASSERT_NOT_EQUAL ((UINTN)EdDsaContext1, (UINTN)EdDsaContext2);

  //
  // Clean up both contexts
  //
  EdDsaFree (EdDsaContext1);
  EdDsaContext1 = NULL;

  EdDsaFree (EdDsaContext2);
  EdDsaContext2 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL EdDSA Key Setting and Getting.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaKeySetGet (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    PublicKey[ED448_KEY_SIZE];
  UINTN    PublicKeySize;
  UINT8    TooSmallBuffer[10];
  UINTN    TooSmallSize;

  //
  // Create EdDSA context
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  //
  // Test EdDsaSetPrivKey with valid key
  //
  Status = EdDsaSetPrivKey (
             EdDsaContext1,
             (UINT8 *)mEd448TestPrivateKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaGetPubKey after setting private key
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = EdDsaGetPubKey (EdDsaContext1, PublicKey, &PublicKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, ED448_KEY_SIZE);
  UT_ASSERT_MEM_EQUAL (PublicKey, mEd448TestPublicKey, ED448_KEY_SIZE);

  //
  // Test EdDsaGetPubKey with too small buffer
  //
  TooSmallSize = sizeof (TooSmallBuffer);
  Status       = EdDsaGetPubKey (EdDsaContext1, TooSmallBuffer, &TooSmallSize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (TooSmallSize, ED448_KEY_SIZE);

  //
  // Clean up context1
  //
  EdDsaFree (EdDsaContext1);
  EdDsaContext1 = NULL;

  //
  // Test EdDsaSetPubKey
  //
  EdDsaContext2 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext2);

  Status = EdDsaSetPubKey (
             EdDsaContext2,
             (UINT8 *)mEd448TestPublicKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaGetPubKey after setting public key
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = EdDsaGetPubKey (EdDsaContext2, PublicKey, &PublicKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, ED448_KEY_SIZE);
  UT_ASSERT_MEM_EQUAL (PublicKey, mEd448TestPublicKey, ED448_KEY_SIZE);

  //
  // Test EdDsaSetPrivKey with NULL context
  //
  Status = EdDsaSetPrivKey (NULL, (UINT8 *)mEd448TestPrivateKey, ED448_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSetPrivKey with NULL key
  //
  Status = EdDsaSetPrivKey (EdDsaContext2, NULL, ED448_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSetPrivKey with wrong size
  //
  Status = EdDsaSetPrivKey (EdDsaContext2, (UINT8 *)mEd448TestPrivateKey, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSetPubKey with NULL context
  //
  Status = EdDsaSetPubKey (NULL, (UINT8 *)mEd448TestPublicKey, ED448_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSetPubKey with NULL key
  //
  Status = EdDsaSetPubKey (EdDsaContext2, NULL, ED448_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSetPubKey with wrong size
  //
  Status = EdDsaSetPubKey (EdDsaContext2, (UINT8 *)mEd448TestPublicKey, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaGetPubKey with NULL context
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = EdDsaGetPubKey (NULL, PublicKey, &PublicKeySize);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaGetPubKey with NULL buffer
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = EdDsaGetPubKey (EdDsaContext2, NULL, &PublicKeySize);
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaGetPubKey with NULL size
  //
  Status = EdDsaGetPubKey (EdDsaContext2, PublicKey, NULL);
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL EdDSA Signing and Verification.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaSignVerify (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    Signature[ED448_SIG_SIZE];
  UINTN    SigSize;
  UINTN    MessageSize;

  MessageSize = AsciiStrLen (mEdDsaTestMessage);

  //
  // Create context with private key for signing
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  Status = EdDsaSetPrivKey (
             EdDsaContext1,
             (UINT8 *)mEd448TestPrivateKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaSign without context string
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              EdDsaContext1,
              NULL,
              0,
              (UINT8 *)mEdDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, ED448_SIG_SIZE);

  //
  // Test EdDsaVerify with the same context
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Create separate context with public key for verification
  //
  EdDsaContext2 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext2);

  Status = EdDsaSetPubKey (
             EdDsaContext2,
             (UINT8 *)mEd448TestPublicKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaVerify with separate public key context
  //
  Status = EdDsaVerify (
             EdDsaContext2,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaVerify with modified message (should fail)
  //
  UINT8  ModifiedMessage[100];

  CopyMem (ModifiedMessage, mEdDsaTestMessage, MessageSize);
  ModifiedMessage[0] ^= 0xFF;

  Status = EdDsaVerify (
             EdDsaContext2,
             NULL,
             0,
             ModifiedMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify with modified signature (should fail)
  //
  Signature[0] ^= 0xFF;
  Status        = EdDsaVerify (
                    EdDsaContext2,
                    NULL,
                    0,
                    (UINT8 *)mEdDsaTestMessage,
                    MessageSize,
                    Signature,
                    SigSize
                    );
  UT_ASSERT_FALSE (Status);
  Signature[0] ^= 0xFF;

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL EdDSA Signing and Verification with Context String.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaSignVerifyWithContext (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN      Status;
  UINT8        Signature[ED448_SIG_SIZE];
  UINTN        SigSize;
  UINTN        MessageSize;
  CONST CHAR8  *ContextString = "test-context";
  UINTN        ContextSize;

  MessageSize = AsciiStrLen (mEdDsaTestMessage);
  ContextSize = AsciiStrLen (ContextString);

  //
  // Create context with private key
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  Status = EdDsaSetPrivKey (
             EdDsaContext1,
             (UINT8 *)mEd448TestPrivateKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaSign with context string
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              EdDsaContext1,
              (UINT8 *)ContextString,
              ContextSize,
              (UINT8 *)mEdDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, ED448_SIG_SIZE);

  //
  // Test EdDsaVerify with matching context string
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             (UINT8 *)ContextString,
             ContextSize,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaVerify with different context string (should fail)
  //
  CONST CHAR8  *WrongContext = "wrong-context";

  Status = EdDsaVerify (
             EdDsaContext1,
             (UINT8 *)WrongContext,
             AsciiStrLen (WrongContext),
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify without context string (should fail)
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL EdDSA Error Cases.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaErrorCases (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    Signature[ED448_SIG_SIZE];
  UINTN    SigSize;
  UINTN    MessageSize;
  UINT8    TooSmallSig[10];
  UINTN    TooSmallSigSize;

  MessageSize = AsciiStrLen (mEdDsaTestMessage);

  //
  // Create context
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  Status = EdDsaSetPrivKey (
             EdDsaContext1,
             (UINT8 *)mEd448TestPrivateKey,
             ED448_KEY_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaSign with NULL context
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              NULL,
              NULL,
              0,
              (UINT8 *)mEdDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSign with NULL message
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              EdDsaContext1,
              NULL,
              0,
              NULL,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaSign with too small signature buffer
  //
  TooSmallSigSize = sizeof (TooSmallSig);
  Status          = EdDsaSign (
                      EdDsaContext1,
                      NULL,
                      0,
                      (UINT8 *)mEdDsaTestMessage,
                      MessageSize,
                      TooSmallSig,
                      &TooSmallSigSize
                      );
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (TooSmallSigSize, ED448_SIG_SIZE);

  //
  // Generate valid signature for verification tests
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              EdDsaContext1,
              NULL,
              0,
              (UINT8 *)mEdDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Test EdDsaVerify with NULL context
  //
  Status = EdDsaVerify (
             NULL,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify with NULL message
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             NULL,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify with NULL signature
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             NULL,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify with wrong signature size
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             32
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test EdDsaVerify with zero signature size
  //
  Status = EdDsaVerify (
             EdDsaContext1,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             0
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate EdDsaGeneratePubKey (placeholder function).

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaGeneratePubKey (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    PublicKey[ED448_KEY_SIZE];

  //
  // Create context
  //
  EdDsaContext1 = EdDsaNewByNid (CRYPTO_NID_ED448);
  UT_ASSERT_NOT_NULL (EdDsaContext1);

  //
  // EdDsaGeneratePubKey is a placeholder that always returns TRUE
  //
  Status = EdDsaGeneratePubKey (EdDsaContext1, PublicKey, ED448_KEY_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // It should return TRUE even with NULL parameters
  //
  Status = EdDsaGeneratePubKey (NULL, NULL, 0);
  UT_ASSERT_TRUE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate EdDSA key retrieval from PEM and X509.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyEdDsaPemX509 (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *EdDsaPrivKey;
  VOID     *EdDsaPubKey;
  UINT8    Signature[ED448_SIG_SIZE];
  UINTN    SigSize;
  UINTN    MessageSize;

  EdDsaPrivKey = NULL;
  EdDsaPubKey  = NULL;
  MessageSize  = AsciiStrLen (mEdDsaTestMessage);

  //
  // Retrieve EdDsa private key from PEM data.
  //
  Status = EdDsaGetPrivateKeyFromPem (
             mEd448TestPemKey,
             sizeof (mEd448TestPemKey),
             NULL,
             &EdDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (EdDsaPrivKey);

  //
  // Retrieve EdDsa public key from X509 certificate.
  //
  Status = EdDsaGetPublicKeyFromX509 (
             mEd448TestCert,
             sizeof (mEd448TestCert),
             &EdDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (EdDsaPubKey);

  //
  // EdDSA signing with key from PEM
  //
  SigSize = sizeof (Signature);
  Status  = EdDsaSign (
              EdDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mEdDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, ED448_SIG_SIZE);

  //
  // EdDSA verification with key from X509
  //
  Status = EdDsaVerify (
             EdDsaPubKey,
             NULL,
             0,
             (UINT8 *)mEdDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  EdDsaFree (EdDsaPrivKey);
  EdDsaFree (EdDsaPubKey);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mEdDsaTest[] = {
  //
  // -----Description------------------------------------Class-------------------------Function----------------------------Pre-------------------Post--------------------Context
  //
  { "TestVerifyEdDsaNew()",                   "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaNew,                   TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaKeySetGet()",             "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaKeySetGet,             TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaSignVerify()",            "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaSignVerify,            TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaSignVerifyWithContext()", "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaSignVerifyWithContext, TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaErrorCases()",            "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaErrorCases,            TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaGeneratePubKey()",        "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaGeneratePubKey,        TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
  { "TestVerifyEdDsaPemX509()",               "CryptoPkg.BaseCryptLib.EdDsa", TestVerifyEdDsaPemX509,               TestVerifyEdDsaPreReq, TestVerifyEdDsaCleanUp, NULL },
};

UINTN  mEdDsaTestNum = ARRAY_SIZE (mEdDsaTest);
