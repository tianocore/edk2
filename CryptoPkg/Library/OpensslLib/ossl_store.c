/** @file
  Dummy implement ossl_store(Store retrieval functions) for UEFI.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/store.h>       /* The OSSL_STORE_INFO type numbers */

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
