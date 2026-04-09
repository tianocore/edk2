/** @file
  Null implementation of PKCS12 and PKCS8 functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/pkcs12.h>

int
PKCS12_PBE_keyivgen_ex (
  EVP_CIPHER_CTX    *ctx,
  const char        *pass,
  int               passlen,
  ASN1_TYPE         *param,
  const EVP_CIPHER  *cipher,
  const EVP_MD      *md,
  int               en_de,
  OSSL_LIB_CTX      *libctx,
  const char        *propq
  )
{
  return -1;
}

int
PKCS12_PBE_keyivgen (
  EVP_CIPHER_CTX    *ctx,
  const char        *pass,
  int               passlen,
  ASN1_TYPE         *param,
  const EVP_CIPHER  *cipher,
  const EVP_MD      *md,
  int               en_de
  )
{
  return -1;
}

X509_SIG *
PKCS8_encrypt (
  int                  pbe_nid,
  const EVP_CIPHER     *cipher,
  const char           *pass,
  int                  passlen,
  unsigned char        *salt,
  int                  saltlen,
  int                  iter,
  PKCS8_PRIV_KEY_INFO  *p8inf
  )
{
  return NULL;
}

PKCS8_PRIV_KEY_INFO *
PKCS8_decrypt (
  const X509_SIG  *p8,
  const char      *pass,
  int             passlen
  )
{
  return NULL;
}

unsigned char *
PKCS12_pbe_crypt_ex (
  const X509_ALGOR     *algor,
  const char           *pass,
  int                  passlen,
  const unsigned char  *in,
  int                  inlen,
  unsigned char        **data,
  int                  *datalen,
  int                  en_de,
  OSSL_LIB_CTX         *libctx,
  const char           *propq
  )
{
  return NULL;
}

X509_SIG *
PKCS8_encrypt_ex (
  int                  pbe_nid,
  const EVP_CIPHER     *cipher,
  const char           *pass,
  int                  passlen,
  unsigned char        *salt,
  int                  saltlen,
  int                  iter,
  PKCS8_PRIV_KEY_INFO  *p8inf,
  OSSL_LIB_CTX         *libctx,
  const char           *propq
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_it (
  void
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_MAC_DATA_it (
  void
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_SAFEBAG_it (
  void
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_BAGS_it (
  void
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_AUTHSAFES_it (
  void
  )
{
  return NULL;
}

const ASN1_ITEM *
PKCS12_SAFEBAGS_it (
  void
  )
{
  return NULL;
}
