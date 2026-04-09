/** @file
  Dummy implement ossl_store(Store retrieval functions) for UEFI.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/types.h>
#include <openssl/core_dispatch.h>
#include <openssl/store.h>

/*
 * This function is cleanup ossl store.
 *
 * Dummy Implement for UEFI
 */
void
ossl_store_cleanup_int (
  void
  )
{
}

int
ossl_store_loader_store_cache_flush (
  OSSL_LIB_CTX  *libctx
  )
{
  return 1;
}

int
ossl_store_loader_store_remove_all_provided (
  OSSL_PROVIDER  *prov
  )
{
  return 1;
}

static void *
file_open (
  void        *provctx,
  const char  *uri
  )
{
  return NULL;
}

const OSSL_DISPATCH  ossl_file_store_functions[] = {
  { OSSL_FUNC_STORE_OPEN, (void (*)(void)) file_open },
  { 0,                    NULL                       },
};

OSSL_STORE_CTX *
OSSL_STORE_open (
  const char                       *uri,
  const UI_METHOD                  *ui_method,
  void                             *ui_data,
  OSSL_STORE_post_process_info_fn  post_process,
  void                             *post_process_data
  )
{
  return NULL;
}

OSSL_STORE_CTX *
OSSL_STORE_open_ex (
  const char                       *uri,
  OSSL_LIB_CTX                     *libctx,
  const char                       *propq,
  const UI_METHOD                  *ui_method,
  void                             *ui_data,
  const OSSL_PARAM                 params[],
  OSSL_STORE_post_process_info_fn  post_process,
  void                             *post_process_data
  )
{
  return NULL;
}

int
OSSL_STORE_INFO_get_type (
  const OSSL_STORE_INFO  *info
  )
{
  return 0;
}

int
OSSL_STORE_find (
  OSSL_STORE_CTX           *ctx,
  const OSSL_STORE_SEARCH  *search
  )
{
  return 0;
}

OSSL_STORE_INFO *
OSSL_STORE_load (
  OSSL_STORE_CTX  *ctx
  )
{
  return NULL;
}

const char *
OSSL_STORE_INFO_get0_NAME (
  const OSSL_STORE_INFO  *info
  )
{
  return NULL;
}

X509 *
OSSL_STORE_INFO_get0_CERT (
  const OSSL_STORE_INFO  *info
  )
{
  return NULL;
}

X509_CRL *
OSSL_STORE_INFO_get0_CRL (
  const OSSL_STORE_INFO  *info
  )
{
  return NULL;
}

int
OSSL_STORE_eof (
  OSSL_STORE_CTX  *ctx
  )
{
  return 0;
}

int
OSSL_STORE_error (
  OSSL_STORE_CTX  *ctx
  )
{
  return 0;
}

int
OSSL_STORE_close (
  OSSL_STORE_CTX  *ctx
  )
{
  return 0;
}

void
OSSL_STORE_INFO_free (
  OSSL_STORE_INFO  *info
  )
{
}

OSSL_STORE_SEARCH *
OSSL_STORE_SEARCH_by_name (
  X509_NAME  *name
  )
{
  return NULL;
}

void
OSSL_STORE_SEARCH_free (
  OSSL_STORE_SEARCH  *search
  )
{
}
