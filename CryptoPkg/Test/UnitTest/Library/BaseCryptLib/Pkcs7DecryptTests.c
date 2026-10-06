/** @file
  Unit tests for the Pkcs7Decrypt() API in BaseCryptLib.

  These tests exercise both the OpenSSL- and mbedtls-backed implementations.
  Each envelopedData input is produced by Pkcs7Encrypt() and then round-tripped
  back through Pkcs7Decrypt() with the recipient's private key to prove the
  payload is recovered byte-for-byte, for every supported AES-CBC cipher and for
  both the single- and multiple-recipient cases. Negative tests cover the
  parameter-validation contract documented in BaseCryptLib.h as well as
  wrong-key, wrong-password, wrong-recipient and malformed-input handling.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

#include "Pkcs7EncryptTestCert.h"

//
// Sample plaintext to encrypt. Length is intentionally not a multiple of the
// AES block size to exercise CBC padding within the implementation.
//
STATIC CONST UINT8  mPkcs7DecryptPlaintext[] = {
  'P', 'k', 'c', 's', '7', 'D', 'e', 'c', 'r', 'y', 'p', 't',
  ' ', 'u', 'n', 'i', 't', '-', 't', 'e', 's', 't', ' ', 'p',
  'a', 'y', 'l', 'o', 'a', 'd'
};

/**
  Decrypt a ContentInfo produced by Pkcs7Encrypt() and verify that the recovered
  content matches mPkcs7DecryptPlaintext exactly.

  @param[in]  ContentInfo        Encoded envelopedData ContentInfo.
  @param[in]  ContentInfoSize    Length of ContentInfo.
  @param[in]  KeyPem             PEM-encoded recipient private key.
  @param[in]  KeyPemSize         Length of KeyPem.
  @param[in]  KeyPass            Passphrase for KeyPem.
  @param[in]  RecipientCert      Optional DER recipient certificate used to select
                                 the matching RecipientInfo. NULL to search all.
  @param[in]  RecipientCertSize  Length of RecipientCert, or 0.

  @retval UNIT_TEST_PASSED            The plaintext round-tripped.
  @retval UNIT_TEST_ERROR_TEST_FAILED Decryption failed or content mismatched.
**/
STATIC
UNIT_TEST_STATUS
CheckPkcs7DecryptRoundTrip (
  IN CONST UINT8  *ContentInfo,
  IN UINTN        ContentInfoSize,
  IN CONST UINT8  *KeyPem,
  IN UINTN        KeyPemSize,
  IN CONST CHAR8  *KeyPass,
  IN CONST UINT8  *RecipientCert,
  IN UINTN        RecipientCertSize
  )
{
  UINT8    *OutData;
  UINTN    OutDataSize;
  BOOLEAN  Result;
  INTN     Compare;

  OutData     = NULL;
  OutDataSize = 0;

  Result = Pkcs7Decrypt (
             KeyPem,
             KeyPemSize,
             KeyPass,
             RecipientCert,
             RecipientCertSize,
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (OutData);

  Compare = 1;
  if (OutDataSize == sizeof (mPkcs7DecryptPlaintext)) {
    Compare = CompareMem (OutData, mPkcs7DecryptPlaintext, sizeof (mPkcs7DecryptPlaintext));
  }

  FreePool (OutData);

  UT_ASSERT_EQUAL (OutDataSize, sizeof (mPkcs7DecryptPlaintext));
  UT_ASSERT_EQUAL (Compare, 0);
  return UNIT_TEST_PASSED;
}

/**
  Build an envelopedData ContentInfo addressed to recipient 1 using AES-256-CBC.

  @param[out]  ContentInfo      Receives the allocated ContentInfo. Free with FreePool().
  @param[out]  ContentInfoSize  Receives the length of ContentInfo.

  @retval UNIT_TEST_PASSED            The envelope was produced.
  @retval UNIT_TEST_ERROR_TEST_FAILED Stack construction or encryption failed.
**/
STATIC
UNIT_TEST_STATUS
BuildPkcs7TestEnvelope (
  OUT UINT8  **ContentInfo,
  OUT UINTN  *ContentInfoSize
  )
{
  UINT8    *X509Stack;
  BOOLEAN  Result;

  X509Stack        = NULL;
  *ContentInfo     = NULL;
  *ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7TestRecipient1Cert,
             (UINTN)sizeof (mPkcs7TestRecipient1Cert),
             NULL
             );
  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (X509Stack);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7DecryptPlaintext,
             sizeof (mPkcs7DecryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             ContentInfo,
             ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (*ContentInfo);
  UT_ASSERT_NOT_EQUAL (*ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  Encrypt the sample plaintext to recipient 1 with the requested cipher and then
  recover it with recipient 1's private key, both by naming the recipient
  certificate explicitly and by letting Pkcs7Decrypt() search every
  RecipientInfo.

  @param[in]  CipherNid  Symmetric cipher NID to request from Pkcs7Encrypt().

  @retval UNIT_TEST_PASSED            The plaintext round-tripped both ways.
  @retval UNIT_TEST_ERROR_TEST_FAILED Stack build, encrypt, or decrypt failed.
**/
STATIC
UNIT_TEST_STATUS
RunPkcs7DecryptRoundTrip (
  IN UINT32  CipherNid
  )
{
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7TestRecipient1Cert,
             (UINTN)sizeof (mPkcs7TestRecipient1Cert),
             NULL
             );
  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (X509Stack);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7DecryptPlaintext,
             sizeof (mPkcs7DecryptPlaintext),
             CipherNid,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_NOT_EQUAL (ContentInfoSize, 0);

  //
  // Recover the plaintext with the recipient certificate identifying the
  // RecipientInfo to use.
  //
  Status = CheckPkcs7DecryptRoundTrip (
             ContentInfo,
             ContentInfoSize,
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert)
             );

  if (Status == UNIT_TEST_PASSED) {
    //
    // Repeat without a certificate so the private key is matched against every
    // RecipientInfo in the envelope.
    //
    Status = CheckPkcs7DecryptRoundTrip (
               ContentInfo,
               ContentInfoSize,
               mPkcs7TestRecipient1KeyPem,
               sizeof (mPkcs7TestRecipient1KeyPem),
               mPkcs7TestRecipient1KeyPass,
               NULL,
               0
               );
  }

  FreePool (ContentInfo);

  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);
  return UNIT_TEST_PASSED;
}

/**
  Round-trip test for AES-128-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptAes128Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7DecryptRoundTrip (CRYPTO_NID_AES128CBC);
}

/**
  Round-trip test for AES-192-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptAes192Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7DecryptRoundTrip (CRYPTO_NID_AES192CBC);
}

/**
  Round-trip test for AES-256-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptAes256Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7DecryptRoundTrip (CRYPTO_NID_AES256CBC);
}

/**
  An envelope addressed to two recipients must be decryptable by either one.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptMultipleRecipients (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *X509Stack;
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7TestRecipient1Cert,
             (UINTN)sizeof (mPkcs7TestRecipient1Cert),
             mPkcs7TestRecipient2Cert,
             (UINTN)sizeof (mPkcs7TestRecipient2Cert),
             NULL
             );
  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (X509Stack);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7DecryptPlaintext,
             sizeof (mPkcs7DecryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_NOT_EQUAL (ContentInfoSize, 0);

  Status = CheckPkcs7DecryptRoundTrip (
             ContentInfo,
             ContentInfoSize,
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert)
             );

  if (Status == UNIT_TEST_PASSED) {
    Status = CheckPkcs7DecryptRoundTrip (
               ContentInfo,
               ContentInfoSize,
               mPkcs7TestRecipient2KeyPem,
               sizeof (mPkcs7TestRecipient2KeyPem),
               mPkcs7TestRecipient2KeyPass,
               mPkcs7TestRecipient2Cert,
               sizeof (mPkcs7TestRecipient2Cert)
               );
  }

  if (Status == UNIT_TEST_PASSED) {
    //
    // Recipient 2 is the second RecipientInfo, so decrypting without a
    // certificate exercises the "try every recipient" search path.
    //
    Status = CheckPkcs7DecryptRoundTrip (
               ContentInfo,
               ContentInfoSize,
               mPkcs7TestRecipient2KeyPem,
               sizeof (mPkcs7TestRecipient2KeyPem),
               mPkcs7TestRecipient2KeyPass,
               NULL,
               0
               );
  }

  FreePool (ContentInfo);

  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);
  return UNIT_TEST_PASSED;
}

/**
  A private key belonging to a recipient that is not present in the envelope must
  be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptWrongRecipient (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient2KeyPem,
             sizeof (mPkcs7TestRecipient2KeyPem),
             mPkcs7TestRecipient2KeyPass,
             mPkcs7TestRecipient2Cert,
             sizeof (mPkcs7TestRecipient2Cert),
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  An incorrect PEM passphrase must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptWrongPassword (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             "not-the-password",
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert),
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  A NULL passphrase for a password-protected PEM key must be rejected rather than
  prompting or asserting.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptMissingPassword (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             NULL,
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert),
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  A ContentInfo that is not an envelopedData must be rejected. A DER-encoded X.509
  certificate is used as a well-formed but wrongly-typed input.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptNotEnvelopedData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *OutData;
  UINTN    OutDataSize;
  BOOLEAN  Result;

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert),
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  A truncated ContentInfo must be rejected without reading out of bounds.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptTruncatedContentInfo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             mPkcs7TestRecipient1Cert,
             sizeof (mPkcs7TestRecipient1Cert),
             ContentInfo,
             ContentInfoSize / 2,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL PemData must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptNullPemData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             NULL,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL ContentInfo must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptNullContentInfo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *OutData;
  UINTN    OutDataSize;
  BOOLEAN  Result;

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             NULL,
             128,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             &OutDataSize
             );

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL OutData output pointer must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptNullOutData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             NULL,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL OutDataSize output pointer must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptNullOutDataSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData = (UINT8 *)(UINTN)0xDEADBEEF;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             ContentInfo,
             ContentInfoSize,
             CRYPTO_PKCS7_DEFAULT,
             &OutData,
             NULL
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  return UNIT_TEST_PASSED;
}

/**
  Any Flags value other than CRYPTO_PKCS7_DEFAULT must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7DecryptUnsupportedFlags (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8             *ContentInfo;
  UINTN             ContentInfoSize;
  UINT8             *OutData;
  UINTN             OutDataSize;
  BOOLEAN           Result;
  UNIT_TEST_STATUS  Status;

  Status = BuildPkcs7TestEnvelope (&ContentInfo, &ContentInfoSize);
  UT_ASSERT_EQUAL (Status, UNIT_TEST_PASSED);

  OutData     = (UINT8 *)(UINTN)0xDEADBEEF;
  OutDataSize = 0xA5A5A5;

  Result = Pkcs7Decrypt (
             mPkcs7TestRecipient1KeyPem,
             sizeof (mPkcs7TestRecipient1KeyPem),
             mPkcs7TestRecipient1KeyPass,
             NULL,
             0,
             ContentInfo,
             ContentInfoSize,
             0xDEADBEEF,
             &OutData,
             &OutDataSize
             );

  FreePool (ContentInfo);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (OutData == NULL);
  UT_ASSERT_EQUAL (OutDataSize, 0);
  return UNIT_TEST_PASSED;
}

//
// Test descriptor table consumed by BaseCryptLibUnitTests and
// BaseCryptLibUnitTestsMbedTls.
//
TEST_DESC  mPkcs7DecryptTest[] = {
  //
  // -----Description--------------------------------------Class-----------------------Function-----------------------------------PreReq-Post-Context
  //
  { "TestPkcs7DecryptAes128Cbc()",            "CryptoPkg.BaseCryptLib", TestPkcs7DecryptAes128Cbc,            NULL, NULL, NULL },
  { "TestPkcs7DecryptAes192Cbc()",            "CryptoPkg.BaseCryptLib", TestPkcs7DecryptAes192Cbc,            NULL, NULL, NULL },
  { "TestPkcs7DecryptAes256Cbc()",            "CryptoPkg.BaseCryptLib", TestPkcs7DecryptAes256Cbc,            NULL, NULL, NULL },
  { "TestPkcs7DecryptMultipleRecipients()",   "CryptoPkg.BaseCryptLib", TestPkcs7DecryptMultipleRecipients,   NULL, NULL, NULL },
  { "TestPkcs7DecryptWrongRecipient()",       "CryptoPkg.BaseCryptLib", TestPkcs7DecryptWrongRecipient,       NULL, NULL, NULL },
  { "TestPkcs7DecryptWrongPassword()",        "CryptoPkg.BaseCryptLib", TestPkcs7DecryptWrongPassword,        NULL, NULL, NULL },
  { "TestPkcs7DecryptMissingPassword()",      "CryptoPkg.BaseCryptLib", TestPkcs7DecryptMissingPassword,      NULL, NULL, NULL },
  { "TestPkcs7DecryptNotEnvelopedData()",     "CryptoPkg.BaseCryptLib", TestPkcs7DecryptNotEnvelopedData,     NULL, NULL, NULL },
  { "TestPkcs7DecryptTruncatedContentInfo()", "CryptoPkg.BaseCryptLib", TestPkcs7DecryptTruncatedContentInfo, NULL, NULL, NULL },
  { "TestPkcs7DecryptNullPemData()",          "CryptoPkg.BaseCryptLib", TestPkcs7DecryptNullPemData,          NULL, NULL, NULL },
  { "TestPkcs7DecryptNullContentInfo()",      "CryptoPkg.BaseCryptLib", TestPkcs7DecryptNullContentInfo,      NULL, NULL, NULL },
  { "TestPkcs7DecryptNullOutData()",          "CryptoPkg.BaseCryptLib", TestPkcs7DecryptNullOutData,          NULL, NULL, NULL },
  { "TestPkcs7DecryptNullOutDataSize()",      "CryptoPkg.BaseCryptLib", TestPkcs7DecryptNullOutDataSize,      NULL, NULL, NULL },
  { "TestPkcs7DecryptUnsupportedFlags()",     "CryptoPkg.BaseCryptLib", TestPkcs7DecryptUnsupportedFlags,     NULL, NULL, NULL },
};

UINTN  mPkcs7DecryptTestNum = ARRAY_SIZE (mPkcs7DecryptTest);
