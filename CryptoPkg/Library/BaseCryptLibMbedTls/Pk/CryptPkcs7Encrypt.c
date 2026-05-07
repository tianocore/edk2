/** @file
  PKCS7 Encryption implementation over mbedtls

  Copyright (c) 2026, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptPkcs7Internal.h"
#include <mbedtls/cipher.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>

#ifndef USE_OAEP
// Provide OAEP implementation for testing and potential future use.
// For consistency with other definitions of Pkcs7Encrypt, use PKCS#1 v1.5
// padding by default.
#define USE_OAEP  0
#endif

//
// id-RSAES-OAEP ::= { pkcs-1 7 }  (not defined by mbedtls headers)
//
#define OID_RSAES_OAEP  MBEDTLS_OID_PKCS1 "\x07"

#define PKCS7_MAX_RECIPIENT_COUNT  256

/**
  Write a KeyTransRecipientInfo structure for one certificate.

  https://datatracker.ietf.org/doc/html/rfc5652#section-6.2

      KeyTransRecipientInfo ::= SEQUENCE {
        version CMSVersion,  -- always set to 0 or 2
        rid RecipientIdentifier,
        keyEncryptionAlgorithm KeyEncryptionAlgorithmIdentifier,
        encryptedKey EncryptedKey }

  @param[in,out]  Ptr             Current write position pointer (writes backwards).
  @param[in]      Start           Start of the buffer.
  @param[in]      Cert            Recipient X.509 certificate.
  @param[in]      EncryptedKey    RSA-encrypted content-encryption key.
  @param[in]      EncryptedKeyLen Length of EncryptedKey.

  @retval >0      Number of bytes written.
  @retval <0      An MBEDTLS_ERR_ASN1_XXX error.
**/
STATIC
INT32
WriteEnvelopedDataKeyTransRecipientInfo (
  UINT8             **Ptr,
  UINT8             *Start,
  mbedtls_x509_crt  *Cert,
  UINT8             *EncryptedKey,
  UINTN             EncryptedKeyLen
  )
{
  INT32   Ret; // Referenced by EDKII_ASN1_CHK_ADD macro.
  UINT32  DerLen;
  UINT32  IssuerSerialLen;

  DerLen = 0;

  //
  // KeyTransRecipientInfo-encryptedKey OCTET STRING = EncryptedKey
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_octet_string (Ptr, Start, EncryptedKey, EncryptedKeyLen)
    );

 #if USE_OAEP

  //
  // KeyTransRecipientInfo-keyEncryptionAlgorithm AlgorithmIdentifier = RSAES-OAEP with SHA-256
  //
  // RSAES-OAEP-params ::= SEQUENCE {
  //   hashAlgorithm      [0] AlgorithmIdentifier { id-sha256, NULL }
  //   maskGenAlgorithm   [1] AlgorithmIdentifier { id-mgf1,
  //                              AlgorithmIdentifier { id-sha256, NULL } }
  // }
  //
  {
    UINT32  OaepParamsLen;
    UINT32  TmpLen;

    OaepParamsLen = 0;

    //
    // Write [1] EXPLICIT maskGenAlgorithm = { id-mgf1, { id-sha256, NULL } }
    //
    TmpLen = 0;

    // Write inner AlgorithmIdentifier { id-sha256, NULL }
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_null (Ptr, Start));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_oid (
        Ptr,
        Start,
        MBEDTLS_OID_DIGEST_ALG_SHA256,
        MBEDTLS_OID_SIZE (MBEDTLS_OID_DIGEST_ALG_SHA256)
        )
      );
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_len (Ptr, Start, TmpLen));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
      );

    // Write id-mgf1 OID before the inner AlgorithmIdentifier
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_oid (
        Ptr,
        Start,
        MBEDTLS_OID_MGF1,
        MBEDTLS_OID_SIZE (MBEDTLS_OID_MGF1)
        )
      );

    // Wrap MGF1 AlgorithmIdentifier in SEQUENCE
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_len (Ptr, Start, TmpLen));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
      );

    // Wrap in [1] EXPLICIT
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_len (Ptr, Start, TmpLen));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_tag (
        Ptr,
        Start,
        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1
        )
      );
    OaepParamsLen += TmpLen;

    //
    // Write [0] EXPLICIT hashAlgorithm = { id-sha256, NULL }
    //
    TmpLen = 0;
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_null (Ptr, Start));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_oid (
        Ptr,
        Start,
        MBEDTLS_OID_DIGEST_ALG_SHA256,
        MBEDTLS_OID_SIZE (MBEDTLS_OID_DIGEST_ALG_SHA256)
        )
      );
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_len (Ptr, Start, TmpLen));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
      );

    // Wrap in [0] EXPLICIT
    EDKII_ASN1_CHK_ADD (TmpLen, mbedtls_asn1_write_len (Ptr, Start, TmpLen));
    EDKII_ASN1_CHK_ADD (
      TmpLen,
      mbedtls_asn1_write_tag (
        Ptr,
        Start,
        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0
        )
      );
    OaepParamsLen += TmpLen;

    // Wrap RSAES-OAEP-params in SEQUENCE
    EDKII_ASN1_CHK_ADD (
      OaepParamsLen,
      mbedtls_asn1_write_len (Ptr, Start, OaepParamsLen)
      );
    EDKII_ASN1_CHK_ADD (
      OaepParamsLen,
      mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
      );

    // Write outer AlgorithmIdentifier { id-RSAES-OAEP, RSAES-OAEP-params }
    EDKII_ASN1_CHK_ADD (
      DerLen,
      mbedtls_asn1_write_algorithm_identifier (
        Ptr,
        Start,
        OID_RSAES_OAEP,
        MBEDTLS_OID_SIZE (OID_RSAES_OAEP),
        OaepParamsLen
        )
      );
  }

 #else // USE_OAEP

  //
  // KeyTransRecipientInfo-keyEncryptionAlgorithm AlgorithmIdentifier = rsaEncryption with NULL params
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_algorithm_identifier (
      Ptr,
      Start,
      MBEDTLS_OID_PKCS1_RSA,
      MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS1_RSA),
      0
      )
    );

 #endif // USE_OAEP

  //
  // KeyTransRecipientInfo-rid RecipientIdentifier = IssuerAndSerialNumber
  //
  // IssuerAndSerialNumber ::= SEQUENCE {
  //   issuer Name,
  //   serialNumber CertificateSerialNumber }
  //
  IssuerSerialLen = 0;

  // KeyTransRecipientInfo-rid-serialNumber INTEGER = (raw serial bytes with INTEGER tag)
  {
    UINT8 const  *SerialEnd;
    UINT8 const  *SerialStart;

    SerialEnd   = Cert->serial.p + Cert->serial.len;
    SerialStart = Cert->serial.p;

    // Write serial value bytes backwards

    if ((size_t)(*Ptr - Start) < Cert->serial.len) {
      return MBEDTLS_ERR_ASN1_BUF_TOO_SMALL;
    }

    while (SerialEnd > SerialStart) {
      *--(*Ptr) = *--SerialEnd;
      IssuerSerialLen++;
    }

    EDKII_ASN1_CHK_ADD (
      IssuerSerialLen,
      mbedtls_asn1_write_len (Ptr, Start, IssuerSerialLen)
      );
    EDKII_ASN1_CHK_ADD (
      IssuerSerialLen,
      mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_INTEGER)
      );
  }

  // KeyTransRecipientInfo-rid-issuer Name
  EDKII_ASN1_CHK_ADD (
    IssuerSerialLen,
    mbedtls_asn1_write_raw_buffer (Ptr, Start, Cert->issuer_raw.p, Cert->issuer_raw.len)
    );

  // KeyTransRecipientInfo-rid-SEQUENCE
  EDKII_ASN1_CHK_ADD (
    IssuerSerialLen,
    mbedtls_asn1_write_len (Ptr, Start, IssuerSerialLen)
    );
  EDKII_ASN1_CHK_ADD (
    IssuerSerialLen,
    mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
    );

  DerLen += IssuerSerialLen;

  //
  // KeyTransRecipientInfo-version INTEGER = 0
  //
  EDKII_ASN1_CHK_ADD (DerLen, mbedtls_asn1_write_int (Ptr, Start, 0));

  //
  // KeyTransRecipientInfo-SEQUENCE
  //
  EDKII_ASN1_CHK_ADD (DerLen, mbedtls_asn1_write_len (Ptr, Start, DerLen));
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
    );

  return DerLen;
}

/**
  Write the EncryptedContentInfo structure.

  EncryptedContentInfo ::= SEQUENCE {
    contentType                 OBJECT IDENTIFIER (id-data),
    contentEncryptionAlgorithm  AlgorithmIdentifier,
    encryptedContent            [0] IMPLICIT OCTET STRING
  }

  @param[in,out]  Ptr              Current write position pointer (writes backwards).
  @param[in]      Start            Start of the buffer.
  @param[in]      CipherOid        OID of the content-encryption algorithm.
  @param[in]      CipherOidLen     Length of CipherOid.
  @param[in]      Iv               Initialization vector.
  @param[in]      IvLen            Length of IV.
  @param[in]      EncryptedData    Encrypted content.
  @param[in]      EncryptedDataLen Length of EncryptedData.

  @retval >0      Number of bytes written.
  @retval <0      An MBEDTLS_ERR_ASN1_XXX error.
**/
STATIC
INT32
WriteEnvelopedDataEncryptedContentInfo (
  UINT8        **Ptr,
  UINT8        *Start,
  CONST CHAR8  *CipherOid,
  UINTN        CipherOidLen,
  UINT8        *Iv,
  UINTN        IvLen,
  UINT8        *EncryptedData,
  UINTN        EncryptedDataLen
  )
{
  INT32   Ret; // Referenced by EDKII_ASN1_CHK_ADD macro.
  UINT32  DerLen;
  UINT32  AlgIdParamLen;
  UINT32  AlgIdLen;

  DerLen = 0;

  //
  // Write encryptedContent [0] IMPLICIT OCTET STRING
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_raw_buffer (Ptr, Start, EncryptedData, EncryptedDataLen)
    );
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_len (Ptr, Start, EncryptedDataLen)
    );
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0)
    );

  //
  // Write contentEncryptionAlgorithm AlgorithmIdentifier
  // The parameters for AES-CBC is the IV as an OCTET STRING.
  //
  AlgIdParamLen = 0;
  EDKII_ASN1_CHK_ADD (
    AlgIdParamLen,
    mbedtls_asn1_write_octet_string (Ptr, Start, Iv, IvLen)
    );

  AlgIdLen = 0;
  // The params are already written, now write OID before them
  EDKII_ASN1_CHK_ADD (
    AlgIdLen,
    mbedtls_asn1_write_oid (Ptr, Start, CipherOid, CipherOidLen)
    );
  AlgIdLen += AlgIdParamLen;

  // Wrap AlgorithmIdentifier in SEQUENCE
  EDKII_ASN1_CHK_ADD (
    AlgIdLen,
    mbedtls_asn1_write_len (Ptr, Start, AlgIdLen)
    );
  EDKII_ASN1_CHK_ADD (
    AlgIdLen,
    mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
    );
  DerLen += AlgIdLen;

  //
  // Write contentType OID (id-data)
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_oid (
      Ptr,
      Start,
      MBEDTLS_OID_PKCS7_DATA,
      MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS7_DATA)
      )
    );

  //
  // Wrap EncryptedContentInfo in SEQUENCE
  //
  EDKII_ASN1_CHK_ADD (DerLen, mbedtls_asn1_write_len (Ptr, Start, DerLen));
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (Ptr, Start, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)
    );

  return DerLen;
}

/**
  Write the recipientInfos SET OF RecipientInfo.

  Writes pre-encoded, sorted blobs in reverse order: since the DER encoder
  writes backwards (right to left), writing highest-sorted blob first
  produces ascending DER order in the final output.

  @param[in,out]  Ptr                 Current write position pointer (writes backwards).
  @param[in]      Start               Start of the buffer.
  @param[in]      RecipientInfoBlobs   Array of pre-encoded RecipientInfo DER blobs.
  @param[in]      RecipientInfoLens    Array of lengths for each blob.
  @param[in]      RecipientCount       Number of recipients.

  @retval >0      Number of bytes written.
  @retval <0      An MBEDTLS_ERR_ASN1_XXX error.
**/
STATIC
INT32
WriteEnvelopedDataRecipientInfos (
  UINT8  **Ptr,
  UINT8  *Start,
  UINT8  **RecipientInfoBlobs,
  UINTN  *RecipientInfoLens,
  UINTN  RecipientCount
  )
{
  INT32   Ret; // Referenced by EDKII_ASN1_CHK_ADD macro.
  UINT32  RecipientInfosLen;
  INTN    Idx;

  RecipientInfosLen = 0;
  for (Idx = (INTN)RecipientCount - 1; Idx >= 0; Idx--) {
    EDKII_ASN1_CHK_ADD (
      RecipientInfosLen,
      mbedtls_asn1_write_raw_buffer (
        Ptr,
        Start,
        RecipientInfoBlobs[Idx],
        RecipientInfoLens[Idx]
        )
      );
  }

  // recipientInfos-SET
  EDKII_ASN1_CHK_ADD (
    RecipientInfosLen,
    mbedtls_asn1_write_len (Ptr, Start, RecipientInfosLen)
    );
  EDKII_ASN1_CHK_ADD (
    RecipientInfosLen,
    mbedtls_asn1_write_tag (
      Ptr,
      Start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET
      )
    );

  return RecipientInfosLen;
}

/**
  Write the EnvelopedData header: version and SEQUENCE wrapper.

  EnvelopedData ::= SEQUENCE {
    version CMSVersion,
    originatorInfo [0] IMPLICIT OriginatorInfo OPTIONAL,
    recipientInfos RecipientInfos,
    encryptedContentInfo EncryptedContentInfo,
    unprotectedAttrs [1] IMPLICIT UnprotectedAttributes OPTIONAL }

  The body content (recipientInfos + encryptedContentInfo) must already
  be written.  This function prepends the version INTEGER and wraps
  everything in a SEQUENCE.

  @param[in,out]  Ptr       Current write position pointer (writes backwards).
  @param[in]      Start     Start of the buffer.
  @param[in]      BodyLen   Length of the body content already written.

  @retval >0      Number of bytes written.
  @retval <0      An MBEDTLS_ERR_ASN1_XXX error.
**/
STATIC
INT32
WriteEnvelopedDataHeader (
  UINT8   **Ptr,
  UINT8   *Start,
  UINT32  BodyLen
  )
{
  INT32   Ret; // Referenced by EDKII_ASN1_CHK_ADD macro.
  UINT32  DerLen;

  DerLen = BodyLen;

  //
  // EnvelopedData-originatorInfo IMPLICIT OPTIONAL = None.
  //

  //
  // EnvelopedData-version INTEGER = 0
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_int (Ptr, Start, 0)
    );

  //
  // EnvelopedData-SEQUENCE
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_len (Ptr, Start, DerLen)
    );
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (
      Ptr,
      Start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE
      )
    );

  return DerLen - BodyLen;
}

/**
  Write the ContentInfo header wrapping an EnvelopedData payload.

  ContentInfo ::= SEQUENCE {
    contentType ContentType,          -- id-envelopedData
    content [0] EXPLICIT ANY DEFINED BY contentType }

  The EnvelopedData body must already be written.  This function
  prepends the [0] EXPLICIT wrapper, the contentType OID, and the
  outer SEQUENCE.

  @param[in,out]  Ptr       Current write position pointer (writes backwards).
  @param[in]      Start     Start of the buffer.
  @param[in]      BodyLen   Length of the EnvelopedData already written.

  @retval >0      Number of bytes written.
  @retval <0      An MBEDTLS_ERR_ASN1_XXX error.
**/
STATIC
INT32
WriteEnvelopedDataContentInfoHeader (
  UINT8   **Ptr,
  UINT8   *Start,
  UINT32  BodyLen
  )
{
  INT32   Ret; // Referenced by EDKII_ASN1_CHK_ADD macro.
  UINT32  DerLen;

  DerLen = BodyLen;

  //
  // ContentInfo-content [0] EXPLICIT EnvelopedData
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_len (Ptr, Start, DerLen)
    );
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (
      Ptr,
      Start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0
      )
    );

  //
  // ContentInfo-contentType ContentType = OID (id-envelopedData)
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_oid (
      Ptr,
      Start,
      MBEDTLS_OID_PKCS7_ENVELOPED_DATA,
      MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS7_ENVELOPED_DATA)
      )
    );

  //
  // ContentInfo-SEQUENCE
  //
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_len (Ptr, Start, DerLen)
    );
  EDKII_ASN1_CHK_ADD (
    DerLen,
    mbedtls_asn1_write_tag (
      Ptr,
      Start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE
      )
    );

  return DerLen - BodyLen;
}

/**
  Creates a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  that wraps content encrypted for secure transmission to one or more recipients.

  If this interface is not supported, return FALSE.

  @param[in]  X509Stack        Pointer to a stack of X.509 certificates for the
                               intended recipients of this message, created using
                               X509ConstructCertificateStack or similar. Each
                               certificate must provide an RSA public key. Any of the
                               corresponding private keys will be able to decrypt the
                               content of the returned ContentInfo.
  @param[in]  InData           Pointer to the content to be encrypted.
  @param[in]  InDataSize       Size of the content to be encrypted in bytes.
  @param[in]  CipherNid        NID of the symmetric cipher to use for encryption.
                               Supported values are CRYPTO_NID_AES128CBC,
                               CRYPTO_NID_AES192CBC, and CRYPTO_NID_AES256CBC.
  @param[in]  Flags            Flags for the encryption operation. Currently only
                               CRYPTO_PKCS7_DEFAULT is supported, which indicates that
                               the input data is treated as binary data.
  @param[out] ContentInfo      Receives a pointer to the output, which is a PKCS#7
                               DER-encoded ContentInfo that wraps an envelopedData. The
                               caller must free the returned buffer with FreePool().
  @param[out] ContentInfoSize  Receives the size of the output in bytes.

  @retval     TRUE             PKCS#7 data encryption succeeded.
  @retval     FALSE            PKCS#7 data encryption failed.
  @retval     FALSE            This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Encrypt (
  IN   UINT8   *X509Stack,
  IN   UINT8   *InData,
  IN   UINTN   InDataSize,
  IN   UINT32  CipherNid,
  IN   UINT32  Flags,
  OUT  UINT8   **ContentInfo,
  OUT  UINTN   *ContentInfoSize
  )
{
  BOOLEAN                      Succeeded;
  INT32                        MbedResult;
  mbedtls_cipher_type_t        CipherType;
  CHAR8 const                  *CipherOid;
  UINTN                        CipherOidLen;
  mbedtls_cipher_info_t const  *CipherInfo;
  mbedtls_cipher_context_t     CipherCtx;
  UINTN                        ContentEncKeyLen;
  UINT8                        ContentEncKey[32]; // 32 = max key length for AES-256
  UINTN                        IvLen;
  UINT8                        Iv[AES_BLOCK_SIZE];
  UINTN                        EncryptedDataLen;
  UINT8                        *EncryptedData;
  UINTN                        DerBufSize;
  UINT8                        *DerBuf;
  UINT8                        *DerWritePtr;
  UINT32                       DerLen;
  UINTN                        RecipientCount;
  UINT8                        **RecipientInfoBlobs;
  UINTN                        *RecipientInfoLens;

  Succeeded          = FALSE;
  EncryptedData      = NULL;
  DerBuf             = NULL;
  RecipientCount     = 0;
  RecipientInfoBlobs = NULL;
  RecipientInfoLens  = NULL;

  mbedtls_cipher_init (&CipherCtx);

  if (ContentInfo != NULL) {
    *ContentInfo = NULL;
  }

  if (ContentInfoSize != NULL) {
    *ContentInfoSize = 0;
  }

  //
  // Validate parameters.
  //

  if ((X509Stack == NULL) ||
      (InData == NULL) ||
      (InDataSize > INT_MAX) ||
      (Flags != CRYPTO_PKCS7_DEFAULT) ||
      (ContentInfo == NULL) ||
      (ContentInfoSize == NULL))
  {
    goto Cleanup;
  }

  //
  // Validate recipients early: enforce RSA-only certs and a hard cap.
  //
  {
    mbedtls_x509_crt  *Cert;

    for (Cert = (mbedtls_x509_crt *)X509Stack; Cert != NULL; Cert = Cert->next) {
      if (mbedtls_pk_rsa (Cert->pk) == NULL) {
        // We currently only support RSA public keys.
        goto Cleanup;
      }

      if (RecipientCount >= PKCS7_MAX_RECIPIENT_COUNT) {
        // Hard cap - avoid infinite loops due to corrupted linked-list.
        goto Cleanup;
      }

      RecipientCount += 1;
    }
  }

  if (RecipientCount == 0) {
    goto Cleanup;
  }

  //
  // Map NID to mbedtls cipher type and OID.
  //

  switch (CipherNid) {
    case CRYPTO_NID_AES128CBC:
      CipherType   = MBEDTLS_CIPHER_AES_128_CBC;
      CipherOid    = MBEDTLS_OID_AES_128_CBC;
      CipherOidLen = MBEDTLS_OID_SIZE (MBEDTLS_OID_AES_128_CBC);
      break;
    case CRYPTO_NID_AES192CBC:
      CipherType   = MBEDTLS_CIPHER_AES_192_CBC;
      CipherOid    = MBEDTLS_OID_AES_192_CBC;
      CipherOidLen = MBEDTLS_OID_SIZE (MBEDTLS_OID_AES_192_CBC);
      break;
    case CRYPTO_NID_AES256CBC:
      CipherType   = MBEDTLS_CIPHER_AES_256_CBC;
      CipherOid    = MBEDTLS_OID_AES_256_CBC;
      CipherOidLen = MBEDTLS_OID_SIZE (MBEDTLS_OID_AES_256_CBC);
      break;
    default:
      goto Cleanup;
  }

  //
  // Look up cipher info and determine key/IV sizes.
  //

  CipherInfo = mbedtls_cipher_info_from_type (CipherType);
  if (CipherInfo == NULL) {
    goto Cleanup;
  }

  MbedResult = mbedtls_cipher_setup (&CipherCtx, CipherInfo);
  if (MbedResult != 0) {
    goto Cleanup;
  }

  ContentEncKeyLen = (unsigned)mbedtls_cipher_get_key_bitlen (&CipherCtx) / 8;
  if ((ContentEncKeyLen == 0) || (ContentEncKeyLen > sizeof (ContentEncKey))) {
    goto Cleanup;
  }

  IvLen = (unsigned)mbedtls_cipher_get_iv_size (&CipherCtx);
  if ((IvLen == 0) || (IvLen > sizeof (Iv))) {
    goto Cleanup;
  }

  //
  // Generate random content-encryption key and IV.
  //

  if (!RandomBytes (ContentEncKey, ContentEncKeyLen)) {
    goto Cleanup;
  }

  if (!RandomBytes (Iv, IvLen)) {
    goto Cleanup;
  }

  //
  // Encrypt the content with the content-encryption key using AES-CBC.
  //

  MbedResult = mbedtls_cipher_setkey (
                 &CipherCtx,
                 ContentEncKey,
                 (int)(ContentEncKeyLen * 8),
                 MBEDTLS_ENCRYPT
                 );
  if (MbedResult != 0) {
    goto Cleanup;
  }

  MbedResult = mbedtls_cipher_set_padding_mode (&CipherCtx, MBEDTLS_PADDING_PKCS7);
  if (MbedResult != 0) {
    goto Cleanup;
  }

  // Output buffer: input size + one full block for padding
  {
    UINTN  BlockSize;

    BlockSize = mbedtls_cipher_get_block_size (&CipherCtx);
    if ((BlockSize == 0) || (InDataSize > MAX_UINTN - BlockSize)) {
      goto Cleanup;
    }

    EncryptedDataLen = InDataSize + BlockSize;
  }

  EncryptedData = AllocatePool (EncryptedDataLen);
  if (EncryptedData == NULL) {
    goto Cleanup;
  }

  {
    size_t  Olen;

    Olen       = 0;
    MbedResult = mbedtls_cipher_crypt (
                   &CipherCtx,
                   Iv,
                   IvLen,
                   InData,
                   InDataSize,
                   EncryptedData,
                   &Olen
                   );
    if (MbedResult != 0) {
      goto Cleanup;
    }

    if (Olen > EncryptedDataLen) {
      ASSERT (FALSE); // Should never happen.
      goto Cleanup;
    }

    EncryptedDataLen = Olen;
  }

  //
  // Pre-encode each RecipientInfo into its own buffer so they can be sorted
  // in DER canonical order before being written to the output SET OF.
  //
  // RFC 5652 Section 6.1: recipientInfos is a SET OF RecipientInfo.
  // DER (X.690) requires SET OF elements to be sorted in ascending
  // lexicographic order of their complete DER encodings.

  RecipientInfoBlobs = AllocateZeroPool (RecipientCount * sizeof (UINT8 *));
  RecipientInfoLens  = AllocateZeroPool (RecipientCount * sizeof (UINTN));
  if ((RecipientInfoBlobs == NULL) || (RecipientInfoLens == NULL)) {
    goto Cleanup;
  }

  {
    UINTN             Idx;
    mbedtls_x509_crt  *Cert;

    Idx = 0;
    for (Cert = (mbedtls_x509_crt *)X509Stack; Cert != NULL; Cert = Cert->next, Idx++) {
      mbedtls_rsa_context  *Rsa;
      UINTN                EncKeyBufSize;

      Rsa = mbedtls_pk_rsa (Cert->pk);
      if (Rsa == NULL) {
        ASSERT (FALSE); // Checked earlier.
        goto Cleanup;
      }

      EncKeyBufSize = mbedtls_pk_get_len (&Cert->pk);
      if (EncKeyBufSize == 0) {
        goto Cleanup;
      }

      UINT8   *EncKeyBuf;
      size_t  EncKeyLen;
      UINTN   TmpBufSize;
      UINT8   *TmpBuf;
      UINT8   *TmpPtr;

      EncKeyBuf = AllocatePool (EncKeyBufSize);
      if (EncKeyBuf == NULL) {
        goto Cleanup;
      }

      EncKeyLen = 0;

 #if USE_OAEP

      MbedResult = mbedtls_rsa_set_padding (Rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
      if (MbedResult != 0) {
        FreePool (EncKeyBuf);
        goto Cleanup;
      }

      MbedResult = mbedtls_rsa_rsaes_oaep_encrypt (
                     Rsa,
                     MbedtlsRand,
                     NULL,
                     NULL,
                     0,
                     ContentEncKeyLen,
                     ContentEncKey,
                     EncKeyBuf
                     );
      EncKeyLen = EncKeyBufSize;

 #else // USE_OAEP

      MbedResult = mbedtls_pk_encrypt (
                     &Cert->pk,
                     ContentEncKey,
                     ContentEncKeyLen,
                     EncKeyBuf,
                     &EncKeyLen,
                     EncKeyBufSize,
                     MbedtlsRand,
                     NULL
                     );

 #endif // USE_OAEP

      if (MbedResult != 0) {
        FreePool (EncKeyBuf);
        goto Cleanup;
      }

      // Allocate a temporary buffer sized generously for this RecipientInfo.

      if ((Cert->issuer_raw.len > MAX_UINTN - Cert->serial.len) ||
          (Cert->issuer_raw.len + Cert->serial.len > MAX_UINTN - EncKeyBufSize) ||
          (Cert->issuer_raw.len + Cert->serial.len + EncKeyBufSize > MAX_UINTN - 256))
      {
        FreePool (EncKeyBuf);
        goto Cleanup;
      }

      TmpBufSize = Cert->issuer_raw.len + Cert->serial.len + EncKeyBufSize + 256;
      TmpBuf     = AllocatePool (TmpBufSize);
      if (TmpBuf == NULL) {
        FreePool (EncKeyBuf);
        goto Cleanup;
      }

      TmpPtr     = TmpBuf + TmpBufSize;
      MbedResult = WriteEnvelopedDataKeyTransRecipientInfo (&TmpPtr, TmpBuf, Cert, EncKeyBuf, EncKeyLen);
      FreePool (EncKeyBuf);

      if (MbedResult < 0) {
        FreePool (TmpBuf);
        goto Cleanup;
      }

      // TmpPtr points to the start of the encoded blob at the end of TmpBuf.
      // Copy it to a compact allocation for sorting and later use.
      RecipientInfoLens[Idx]  = (unsigned)MbedResult;
      RecipientInfoBlobs[Idx] = AllocateCopyPool ((unsigned)MbedResult, TmpPtr);
      FreePool (TmpBuf);
      if (RecipientInfoBlobs[Idx] == NULL) {
        goto Cleanup;
      }
    }
  }

  //
  // Sort RecipientInfoBlobs in ascending DER lexicographic order using
  // insertion sort (recipient count is typically small).
  //
  {
    UINTN  Outer;

    Outer = 1;
    for ( ; Outer < RecipientCount; Outer++) {
      UINT8  *OuterBlob;
      UINTN  OuterLen;
      INTN   Inner;

      OuterBlob = RecipientInfoBlobs[Outer];
      OuterLen  = RecipientInfoLens[Outer];
      Inner     = (INTN)Outer - 1;
      while (Inner >= 0) {
        UINTN  InnerLen;
        UINTN  MinLen;
        INTN   Cmp;

        InnerLen = RecipientInfoLens[Inner];
        MinLen   = MIN (OuterLen, InnerLen);
        Cmp      = CompareMem (OuterBlob, RecipientInfoBlobs[Inner], MinLen);
        if (Cmp == 0) {
          if (OuterLen < InnerLen) {
            Cmp = -1;
          } else if (OuterLen > InnerLen) {
            Cmp = 1;
          }
        }

        if (Cmp >= 0) {
          break;
        }

        RecipientInfoBlobs[Inner + 1] = RecipientInfoBlobs[Inner];
        RecipientInfoLens[Inner + 1]  = RecipientInfoLens[Inner];
        Inner--;
      }

      RecipientInfoBlobs[Inner + 1] = OuterBlob;
      RecipientInfoLens[Inner + 1]  = OuterLen;
    }
  }

  //
  // Allocate a working buffer for DER encoding (written backwards).
  // Size: encrypted data + exact pre-encoded RecipientInfo sizes + ASN.1 overhead.
  //
  if (EncryptedDataLen > MAX_UINTN - 4096) {
    goto Cleanup;
  }

  DerBufSize = EncryptedDataLen + 4096;
  {
    UINTN  Idx;

    Idx = 0;
    for ( ; Idx < RecipientCount; Idx++) {
      if (RecipientInfoLens[Idx] > MAX_UINTN - DerBufSize) {
        goto Cleanup;
      }

      DerBufSize += RecipientInfoLens[Idx];
    }
  }

  DerBuf = AllocatePool (DerBufSize);
  if (DerBuf == NULL) {
    goto Cleanup;
  }

  DerWritePtr = DerBuf + DerBufSize;
  DerLen      = 0;

  //
  // Overall PKCS7 structure root is ContentInfo.
  //
  // ContentInfo ::= SEQUENCE {
  //   contentType ContentType,
  //   content [0] EXPLICIT ANY DEFINED BY contentType }
  //
  // content is EnvelopedData.
  //
  // EnvelopedData ::= SEQUENCE {
  //   version CMSVersion,
  //   originatorInfo [0] IMPLICIT OriginatorInfo OPTIONAL,
  //   recipientInfos RecipientInfos,
  //   encryptedContentInfo EncryptedContentInfo,
  //   unprotectedAttrs [1] IMPLICIT UnprotectedAttributes OPTIONAL }
  //

  //
  // EnvelopedData-unprotectedAttrs IMPLICIT OPTIONAL = None.
  //

  //
  // EnvelopedData-encryptedContentInfo
  //
  MbedResult = WriteEnvelopedDataEncryptedContentInfo (
                 &DerWritePtr,
                 DerBuf,
                 CipherOid,
                 CipherOidLen,
                 Iv,
                 IvLen,
                 EncryptedData,
                 EncryptedDataLen
                 );
  if (MbedResult < 0) {
    goto Cleanup;
  }

  DerLen += (UINT32)MbedResult;

  //
  // EnvelopedData-recipientInfos SET OF RecipientInfo
  //
  MbedResult = WriteEnvelopedDataRecipientInfos (
                 &DerWritePtr,
                 DerBuf,
                 RecipientInfoBlobs,
                 RecipientInfoLens,
                 RecipientCount
                 );
  if (MbedResult < 0) {
    goto Cleanup;
  }

  DerLen += (UINT32)MbedResult;

  //
  // EnvelopedData header (version + SEQUENCE)
  //
  MbedResult = WriteEnvelopedDataHeader (&DerWritePtr, DerBuf, DerLen);
  if (MbedResult < 0) {
    goto Cleanup;
  }

  DerLen += (UINT32)MbedResult;

  //
  // ContentInfo header (content [0] EXPLICIT + OID + SEQUENCE)
  //
  MbedResult = WriteEnvelopedDataContentInfoHeader (&DerWritePtr, DerBuf, DerLen);
  if (MbedResult < 0) {
    goto Cleanup;
  }

  DerLen += (UINT32)MbedResult;

  //
  // Copy the result out.
  //
  *ContentInfo = AllocatePool (DerLen);
  if (*ContentInfo == NULL) {
    goto Cleanup;
  }

  CopyMem (*ContentInfo, DerWritePtr, DerLen);
  *ContentInfoSize = DerLen;
  Succeeded        = TRUE;

Cleanup:

  ZeroMem (ContentEncKey, sizeof (ContentEncKey));
  ZeroMem (Iv, sizeof (Iv));

  if (RecipientInfoBlobs != NULL) {
    UINTN  Idx;

    Idx = 0;
    for ( ; Idx < RecipientCount; Idx++) {
      if (RecipientInfoBlobs[Idx] != NULL) {
        FreePool (RecipientInfoBlobs[Idx]);
      }
    }

    FreePool (RecipientInfoBlobs);
  }

  if (RecipientInfoLens != NULL) {
    FreePool (RecipientInfoLens);
  }

  if (EncryptedData != NULL) {
    FreePool (EncryptedData);
  }

  if (DerBuf != NULL) {
    FreePool (DerBuf);
  }

  mbedtls_cipher_free (&CipherCtx);

  return Succeeded;
}
