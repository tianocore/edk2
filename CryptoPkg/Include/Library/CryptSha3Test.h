/** @file
  ParallelHash related function and type declaration.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Copyright 2022 The OpenSSL Project Authors. All Rights Reserved.
Licensed under the OpenSSL license (the "License").  You may not use
this file except in compliance with the License.  You can obtain a copy
in the file LICENSE in the source distribution or at
https://www.openssl.org/source/license.html

Copyright 2022 The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP
Keccak, designed by Guido Bertoni, Joan Daemen, Michael Peeters and Gilles Van Assche.
Implementation by the designers, hereby denoted as "the implementer".
For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/
To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
**/

#ifndef __CRYPT_SHA3_TEST_
#define __CRYPT_SHA3_TEST_

#include <CrtLibSupport.h>

#define KECCAK1600_WIDTH  1600

typedef UINT64 uint64_t;

//
// This struct referring to m_sha3.c from opessl and modified its type name.
//
typedef struct {
  uint64_t         A[5][5];
  size_t           block_size;  /* cached ctx->digest->block_size */
  size_t           md_size;     /* output length, variable in XOF */
  size_t           num;         /* used bytes in below buffer */
  unsigned char    buf[KECCAK1600_WIDTH / 8 - 32];
  unsigned char    pad;
} Keccak1600_Ctx;

/**
  Keccak initial fuction.

  Set up state with specified capacity.

  @param[out] Context           Pointer to the context being initialized.
  @param[in]  Pad               Delimited Suffix.
  @param[in]  BlockSize         Size of context block.
  @param[in]  MessageDigestLen  Size of message digest in bytes.

  @retval 1  Initialize successfully.
  @retval 0  Fail to initialize.
**/
UINT8
EFIAPI
KeccakInit (
  OUT Keccak1600_Ctx  *Context,
  IN  UINT8           Pad,
  IN  UINTN           BlockSize,
  IN  UINTN           MessageDigstLen
  );

/**
  Sha3 update fuction.

  This function performs Sha3 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.

  @param[in,out] Context   Pointer to the Keccak context.
  @param[in]     Data      Pointer to the buffer containing the data to be hashed.
  @param[in]     DataSize  Size of Data buffer in bytes.

  @retval 1  Update successfully.
**/
UINT8
EFIAPI
Sha3Update (
  IN OUT Keccak1600_Ctx  *Context,
  IN const VOID          *Data,
  IN UINTN               DataSize
  );

/**
  Completes computation of Sha3 message digest.

  This function completes sha3 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the keccak context cannot
  be used again.

  @param[in, out]  Context        Pointer to the keccak context.
  @param[out]      MessageDigest  Pointer to a buffer that receives the message digest.

  @retval 1   Meaasge digest computation succeeded.
**/
UINT8
EFIAPI
Sha3Final (
  IN OUT Keccak1600_Ctx  *Context,
  OUT    UINT8           *MessageDigest
  );

#endif
