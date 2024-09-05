/*
 * Copyright 2017-2021 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright 2017 Ribose Inc. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 *
 * Taken from OpenSSL release 3.0.9.
 */

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

typedef UINTN size_t;

#define SM3_DIGEST_LENGTH  32
#define SM3_WORD           unsigned int

#define SM3_CBLOCK  64
#define SM3_LBLOCK  (SM3_CBLOCK/4)

typedef struct SM3state_st {
  SM3_WORD    A, B, C, D, E, F, G, H;

  UINTN       TotalInputSize;
  UINTN       NumBufferedBytes;
  UINT8       Buffer[SM3_CBLOCK];
} SM3_CTX;

int
ossl_sm3_init (
  SM3_CTX  *c
  );

void
ossl_sm3_block_data_order (
  SM3_CTX     *ctx,
  const void  *p,
  size_t      num
  );
