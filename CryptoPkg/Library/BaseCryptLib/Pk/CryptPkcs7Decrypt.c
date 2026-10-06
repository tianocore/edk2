/** @file
  PKCS7 Decryption implementation over OpenSSL

  Copyright (c) 2026, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/evp.h>
#include <openssl/pkcs7.h>
#include <openssl/objects.h>
#include <openssl/x509.h>

#include <Library/MemoryAllocationLib.h>

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
  BOOLEAN      Succeeded;
  UINT8        *ReturnData;
  UINTN        ReturnSize;
  VOID         *Rsa;
  BIO          *OutBio;
  EVP_PKEY     *Key;
  X509         *Cert;
  PKCS7        *Pkcs7;
  CONST UINT8  *DerCursor;
  long         OutLength;
  char         *OutBuffer;

  ReturnData = NULL;
  ReturnSize = 0;
  Rsa        = NULL;
  OutBio     = NULL;
  Key        = NULL;
  Cert       = NULL;
  Pkcs7      = NULL;

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
    Succeeded = FALSE; // Invalid argument.
    goto Done;
  }

  if ((RecipientCert != NULL) &&
      ((RecipientCertSize == 0) || (RecipientCertSize > INT_MAX)))
  {
    Succeeded = FALSE; // Invalid certificate argument.
    goto Done;
  }

  //
  // Retrieve the recipient's private key from the PEM data and wrap it in an
  // EVP_PKEY for PKCS7_decrypt.
  //
  if (!RsaGetPrivateKeyFromPem (PemData, PemSize, Password, &Rsa)) {
    Succeeded = FALSE; // Invalid PEM key data or incorrect password.
    goto Done;
  }

  Key = EVP_PKEY_new ();
  if (Key == NULL) {
    Succeeded = FALSE; // Memory allocation failed.
    goto Done;
  }

  if (EVP_PKEY_assign_RSA (Key, (RSA *)Rsa) == 0) {
    Succeeded = FALSE; // Key conversion failed.
    goto Done;
  }

  //
  // Key now owns Rsa and frees it via EVP_PKEY_free.
  //
  Rsa = NULL;

  //
  // Optionally parse the recipient certificate. When supplied, OpenSSL uses its
  // issuer name and serial number to select the matching recipient in the envelope.
  //
  if (RecipientCert != NULL) {
    DerCursor = RecipientCert;
    Cert      = d2i_X509 (NULL, &DerCursor, (long)RecipientCertSize);
    if (Cert == NULL) {
      Succeeded = FALSE; // Invalid certificate.
      goto Done;
    }
  }

  //
  // Parse the DER-encoded PKCS#7 ContentInfo.
  //
  DerCursor = ContentInfo;
  Pkcs7     = d2i_PKCS7 (NULL, &DerCursor, (long)ContentInfoSize);
  if (Pkcs7 == NULL) {
    Succeeded = FALSE; // Invalid PKCS#7 data.
    goto Done;
  }

  if (!PKCS7_type_is_enveloped (Pkcs7)) {
    Succeeded = FALSE; // Not an envelopedData structure.
    goto Done;
  }

  //
  // Decrypt into a memory BIO.
  //
  OutBio = BIO_new (BIO_s_mem ());
  if (OutBio == NULL) {
    Succeeded = FALSE; // BIO creation failed.
    goto Done;
  }

  if (PKCS7_decrypt (Pkcs7, Key, Cert, OutBio, PKCS7_BINARY) != 1) {
    Succeeded = FALSE; // Decryption failed (wrong key, no matching recipient, etc.).
    goto Done;
  }

  OutLength = BIO_get_mem_data (OutBio, &OutBuffer);
  if ((OutLength < 0) || ((UINTN)OutLength > INT_MAX)) {
    Succeeded = FALSE; // Unexpected decrypted length.
    goto Done;
  }

  ReturnSize = (UINTN)OutLength;
  //
  // Always allocate at least one byte so that a successful decryption of empty
  // content still yields a non-NULL, freeable buffer.
  //
  ReturnData = (UINT8 *)AllocateZeroPool (ReturnSize == 0 ? 1 : ReturnSize);
  if (ReturnData == NULL) {
    ReturnSize = 0;
    Succeeded  = FALSE; // Memory allocation failed.
    goto Done;
  }

  if (ReturnSize != 0) {
    CopyMem (ReturnData, OutBuffer, ReturnSize);
  }

  Succeeded = TRUE; // Success, result in ReturnData/ReturnSize.

Done:

  if (Pkcs7 != NULL) {
    PKCS7_free (Pkcs7);
    Pkcs7 = NULL;
  }

  if (Cert != NULL) {
    X509_free (Cert);
    Cert = NULL;
  }

  if (Key != NULL) {
    EVP_PKEY_free (Key);
    Key = NULL;
  }

  if (Rsa != NULL) {
    RsaFree (Rsa);
    Rsa = NULL;
  }

  if (OutBio != NULL) {
    BIO_free (OutBio);
    OutBio = NULL;
  }

  if (OutData != NULL) {
    *OutData = ReturnData;
  }

  if (OutDataSize != NULL) {
    *OutDataSize = ReturnSize;
  }

  return Succeeded;
}
