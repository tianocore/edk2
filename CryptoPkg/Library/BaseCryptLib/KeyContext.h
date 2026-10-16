/** @file
  Key Context structure for EdDsa, ML-DSA and SLH-DSA APIs.

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <openssl/evp.h>

typedef struct {
  INT32       Nid;
  EVP_PKEY    *EvpPkey;
  EVP_PKEY    *EvpParam;
} KEY_CONTEXT;

/** Define an EC Key context.
*/
typedef KEY_CONTEXT EC_CONTEXT;
