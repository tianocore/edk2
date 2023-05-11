/** @file
  Null implementation of Pem functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "openssl/pem.h"

X509 *
PEM_read_bio_X509 (
  BIO              *out,
  X509             **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

int
PEM_read_bio (
  BIO            *bp,
  char           **name,
  char           **header,
  unsigned char  **data,
  long           *len
  )
{
  return -1;
}

X509 *
PEM_read_bio_X509_AUX (
  BIO              *out,
  X509             **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

EVP_PKEY *
PEM_read_bio_PrivateKey_ex (
  BIO              *bp,
  EVP_PKEY         **x,
  pem_password_cb  *cb,
  void             *u,
  OSSL_LIB_CTX     *libctx,
  const char       *propq
  )
{
  return NULL;
}

EVP_PKEY *
PEM_read_bio_PrivateKey (
  BIO              *bp,
  EVP_PKEY         **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

RSA *
PEM_read_bio_RSAPrivateKey (
  BIO              *bp,
  RSA              **rsa,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

void *
PEM_ASN1_read_bio (
  d2i_of_void      *d2i,
  const char       *name,
  BIO              *bp,
  void             **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

int
PEM_ASN1_write_bio (
  i2d_of_void          *i2d,
  const char           *name,
  BIO                  *bp,
  const void           *x,
  const EVP_CIPHER     *enc,
  const unsigned char  *kstr,
  int                  klen,
  pem_password_cb      *callback,
  void                 *u
  )
{
  return -1;
}

int
PEM_def_callback (
  char  *buf,
  int   num,
  int   rwflag,
  void  *userdata
  )
{
  return -1;
}

STACK_OF (X509_INFO) *PEM_X509_INFO_read_bio_ex (
                        BIO *bp,
                        STACK_OF (X509_INFO) *sk,
                        pem_password_cb *cb,
                        void *u,
                        OSSL_LIB_CTX *libctx,
                        const char *propq
                        ) {
  return NULL;
}
X509_CRL *
PEM_read_bio_X509_CRL (
  BIO              *out,
  X509_CRL         **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

X509_REQ *
PEM_read_bio_X509_REQ (
  BIO              *out,
  X509_REQ         **x,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

#ifndef OPENSSL_NO_EC
EC_KEY *
PEM_read_bio_ECPrivateKey (
  BIO              *bp,
  EC_KEY           **key,
  pem_password_cb  *cb,
  void             *u
  )
{
  return NULL;
}

#endif

RSA *
ossl_b2i_RSA_after_header (
  const unsigned char  **in,
  unsigned int         bitlen,
  int                  ispub
  )
{
  return NULL;
}

int
ossl_do_blob_header (
  const unsigned char  **in,
  unsigned int         length,
  unsigned int         *pmagic,
  unsigned int         *pbitlen,
  int                  *pisdss,
  int                  *pispub
  )
{
  return -1;
}

unsigned int
ossl_blob_length (
  unsigned  bitlen,
  int       isdss,
  int       ispub
  )
{
  return 0;
}

RSA *
b2i_RSA_PVK_bio_ex (
  BIO              *in,
  pem_password_cb  *cb,
  void             *u,
  OSSL_LIB_CTX     *libctx,
  const char       *propq
  )
{
  return NULL;
}

int
PEM_get_EVP_CIPHER_INFO (
  char             *header,
  EVP_CIPHER_INFO  *cipher
  )
{
  return -1;
}

int
PEM_do_header (
  EVP_CIPHER_INFO  *cipher,
  unsigned char    *data,
  long             *plen,
  pem_password_cb  *callback,
  void             *u
  )
{
  return -1;
}
