/** @file
  Unit tests for the Pkcs7Encrypt() API in BaseCryptLib.

  These tests exercise both the OpenSSL- and mbedtls-backed implementations.
  They build envelopedData ContentInfo blobs for one or more recipients with
  each supported AES-CBC cipher and verify the result structurally (DER walk)
  without requiring the matching private key. Negative tests cover the
  parameter-validation contract documented in BaseCryptLib.h.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"

#include "Pkcs7EncryptTestCert.h"

//
// Sample plaintext to encrypt. Length is intentionally not a multiple of the
// AES block size to exercise CBC padding within the implementation.
//
STATIC CONST UINT8  mPkcs7EncryptPlaintext[] = {
  'P', 'k', 'c', 's', '7', 'E', 'n', 'c', 'r', 'y', 'p', 't',
  ' ', 'u', 'n', 'i', 't', '-', 't', 'e', 's', 't', ' ', 'p',
  'a', 'y', 'l', 'o', 'a', 'd'
};

//
// DER OIDs (full tag-length-value form) of the values we expect to see inside
// a well-formed envelopedData ContentInfo produced by Pkcs7Encrypt().
//
// id-envelopedData = 1.2.840.113549.1.7.3
//
STATIC CONST UINT8  mOidEnvelopedData[] = {
  0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x03
};

// aes-128-cbc = 2.16.840.1.101.3.4.1.2
STATIC CONST UINT8  mOidAes128Cbc[] = {
  0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x02
};

// aes-192-cbc = 2.16.840.1.101.3.4.1.22
STATIC CONST UINT8  mOidAes192Cbc[] = {
  0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x16
};

// aes-256-cbc = 2.16.840.1.101.3.4.1.42
STATIC CONST UINT8  mOidAes256Cbc[] = {
  0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2A
};

//
// Minimal ASN.1 / DER walker. Just enough to validate Pkcs7Encrypt output
// without pulling in OpenSSL/mbedtls headers from the test.
//
typedef struct {
  UINT8          Tag;     ///< Identifier octet.
  UINTN          Length;  ///< Decoded content length (excludes T+L bytes).
  CONST UINT8    *Value;  ///< Pointer to the first content octet.
  UINTN          Total;   ///< Total bytes consumed (T + L + V).
} DER_TLV;

/**
  Decode a single ASN.1 BER/DER TLV starting at Buf.

  @param[in]   Buf      Input buffer.
  @param[in]   BufLen   Length of Buf in bytes.
  @param[out]  Tlv      Decoded TLV on success.

  @retval TRUE   Decode succeeded.
  @retval FALSE  Truncated or malformed input.
**/
STATIC
BOOLEAN
DerParseTlv (
  IN CONST UINT8  *Buf,
  IN UINTN        BufLen,
  OUT DER_TLV     *Tlv
  )
{
  UINTN  Pos;
  UINTN  Len;
  UINTN  LenOctets;
  UINTN  Index;

  if ((Buf == NULL) || (Tlv == NULL) || (BufLen < 2)) {
    return FALSE;
  }

  Tlv->Tag = Buf[0];
  Pos      = 1;

  if ((Buf[Pos] & 0x80) == 0) {
    Len  = Buf[Pos];
    Pos += 1;
  } else {
    LenOctets = Buf[Pos] & 0x7Fu;
    Pos      += 1;
    if ((LenOctets == 0) || (LenOctets > sizeof (UINTN)) || (BufLen < Pos + LenOctets)) {
      return FALSE;
    }

    Len = 0;
    for (Index = 0; Index < LenOctets; Index++) {
      Len  = (Len << 8) | Buf[Pos];
      Pos += 1;
    }
  }

  if (Len > BufLen - Pos) {
    return FALSE;
  }

  Tlv->Length = Len;
  Tlv->Value  = Buf + Pos;
  Tlv->Total  = Pos + Len;
  return TRUE;
}

/**
  Compare the leading bytes of the TLV's encoded form (tag+length+value) against
  a reference encoded OID.

  @param[in]  Buf            Buffer containing the encoded OID TLV.
  @param[in]  BufLen         Bytes available at Buf.
  @param[in]  ExpectedOid    Reference DER-encoded OID (T+L+V).
  @param[in]  ExpectedOidLen Length of ExpectedOid.

  @retval TRUE   The first ExpectedOidLen bytes of Buf match ExpectedOid.
  @retval FALSE  Mismatch, or Buf too short.
**/
STATIC
BOOLEAN
DerOidEquals (
  IN CONST UINT8  *Buf,
  IN UINTN        BufLen,
  IN CONST UINT8  *ExpectedOid,
  IN UINTN        ExpectedOidLen
  )
{
  if (BufLen < ExpectedOidLen) {
    return FALSE;
  }

  return (BOOLEAN)(CompareMem (Buf, ExpectedOid, ExpectedOidLen) == 0);
}

/**
  Walk a PKCS#7 envelopedData ContentInfo produced by Pkcs7Encrypt() and verify
  it carries the expected content-encryption algorithm and recipient count.

  Reference: RFC 5652, Sections 3 and 6.

      ContentInfo ::= SEQUENCE {
        contentType ContentType,                  -- id-envelopedData
        content     [0] EXPLICIT EnvelopedData
      }
      EnvelopedData ::= SEQUENCE {
        version              CMSVersion,
        [originatorInfo  [0] IMPLICIT OriginatorInfo OPTIONAL,]  -- v>=2
        recipientInfos       RecipientInfos,                     -- SET OF
        encryptedContentInfo EncryptedContentInfo
        [, unprotectedAttrs [1] IMPLICIT UnprotectedAttributes OPTIONAL]
      }
      EncryptedContentInfo ::= SEQUENCE {
        contentType                ContentType,
        contentEncryptionAlgorithm ContentEncryptionAlgorithmIdentifier,
        encryptedContent       [0] IMPLICIT EncryptedContent OPTIONAL
      }

  @param[in]  Buf                     Encoded ContentInfo.
  @param[in]  BufLen                  Length of Buf.
  @param[in]  ExpectedCipherOid       DER-encoded OID for the expected
                                      content-encryption algorithm.
  @param[in]  ExpectedCipherOidLen    Length of ExpectedCipherOid.
  @param[in]  ExpectedRecipientCount  Number of RecipientInfo entries expected.

  @retval TRUE   The structure parses and matches expectations.
  @retval FALSE  Parse error or unexpected content.
**/
STATIC
BOOLEAN
ValidatePkcs7EnvelopedData (
  IN CONST UINT8  *Buf,
  IN UINTN        BufLen,
  IN CONST UINT8  *ExpectedCipherOid,
  IN UINTN        ExpectedCipherOidLen,
  IN UINTN        ExpectedRecipientCount
  )
{
  DER_TLV      Outer;
  DER_TLV      ContentTypeOid;
  DER_TLV      Explicit0;
  DER_TLV      EnvelopedData;
  DER_TLV      Version;
  DER_TLV      Recipients;
  DER_TLV      EncryptedContentInfo;
  DER_TLV      EncContentTypeOid;
  DER_TLV      AlgorithmId;
  DER_TLV      AlgorithmOid;
  DER_TLV      RecipientInfo;
  CONST UINT8  *Cursor;
  UINTN        Remaining;
  UINTN        Count;
  UINT8        VersionValue;

  //
  // Outer SEQUENCE (ContentInfo).
  //
  if (!DerParseTlv (Buf, BufLen, &Outer) || (Outer.Tag != 0x30)) {
    return FALSE;
  }

  Cursor    = Outer.Value;
  Remaining = Outer.Length;

  //
  // contentType OID.
  //
  if (!DerParseTlv (Cursor, Remaining, &ContentTypeOid) || (ContentTypeOid.Tag != 0x06)) {
    return FALSE;
  }

  if (!DerOidEquals (Cursor, Remaining, mOidEnvelopedData, sizeof (mOidEnvelopedData))) {
    return FALSE;
  }

  Cursor    += ContentTypeOid.Total;
  Remaining -= ContentTypeOid.Total;

  //
  // content [0] EXPLICIT EnvelopedData.
  //
  if (!DerParseTlv (Cursor, Remaining, &Explicit0) || (Explicit0.Tag != 0xA0)) {
    return FALSE;
  }

  if (!DerParseTlv (Explicit0.Value, Explicit0.Length, &EnvelopedData) || (EnvelopedData.Tag != 0x30)) {
    return FALSE;
  }

  Cursor    = EnvelopedData.Value;
  Remaining = EnvelopedData.Length;

  //
  // version INTEGER.
  //
  if (!DerParseTlv (Cursor, Remaining, &Version) || (Version.Tag != 0x02)) {
    return FALSE;
  }

  if ((Version.Length != 1) || (Version.Value == NULL)) {
    return FALSE;
  }

  VersionValue = Version.Value[0];
  if ((VersionValue != 0) && (VersionValue != 2)) {
    return FALSE;
  }

  Cursor    += Version.Total;
  Remaining -= Version.Total;

  //
  // Optional originatorInfo [0] IMPLICIT -- skip if present (v2 producers).
  //
  if ((Remaining > 0) && (Cursor[0] == 0xA0)) {
    DER_TLV  Skip;
    if (!DerParseTlv (Cursor, Remaining, &Skip)) {
      return FALSE;
    }

    Cursor    += Skip.Total;
    Remaining -= Skip.Total;
  }

  //
  // recipientInfos SET OF RecipientInfo.
  //
  if (!DerParseTlv (Cursor, Remaining, &Recipients) || (Recipients.Tag != 0x31)) {
    return FALSE;
  }

  //
  // Count top-level TLVs inside the SET.
  //
  {
    CONST UINT8  *RcptCursor;
    UINTN        RcptRemaining;

    RcptCursor    = Recipients.Value;
    RcptRemaining = Recipients.Length;
    Count         = 0;
    while (RcptRemaining > 0) {
      if (!DerParseTlv (RcptCursor, RcptRemaining, &RecipientInfo)) {
        return FALSE;
      }

      Count         += 1;
      RcptCursor    += RecipientInfo.Total;
      RcptRemaining -= RecipientInfo.Total;
    }
  }

  if (Count != ExpectedRecipientCount) {
    return FALSE;
  }

  Cursor    += Recipients.Total;
  Remaining -= Recipients.Total;

  //
  // encryptedContentInfo SEQUENCE { contentType, contentEncryptionAlgorithm, ... }.
  //
  if (!DerParseTlv (Cursor, Remaining, &EncryptedContentInfo) || (EncryptedContentInfo.Tag != 0x30)) {
    return FALSE;
  }

  Cursor    = EncryptedContentInfo.Value;
  Remaining = EncryptedContentInfo.Length;

  if (!DerParseTlv (Cursor, Remaining, &EncContentTypeOid) || (EncContentTypeOid.Tag != 0x06)) {
    return FALSE;
  }

  Cursor    += EncContentTypeOid.Total;
  Remaining -= EncContentTypeOid.Total;

  //
  // AlgorithmIdentifier SEQUENCE { OID, parameters OPTIONAL }.
  //
  if (!DerParseTlv (Cursor, Remaining, &AlgorithmId) || (AlgorithmId.Tag != 0x30)) {
    return FALSE;
  }

  if (!DerParseTlv (AlgorithmId.Value, AlgorithmId.Length, &AlgorithmOid) || (AlgorithmOid.Tag != 0x06)) {
    return FALSE;
  }

  if (!DerOidEquals (
         AlgorithmId.Value,
         AlgorithmId.Length,
         ExpectedCipherOid,
         ExpectedCipherOidLen
         ))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Run an end-to-end encryption with a single recipient and validate the output.

  @param[in]  CipherNid             Symmetric cipher NID to request.
  @param[in]  ExpectedCipherOid     Reference DER-encoded OID.
  @param[in]  ExpectedCipherOidLen  Length of ExpectedCipherOid.

  @retval UNIT_TEST_PASSED            All assertions held.
  @retval UNIT_TEST_ERROR_TEST_FAILED Stack build, encrypt, or validation failed.
**/
STATIC
UNIT_TEST_STATUS
RunPkcs7EncryptSingleRecipient (
  IN UINT32       CipherNid,
  IN CONST UINT8  *ExpectedCipherOid,
  IN UINTN        ExpectedCipherOidLen
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (X509Stack);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CipherNid,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_NOT_EQUAL (ContentInfoSize, 0);

  Result = ValidatePkcs7EnvelopedData (
             ContentInfo,
             ContentInfoSize,
             ExpectedCipherOid,
             ExpectedCipherOidLen,
             1
             );

  FreePool (ContentInfo);

  UT_ASSERT_TRUE (Result);
  return UNIT_TEST_PASSED;
}

/**
  Happy-path test for AES-128-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptAes128Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7EncryptSingleRecipient (
           CRYPTO_NID_AES128CBC,
           mOidAes128Cbc,
           sizeof (mOidAes128Cbc)
           );
}

/**
  Happy-path test for AES-192-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptAes192Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7EncryptSingleRecipient (
           CRYPTO_NID_AES192CBC,
           mOidAes192Cbc,
           sizeof (mOidAes192Cbc)
           );
}

/**
  Happy-path test for AES-256-CBC content encryption.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptAes256Cbc (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  return RunPkcs7EncryptSingleRecipient (
           CRYPTO_NID_AES256CBC,
           mOidAes256Cbc,
           sizeof (mOidAes256Cbc)
           );
}

/**
  Build a stack with two recipients (the same certificate added twice) and
  verify the resulting envelopedData carries two RecipientInfo entries.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptMultipleRecipients (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (X509Stack);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_TRUE (Result);
  UT_ASSERT_NOT_NULL (ContentInfo);
  UT_ASSERT_NOT_EQUAL (ContentInfoSize, 0);

  Result = ValidatePkcs7EnvelopedData (
             ContentInfo,
             ContentInfoSize,
             mOidAes256Cbc,
             sizeof (mOidAes256Cbc),
             2
             );

  FreePool (ContentInfo);

  UT_ASSERT_TRUE (Result);
  return UNIT_TEST_PASSED;
}

/**
  NULL X509Stack must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullX509Stack (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  ContentInfo     = (UINT8 *)(UINTN)0xDEADBEEF;
  ContentInfoSize = 0xA5A5A5;

  Result = Pkcs7Encrypt (
             NULL,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (ContentInfo == NULL);
  UT_ASSERT_EQUAL (ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL InData must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullInData (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);

  Result = Pkcs7Encrypt (
             X509Stack,
             NULL,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (ContentInfo == NULL);
  UT_ASSERT_EQUAL (ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL ContentInfo output pointer must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullContentInfo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             NULL,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_EQUAL (ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  NULL ContentInfoSize output pointer must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptNullContentInfoSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  BOOLEAN  Result;

  X509Stack   = NULL;
  ContentInfo = NULL;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             NULL
             );

  X509StackFree (X509Stack);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (ContentInfo == NULL);
  return UNIT_TEST_PASSED;
}

/**
  An unsupported CipherNid (e.g. 0) must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptUnsupportedCipher (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             0,
             CRYPTO_PKCS7_DEFAULT,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (ContentInfo == NULL);
  UT_ASSERT_EQUAL (ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

/**
  Any Flags value other than CRYPTO_PKCS7_DEFAULT must be rejected.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestPkcs7EncryptUnsupportedFlags (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT8    *X509Stack;
  UINT8    *ContentInfo;
  UINTN    ContentInfoSize;
  BOOLEAN  Result;

  X509Stack       = NULL;
  ContentInfo     = NULL;
  ContentInfoSize = 0;

  Result = X509ConstructCertificateStack (
             &X509Stack,
             mPkcs7EncryptTestCert,
             (UINTN)sizeof (mPkcs7EncryptTestCert),
             NULL
             );
  UT_ASSERT_TRUE (Result);

  Result = Pkcs7Encrypt (
             X509Stack,
             (UINT8 *)mPkcs7EncryptPlaintext,
             sizeof (mPkcs7EncryptPlaintext),
             CRYPTO_NID_AES256CBC,
             0xDEADBEEF,
             &ContentInfo,
             &ContentInfoSize
             );

  X509StackFree (X509Stack);

  UT_ASSERT_FALSE (Result);
  UT_ASSERT_TRUE (ContentInfo == NULL);
  UT_ASSERT_EQUAL (ContentInfoSize, 0);
  return UNIT_TEST_PASSED;
}

//
// Test descriptor table consumed by BaseCryptLibUnitTests and
// BaseCryptLibUnitTestsMbedTls.
//
TEST_DESC  mPkcs7EncryptTest[] = {
  //
  // -----Description--------------------------------------Class-----------------------Function-----------------------------------PreReq-Post-Context
  //
  { "TestPkcs7EncryptAes128Cbc()",           "CryptoPkg.BaseCryptLib", TestPkcs7EncryptAes128Cbc,           NULL, NULL, NULL },
  { "TestPkcs7EncryptAes192Cbc()",           "CryptoPkg.BaseCryptLib", TestPkcs7EncryptAes192Cbc,           NULL, NULL, NULL },
  { "TestPkcs7EncryptAes256Cbc()",           "CryptoPkg.BaseCryptLib", TestPkcs7EncryptAes256Cbc,           NULL, NULL, NULL },
  { "TestPkcs7EncryptMultipleRecipients()",  "CryptoPkg.BaseCryptLib", TestPkcs7EncryptMultipleRecipients,  NULL, NULL, NULL },
  { "TestPkcs7EncryptNullX509Stack()",       "CryptoPkg.BaseCryptLib", TestPkcs7EncryptNullX509Stack,       NULL, NULL, NULL },
  { "TestPkcs7EncryptNullInData()",          "CryptoPkg.BaseCryptLib", TestPkcs7EncryptNullInData,          NULL, NULL, NULL },
  { "TestPkcs7EncryptNullContentInfo()",     "CryptoPkg.BaseCryptLib", TestPkcs7EncryptNullContentInfo,     NULL, NULL, NULL },
  { "TestPkcs7EncryptNullContentInfoSize()", "CryptoPkg.BaseCryptLib", TestPkcs7EncryptNullContentInfoSize, NULL, NULL, NULL },
  { "TestPkcs7EncryptUnsupportedCipher()",   "CryptoPkg.BaseCryptLib", TestPkcs7EncryptUnsupportedCipher,   NULL, NULL, NULL },
  { "TestPkcs7EncryptUnsupportedFlags()",    "CryptoPkg.BaseCryptLib", TestPkcs7EncryptUnsupportedFlags,    NULL, NULL, NULL },
};

UINTN  mPkcs7EncryptTestNum = ARRAY_SIZE (mPkcs7EncryptTest);
