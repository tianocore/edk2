/** @file
  Application for SLH-DSA Primitives Validation.

  This file contains unit tests for the SLH-DSA (Stateless Hash-Based Digital Signature Algorithm)
  cryptographic functions defined in CryptSlhDsa.c. SLH-DSA is a post-quantum digital signature
  scheme based on the SPHINCS+ algorithm and standardized in FIPS 205.

  The test vectors are provided in SlhDsaTestVectors.h which contains:
  - mSlhDsaShake256sTestCert[] - X.509 DER certificate with SLH-DSA-SHAKE-256s public key
  - mSlhDsaShake256sTestPemKey[] - PEM-encoded SLH-DSA-SHAKE-256s private key

  The test structure mirrors SlhDsaTests.c and validates:
  - Context creation and destruction (SlhDsaNewByNid, SlhDsaFree)
  - Key setting and retrieval error cases (SlhDsaSetPrivKey, SlhDsaSetPubKey, SlhDsaGetPubKey)
  - Signature generation and verification via PEM/X509 (TestVerifySlhDsaPemX509)
  - Error handling for invalid inputs

Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

#define SLH_DSA_SHAKE_256S_PRIVATE_KEY_SIZE  128
#define SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE   64
#define SLH_DSA_SHAKE_256S_SIGNATURE_SIZE    29792
#define SLH_DSA_MAX_CONTEXT_SIZE             255

//
// SLH-DSA-SHAKE-256s test vectors - include generated certificate and PEM key
//
#include "SlhDsaTestVectors.h"

//
// Test message for signing and verification
//
CONST CHAR8  *mSlhDsaTestMessage = "Test message for SLH-DSA signing and verification";

//
// Optional context string for domain separation
//
CONST CHAR8  *mSlhDsaTestContext = "SLH-DSA test context";

VOID  *SlhDsaContext1;
VOID  *SlhDsaContext2;

/**
  Prerequisite function for SLH-DSA tests.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaPreReq (
  UNIT_TEST_CONTEXT  Context
  )
{
  SlhDsaContext1 = NULL;
  SlhDsaContext2 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Cleanup function for SLH-DSA tests.

  @param[in]  Context  Unit test context.
**/
VOID
EFIAPI
TestVerifySlhDsaCleanUp (
  UNIT_TEST_CONTEXT  Context
  )
{
  if (SlhDsaContext1 != NULL) {
    SlhDsaFree (SlhDsaContext1);
    SlhDsaContext1 = NULL;
  }

  if (SlhDsaContext2 != NULL) {
    SlhDsaFree (SlhDsaContext2);
    SlhDsaContext2 = NULL;
  }
}

/**
  Validate UEFI-OpenSSL SLH-DSA Context Creation.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaNew (
  UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Test SlhDsaNewByNid with SLH-DSA-87
  //
  SlhDsaContext1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (SlhDsaContext1);

  //
  // Test SlhDsaNewByNid with invalid NID
  //
  SlhDsaContext2 = SlhDsaNewByNid (CRYPTO_NID_NULL);
  UT_ASSERT_EQUAL ((UINTN)SlhDsaContext2, (UINTN)NULL);

  //
  // Test SlhDsaFree
  //
  SlhDsaFree (SlhDsaContext1);
  SlhDsaContext1 = NULL;

  //
  // Test SlhDsaFree with NULL (should not crash)
  //
  SlhDsaFree (NULL);

  return UNIT_TEST_PASSED;
}

/**
  Validate UEFI-OpenSSL SLH-DSA Key Setting and Getting.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaKeySetGet (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    PublicKey[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINTN    PublicKeySize;
  UINT8    TooSmallBuffer[10];
  UINTN    TooSmallSize;

  //
  // Create SLH-DSA context
  //
  SlhDsaContext1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (SlhDsaContext1);

  //
  // Test SlhDsaGetPubKey with too small buffer (before key is set)
  //
  TooSmallSize = sizeof (TooSmallBuffer);
  Status       = SlhDsaGetPubKey (SlhDsaContext1, TooSmallBuffer, &TooSmallSize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_NOT_EQUAL (TooSmallSize, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);

  //
  // Test SlhDsaSetPrivKey with NULL context
  //
  Status = SlhDsaSetPrivKey (NULL, (UINT8 *)mSlhDsaShake256sTestPemKey, 100);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSetPrivKey with NULL key
  //
  Status = SlhDsaSetPrivKey (SlhDsaContext1, NULL, SLH_DSA_SHAKE_256S_PRIVATE_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSetPrivKey with wrong size
  //
  Status = SlhDsaSetPrivKey (SlhDsaContext1, (UINT8 *)mSlhDsaShake256sTestPemKey, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSetPubKey with NULL context
  //
  Status = SlhDsaSetPubKey (NULL, (UINT8 *)mSlhDsaShake256sTestCert, 100);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSetPubKey with NULL key
  //
  Status = SlhDsaSetPubKey (SlhDsaContext1, NULL, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSetPubKey with wrong size
  //
  Status = SlhDsaSetPubKey (SlhDsaContext1, (UINT8 *)mSlhDsaShake256sTestCert, 32);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPubKey with NULL context
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = SlhDsaGetPubKey (NULL, PublicKey, &PublicKeySize);
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPubKey with NULL size
  //
  Status = SlhDsaGetPubKey (SlhDsaContext1, PublicKey, NULL);
  UT_ASSERT_FALSE (Status);

  //
  // Clean up context
  //
  SlhDsaFree (SlhDsaContext1);
  SlhDsaContext1 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA error cases.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaErrorCases (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    *Signature;
  UINTN    SigSize;
  UINT8    TooSmallBuffer[10];
  UINTN    TooSmallSize;

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Create SLH-DSA context
  //
  SlhDsaContext1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (SlhDsaContext1);

  //
  // Test SlhDsaSign with NULL context
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              NULL,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              AsciiStrLen (mSlhDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSign with NULL message
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaContext1,
              NULL,
              0,
              NULL,
              0,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaSign with too small buffer
  //
  TooSmallSize = sizeof (TooSmallBuffer);
  Status       = SlhDsaSign (
                   SlhDsaContext1,
                   NULL,
                   0,
                   (UINT8 *)mSlhDsaTestMessage,
                   AsciiStrLen (mSlhDsaTestMessage),
                   TooSmallBuffer,
                   &TooSmallSize
                   );
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_NOT_EQUAL (TooSmallSize, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);

  //
  // Test SlhDsaVerify with NULL context
  //
  Status = SlhDsaVerify (
             NULL,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             AsciiStrLen (mSlhDsaTestMessage),
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaVerify with NULL message
  //
  Status = SlhDsaVerify (
             SlhDsaContext1,
             NULL,
             0,
             NULL,
             0,
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaVerify with NULL signature
  //
  Status = SlhDsaVerify (
             SlhDsaContext1,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             AsciiStrLen (mSlhDsaTestMessage),
             NULL,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaVerify with zero signature size
  //
  Status = SlhDsaVerify (
             SlhDsaContext1,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             AsciiStrLen (mSlhDsaTestMessage),
             Signature,
             0
             );
  UT_ASSERT_FALSE (Status);

  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA key retrieval from PEM and X509.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaPemX509 (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  VOID     *SlhDsaPubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINTN    MessageSize;

  SlhDsaPrivKey = NULL;
  SlhDsaPubKey  = NULL;
  MessageSize   = AsciiStrLen (mSlhDsaTestMessage);

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Retrieve SLH-DSA private key from PEM data.
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPrivKey);

  //
  // Retrieve SLH-DSA public key from X509 certificate.
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &SlhDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPubKey);

  //
  // SLH-DSA signing with key from PEM (no context string)
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);

  //
  // SLH-DSA verification with key from X509
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  SlhDsaFree (SlhDsaPrivKey);
  SlhDsaFree (SlhDsaPubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA signing and verification with context string.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaSignVerifyWithContext (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  VOID     *SlhDsaPubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINTN    MessageSize;
  UINTN    ContextSize;

  SlhDsaPrivKey = NULL;
  SlhDsaPubKey  = NULL;
  MessageSize   = AsciiStrLen (mSlhDsaTestMessage);
  ContextSize   = AsciiStrLen (mSlhDsaTestContext);

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Retrieve SLH-DSA private key from PEM data.
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPrivKey);

  //
  // Retrieve SLH-DSA public key from X509 certificate.
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &SlhDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPubKey);

  //
  // SLH-DSA signing with context string
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              (UINT8 *)mSlhDsaTestContext,
              ContextSize,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);

  //
  // SLH-DSA verification with matching context string
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             (UINT8 *)mSlhDsaTestContext,
             ContextSize,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // SLH-DSA verification should fail with mismatched context string
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             (UINT8 *)"Different context",
             AsciiStrLen ("Different context"),
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  //
  // SLH-DSA verification should fail with no context when signature used context
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (SlhDsaPrivKey);
  SlhDsaFree (SlhDsaPubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA public key extraction and generation.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaGetPubKey (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  UINT8    PublicKey[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINTN    PublicKeySize;

  SlhDsaPrivKey = NULL;

  //
  // Retrieve SLH-DSA private key from PEM data.
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPrivKey);

  //
  // Get public key from private key context
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = SlhDsaGetPubKey (
                    SlhDsaPrivKey,
                    PublicKey,
                    &PublicKeySize
                    );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);

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

  SlhDsaFree (SlhDsaPrivKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA signature verification fails with tampered data.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaTamperedData (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  VOID     *SlhDsaPubKey;
  UINT8    *Signature;
  UINT8    *TamperedSignature;
  CHAR8    TamperedMessage[100];
  UINTN    SigSize;
  UINTN    MessageSize;

  SlhDsaPrivKey = NULL;
  SlhDsaPubKey  = NULL;
  MessageSize   = AsciiStrLen (mSlhDsaTestMessage);

  //
  // Allocate signature buffers on the heap to avoid large stack frames.
  //
  Signature         = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  TamperedSignature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  if ((Signature == NULL) || (TamperedSignature == NULL)) {
    if (Signature != NULL) {
      FreePool (Signature);
    }

    if (TamperedSignature != NULL) {
      FreePool (TamperedSignature);
    }

    UT_ASSERT_TRUE ((Signature != NULL) && (TamperedSignature != NULL));
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Retrieve SLH-DSA private key from PEM data.
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPrivKey);

  //
  // Retrieve SLH-DSA public key from X509 certificate.
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &SlhDsaPubKey
             );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_NOT_NULL (SlhDsaPubKey);

  //
  // Generate valid signature
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Verify original signature works
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test with tampered message (should fail)
  //
  AsciiStrCpyS (TamperedMessage, sizeof (TamperedMessage), mSlhDsaTestMessage);
  TamperedMessage[0] = 'X';
  Status             = SlhDsaVerify (
                         SlhDsaPubKey,
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
  CopyMem (TamperedSignature, Signature, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);

  TamperedSignature[0] ^= 0x01;
  Status                = SlhDsaVerify (
                            SlhDsaPubKey,
                            NULL,
                            0,
                            (UINT8 *)mSlhDsaTestMessage,
                            MessageSize,
                            TamperedSignature,
                            SigSize
                            );
  UT_ASSERT_FALSE (Status);

  //
  // Test with wrong signature size (should fail)
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize - 1
             );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (SlhDsaPrivKey);
  SlhDsaFree (SlhDsaPubKey);
  FreePool (Signature);
  FreePool (TamperedSignature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA PEM loading error cases.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaPemErrors (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaKey;

  SlhDsaKey = NULL;

  //
  // Test SlhDsaGetPrivateKeyFromPem with NULL PEM data
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             NULL,
             100,
             NULL,
             &SlhDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPrivateKeyFromPem with NULL context pointer
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             NULL
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPrivateKeyFromPem with invalid PEM data
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             (UINT8 *)"Invalid PEM data",
             16,
             NULL,
             &SlhDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPublicKeyFromX509 with NULL certificate
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             NULL,
             100,
             &SlhDsaKey
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPublicKeyFromX509 with NULL context pointer
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             NULL
             );
  UT_ASSERT_FALSE (Status);

  //
  // Test SlhDsaGetPublicKeyFromX509 with invalid certificate data
  //
  Status = SlhDsaGetPublicKeyFromX509 (
             (UINT8 *)"Invalid cert data",
             17,
             &SlhDsaKey
             );
  UT_ASSERT_FALSE (Status);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA operations on context without keys.

  This test validates that operations fail properly when no key is set.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaNoKeyOperations (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  UINT8    *Signature;
  UINT8    PublicKey[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINTN    SigSize;
  UINTN    PublicKeySize;

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Create context without setting any key
  //
  SlhDsaContext1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (SlhDsaContext1);

  //
  // SlhDsaSign should fail when EvpPkey is NULL
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaContext1,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              AsciiStrLen (mSlhDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // SlhDsaVerify should fail when EvpPkey is NULL
  //
  Status = SlhDsaVerify (
             SlhDsaContext1,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             AsciiStrLen (mSlhDsaTestMessage),
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE
             );
  UT_ASSERT_FALSE (Status);

  //
  // SlhDsaGetPubKey should fail when EvpPkey is NULL
  //
  PublicKeySize = sizeof (PublicKey);
  Status        = SlhDsaGetPubKey (
                    SlhDsaContext1,
                    PublicKey,
                    &PublicKeySize
                    );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (SlhDsaContext1);
  SlhDsaContext1 = NULL;
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA context string parameter validation.

  This test validates the check: (ContextSize > 0) && (Context == NULL).

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaInvalidContextParams (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  VOID     *SlhDsaPubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINTN    MessageSize;

  SlhDsaPrivKey = NULL;
  SlhDsaPubKey  = NULL;
  MessageSize   = AsciiStrLen (mSlhDsaTestMessage);

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Get valid keys
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &SlhDsaPubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test SlhDsaSign with NULL Context but ContextSize > 0 (invalid combination)
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              NULL,
              10,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_FALSE (Status);

  //
  // Generate valid signature for verify test
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Test SlhDsaVerify with NULL Context but ContextSize > 0 (invalid combination)
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             5,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SigSize
             );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (SlhDsaPrivKey);
  SlhDsaFree (SlhDsaPubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Validate SlhDsaGetPubKey with NULL buffer parameter.

  This test validates that NULL PublicKey buffer returns FALSE and sets size to 0.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaGetPubKeyNullBuffer (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  UINTN    PublicKeySize;

  SlhDsaPrivKey = NULL;

  //
  // Get valid private key
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Test SlhDsaGetPubKey with NULL buffer
  // Should return FALSE and set PublicKeySize to 0
  //
  PublicKeySize = 9999;
  Status        = SlhDsaGetPubKey (
                    SlhDsaPrivKey,
                    NULL,
                    &PublicKeySize
                    );
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_EQUAL (PublicKeySize, 0);

  SlhDsaFree (SlhDsaPrivKey);

  return UNIT_TEST_PASSED;
}

/**
  Validate SLH-DSA signature size exact match validation.

  This test validates that SlhDsaVerify properly validates signature size.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaSignatureSizeExact (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *SlhDsaPrivKey;
  VOID     *SlhDsaPubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINTN    MessageSize;

  SlhDsaPrivKey = NULL;
  SlhDsaPubKey  = NULL;
  MessageSize   = AsciiStrLen (mSlhDsaTestMessage);

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Get valid keys
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &SlhDsaPrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &SlhDsaPubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Generate valid signature
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              SlhDsaPrivKey,
              NULL,
              0,
              (UINT8 *)mSlhDsaTestMessage,
              MessageSize,
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (SigSize, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);

  //
  // Verify with exact size - should succeed
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE
             );
  UT_ASSERT_TRUE (Status);

  //
  // Verify with size + 1 - should fail
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE + 1
             );
  UT_ASSERT_FALSE (Status);

  //
  // Verify with size - 1 - should fail
  //
  Status = SlhDsaVerify (
             SlhDsaPubKey,
             NULL,
             0,
             (UINT8 *)mSlhDsaTestMessage,
             MessageSize,
             Signature,
             SLH_DSA_SHAKE_256S_SIGNATURE_SIZE - 1
             );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (SlhDsaPrivKey);
  SlhDsaFree (SlhDsaPubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA context lifecycle with multiple allocations.

  Tests proper resource management including multiple successive allocations,
  cleanup verification, and null pointer safety.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaContextLifecycle (
  UNIT_TEST_CONTEXT  Context
  )
{
  VOID  *TempCtx1;
  VOID  *TempCtx2;
  VOID  *TempCtx3;

  //
  // Test creating multiple contexts
  //
  TempCtx1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (TempCtx1);

  TempCtx2 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (TempCtx2);

  TempCtx3 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (TempCtx3);

  //
  // Free in different order
  //
  SlhDsaFree (TempCtx2);
  SlhDsaFree (TempCtx1);
  SlhDsaFree (TempCtx3);

  //
  // Double-free safety (should not crash)
  //
  SlhDsaFree (NULL);
  SlhDsaFree (NULL);

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA empty message signing and verification.

  Tests edge case of zero-length messages.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaEmptyMessage (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINT8    EmptyMsg[1];

  PrivKey = NULL;
  PubKey  = NULL;

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Load keys
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Sign empty message - should fail with NULL message
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
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
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
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
  Status = SlhDsaVerify (
             PubKey,
             NULL,
             0,
             EmptyMsg,
             0,
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  SlhDsaFree (PrivKey);
  SlhDsaFree (PubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA maximum length context string.

  Tests the maximum allowed context string size per FIPS 204.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaMaxContextString (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    *Signature;
  UINTN    SigSize;
  UINT8    MaxContext[SLH_DSA_MAX_CONTEXT_SIZE];
  UINTN    Index;

  PrivKey = NULL;
  PubKey  = NULL;

  //
  // Allocate signature buffer on the heap to avoid large stack frames.
  //
  Signature = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_NOT_NULL (Signature);
  if (Signature == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Fill context with pattern
  //
  for (Index = 0; Index < SLH_DSA_MAX_CONTEXT_SIZE; Index++) {
    MaxContext[Index] = (UINT8)(Index & 0xFF);
  }

  //
  // Load keys
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Sign with maximum context
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (
              PrivKey,
              MaxContext,
              SLH_DSA_MAX_CONTEXT_SIZE,
              (UINT8 *)mSlhDsaTestMessage,
              AsciiStrLen (mSlhDsaTestMessage),
              Signature,
              &SigSize
              );
  UT_ASSERT_TRUE (Status);

  //
  // Verify with matching context
  //
  Status = SlhDsaVerify (
             PubKey,
             MaxContext,
             SLH_DSA_MAX_CONTEXT_SIZE,
             (UINT8 *)mSlhDsaTestMessage,
             AsciiStrLen (mSlhDsaTestMessage),
             Signature,
             SigSize
             );
  UT_ASSERT_TRUE (Status);

  //
  // Verify should fail with one byte different
  //
  MaxContext[0] ^= 1;
  Status         = SlhDsaVerify (
                     PubKey,
                     MaxContext,
                     SLH_DSA_MAX_CONTEXT_SIZE,
                     (UINT8 *)mSlhDsaTestMessage,
                     AsciiStrLen (mSlhDsaTestMessage),
                     Signature,
                     SigSize
                     );
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (PrivKey);
  SlhDsaFree (PubKey);
  FreePool (Signature);

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA key replacement in context.

  Tests that replacing keys properly frees old key and sets new key.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaKeyReplacement (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  UINT8    PublicKey1[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINT8    PublicKey2[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINTN    PubKeySize;

  PrivKey = NULL;

  //
  // Load first key
  //
  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Get public key
  //
  PubKeySize = sizeof (PublicKey1);
  Status     = SlhDsaGetPubKey (PrivKey, PublicKey1, &PubKeySize);
  UT_ASSERT_TRUE (Status);

  //
  // Now replace with a public-only key
  //
  SlhDsaContext1 = SlhDsaNewByNid (CRYPTO_NID_SLH_DSA_SHAKE_256S);
  UT_ASSERT_NOT_NULL (SlhDsaContext1);

  Status = SlhDsaSetPubKey (SlhDsaContext1, PublicKey1, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Replace with same public key again (tests EVP_PKEY_free path)
  //
  Status = SlhDsaSetPubKey (SlhDsaContext1, PublicKey1, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Verify we can still get the key
  //
  PubKeySize = sizeof (PublicKey2);
  Status     = SlhDsaGetPubKey (SlhDsaContext1, PublicKey2, &PubKeySize);
  UT_ASSERT_TRUE (Status);
  UT_ASSERT_EQUAL (PubKeySize, SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE);

  SlhDsaFree (PrivKey);
  SlhDsaFree (SlhDsaContext1);
  SlhDsaContext1 = NULL;

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA multiple signatures with same key.

  Tests that the same key can generate multiple signatures.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaMultipleSignatures (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  VOID     *PubKey;
  UINT8    *Sig1;
  UINT8    *Sig2;
  UINT8    *Sig3;
  UINTN    SigSize;
  CHAR8    *Msg1 = "First message";
  CHAR8    *Msg2 = "Second message";
  CHAR8    *Msg3 = "Third message";

  PrivKey = NULL;
  PubKey  = NULL;

  //
  // Allocate signature buffers on the heap to avoid large stack frames.
  //
  Sig1 = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  Sig2 = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  Sig3 = AllocatePool (SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  if ((Sig1 == NULL) || (Sig2 == NULL) || (Sig3 == NULL)) {
    if (Sig1 != NULL) {
      FreePool (Sig1);
    }

    if (Sig2 != NULL) {
      FreePool (Sig2);
    }

    if (Sig3 != NULL) {
      FreePool (Sig3);
    }

    UT_ASSERT_TRUE ((Sig1 != NULL) && (Sig2 != NULL) && (Sig3 != NULL));
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaGetPublicKeyFromX509 (
             mSlhDsaShake256sTestCert,
             sizeof (mSlhDsaShake256sTestCert),
             &PubKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Generate three different signatures
  //
  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig1, &SigSize);
  UT_ASSERT_TRUE (Status);

  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig2, &SigSize);
  UT_ASSERT_TRUE (Status);

  SigSize = SLH_DSA_SHAKE_256S_SIGNATURE_SIZE;
  Status  = SlhDsaSign (PrivKey, NULL, 0, (UINT8 *)Msg3, AsciiStrLen (Msg3), Sig3, &SigSize);
  UT_ASSERT_TRUE (Status);

  //
  // Verify all three with correct messages
  //
  Status = SlhDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig1, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig2, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_TRUE (Status);

  Status = SlhDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg3, AsciiStrLen (Msg3), Sig3, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Cross-verify should fail (wrong message/signature pairs)
  //
  Status = SlhDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg1, AsciiStrLen (Msg1), Sig2, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_FALSE (Status);

  Status = SlhDsaVerify (PubKey, NULL, 0, (UINT8 *)Msg2, AsciiStrLen (Msg2), Sig3, SLH_DSA_SHAKE_256S_SIGNATURE_SIZE);
  UT_ASSERT_FALSE (Status);

  SlhDsaFree (PrivKey);
  SlhDsaFree (PubKey);
  FreePool (Sig1);
  FreePool (Sig2);
  FreePool (Sig3);

  return UNIT_TEST_PASSED;
}

/**
  Test SLH-DSA GetPubKey called multiple times.

  Tests that repeated calls to GetPubKey return consistent results.

  @param[in]  Context  Unit test context.

  @retval UNIT_TEST_PASSED               The unit test has completed successfully.
  @retval UNIT_TEST_ERROR_TEST_FAILED    A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
TestVerifySlhDsaGetPubKeyRepeated (
  UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN  Status;
  VOID     *PrivKey;
  UINT8    PubKey1[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINT8    PubKey2[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINT8    PubKey3[SLH_DSA_SHAKE_256S_PUBLIC_KEY_SIZE];
  UINTN    Size1, Size2, Size3;

  PrivKey = NULL;

  Status = SlhDsaGetPrivateKeyFromPem (
             mSlhDsaShake256sTestPemKey,
             sizeof (mSlhDsaShake256sTestPemKey),
             NULL,
             &PrivKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Get public key three times
  //
  Size1  = sizeof (PubKey1);
  Status = SlhDsaGetPubKey (PrivKey, PubKey1, &Size1);
  UT_ASSERT_TRUE (Status);

  Size2  = sizeof (PubKey2);
  Status = SlhDsaGetPubKey (PrivKey, PubKey2, &Size2);
  UT_ASSERT_TRUE (Status);

  Size3  = sizeof (PubKey3);
  Status = SlhDsaGetPubKey (PrivKey, PubKey3, &Size3);
  UT_ASSERT_TRUE (Status);

  //
  // All should be identical
  //
  UT_ASSERT_EQUAL (Size1, Size2);
  UT_ASSERT_EQUAL (Size2, Size3);
  UT_ASSERT_MEM_EQUAL (PubKey1, PubKey2, Size1);
  UT_ASSERT_MEM_EQUAL (PubKey2, PubKey3, Size2);

  SlhDsaFree (PrivKey);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mSlhDsaTest[] = {
  //
  // -----Description------------------------------------Class-------------------------Function----------------------------Pre-------------------Post--------------------Context
  //
  { "TestVerifySlhDsaNew()",                   "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaNew,                   TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaKeySetGet()",             "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaKeySetGet,             TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaErrorCases()",            "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaErrorCases,            TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaPemX509()",               "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaPemX509,               TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaSignVerifyWithContext()", "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaSignVerifyWithContext, TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaGetPubKey()",             "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaGetPubKey,             TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaTamperedData()",          "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaTamperedData,          TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaPemErrors()",             "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaPemErrors,             TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaNoKeyOperations()",       "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaNoKeyOperations,       TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaInvalidContextParams()",  "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaInvalidContextParams,  TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaGetPubKeyNullBuffer()",   "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaGetPubKeyNullBuffer,   TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaSignatureSizeExact()",    "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaSignatureSizeExact,    TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaContextLifecycle()",      "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaContextLifecycle,      TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaEmptyMessage()",          "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaEmptyMessage,          TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaMaxContextString()",      "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaMaxContextString,      TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaKeyReplacement()",        "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaKeyReplacement,        TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaMultipleSignatures()",    "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaMultipleSignatures,    TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
  { "TestVerifySlhDsaGetPubKeyRepeated()",     "CryptoPkg.BaseCryptLib.SlhDsa", TestVerifySlhDsaGetPubKeyRepeated,     TestVerifySlhDsaPreReq, TestVerifySlhDsaCleanUp, NULL },
};

UINTN  mSlhDsaTestNum = ARRAY_SIZE (mSlhDsaTest);
