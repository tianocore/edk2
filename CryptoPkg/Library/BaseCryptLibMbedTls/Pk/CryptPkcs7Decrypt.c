/** @file
  PKCS7 Decryption implementation over mbedtls

  Mbed-TLS does not provide a PKCS#7 envelopedData decrypt API (its pkcs7 module
  only handles signedData), so, like the matching Pkcs7Encrypt, this file parses
  the ASN.1/DER by hand. It is the parsing mirror of CryptPkcs7Encrypt.c and is
  able to decrypt envelopes produced by that writer as well as standards-conformant
  envelopes produced by other implementations (e.g. OpenSSL), covering both
  RSAES-PKCS1-v1.5 and RSAES-OAEP (with any digest the mbedtls configuration
  supports, where the label digest and MGF1 use the same hash) key transport and
  AES-CBC content encryption.

  Copyright (c) 2026, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CryptPkcs7Internal.h"
#include <mbedtls/cipher.h>
#include <mbedtls/md.h>
#include <mbedtls/rsa.h>

#include <Library/MemoryAllocationLib.h>

//
// Largest supported content-encryption key (AES-256).
//
#define MAX_CONTENT_ENC_KEY_LEN  32

/**
  Read an OBJECT IDENTIFIER at *P into an ASN.1 buffer, advancing *P past it.

  @param[in,out]  P    Current read position.
  @param[in]      End  End of the buffer.
  @param[out]     Oid  Receives the OID contents.

  @retval TRUE   Success.
  @retval FALSE  The element at *P is not a well-formed OBJECT IDENTIFIER.
**/
STATIC
BOOLEAN
Pkcs7Asn1GetOid (
  UINT8             **P,
  CONST UINT8       *End,
  mbedtls_asn1_buf  *Oid
  )
{
  size_t  Len;

  if (mbedtls_asn1_get_tag (P, End, &Len, MBEDTLS_ASN1_OID) != 0) {
    return FALSE;
  }

  Oid->tag = MBEDTLS_ASN1_OID;
  Oid->len = Len;
  Oid->p   = *P;
  *P      += Len;
  return TRUE;
}

/**
  Compare an OID against a reference OID.

  This is the equivalent of the MBEDTLS_OID_CMP macro, expressed with CompareMem
  so that this file does not depend on the C library memcmp.

  @param[in]  Oid     OID to test.
  @param[in]  Ref     Reference OID contents.
  @param[in]  RefLen  Length of Ref.

  @retval TRUE   The OIDs match.
  @retval FALSE  The OIDs differ.
**/
STATIC
BOOLEAN
Pkcs7OidEquals (
  CONST mbedtls_asn1_buf  *Oid,
  CONST CHAR8             *Ref,
  UINTN                   RefLen
  )
{
  return (BOOLEAN)((Oid->len == RefLen) && (CompareMem (Oid->p, Ref, RefLen) == 0));
}

/**
  Read a DigestAlgorithmIdentifier SEQUENCE { OID, optional NULL parameters } at *P
  and map its OID to an mbedtls message-digest type, advancing *P past the SEQUENCE.

  @param[in,out]  P       Current read position.
  @param[in]      End     End of the buffer.
  @param[out]     MdType  Receives the mbedtls digest type.

  @retval TRUE   Parsed successfully and the digest is supported.
  @retval FALSE  Parse error or unsupported digest.
**/
STATIC
BOOLEAN
Pkcs7ReadDigestAlgId (
  UINT8              **P,
  CONST UINT8        *End,
  mbedtls_md_type_t  *MdType
  )
{
  mbedtls_asn1_buf  Alg;

  if (mbedtls_asn1_get_alg_null (P, End, &Alg) != 0) {
    return FALSE;
  }

  return (BOOLEAN)(mbedtls_oid_get_md_alg (&Alg, MdType) == 0);
}

/**
  Parse RSAES-OAEP-params to determine the hash used for the OAEP label digest and
  for MGF1. Absent or DEFAULT fields imply SHA-1. Because mbedtls configures a single
  hash for both the OAEP digest and MGF1, this fails if the two differ.

      RSAES-OAEP-params ::= SEQUENCE {
        hashAlgorithm    [0] AlgorithmIdentifier DEFAULT sha1,
        maskGenAlgorithm [1] AlgorithmIdentifier DEFAULT mgf1SHA1,
        pSourceAlgorithm [2] AlgorithmIdentifier DEFAULT pSpecifiedEmpty }

  @param[in]   Params  The keyEncryptionAlgorithm parameters, as returned by
                       mbedtls_asn1_get_alg(). A zeroized (absent) or NULL
                       parameter selects all DEFAULTs.
  @param[out]  MdType  Receives the hash to pass to mbedtls_rsa_set_padding.

  @retval TRUE   Parameters parsed and supported.
  @retval FALSE  Parse error or unsupported parameters.
**/
STATIC
BOOLEAN
Pkcs7ParseOaepHash (
  CONST mbedtls_asn1_buf  *Params,
  mbedtls_md_type_t       *MdType
  )
{
  mbedtls_md_type_t  HashMd;
  mbedtls_md_type_t  MgfMd;
  UINT8              *P;
  UINT8              *SeqEnd;
  size_t             Len;

  //
  // Absent parameters (or an explicit NULL) imply all DEFAULTs (SHA-1).
  //
  if ((Params->tag == 0) || (Params->tag == MBEDTLS_ASN1_NULL)) {
    *MdType = MBEDTLS_MD_SHA1;
    return TRUE;
  }

  if (Params->tag != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
    return FALSE;
  }

  HashMd = MBEDTLS_MD_SHA1;
  MgfMd  = MBEDTLS_MD_SHA1;
  P      = Params->p;
  SeqEnd = Params->p + Params->len;

  //
  // hashAlgorithm [0] EXPLICIT AlgorithmIdentifier
  //
  if ((P < SeqEnd) && (*P == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0))) {
    UINT8  *FieldEnd;

    if (mbedtls_asn1_get_tag (&P, SeqEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) != 0) {
      return FALSE;
    }

    FieldEnd = P + Len;
    if (!Pkcs7ReadDigestAlgId (&P, FieldEnd, &HashMd)) {
      return FALSE;
    }

    P = FieldEnd;
  }

  //
  // maskGenAlgorithm [1] EXPLICIT AlgorithmIdentifier { id-mgf1, HashAlgorithm }
  //
  if ((P < SeqEnd) && (*P == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1))) {
    UINT8             *FieldEnd;
    UINT8             *MgfEnd;
    mbedtls_asn1_buf  MgfOid;

    if (mbedtls_asn1_get_tag (&P, SeqEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 1) != 0) {
      return FALSE;
    }

    FieldEnd = P + Len;

    if (mbedtls_asn1_get_tag (&P, FieldEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
      return FALSE;
    }

    MgfEnd = P + Len;
    if (!Pkcs7Asn1GetOid (&P, MgfEnd, &MgfOid) ||
        !Pkcs7OidEquals (&MgfOid, MBEDTLS_OID_MGF1, MBEDTLS_OID_SIZE (MBEDTLS_OID_MGF1)))
    {
      return FALSE; // Only MGF1 is supported.
    }

    //
    // The MGF1 parameter is itself a HashAlgorithm AlgorithmIdentifier.
    //
    if (!Pkcs7ReadDigestAlgId (&P, MgfEnd, &MgfMd)) {
      return FALSE;
    }

    P = FieldEnd;
  }

  //
  // pSourceAlgorithm [2] is ignored: mbedtls uses an empty label, which is the
  // DEFAULT (pSpecifiedEmpty). A non-empty label would simply fail to decrypt.
  //

  if (HashMd != MgfMd) {
    return FALSE; // mbedtls uses one hash for both the OAEP digest and MGF1.
  }

  *MdType = HashMd;
  return TRUE;
}

/**
  AES-CBC-decrypt EncData (with PKCS#7 padding) using the content-encryption key Cek
  and initialization vector Iv into a freshly allocated buffer.

  @param[in]   CipherInfo   mbedtls cipher description to use.
  @param[in]   Cek          Content-encryption key.
  @param[in]   CekLen       Length of Cek in bytes.
  @param[in]   Iv           Initialization vector.
  @param[in]   IvLen        Length of Iv in bytes.
  @param[in]   EncData      Encrypted content.
  @param[in]   EncDataLen   Length of EncData in bytes.
  @param[out]  OutData      Receives the allocated plaintext buffer.
  @param[out]  OutDataSize  Receives the plaintext length in bytes.

  @retval TRUE   Decryption succeeded.
  @retval FALSE  Decryption failed (wrong key, bad padding, allocation failure).
**/
STATIC
BOOLEAN
Pkcs7AesCbcDecryptContent (
  mbedtls_cipher_info_t const  *CipherInfo,
  CONST UINT8                  *Cek,
  UINTN                        CekLen,
  CONST UINT8                  *Iv,
  UINTN                        IvLen,
  CONST UINT8                  *EncData,
  UINTN                        EncDataLen,
  UINT8                        **OutData,
  UINTN                        *OutDataSize
  )
{
  BOOLEAN                   Succeeded;
  mbedtls_cipher_context_t  CipherCtx;
  UINT8                     *Plain;
  size_t                    Olen;

  Succeeded = FALSE;
  Plain     = NULL;
  Olen      = 0;

  mbedtls_cipher_init (&CipherCtx);

  if (EncDataLen == 0) {
    goto Cleanup;
  }

  //
  // The IV must be exactly one cipher block. mbedtls copies a full block from the
  // IV pointer regardless of the length passed to mbedtls_cipher_crypt, so an IV
  // shorter than the block size would read past the supplied bytes; reject any
  // size mismatch here.
  //
  if (IvLen != mbedtls_cipher_info_get_iv_size (CipherInfo)) {
    goto Cleanup;
  }

  if (mbedtls_cipher_setup (&CipherCtx, CipherInfo) != 0) {
    goto Cleanup;
  }

  if (mbedtls_cipher_setkey (&CipherCtx, Cek, (int)(CekLen * 8), MBEDTLS_DECRYPT) != 0) {
    goto Cleanup;
  }

  if (mbedtls_cipher_set_padding_mode (&CipherCtx, MBEDTLS_PADDING_PKCS7) != 0) {
    goto Cleanup;
  }

  //
  // The decrypted output (after padding removal) is never larger than the input.
  //
  Plain = AllocatePool (EncDataLen);
  if (Plain == NULL) {
    goto Cleanup;
  }

  if (mbedtls_cipher_crypt (&CipherCtx, Iv, IvLen, EncData, EncDataLen, Plain, &Olen) != 0) {
    //
    // Bad padding etc. -- the recipient/key did not match.
    //
    goto Cleanup;
  }

  *OutData     = Plain;
  *OutDataSize = (UINTN)Olen;
  Plain        = NULL;
  Succeeded    = TRUE;

Cleanup:

  if (Plain != NULL) {
    FreePool (Plain);
  }

  mbedtls_cipher_free (&CipherCtx);
  return Succeeded;
}

/**
  Decrypts a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  (such as one produced by Pkcs7Encrypt) and recovers the original content.

  The private key is supplied in PEM form so that this interface is not tied to any
  particular key algorithm (currently supports RSA).

  The correct recipient within the envelopedData is identified automatically. The
  envelopedData carries, for each recipient, an issuer name and serial number that
  identify the recipient certificate. If RecipientCert is supplied, its issuer name
  and serial number are used to select the matching recipient. If RecipientCert is
  NULL, the supplied private key is tried against every recipient in the envelope.

  If this interface is not supported, return FALSE.

  @param[in]  PemData           Pointer to the PEM-encoded private key of a recipient
                                of the message. May be an RSA or other supported key
                                type.
  @param[in]  PemSize           Size of the PEM key data in bytes.
  @param[in]  Password          [Optional] NULL-terminated passphrase used to decrypt an
                                encrypted PEM key. Pass NULL if the PEM key is not
                                password protected.
  @param[in]  RecipientCert     [Optional] Pointer to the DER-encoded X.509 certificate
                                of the recipient whose private key is supplied in
                                PemData. If provided, it is used to select the matching
                                recipient in the envelope by issuer name and serial
                                number. Pass NULL to try the private key against every
                                recipient.
  @param[in]  RecipientCertSize Size of the DER-encoded certificate in bytes. Ignored
                                (and may be 0) when RecipientCert is NULL.
  @param[in]  ContentInfo       Pointer to the PKCS#7 DER-encoded ContentInfo that wraps
                                an envelopedData to be decrypted.
  @param[in]  ContentInfoSize   Size of the ContentInfo in bytes.
  @param[in]  Flags             Flags for the decryption operation. Currently only
                                CRYPTO_PKCS7_DEFAULT is supported, which indicates that
                                the decrypted content is treated as binary data.
  @param[out] OutData           Receives a pointer to the newly allocated buffer
                                containing the decrypted content. The caller must free
                                the returned buffer with FreePool().
  @param[out] OutDataSize       Receives the size of the decrypted content in bytes.

  @retval     TRUE              PKCS#7 data decryption succeeded.
  @retval     FALSE             PKCS#7 data decryption failed.
  @retval     FALSE             This interface is not supported.

**/
BOOLEAN
EFIAPI
Pkcs7Decrypt (
  IN   CONST UINT8  *PemData,
  IN   UINTN        PemSize,
  IN   CONST CHAR8  *Password           OPTIONAL,
  IN   CONST UINT8  *RecipientCert      OPTIONAL,
  IN   UINTN        RecipientCertSize   OPTIONAL,
  IN   CONST UINT8  *ContentInfo,
  IN   UINTN        ContentInfoSize,
  IN   UINT32       Flags,
  OUT  UINT8        **OutData,
  OUT  UINTN        *OutDataSize
  )
{
  BOOLEAN           Succeeded;
  VOID              *Rsa;
  mbedtls_x509_crt  Cert;
  UINT8             *ReturnData;
  UINTN             ReturnSize;

  UINT8   *P;
  UINT8   *End;
  size_t  Len;

  //
  // Parsed EncryptedContentInfo.
  //
  mbedtls_cipher_info_t const  *CipherInfo;
  UINTN                        ContentKeyLen;
  UINT8                        *Iv;
  UINTN                        IvLen;
  UINT8                        *EncContent;
  UINTN                        EncContentLen;

  //
  // Bounds of the recipientInfos SET OF contents.
  //
  UINT8  *RiCur;
  UINT8  *RiEnd;

  Succeeded  = FALSE;
  Rsa        = NULL;
  ReturnData = NULL;
  ReturnSize = 0;

  CipherInfo    = NULL;
  ContentKeyLen = 0;
  Iv            = NULL;
  IvLen         = 0;
  EncContent    = NULL;
  EncContentLen = 0;

  RiCur = NULL;
  RiEnd = NULL;

  mbedtls_x509_crt_init (&Cert);

  //
  // Validate parameters (mirrors the OpenSSL instance).
  //
  if ((PemData == NULL) ||
      (PemSize == 0) ||
      (PemSize > INT_MAX) ||
      (ContentInfo == NULL) ||
      (ContentInfoSize == 0) ||
      (ContentInfoSize > INT_MAX) ||
      (Flags != CRYPTO_PKCS7_DEFAULT) ||
      (OutData == NULL) ||
      (OutDataSize == NULL))
  {
    goto Done;
  }

  if ((RecipientCert != NULL) &&
      ((RecipientCertSize == 0) || (RecipientCertSize > INT_MAX)))
  {
    goto Done;
  }

  //
  // Retrieve the recipient's private key from the PEM data.
  //
  if (!RsaGetPrivateKeyFromPem (PemData, PemSize, Password, &Rsa)) {
    goto Done; // Invalid PEM key data or incorrect password.
  }

  //
  // Optionally parse the recipient certificate for issuer/serial recipient selection.
  //
  if (RecipientCert != NULL) {
    if (mbedtls_x509_crt_parse_der (&Cert, RecipientCert, RecipientCertSize) != 0) {
      goto Done;
    }
  }

  //
  // Parse the outer ContentInfo:
  //   ContentInfo ::= SEQUENCE { contentType OID(envelopedData), content [0] EXPLICIT EnvelopedData }
  //
  P   = (UINT8 *)ContentInfo;
  End = (UINT8 *)ContentInfo + ContentInfoSize;

  if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    goto Done;
  }

  End = P + Len; // Restrict to the ContentInfo contents.

  {
    mbedtls_asn1_buf  Oid;

    if (!Pkcs7Asn1GetOid (&P, End, &Oid) ||
        !Pkcs7OidEquals (&Oid, MBEDTLS_OID_PKCS7_ENVELOPED_DATA, MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS7_ENVELOPED_DATA)))
    {
      goto Done; // Not an envelopedData.
    }
  }

  //
  // content [0] EXPLICIT
  //
  if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) != 0) {
    goto Done;
  }

  End = P + Len;

  //
  // EnvelopedData ::= SEQUENCE {
  //   version, [0] originatorInfo OPTIONAL, recipientInfos SET, encryptedContentInfo, ... }
  //
  if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    goto Done;
  }

  End = P + Len;

  {
    int  Version;

    if (mbedtls_asn1_get_int (&P, End, &Version) != 0) {
      goto Done;
    }
  }

  //
  // Skip optional originatorInfo [0] IMPLICIT.
  //
  if ((P < End) && (*P == (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0))) {
    if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) != 0) {
      goto Done;
    }

    P += Len;
  }

  //
  // recipientInfos SET OF RecipientInfo -- record its bounds and skip past it.
  //
  if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SET) != 0) {
    goto Done;
  }

  RiCur = P;
  RiEnd = P + Len;
  P     = RiEnd;

  //
  // EncryptedContentInfo ::= SEQUENCE {
  //   contentType OID(id-data),
  //   contentEncryptionAlgorithm AlgorithmIdentifier { aes-cbc OID, IV OCTET STRING },
  //   encryptedContent [0] IMPLICIT OCTET STRING }
  //
  if (mbedtls_asn1_get_tag (&P, End, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
    goto Done;
  }

  {
    UINT8                  *EciEnd;
    mbedtls_asn1_buf       Oid;
    mbedtls_asn1_buf       Alg;
    mbedtls_asn1_buf       Params;
    mbedtls_cipher_type_t  CipherType;

    EciEnd = P + Len;

    //
    // contentType (id-data)
    //
    if (!Pkcs7Asn1GetOid (&P, EciEnd, &Oid) ||
        !Pkcs7OidEquals (&Oid, MBEDTLS_OID_PKCS7_DATA, MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS7_DATA)))
    {
      goto Done;
    }

    //
    // contentEncryptionAlgorithm AlgorithmIdentifier { aes-cbc OID, IV OCTET STRING }
    //
    if (mbedtls_asn1_get_alg (&P, EciEnd, &Alg, &Params) != 0) {
      goto Done;
    }

    if (mbedtls_oid_get_cipher_alg (&Alg, &CipherType) != 0) {
      goto Done; // Unsupported content-encryption algorithm.
    }

    CipherInfo = mbedtls_cipher_info_from_type (CipherType);
    if (CipherInfo == NULL) {
      goto Done; // Cipher not built into this configuration.
    }

    ContentKeyLen = mbedtls_cipher_info_get_key_bitlen (CipherInfo) / 8;

    //
    // The AES-CBC parameter is the IV as an OCTET STRING.
    //
    if (Params.tag != MBEDTLS_ASN1_OCTET_STRING) {
      goto Done;
    }

    Iv    = Params.p;
    IvLen = Params.len;

    //
    // encryptedContent [0] IMPLICIT OCTET STRING (primitive)
    //
    if (mbedtls_asn1_get_tag (&P, EciEnd, &Len, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0) != 0) {
      goto Done;
    }

    EncContent    = P;
    EncContentLen = Len;
  }

  if ((ContentKeyLen == 0) || (ContentKeyLen > MAX_CONTENT_ENC_KEY_LEN) || (EncContentLen == 0)) {
    goto Done;
  }

  //
  // Walk the recipientInfos, attempting to recover the content-encryption key.
  //
  while (RiCur < RiEnd) {
    UINT8             *KtEnd;
    UINT8             *IssuerName;
    UINTN             IssuerNameLen;
    UINT8             *Serial;
    UINTN             SerialLen;
    mbedtls_asn1_buf  KeaAlg;
    mbedtls_asn1_buf  KeaParams;
    UINT8             *EncKey;
    UINTN             EncKeyLen;
    BOOLEAN           UseOaep;

    //
    // RecipientInfo is a CHOICE. Only KeyTransRecipientInfo (ktri), encoded as a
    // plain SEQUENCE, is supported. Skip the other alternatives (kari [1],
    // kekri [2], pwri [3], ori [4]) rather than failing, so an envelope that also
    // carries a supported ktri recipient can still be decrypted.
    //
    if (*RiCur != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE)) {
      UINT8  *SkipCur;

      SkipCur = RiCur;
      if (mbedtls_asn1_get_tag (&SkipCur, RiEnd, &Len, *RiCur) != 0) {
        goto Done; // Malformed length.
      }

      RiCur = SkipCur + Len;
      continue;
    }

    //
    // RecipientInfo (KeyTransRecipientInfo) SEQUENCE
    //
    if (mbedtls_asn1_get_tag (&RiCur, RiEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
      goto Done;
    }

    KtEnd = RiCur + Len;

    {
      int  Version;

      if (mbedtls_asn1_get_int (&RiCur, KtEnd, &Version) != 0) {
        goto Done;
      }
    }

    //
    // rid IssuerAndSerialNumber ::= SEQUENCE { issuer Name, serialNumber INTEGER }
    //
    // rid is a CHOICE of issuerAndSerialNumber (SEQUENCE) or subjectKeyIdentifier
    // [0]. Only issuerAndSerialNumber is supported; skip recipients that use the
    // subjectKeyIdentifier form rather than failing.
    //
    if ((RiCur >= KtEnd) || (*RiCur != (MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE))) {
      RiCur = KtEnd;
      continue;
    }

    if (mbedtls_asn1_get_tag (&RiCur, KtEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
      goto Done;
    }

    {
      UINT8  *IsnEnd;

      IsnEnd = RiCur + Len;

      //
      // issuer Name -- capture the full SEQUENCE (tag+len+value) for comparison.
      //
      IssuerName = RiCur;
      if (mbedtls_asn1_get_tag (&RiCur, IsnEnd, &Len, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE) != 0) {
        goto Done;
      }

      RiCur         = RiCur + Len;
      IssuerNameLen = (UINTN)(RiCur - IssuerName);

      //
      // serialNumber INTEGER -- capture its content bytes.
      //
      if (mbedtls_asn1_get_tag (&RiCur, IsnEnd, &Len, MBEDTLS_ASN1_INTEGER) != 0) {
        goto Done;
      }

      Serial    = RiCur;
      SerialLen = Len;
      RiCur     = IsnEnd;
    }

    if ((RecipientCert != NULL) &&
        ((IssuerNameLen != Cert.issuer_raw.len) ||
         (CompareMem (IssuerName, Cert.issuer_raw.p, IssuerNameLen) != 0) ||
         (SerialLen != Cert.serial.len) ||
         (CompareMem (Serial, Cert.serial.p, SerialLen) != 0)))
    {
      RiCur = KtEnd;
      continue; // Not this recipient.
    }

    //
    // keyEncryptionAlgorithm AlgorithmIdentifier
    //
    if (mbedtls_asn1_get_alg (&RiCur, KtEnd, &KeaAlg, &KeaParams) != 0) {
      goto Done;
    }

    UseOaep = Pkcs7OidEquals (&KeaAlg, MBEDTLS_OID_RSAES_OAEP, MBEDTLS_OID_SIZE (MBEDTLS_OID_RSAES_OAEP));
    if (!UseOaep &&
        !Pkcs7OidEquals (&KeaAlg, MBEDTLS_OID_PKCS1_RSA, MBEDTLS_OID_SIZE (MBEDTLS_OID_PKCS1_RSA)))
    {
      //
      // Unsupported key transport algorithm; skip this recipient.
      //
      RiCur = KtEnd;
      continue;
    }

    //
    // encryptedKey OCTET STRING
    //
    if (mbedtls_asn1_get_tag (&RiCur, KtEnd, &Len, MBEDTLS_ASN1_OCTET_STRING) != 0) {
      goto Done;
    }

    EncKey    = RiCur;
    EncKeyLen = Len;
    RiCur     = KtEnd;

    //
    // mbedtls_rsa_pkcs1_decrypt always reads a full modulus worth of input, so
    // reject any recipient whose encryptedKey is a different size.
    //
    if (EncKeyLen != mbedtls_rsa_get_len ((mbedtls_rsa_context *)Rsa)) {
      continue;
    }

    //
    // Select the RSA padding scheme indicated by the recipient info.
    //
    if (UseOaep) {
      mbedtls_md_type_t  OaepMd;

      if (!Pkcs7ParseOaepHash (&KeaParams, &OaepMd) ||
          (mbedtls_rsa_set_padding ((mbedtls_rsa_context *)Rsa, MBEDTLS_RSA_PKCS_V21, OaepMd) != 0))
      {
        continue;
      }
    } else {
      (VOID)mbedtls_rsa_set_padding ((mbedtls_rsa_context *)Rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    }

    //
    // RSA-decrypt the content-encryption key.
    //
    {
      UINT8    Cek[MAX_CONTENT_ENC_KEY_LEN];
      size_t   CekLen;
      BOOLEAN  DecryptOk;

      CekLen = 0;

      if (mbedtls_rsa_pkcs1_decrypt (
            (mbedtls_rsa_context *)Rsa,
            MbedtlsRand,
            NULL,
            &CekLen,
            EncKey,
            Cek,
            sizeof (Cek)
            ) != 0)
      {
        ZeroMem (Cek, sizeof (Cek));
        continue; // Try the next recipient.
      }

      if (CekLen != ContentKeyLen) {
        ZeroMem (Cek, sizeof (Cek));
        continue;
      }

      DecryptOk = Pkcs7AesCbcDecryptContent (
                    CipherInfo,
                    Cek,
                    CekLen,
                    Iv,
                    IvLen,
                    EncContent,
                    EncContentLen,
                    &ReturnData,
                    &ReturnSize
                    );
      ZeroMem (Cek, sizeof (Cek));

      if (DecryptOk) {
        Succeeded = TRUE;
        goto Done;
      }

      //
      // Otherwise fall through and try the next recipient.
      //
    }
  }

Done:

  //
  // Normalize a zero-length success to a 1-byte allocation so callers always get a
  // freeable, non-NULL buffer (matching the OpenSSL instance).
  //
  if (Succeeded && (ReturnData == NULL)) {
    ReturnData = AllocateZeroPool (1);
    if (ReturnData == NULL) {
      Succeeded = FALSE;
    } else {
      ReturnSize = 0;
    }
  }

  if (!Succeeded && (ReturnData != NULL)) {
    FreePool (ReturnData);
    ReturnData = NULL;
    ReturnSize = 0;
  }

  if (Rsa != NULL) {
    RsaFree (Rsa);
  }

  mbedtls_x509_crt_free (&Cert);

  if (OutData != NULL) {
    *OutData = ReturnData;
  }

  if (OutDataSize != NULL) {
    *OutDataSize = ReturnSize;
  }

  return Succeeded;
}
