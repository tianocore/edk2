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
#define ML_DSA_MAX_CONTEXT_SIZE     255

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
  UT_ASSERT_NOT_EQUAL (TooSmallSize, ML_DSA_87_PUBLIC_KEY_SIZE);

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
  UT_ASSERT_NOT_EQUAL (TooSmallSize, sizeof (Signature));

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

/**
  Validate ML-DSA operations on context without keys.

  This test validates that operations fail properly when no key is set.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaNoKeyOperations (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINT8    PublicKey[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINTN    SigSize;
  UINTN    PublicKeySize;

  //
  // Create context without setting any key
  //
  MlDsaContext1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (MlDsaContext1);

  //
  // MlDsaSign should fail when EvpPkey is NULL
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaContext1,
              NULL,
              0,
              (UINT8 *)mMlDsaTestMessage,
              AsciiStrLen (mMlDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // MlDsaVerify should fail when EvpPkey is NULL
  //
  Status = MlDsaVerify (
             MlDsaContext1,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             AsciiStrLen (mMlDsaTestMessage),
             Signature,
             sizeof (Signature)
             );
  UT_ASSERT_FALSE (Status);

  //
  // MlDsaGetPubKey should fail when EvpPkey is NULL
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = MlDsaGetPubKey (
                    MlDsaContext1,
                    PublicKey,
                    &PublicKeySize
                    );
  UT_ASSERT_FALSE (Status);

  MlDsaFree (MlDsaContext1);
  MlDsaContext1 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA context string parameter validation.

  This test validates the check: (ContextSize > 0) && (Context == NULL).

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaInvalidContextParams (
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
  // Get valid keys
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &MlDsaPubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test MlDsaSign with NULL Context but ContextSize > 0 (invalid combination)
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              MlDsaPrivKey,
              NULL,
              10,
              (UINT8 *)mMlDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Generate valid signature for verify test
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
  // Test MlDsaVerify with NULL Context but ContextSize > 0 (invalid combination)
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             5,
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
  Validate MlDsaGetPubKey with NULL buffer parameter.

  This test validates that NULL PublicKey buffer returns FALSE and sets size to 0.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaGetPubKeyNullBuffer (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *MlDsaPrivKey;
  UINTN    PublicKeySize;

  MlDsaPrivKey = NULL;

  //
  // Get valid private key
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test MlDsaGetPubKey with NULL buffer
  // Should return FALSE and set PublicKeySize to 0
  //
  PublicKeySize = 9999;
  Status        = MlDsaGetPubKey (
                    MlDsaPrivKey,
                    NULL,
                    &PublicKeySize
                    );
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, 0);

  MlDsaFree (MlDsaPrivKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate ML-DSA signature size exact match validation.

  This test validates that MlDsaVerify properly validates signature size.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaSignatureSizeExact (
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
  // Get valid keys
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &MlDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &MlDsaPubKey
             );
  UT_ASSERT_TRUE (Status);

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
  UT_ASSERT_EQUAL (SigSize, ML_DSA_87_SIGNATURE_SIZE);

  //
  // Verify with exact size - should succeed
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             ML_DSA_87_SIGNATURE_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Verify with size + 1 - should fail
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             ML_DSA_87_SIGNATURE_SIZE + 1
             );
  UT_ASSERT_FALSE (Status);

  //
  // Verify with size - 1 - should fail
  //
  Status = MlDsaVerify (
             MlDsaPubKey,
             NULL,
             0,
             (UINT8 *)mMlDsaTestMessage,
             MessageSize,
             Signature,
             ML_DSA_87_SIGNATURE_SIZE - 1
             );
  UT_ASSERT_FALSE (Status);

  MlDsaFree (MlDsaPrivKey);
  MlDsaFree (MlDsaPubKey);

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA context lifecycle with multiple allocations.

  Tests proper resource management including multiple successive allocations,
  cleanup verification, and null pointer safety.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaContextLifecycle (
  UNIT_TEST_CONTEXT  Context
  )
{
  VOID  *TempCtx1;
  VOID  *TempCtx2;
  VOID  *TempCtx3;

  //
  // Test creating multiple contexts
  //
  TempCtx1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (TempCtx1);

  TempCtx2 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (TempCtx2);

  TempCtx3 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (TempCtx3);

  //
  // Free in different order
  //
  MlDsaFree (TempCtx2);
  MlDsaFree (TempCtx1);
  MlDsaFree (TempCtx3);

  //
  // Double-free safety (should not crash)
  //
  MlDsaFree (NULL);
  MlDsaFree (NULL);

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA empty message signing and verification.

  Tests edge case of zero-length messages.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaEmptyMessage (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  UINT8    EmptyMsg[1];

  PrivKey = NULL;
  PubKey  = NULL;

  //
  // Load keys
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Sign empty message - should fail with NULL message
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              PrivKey,
              NULL,
              0,
              NULL,
              0,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Sign with valid pointer but zero size - should succeed
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              PrivKey,
              NULL,
              0,
              EmptyMsg,
              0,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Verify the zero-length message signature
  //
  Status = MlDsaVerify (
             PubKey,
             NULL,
             0,
             EmptyMsg,
             0,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  MlDsaFree (PrivKey);
  MlDsaFree (PubKey);

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA maximum length context string.

  Tests the maximum allowed context string size per FIPS 204.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaMaxContextString (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    Signature[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  UINT8    MaxContext[ML_DSA_MAX_CONTEXT_SIZE];
  UINTN    Index;

  PrivKey = NULL;
  PubKey  = NULL;

  //
  // Fill context with pattern
  //
  for (Index = 0; Index < ML_DSA_MAX_CONTEXT_SIZE; Index++) {
    MaxContext[Index] = (UINT8)(Index & 0xFF);
  }

  //
  // Load keys
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Sign with maximum context
  //
  SigSize = sizeof (Signature);
  Status  = MlDsaSign (
              PrivKey,
              MaxContext,
              ML_DSA_MAX_CONTEXT_SIZE,
              (UINT8 *)mMlDsaTestMessage,
              AsciiStrLen (mMlDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Verify with matching context
  //
  Status = MlDsaVerify (
             PubKey,
             MaxContext,
             ML_DSA_MAX_CONTEXT_SIZE,
             (UINT8 *)mMlDsaTestMessage,
             AsciiStrLen (mMlDsaTestMessage),
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Verify should fail with one byte different
  //
  MaxContext[0] ^= 1;
  Status         = MlDsaVerify (
                     PubKey,
                     MaxContext,
                     ML_DSA_MAX_CONTEXT_SIZE,
                     (UINT8 *)mMlDsaTestMessage,
                     AsciiStrLen (mMlDsaTestMessage),
                     Signature,
                     SigSize
                     );
  UT_ASSERT_FALSE (Status);

  MlDsaFree (PrivKey);
  MlDsaFree (PubKey);

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA key replacement in context.

  Tests that replacing keys properly frees old key and sets new key.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaKeyReplacement (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  UINT8    PublicKey1[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINT8    PublicKey2[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINTN    PubKeySize;

  PrivKey = NULL;

  //
  // Load first key
  //
  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Get public key
  //
  PubKeySize = sizeof (PublicKey1);
  Status     = MlDsaGetPubKey (PrivKey, PublicKey1, &PubKeySize);
  UT_ASSERT_TRUE (Status);

  //
  // Now replace with a public-only key
  //
  MlDsaContext1 = MlDsaNewByNid (CRYPTO_NID_ML_DSA_87);
  UT_ASSERT_NOT_NULL (MlDsaContext1);

  Status = MlDsaSetPubKey (MlDsaContext1, PublicKey1, ML_DSA_87_PUBLIC_KEY_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Replace with same public key again (tests EVP_PKEY_free path)
  //
  Status = MlDsaSetPubKey (MlDsaContext1, PublicKey1, ML_DSA_87_PUBLIC_KEY_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Verify we can still get the key
  //
  PubKeySize = sizeof (PublicKey2);
  Status     = MlDsaGetPubKey (MlDsaContext1, PublicKey2, &PubKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PubKeySize, ML_DSA_87_PUBLIC_KEY_SIZE);

  MlDsaFree (PrivKey);
  MlDsaFree (MlDsaContext1);
  MlDsaContext1 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA multiple signatures with same key.

  Tests that the same key can generate multiple signatures.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaMultipleSignatures (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    Sig1[ML_DSA_87_SIGNATURE_SIZE];
  UINT8    Sig2[ML_DSA_87_SIGNATURE_SIZE];
  UINT8    Sig3[ML_DSA_87_SIGNATURE_SIZE];
  UINTN    SigSize;
  CHAR8    *Msg1 = "First message";
  CHAR8    *Msg2 = "Second message";
  CHAR8    *Msg3 = "Third message";

  PrivKey = NULL;
  PubKey  = NULL;

  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = MlDsaGetPublicKeyFromX509 (
             mMlDsa87TestCert,
             sizeof (mMlDsa87TestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Generate three different signatures
  //
  SigSize = sizeof (Sig1);
  Status  = MlDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig1, &SigSize);
  UT_ASSERT_TRUE (Status);

  SigSize = sizeof (Sig2);
  Status  = MlDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig2, &SigSize);
  UT_ASSERT_TRUE (Status);

  SigSize = sizeof (Sig3);
  Status  = MlDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg3, AsciiStrLen (Msg3), Sig3, &SigSize);
  UT_ASSERT_TRUE (Status);

  //
  // Verify all three with correct messages
  //
  Status = MlDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig1, sizeof (Sig1));
  UT_ASSERT_TRUE (Status);

  Status = MlDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig2, sizeof (Sig2));
  UT_ASSERT_TRUE (Status);

  Status = MlDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg3, AsciiStrLen (Msg3), Sig3, sizeof (Sig3));
  UT_ASSERT_TRUE (Status);

  //
  // Cross-verify should fail (wrong message/signature pairs)
  //
  Status = MlDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig2, sizeof (Sig2));
  UT_ASSERT_FALSE (Status);

  Status = MlDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig3, sizeof (Sig3));
  UT_ASSERT_FALSE (Status);

  MlDsaFree (PrivKey);
  MlDsaFree (PubKey);

  return UNIT_TEST_PASSED;
}

/**
  Test ML-DSA GetPubKey called multiple times.

  Tests that repeated calls to GetPubKey return consistent results.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifyMlDsaGetPubKeyRepeated (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  UINT8    PubKey1[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINT8    PubKey2[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINT8    PubKey3[ML_DSA_87_PUBLIC_KEY_SIZE];
  UINTN    Size1, Size2, Size3;

  PrivKey = NULL;

  Status = MlDsaGetPrivateKeyFromPem (
             mMlDsa87TestPemKey,
             sizeof (mMlDsa87TestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Get public key three times
  //
  Size1  = sizeof (PubKey1);
  Status = MlDsaGetPubKey (PrivKey, PubKey1, &Size1);
  UT_ASSERT_TRUE (Status);

  Size2  = sizeof (PubKey2);
  Status = MlDsaGetPubKey (PrivKey, PubKey2, &Size2);
  UT_ASSERT_TRUE (Status);

  Size3  = sizeof (PubKey3);
  Status = MlDsaGetPubKey (PrivKey, PubKey3, &Size3);
  UT_ASSERT_TRUE (Status);

  //
  // All should be identical
  //
  UT_ASSERT_EQUAL (Size1, Size2);
  UT_ASSERT_EQUAL (Size2, Size3);
  UT_ASSERT_MEM_EQUAL (PubKey1, PubKey2, Size1);
  UT_ASSERT_MEM_EQUAL (PubKey2, PubKey3, Size2);

  MlDsaFree (PrivKey);

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
  { "TestVerifyMlDsaNoKeyOperations()",       "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaNoKeyOperations,       TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaInvalidContextParams()",  "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaInvalidContextParams,  TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaGetPubKeyNullBuffer()",   "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaGetPubKeyNullBuffer,   TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaSignatureSizeExact()",    "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaSignatureSizeExact,    TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaContextLifecycle()",      "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaContextLifecycle,      TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaEmptyMessage()",          "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaEmptyMessage,          TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaMaxContextString()",      "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaMaxContextString,      TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaKeyReplacement()",        "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaKeyReplacement,        TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaMultipleSignatures()",    "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaMultipleSignatures,    TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
  { "TestVerifyMlDsaGetPubKeyRepeated()",     "CryptoPkg.BaseCryptLib.MlDsa", TestVerifyMlDsaGetPubKeyRepeated,     TestVerifyMlDsaPreReq, TestVerifyMlDsaCleanUp, NULL },
};

UINTN  mMlDsaTestNum = ARRAY_SIZE (mMlDsaTest);
