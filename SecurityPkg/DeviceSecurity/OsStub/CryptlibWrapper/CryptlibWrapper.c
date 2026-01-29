/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "hal/base.h"
#include "hal/library/cryptlib.h"

void *
libspdm_sha256_new (
  void
  )
{
  size_t  CtxSize;
  void    *HashCtx;

  HashCtx = NULL;
  CtxSize = Sha256GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  return HashCtx;
}

void
libspdm_sha256_free (
  void  *Sha256Ctx
  )
{
  if (Sha256Ctx != NULL) {
    FreePool (Sha256Ctx);
    Sha256Ctx = NULL;
  }
}

bool
libspdm_sha256_init (
  void  *Sha256Ctx
  )
{
  return Sha256Init (Sha256Ctx);
}

bool
libspdm_sha256_duplicate (
  const void  *Sha256Context,
  void        *NewSha256Context
  )
{
  return Sha256Duplicate (Sha256Context, NewSha256Context);
}

bool
libspdm_sha256_update (
  void        *Sha256Context,
  const void  *Data,
  size_t      DataSize
  )
{
  return Sha256Update (Sha256Context, Data, DataSize);
}

bool
libspdm_sha256_final (
  void     *sha256_context,
  uint8_t  *hash_value
  )
{
  return Sha256Final (sha256_context, hash_value);
}

bool
libspdm_sha256_hash_all (
  const void  *data,
  size_t      data_size,
  uint8_t     *hash_value
  )
{
  return Sha256HashAll (data, data_size, hash_value);
}

void *
libspdm_sha384_new (
  void
  )
{
  size_t  CtxSize;
  void    *HashCtx;

  HashCtx = NULL;
  CtxSize = Sha384GetContextSize ();
  HashCtx = AllocatePool (CtxSize);

  return HashCtx;
}

void
libspdm_sha384_free (
  void  *Sha384Ctx
  )
{
  if (Sha384Ctx != NULL) {
    FreePool (Sha384Ctx);
    Sha384Ctx = NULL;
  }
}

bool
libspdm_sha384_init (
  void  *sha384_context
  )
{
  return Sha384Init (sha384_context);
}

bool
libspdm_sha384_duplicate (
  const void  *sha384_context,
  void        *new_sha384_context
  )
{
  return Sha384Duplicate (sha384_context, new_sha384_context);
}

bool
libspdm_sha384_update (
  void        *sha384_context,
  const void  *data,
  size_t      data_size
  )
{
  return Sha384Update (sha384_context, data, data_size);
}

bool
libspdm_sha384_final (
  void     *sha384_context,
  uint8_t  *hash_value
  )
{
  return Sha384Final (sha384_context, hash_value);
}

bool
libspdm_sha384_hash_all (
  const void  *data,
  size_t      data_size,
  uint8_t     *hash_value
  )
{
  return Sha384HashAll (data, data_size, hash_value);
}

void *
libspdm_hmac_sha256_new (
  void
  )
{
  return HmacSha256New ();
}

void
libspdm_hmac_sha256_free (
  void  *hmac_sha256_ctx
  )
{
  HmacSha256Free (hmac_sha256_ctx);
}

bool
libspdm_hmac_sha256_set_key (
  void           *hmac_sha256_ctx,
  const uint8_t  *key,
  size_t         key_size
  )
{
  return HmacSha256SetKey (hmac_sha256_ctx, key, key_size);
}

bool
libspdm_hmac_sha256_duplicate (
  const void  *hmac_sha256_ctx,
  void        *new_hmac_sha256_ctx
  )
{
  return HmacSha256Duplicate (hmac_sha256_ctx, new_hmac_sha256_ctx);
}

bool
libspdm_hmac_sha256_update (
  void        *hmac_sha256_ctx,
  const void  *data,
  size_t      data_size
  )
{
  return HmacSha256Update (hmac_sha256_ctx, data, data_size);
}

bool
libspdm_hmac_sha256_final (
  void     *hmac_sha256_ctx,
  uint8_t  *hmac_value
  )
{
  return HmacSha256Final (hmac_sha256_ctx, hmac_value);
}

bool
libspdm_hmac_sha256_all (
  const void     *data,
  size_t         data_size,
  const uint8_t  *key,
  size_t         key_size,
  uint8_t        *hmac_value
  )
{
  return HmacSha256All (data, data_size, key, key_size, hmac_value);
}

void *
libspdm_hmac_sha384_new (
  void
  )
{
  return HmacSha384New ();
}

void
libspdm_hmac_sha384_free (
  void  *hmac_sha384_ctx
  )
{
  HmacSha384Free (hmac_sha384_ctx);
}

bool
libspdm_hmac_sha384_set_key (
  void           *hmac_sha384_ctx,
  const uint8_t  *key,
  size_t         key_size
  )
{
  return HmacSha384SetKey (hmac_sha384_ctx, key, key_size);
}

bool
libspdm_hmac_sha384_duplicate (
  const void  *hmac_sha384_ctx,
  void        *new_hmac_sha384_ctx
  )
{
  return HmacSha384Duplicate (hmac_sha384_ctx, new_hmac_sha384_ctx);
}

bool
libspdm_hmac_sha384_update (
  void        *hmac_sha384_ctx,
  const void  *data,
  size_t      data_size
  )
{
  return HmacSha384Update (hmac_sha384_ctx, data, data_size);
}

bool
libspdm_hmac_sha384_final (
  void     *hmac_sha384_ctx,
  uint8_t  *hmac_value
  )
{
  return HmacSha384Final (hmac_sha384_ctx, hmac_value);
}

bool
libspdm_hmac_sha384_all (
  const void     *data,
  size_t         data_size,
  const uint8_t  *key,
  size_t         key_size,
  uint8_t        *hmac_value
  )
{
  return HmacSha384All (data, data_size, key, key_size, hmac_value);
}

bool
libspdm_aead_aes_gcm_encrypt (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *iv,
  size_t         iv_size,
  const uint8_t  *a_data,
  size_t         a_data_size,
  const uint8_t  *data_in,
  size_t         data_in_size,
  uint8_t        *tag_out,
  size_t         tag_size,
  uint8_t        *data_out,
  size_t         *data_out_size
  )
{
  return AeadAesGcmEncrypt (
           key,
           key_size,
           iv,
           iv_size,
           a_data,
           a_data_size,
           data_in,
           data_in_size,
           tag_out,
           tag_size,
           data_out,
           data_out_size
           );
}

bool
libspdm_aead_aes_gcm_decrypt (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *iv,
  size_t         iv_size,
  const uint8_t  *a_data,
  size_t         a_data_size,
  const uint8_t  *data_in,
  size_t         data_in_size,
  const uint8_t  *tag,
  size_t         tag_size,
  uint8_t        *data_out,
  size_t         *data_out_size
  )
{
  return AeadAesGcmDecrypt (
           key,
           key_size,
           iv,
           iv_size,
           a_data,
           a_data_size,
           data_in,
           data_in_size,
           tag,
           tag_size,
           data_out,
           data_out_size
           );
}

void
libspdm_rsa_free (
  void  *rsa_context
  )
{
  RsaFree (rsa_context);
}

bool
libspdm_rsa_pkcs1_sign_with_nid (
  void           *rsa_context,
  size_t         hash_nid,
  const uint8_t  *message_hash,
  size_t         hash_size,
  uint8_t        *signature,
  size_t         *sig_size
  )
{
  switch (hash_nid) {
    case CRYPTO_NID_SHA256:
      if (hash_size != SHA256_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA384:
      if (hash_size != SHA384_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    case CRYPTO_NID_SHA512:
      if (hash_size != SHA512_DIGEST_SIZE) {
        return FALSE;
      }

      break;

    default:
      return FALSE;
  }

  return RsaPkcs1Sign (
           rsa_context,
           message_hash,
           hash_size,
           signature,
           sig_size
           );
}

bool
libspdm_rsa_pkcs1_verify_with_nid (
  void           *rsa_context,
  size_t         hash_nid,
  const uint8_t  *message_hash,
  size_t         hash_size,
  const uint8_t  *signature,
  size_t         sig_size
  )
{
  switch (hash_nid) {
    case CRYPTO_NID_SHA256:
      if (hash_size != SHA256_DIGEST_SIZE) {
        return false;
      }

      break;

    case CRYPTO_NID_SHA384:
      if (hash_size != SHA384_DIGEST_SIZE) {
        return false;
      }

      break;

    case CRYPTO_NID_SHA512:
      if (hash_size != SHA512_DIGEST_SIZE) {
        return false;
      }

      break;

    default:
      return false;
  }

  return RsaPkcs1Verify (
           rsa_context,
           message_hash,
           hash_size,
           signature,
           sig_size
           );
}

bool
libspdm_rsa_get_private_key_from_pem (
  const uint8_t  *pem_data,
  size_t         pem_size,
  const char     *password,
  void           **rsa_context
  )
{
  return RsaGetPrivateKeyFromPem (pem_data, pem_size, password, rsa_context);
}

bool
libspdm_rsa_get_public_key_from_x509 (
  const uint8_t  *cert,
  size_t         cert_size,
  void           **rsa_context
  )
{
  return RsaGetPublicKeyFromX509 (cert, cert_size, rsa_context);
}

bool
libspdm_ec_get_public_key_from_der (
  const uint8_t  *der_data,
  size_t         der_size,
  void           **ec_context
  )
{
  return false;
}

bool
libspdm_rsa_get_public_key_from_der (
  const uint8_t  *der_data,
  size_t         der_size,
  void           **rsa_context
  )
{
  return false;
}

bool
libspdm_ec_get_private_key_from_pem (
  const uint8_t  *pem_data,
  size_t         pem_size,
  const char     *password,
  void           **ec_context
  )
{
  return EcGetPrivateKeyFromPem (pem_data, pem_size, password, ec_context);
}

bool
libspdm_ec_get_public_key_from_x509 (
  const uint8_t  *cert,
  size_t         cert_size,
  void           **ec_context
  )
{
  return EcGetPublicKeyFromX509 (cert, cert_size, ec_context);
}

bool
libspdm_asn1_get_tag (
  uint8_t        **ptr,
  const uint8_t  *end,
  size_t         *length,
  uint32_t       tag
  )
{
  return Asn1GetTag (ptr, end, length, tag);
}

bool
libspdm_x509_get_subject_name (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *cert_subject,
  size_t         *subject_size
  )
{
  return X509GetSubjectName (cert, cert_size, cert_subject, subject_size);
}

bool
libspdm_x509_get_common_name (
  const uint8_t  *cert,
  size_t         cert_size,
  char           *common_name,
  size_t         *common_name_size
  )
{
  EFI_STATUS  Status;

  Status = X509GetCommonName (cert, cert_size, common_name, common_name_size);
  if (EFI_ERROR (Status)) {
    return false;
  } else {
    return true;
  }
}

bool
libspdm_x509_get_organization_name (
  const uint8_t  *cert,
  size_t         cert_size,
  char           *name_buffer,
  size_t         *name_buffer_size
  )
{
  EFI_STATUS  Status;

  Status = X509GetOrganizationName (cert, cert_size, name_buffer, name_buffer_size);
  if (EFI_ERROR (Status)) {
    return false;
  } else {
    return true;
  }
}

bool
libspdm_x509_get_version (
  const uint8_t  *cert,
  size_t         cert_size,
  size_t         *version
  )
{
  return X509GetVersion (cert, cert_size, version);
}

bool
libspdm_x509_get_serial_number (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *serial_number,
  size_t         *serial_number_size
  )
{
  return X509GetSerialNumber (cert, cert_size, serial_number, serial_number_size);
}

bool
libspdm_x509_get_issuer_name (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *cert_issuer,
  size_t         *issuer_size
  )
{
  return X509GetIssuerName (cert, cert_size, cert_issuer, issuer_size);
}

bool
libspdm_x509_get_signature_algorithm (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *oid,
  size_t         *oid_size
  )
{
  return X509GetSignatureAlgorithm (cert, cert_size, oid, oid_size);
}

bool
libspdm_x509_get_extension_data (
  const uint8_t  *cert,
  size_t         cert_size,
  const uint8_t  *oid,
  size_t         oid_size,
  uint8_t        *extension_data,
  size_t         *extension_data_size
  )
{
  return X509GetExtensionData (
           cert,
           cert_size,
           oid,
           oid_size,
           extension_data,
           extension_data_size
           );
}

bool
libspdm_x509_get_validity (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *from,
  size_t         *from_size,
  uint8_t        *to,
  size_t         *to_size
  )
{
  return X509GetValidity (cert, cert_size, from, from_size, to, to_size);
}

bool
libspdm_x509_set_date_time (
  const char  *date_time_str,
  void        *date_time,
  size_t      *date_time_size
  )
{
  return X509FormatDateTime (date_time_str, date_time, date_time_size);
}

int32_t
libspdm_x509_compare_date_time (
  const void  *date_time1,
  const void  *date_time2
  )
{
  return X509CompareDateTime (date_time1, date_time2);
}

bool
libspdm_x509_get_key_usage (
  const uint8_t  *cert,
  size_t         cert_size,
  size_t         *usage
  )
{
  return X509GetKeyUsage (cert, cert_size, usage);
}

bool
libspdm_x509_get_extended_key_usage (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *usage,
  size_t         *usage_size
  )
{
  return X509GetExtendedKeyUsage (cert, cert_size, usage, usage_size);
}

bool
libspdm_x509_verify_cert (
  const uint8_t  *cert,
  size_t         cert_size,
  const uint8_t  *ca_cert,
  size_t         ca_cert_size
  )
{
  return X509VerifyCert (cert, cert_size, ca_cert, ca_cert_size);
}

bool
libspdm_x509_verify_cert_chain (
  const uint8_t  *root_cert,
  size_t         root_cert_length,
  const uint8_t  *cert_chain,
  size_t         cert_chain_length
  )
{
  return X509VerifyCertChain (root_cert, root_cert_length, cert_chain, cert_chain_length);
}

bool
libspdm_x509_get_cert_from_cert_chain (
  const uint8_t  *cert_chain,
  size_t         cert_chain_length,
  const int32_t  cert_index,
  const uint8_t  **cert,
  size_t         *cert_length
  )
{
  return X509GetCertFromCertChain (
           cert_chain,
           cert_chain_length,
           cert_index,
           cert,
           cert_length
           );
}

bool
libspdm_x509_construct_certificate (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        **single_x509_cert
  )
{
  return X509ConstructCertificate (cert, cert_size, single_x509_cert);
}

bool
libspdm_x509_get_extended_basic_constraints (
  const uint8_t  *cert,
  size_t         cert_size,
  uint8_t        *basic_constraints,
  size_t         *basic_constraints_size
  )
{
  return X509GetExtendedBasicConstraints (
           cert,
           cert_size,
           basic_constraints,
           basic_constraints_size
           );
}

void *
libspdm_ec_new_by_nid (
  size_t  nid
  )
{
  return EcNewByNid (nid);
}

void
libspdm_ec_free (
  void  *ec_context
  )
{
  EcFree (ec_context);
}

bool
libspdm_ec_generate_key (
  void     *ec_context,
  uint8_t  *public_data,
  size_t   *public_size
  )
{
  return EcGenerateKey (ec_context, public_data, public_size);
}

bool
libspdm_ec_compute_key (
  void           *ec_context,
  const uint8_t  *peer_public,
  size_t         peer_public_size,
  uint8_t        *key,
  size_t         *key_size
  )
{
  return EcDhComputeKey (ec_context, peer_public, peer_public_size, NULL, key, key_size);
}

bool
libspdm_ecdsa_sign (
  void           *ec_context,
  size_t         hash_nid,
  const uint8_t  *message_hash,
  size_t         hash_size,
  uint8_t        *signature,
  size_t         *sig_size
  )
{
  return EcDsaSign (
           ec_context,
           hash_nid,
           message_hash,
           hash_size,
           signature,
           sig_size
           );
}

bool
libspdm_ecdsa_verify (
  void           *ec_context,
  size_t         hash_nid,
  const uint8_t  *message_hash,
  size_t         hash_size,
  const uint8_t  *signature,
  size_t         sig_size
  )
{
  return EcDsaVerify (
           ec_context,
           hash_nid,
           message_hash,
           hash_size,
           signature,
           sig_size
           );
}

bool
libspdm_random_bytes (
  uint8_t  *output,
  size_t   size
  )
{
  return RandomBytes (output, size);
}

bool
libspdm_hkdf_sha256_extract_and_expand (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *salt,
  size_t         salt_size,
  const uint8_t  *info,
  size_t         info_size,
  uint8_t        *out,
  size_t         out_size
  )
{
  return HkdfSha256ExtractAndExpand (
           key,
           key_size,
           salt,
           salt_size,
           info,
           info_size,
           out,
           out_size
           );
}

bool
libspdm_hkdf_sha256_extract (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *salt,
  size_t         salt_size,
  uint8_t        *prk_out,
  size_t         prk_out_size
  )
{
  return HkdfSha256Extract (
           key,
           key_size,
           salt,
           salt_size,
           prk_out,
           prk_out_size
           );
}

bool
libspdm_hkdf_sha256_expand (
  const uint8_t  *prk,
  size_t         prk_size,
  const uint8_t  *info,
  size_t         info_size,
  uint8_t        *out,
  size_t         out_size
  )
{
  return HkdfSha256Expand (
           prk,
           prk_size,
           info,
           info_size,
           out,
           out_size
           );
}

bool
libspdm_hkdf_sha384_extract_and_expand (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *salt,
  size_t         salt_size,
  const uint8_t  *info,
  size_t         info_size,
  uint8_t        *out,
  size_t         out_size
  )
{
  return HkdfSha384ExtractAndExpand (
           key,
           key_size,
           salt,
           salt_size,
           info,
           info_size,
           out,
           out_size
           );
}

bool
libspdm_hkdf_sha384_extract (
  const uint8_t  *key,
  size_t         key_size,
  const uint8_t  *salt,
  size_t         salt_size,
  uint8_t        *prk_out,
  size_t         prk_out_size
  )
{
  return HkdfSha384Extract (
           key,
           key_size,
           salt,
           salt_size,
           prk_out,
           prk_out_size
           );
}

bool
libspdm_hkdf_sha384_expand (
  const uint8_t  *prk,
  size_t         prk_size,
  const uint8_t  *info,
  size_t         info_size,
  uint8_t        *out,
  size_t         out_size
  )
{
  return HkdfSha384Expand (
           prk,
           prk_size,
           info,
           info_size,
           out,
           out_size
           );
}
