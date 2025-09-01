/** @file
  Null implementation of ENCODER functions called by BaseCryptLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/encoder.h>

OSSL_ENCODER *
OSSL_ENCODER_fetch (
  OSSL_LIB_CTX  *libctx,
  const char    *name,
  const char    *properties
  )
{
  return NULL;
}

int
OSSL_ENCODER_up_ref (
  OSSL_ENCODER  *encoder
  )
{
  return 0;
}

void
OSSL_ENCODER_free (
  OSSL_ENCODER  *encoder
  )
{
}

const OSSL_PROVIDER *
OSSL_ENCODER_get0_provider (
  const OSSL_ENCODER  *encoder
  )
{
  return NULL;
}

const char *
OSSL_ENCODER_get0_properties (
  const OSSL_ENCODER  *encoder
  )
{
  return NULL;
}

const char *
OSSL_ENCODER_get0_name (
  const OSSL_ENCODER  *kdf
  )
{
  return NULL;
}

const char *
OSSL_ENCODER_get0_description (
  const OSSL_ENCODER  *kdf
  )
{
  return NULL;
}

int
OSSL_ENCODER_is_a (
  const OSSL_ENCODER  *encoder,
  const char          *name
  )
{
  return 0;
}

void
OSSL_ENCODER_do_all_provided (
  OSSL_LIB_CTX *libctx,
  void ( *fn )(OSSL_ENCODER *encoder, void *arg),
  void *arg
  )
{
}

int
OSSL_ENCODER_names_do_all (
  const OSSL_ENCODER *encoder,
  void ( *fn )(const char *name, void *data),
  void *data
  )
{
  return 0;
}

const OSSL_PARAM *
OSSL_ENCODER_gettable_params (
  OSSL_ENCODER  *encoder
  )
{
  return NULL;
}

int
OSSL_ENCODER_get_params (
  OSSL_ENCODER  *encoder,
  OSSL_PARAM    params[]
  )
{
  return 0;
}

const OSSL_PARAM *
OSSL_ENCODER_settable_ctx_params (
  OSSL_ENCODER  *encoder
  )
{
  return NULL;
}

OSSL_ENCODER_CTX *
OSSL_ENCODER_CTX_new (
  void
  )
{
  return NULL;
}

int
OSSL_ENCODER_CTX_set_params (
  OSSL_ENCODER_CTX  *ctx,
  const OSSL_PARAM  params[]
  )
{
  return 0;
}

void
OSSL_ENCODER_CTX_free (
  OSSL_ENCODER_CTX  *ctx
  )
{
}

/* Utilities that help set specific parameters */
int
OSSL_ENCODER_CTX_set_passphrase (
  OSSL_ENCODER_CTX     *ctx,
  const unsigned char  *kstr,
  size_t               klen
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_pem_password_cb (
  OSSL_ENCODER_CTX  *ctx,
  pem_password_cb   *cb,
  void              *cbarg
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_passphrase_cb (
  OSSL_ENCODER_CTX          *ctx,
  OSSL_PASSPHRASE_CALLBACK  *cb,
  void                      *cbarg
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_passphrase_ui (
  OSSL_ENCODER_CTX  *ctx,
  const UI_METHOD   *ui_method,
  void              *ui_data
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_cipher (
  OSSL_ENCODER_CTX  *ctx,
  const char        *cipher_name,
  const char        *propquery
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_selection (
  OSSL_ENCODER_CTX  *ctx,
  int               selection
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_output_type (
  OSSL_ENCODER_CTX  *ctx,
  const char        *output_type
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_output_structure (
  OSSL_ENCODER_CTX  *ctx,
  const char        *output_structure
  )
{
  return 0;
}

/* Utilities to add encoders */
int
OSSL_ENCODER_CTX_add_encoder (
  OSSL_ENCODER_CTX  *ctx,
  OSSL_ENCODER      *encoder
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_add_extra (
  OSSL_ENCODER_CTX  *ctx,
  OSSL_LIB_CTX      *libctx,
  const char        *propq
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_get_num_encoders (
  OSSL_ENCODER_CTX  *ctx
  )
{
  return 0;
}

OSSL_ENCODER *
OSSL_ENCODER_INSTANCE_get_encoder (
  OSSL_ENCODER_INSTANCE  *encoder_inst
  )
{
  return NULL;
}

void *
OSSL_ENCODER_INSTANCE_get_encoder_ctx (
  OSSL_ENCODER_INSTANCE  *encoder_inst
  )
{
  return NULL;
}

const char *
OSSL_ENCODER_INSTANCE_get_output_type (
  OSSL_ENCODER_INSTANCE  *encoder_inst
  )
{
  return NULL;
}

const char *
OSSL_ENCODER_INSTANCE_get_output_structure (
  OSSL_ENCODER_INSTANCE  *encoder_inst
  )
{
  return NULL;
}

int
OSSL_ENCODER_CTX_set_construct (
  OSSL_ENCODER_CTX        *ctx,
  OSSL_ENCODER_CONSTRUCT  *construct
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_construct_data (
  OSSL_ENCODER_CTX  *ctx,
  void              *construct_data
  )
{
  return 0;
}

int
OSSL_ENCODER_CTX_set_cleanup (
  OSSL_ENCODER_CTX      *ctx,
  OSSL_ENCODER_CLEANUP  *cleanup
  )
{
  return 0;
}

/* Utilities to output the object to encode */
int
OSSL_ENCODER_to_bio (
  OSSL_ENCODER_CTX  *ctx,
  BIO               *out
  )
{
  return 0;
}

#ifndef OPENSSL_NO_STDIO
int
OSSL_ENCODER_to_fp (
  OSSL_ENCODER_CTX  *ctx,
  FILE              *fp
  );

#endif
int
OSSL_ENCODER_to_data (
  OSSL_ENCODER_CTX  *ctx,
  unsigned char     **pdata,
  size_t            *pdata_len
  )
{
  return 0;
}

OSSL_ENCODER_CTX *
OSSL_ENCODER_CTX_new_for_pkey (
  const EVP_PKEY  *pkey,
  int             selection,
  const char      *output_type,
  const char      *output_struct,
  const char      *propquery
  )
{
  return NULL;
}

int
ossl_encoder_store_remove_all_provided (
  const OSSL_PROVIDER  *prov
  )
{
  return -1;
}

int
ossl_encoder_store_cache_flush (
  OSSL_LIB_CTX  *libctx
  )
{
  return -1;
}

int
ossl_bio_print_labeled_buf (
  BIO                  *out,
  const char           *label,
  const unsigned char  *buf,
  size_t               buflen
  )
{
  return -1;
}
