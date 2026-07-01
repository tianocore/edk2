/** @file
  Application for ML-DSA Primitives Validation.

  This file contains unit tests for the ML-DSA (Module-Lattice-Based Digital Signature Algorithm)
  cryptographic functions defined in CryptMlDsa.c. ML-DSA is a post-quantum digital signature
  scheme based on the CRYSTALS-Dilithium algorithm and standardized in FIPS 204.

  The test vectors are provided in MlDsaTestVectors.h which contains:
  - mMlDsa87TestCert[] - X.509 DER certificate with ML-DSA-87 public key
  - mMlDsa87TestPemKey[] - PEM-encoded ML-DSA-87 private key

  The test structure mirrors EdDsaTests.c and validates:
  - Context creation and destruction (MlDsaNewByNid, MlDsaFree)
  - Key setting and retrieval error cases (MlDsaSetPrivKey, MlDsaSetPubKey, MlDsaGetPubKey)
  - Signature generation and verification via PEM/X509 (TestVerifyMlDsaPemX509)
  - Error handling for invalid inputs

  NOTE: TestVerifyMlDsaSignVerify() and TestVerifyMlDsaSignVerifyWithContext() are
  currently skipped as they require raw key arrays. The main signing/verification test
  is TestVerifyMlDsaPemX509() which uses real PEM and X509 test vectors.

Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define ML_DSA_87_PRIVATE_KEY_SIZE  4896
#define ML_DSA_87_PUBLIC_KEY_SIZE   2592
#define ML_DSA_87_SIGNATURE_SIZE    4627

//
// ML-DSA-87 test vectors - include generated certificate and PEM key
//
#include "MlDsaTestVectors.h"

//
// Test message for signing and verification
//
CONST CHAR8  *mMlDsaTestMessage = "Test message for ML-DSA signing and verification";

//
// Optional context string for domain separation
//
CONST CHAR8  *mMlDsaTestContext = "ML-DSA test context";

VOID  *MlDsaContext1;
VOID  *MlDsaContext2;

/**
  Prerequisite function for ML-DSA tests.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  MlDsaContext1 = NULL;
  MlDsaContext2 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Cleanup function for ML-DSA tests.

  @param[in]  Context  Unit test context.
**/
VOID
EFIAPI
TestVerifyMlDsaCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (MlDsaContext1 != NULL) {
    MlDsaFree (MlDsaContext1);
    MlDsaContext1 = NULL;
  }

  if (MlDsaContext2 != NULL) {
    MlDsaFree (MlDsaContext2);
    MlDsaContext2 = NULL;
  }
}

/**
  Validate UEFI-OpenSSL ML-DSA Context Creation.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaNew (
  UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Test MlDsaNewByNid with ML-DSA-87
  //
  MlDsaContext1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (MlDsaContext1);

  //
  // Test MlDsaNewByNid with invalid NID
  //
  MlDsaContext2 = MlDsaNewByNid (CRYPTO_NID_NULL);
  UT_ASSERT_EQUAL ((UINTN)MlDsaContext2, (UINTN)NULL);

  //
  // Test MlDsaFree
  //
  MlDsaFree (MlDsaContext1);
  MlDsaContext1 = NULL;

  //
  // Test MlDsaFree with NULL (should not crash)
  //
  MlDsaFree (NULL);

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL ML-DSA Key Setting and Getting.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaKeySetGet (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    PublicKey[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINTN    PublicKeySize;
  UINT8    TooSmallBuffer[10];
  UINTN    TooSmallSize;

  //
  // Create ML-DSA context
  //
  MlDsaContext1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (MlDsaContext1);

  //
  // Test MlDsaGetPubKey with too small buffer (before key is set)
  //
  TooSmallSize = sizeof (TooSmallBuffer);
  Status       = MlDsaGetPubKey (MlDsaContext1, TooSmallBuffer, &TooSmallSize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (TooSmallSize, ML_DSA_87_PUBLIC_KEY_SIZE);

  //
  // Test MlDsaSetPrivKey with NULL context
  //
  Status = MlDsaSetPrivKey (NULL, (UINT8 *)mMlDsa87TestPemKey, 100);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSetPrivKey with NULL key
  //
  Status = MlDsaSetPrivKey (MlDsaContext1, NULL, ML_DSA_87_PRIVATE_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSetPrivKey with wrong size
  //
  Status = MlDsaSetPrivKey (MlDsaContext1, (UINT8 *)mMlDsa87TestPemKey, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSetPubKey with NULL context
  //
  Status = MlDsaSetPubKey (NULL, (UINT8 *)mMlDsa87TestCert, 100);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSetPubKey with NULL key
  //
  Status = MlDsaSetPubKey (MlDsaContext1, NULL, ML_DSA_87_PUBLIC_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSetPubKey with wrong size
  //
  Status = MlDsaSetPubKey (MlDsaContext1, (UINT8 *)mMlDsa87TestCert, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPubKey with NULL context
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = MlDsaGetPubKey (NULL, PublicKey, &PublicKeySize);
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPubKey with NULL size
  //
  Status = MlDsaGetPubKey (MlDsaContext1, PublicKey, NULL);
  UT_ASSERT_FALSE (Status);

  //
  // Clean up context
  //
  MlDsaFree (MlDsaContext1);
  MlDsaContext1 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA error cases.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaErrorCases (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  UINT8    TooSmallBuffer[10];
  UINTN    TooSmallSize;

  //
  // Create ML-DSA context
  //
  MlDsaContext1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (MlDsaContext1);

  //
  // Test MlDsaSign with NULL context
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              NULL,
              NULL,
              0,
              (UINT8 *)mMlDsaTestMessage,
              AsciiStrLen (mMlDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSign with NULL message
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaContext1,
              NULL,
              0,
              NULL,
              0,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaSign with too small buffer
  //
  TooSmallSize = sizeof (TooSmallBuffer);
  Status       = MlDsaSign (
                   MlDsaContext1,
                   NULL,
                   0,
                   (UINT8 *)mMlDsaTestMessage,
                   AsciiStrLen (mMlDsaTestMessage),
                   TooSmallBuffer,
                   &TooSmallSize
                   );
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (TooSmallSize, ML_DSA_87_SIGNATURE_SIZE);

  //
  // Test MlDsaVerify with NULL context
  //
  Status = MlDsaVerify (
             NULL,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             AsciiStrLen (mMlDsaTestMessage),
             Signature,
             sizeof (Signature)
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaVerify with NULL message
  //
  Status = MlDsaVerify (
             MlDsaContext1,
             NULL,
             0,
             NULL,
             0,
             Signature,
             sizeof (Signature)
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaVerify with NULL signature
  //
  Status = MlDsaVerify (
             MlDsaContext1,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             AsciiStrLen (mMlDsaTestMessage),
             NULL,
             sizeof (Signature)
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaVerify with zero signature size
  //
  Status = MlDsaVerify (
             MlDsaContext1,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             AsciiStrLen (mMlDsaTestMessage),
             Signature,
             0
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA key retrieval from PEM and X509.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaPemX509 (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaPrivKey;
  VOID     *MlDsaPubKey;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  UINTN    MessageSize;

  MlDsaPrivKey = NULL;
  MlDsaPubKey  = NULL;
  MessageSize  = AsciiStrLen (mMlDsaTestMessage);

  //
  // Retrieve ML-DSA private key from PEM data.
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPrivKey);

  //
  // Retrieve ML-DSA public key from X509 certificate.
  //
  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &MlDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPubKey);

  //
  // ML-DSA signing with key from PEM (no context string)
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mMlDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, ML_DSA_87_SIGNATURE_SIZE);

  //
  // ML-DSA verification with key from X509
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  MlDsaFree (MlDsaPrivKey);
  MlDsaFree (MlDsaPubKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA signing and verification with context string.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaSignVerifyWithContext (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaPrivKey;
  VOID     *MlDsaPubKey;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  UINTN    MessageSize;
  UINTN    ContextSize;

  MlDsaPrivKey = NULL;
  MlDsaPubKey  = NULL;
  MessageSize  = AsciiStrLen (mMlDsaTestMessage);
  ContextSize  = AsciiStrLen (mMlDsaTestContext);

  //
  // Retrieve ML-DSA private key from PEM data.
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPrivKey);

  //
  // Retrieve ML-DSA public key from X509 certificate.
  //
  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &MlDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPubKey);

  //
  // ML-DSA signing with context string
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaPrivKey,
              (UINT8 *)mMlDsaTestContext,
              ContextSize,
              (UINT8 *)mMlDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, ML_DSA_87_SIGNATURE_SIZE);

  //
  // ML-DSA verification with matching context string
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             (UINT8 *)mMlDsaTestContext,
             ContextSize,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // ML-DSA verification should fail with mismatched context string
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             (UINT8 *)"Different context",
             AsciiStrLen ("Different context"),
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // ML-DSA verification should fail with no context when signature used context
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  MlDsaFree (MlDsaPrivKey);
  MlDsaFree (MlDsaPubKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA public key extraction and generation.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaGetPubKey (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaPrivKey;
  UINT8    PublicKey[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINTN    PublicKeySize;

  MlDsaPrivKey = NULL;

  //
  // Retrieve ML-DSA private key from PEM data.
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPrivKey);

  //
  // Get public key from private key context
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = MlDsaGetPubKey (
                    MlDsaPrivKey,
                    PublicKey,
                    &PublicKeySize
                    );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, ML_DSA_87_PUBLIC_KEY_SIZE);

  //
  // Verify the public key buffer is not all zeros
  //
  BOOLEAN  AllZeros;
  UINTN    Index;

  AllZeros = TRUE;
  for (Index = 0; Index < PublicKeySize; Index++) {
    if (PublicKey[Index] != 0) {
      AllZeros = FALSE;
      break;
    }
  }

  UT_ASSERT_FALSE (AllZeros);

  MlDsaFree (MlDsaPrivKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA signature verification fails with tampered data.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaTamperedData (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaPrivKey;
  VOID     *MlDsaPubKey;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINT8    TamperedSignature[ML_DSA_87_SIGNATURE_SIZE];
  CHAR8    TamperedMessage[100];
  UINTN    SigSize;
  UINTN    MessageSize;

  MlDsaPrivKey = NULL;
  MlDsaPubKey  = NULL;
  MessageSize  = AsciiStrLen (mMlDsaTestMessage);

  //
  // Retrieve ML-DSA private key from PEM data.
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPrivKey);

  //
  // Retrieve ML-DSA public key from X509 certificate.
  //
  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &MlDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (MlDsaPubKey);

  //
  // Generate valid signature
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mMlDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Verify original signature works
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test with tampered message (should fail)
  //
  AsciiStrCpyS (TamperedMessage, sizeof (TamperedMessage), mMlDsaTestMessage);
  TamperedMessage[0] = 'X';
  Status             = MlDsaVerify (
                         MlDsaPubKey,
                         NULL,
                         0,
                         (UINT8 *)TamperedMessage,
                         MessageSize,
                         Signature,
                         SigSize
                         );
  UT_ASSERT_FALSE (Status);

  //
  // Test with tampered signature (should fail)
  //
  CopyMem (TamperedSignature, Signature, sizeof (Signature));
  TamperedSignature[0] ^= 0x01;
  Status                = MlDsaVerify (
                            MlDsaPubKey,
                            NULL,
                            0,
                            (UINT8 *)mMlDsaTestMessage,
                            MessageSize,
                            TamperedSignature,
                            SigSize
                            );
  UT_ASSERT_FALSE (Status);

  //
  // Test with wrong signature size (should fail)
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             SigSize - 1
             );
  UT_ASSERT_FALSE (Status);

  MlDsaFree (MlDsaPrivKey);
  MlDsaFree (MlDsaPubKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA PEM loading error cases.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaPemErrors (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaKey;

  MlDsaKey = NULL;

  //
  // Test MlDsaGetPrivateKeyFromPem with NULL PEM data
  //
  Status = MlDsaGetPrivateKeyFromPem (
             NULL,
             100,
             NULL,
             &MlDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPrivateKeyFromPem with NULL context pointer
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             NULL
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPrivateKeyFromPem with invalid PEM data
  //
  Status = MlDsaGetPrivateKeyFromPem (
             (UINT8 *)"Invalid PEM data",
             16,
             NULL,
             &MlDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPublicKeyFromX509 with NULL certificate
  //
  Status = MlDsaGetPublicKeyFromX509 (
             NULL,
             100,
             &MlDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPublicKeyFromX509 with NULL context pointer
  //
  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             NULL
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test MlDsaGetPublicKeyFromX509 with invalid certificate data
  //
  Status = MlDsaGetPublicKeyFromX509 (
             (UINT8 *)"Invalid cert data",
             17,
             &MlDsaKey
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mMlDsaTest[] = {
  //
  // -----Description------------------------------------Class-------------------------Function----------------------------Pre-------------------Post--------------------Context
  //
  { "TestVerifyMlDsaNew()",                   "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaNew,                   TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaKeySetGet()",             "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaKeySetGet,             TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaErrorCases()",            "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaErrorCases,            TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaPemX509()",               "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaPemX509,               TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaSignVerifyWithContext()", "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaSignVerifyWithContext, TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaGetPubKey()",             "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaGetPubKey,             TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaTamperedData()",          "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaTamperedData,          TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaPemErrors()",             "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaPemErrors,             TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
};

UINTN  mMlDsaTestNum = ARRAY_SIZE (mMlDsaTest);
