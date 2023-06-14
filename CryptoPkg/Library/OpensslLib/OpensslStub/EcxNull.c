/** @file
  Null implementation of ECX functions called by BaseCryptLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "openssl/types.h"
#include "crypto/asn1.h"
#include "crypto/evp.h"

const EVP_PKEY_METHOD *
ossl_ecx25519_pkey_method (
  void
  )
{
  return NULL;
}

const EVP_PKEY_METHOD *
ossl_ecx448_pkey_method (
  void
  )
{
  return NULL;
}

const EVP_PKEY_METHOD *
ossl_ed25519_pkey_method (
  void
  )
{
  return NULL;
}

const EVP_PKEY_METHOD *
ossl_ed448_pkey_method (
  void
  )
{
  return NULL;
}

const EVP_PKEY_ASN1_METHOD  ossl_ecx25519_asn1_meth = {
  EVP_PKEY_X25519,
  EVP_PKEY_X25519,
  0,
  "X25519",
  "OpenSSL X25519 algorithm",
  NULL
};

const EVP_PKEY_ASN1_METHOD  ossl_ecx448_asn1_meth = {
  EVP_PKEY_X448,
  EVP_PKEY_X448,
  0,
  "X448",
  "OpenSSL X448 algorithm",
  NULL
};

const EVP_PKEY_ASN1_METHOD  ossl_ed25519_asn1_meth = {
  EVP_PKEY_ED25519,
  EVP_PKEY_ED25519,
  0,
  "ED25519",
  "OpenSSL ED25519 algorithm",
  NULL
};

const EVP_PKEY_ASN1_METHOD  ossl_ed448_asn1_meth = {
  EVP_PKEY_ED448,
  EVP_PKEY_ED448,
  0,
  "ED448",
  "OpenSSL ED448 algorithm",
  NULL
};
