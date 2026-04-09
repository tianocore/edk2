/** @file
  Null implementation of SSL functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

int
OPENSSL_init_ssl (
  uint64_t                     opts,
  const OPENSSL_INIT_SETTINGS  *settings
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur uint32_t
SSL_CIPHER_get_id (
  const SSL_CIPHER  *c
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_COMP_add_compression_method (
  int          id,
  COMP_METHOD  *cm
  )
{
  ASSERT (FALSE);
  return 0;
}

long
SSL_CTX_ctrl (
  SSL_CTX  *ctx,
  int      cmd,
  long     larg,
  void     *parg
  )
{
  ASSERT (FALSE);
  return 0;
}

void
SSL_CTX_free (
  SSL_CTX  *x
  )
{
  ASSERT (FALSE);
  return;
}

__owur X509_STORE *
SSL_CTX_get_cert_store (
  const SSL_CTX  *x
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur SSL_CTX *
SSL_CTX_new (
  const SSL_METHOD  *meth
  )
{
  ASSERT (FALSE);
  return NULL;
}

uint64_t
SSL_CTX_set_options (
  SSL_CTX   *ctx,
  uint64_t  op
  )
{
  ASSERT (FALSE);
  return 0;
}

const unsigned char *
SSL_SESSION_get_id (
  const SSL_SESSION  *s,
  unsigned int       *len
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur size_t
SSL_SESSION_get_master_key (
  const SSL_SESSION  *sess,
  unsigned char      *out,
  size_t             outlen
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_SESSION_set1_id (
  SSL_SESSION          *s,
  const unsigned char  *sid,
  unsigned int         sid_len
  )
{
  ASSERT (FALSE);
  return 0;
}

long
SSL_ctrl (
  SSL   *ssl,
  int   cmd,
  long  larg,
  void  *parg
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_do_handshake (
  SSL  *s
  )
{
  ASSERT (FALSE);
  return 0;
}

void
SSL_free (
  SSL  *ssl
  )
{
  ASSERT (FALSE);
  return;
}

__owur X509 *
SSL_get_certificate (
  const SSL  *ssl
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur size_t
SSL_get_client_random (
  const SSL      *ssl,
  unsigned char  *out,
  size_t         outlen
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur const SSL_CIPHER *
SSL_get_current_cipher (
  const SSL  *s
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur int
SSL_get_error (
  const SSL  *s,
  int        ret_code
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur size_t
SSL_get_server_random (
  const SSL      *ssl,
  unsigned char  *out,
  size_t         outlen
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur SSL_SESSION *
SSL_get_session (
  const SSL  *ssl
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur SSL_CTX *
SSL_get_SSL_CTX (
  const SSL  *ssl
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur OSSL_HANDSHAKE_STATE
SSL_get_state (
  const SSL  *ssl
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_get_verify_mode (
  const SSL  *s
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur X509_VERIFY_PARAM *
SSL_get0_param (
  SSL  *ssl
  )
{
  ASSERT (FALSE);
  return NULL;
}

int
SSL_is_init_finished (
  const SSL  *s
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_is_server (
  const SSL  *s
  )
{
  ASSERT (FALSE);
  return 0;
}

SSL *
SSL_new (
  SSL_CTX  *ctx
  )
{
  ASSERT (FALSE);
  return NULL;
}

__owur int
SSL_read (
  SSL   *ssl,
  void  *buf,
  int   num
  )
{
  ASSERT (FALSE);
  return 0;
}

void
SSL_set_bio (
  SSL  *s,
  BIO  *rbio,
  BIO  *wbio
  )
{
  ASSERT (FALSE);
  return;
}

__owur int
SSL_set_cipher_list (
  SSL         *s,
  const char  *str
  )
{
  ASSERT (FALSE);
  return 0;
}

void
SSL_set_connect_state (
  SSL  *s
  )
{
  ASSERT (FALSE);
  return;
}

void
SSL_set_hostflags (
  SSL           *s,
  unsigned int  flags
  )
{
  ASSERT (FALSE);
  return;
}

void
SSL_set_info_callback (
  SSL *ssl,
  void ( *cb )(const SSL *ssl, int type, int val)
  )
{
  ASSERT (FALSE);
  return;
}

void
SSL_set_security_level (
  SSL  *s,
  int  level
  )
{
  ASSERT (FALSE);
  return;
}

void
SSL_set_verify (
  SSL            *s,
  int            mode,
  SSL_verify_cb  callback
  )
{
  ASSERT (FALSE);
  return;
}

int
SSL_shutdown (
  SSL  *s
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_use_certificate (
  SSL   *ssl,
  X509  *x
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_version (
  const SSL  *ssl
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur int
SSL_write (
  SSL         *ssl,
  const void  *buf,
  int         num
  )
{
  ASSERT (FALSE);
  return 0;
}

__owur const SSL_METHOD *
TLS_client_method (
  void
  )
{
  ASSERT (FALSE);
  return NULL;
}
