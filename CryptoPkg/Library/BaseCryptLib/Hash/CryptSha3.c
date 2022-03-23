/** @file
  SHA3 realted functions from OpenSSL.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Copyright 2022 The OpenSSL Project Authors. All Rights Reserved.
Licensed under the OpenSSL license (the "License").  You may not use
this file except in compliance with the License.  You can obtain a copy
in the file LICENSE in the source distribution or at
https://www.openssl.org/source/license.html
**/

#include "CryptParallelHash.h"

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
  IN  UINTN           MessageDigestLen
  )
{
  if (BlockSize <= sizeof (Context->buf)) {
    memset (Context->A, 0, sizeof (Context->A));

    Context->num        = 0;
    Context->block_size = BlockSize;
    Context->md_size    = MessageDigestLen;
    Context->pad        = Pad;

    return 1;
  }

  return 0;
}

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
  )
{
  const UINT8  *DataCopy;
  UINTN        BlockSize;
  UINTN        Num;
  UINTN        Rem;

  DataCopy  = Data;
  BlockSize = (UINT8)(Context->block_size);

  if (DataSize == 0) {
    return 1;
  }

  if ((Num = Context->num) != 0) {
    //
    // process intermediate buffer
    //
    Rem = BlockSize - Num;

    if (DataSize < Rem) {
      memcpy (Context->buf + Num, DataCopy, DataSize);
      Context->num += DataSize;
      return 1;
    }

    //
    // We have enough data to fill or overflow the intermediate
    // buffer. So we append |Rem| bytes and process the block,
    // leaving the rest for later processing.
    //
    memcpy (Context->buf + Num, DataCopy, Rem);
    DataCopy += Rem;
    DataSize -= Rem;
    (void)SHA3_absorb (Context->A, Context->buf, BlockSize, BlockSize);
    Context->num = 0;
    // Context->buf is processed, Context->num is guaranteed to be zero.
  }

  if (DataSize >= BlockSize) {
    Rem = SHA3_absorb (Context->A, DataCopy, DataSize, BlockSize);
  } else {
    Rem = DataSize;
  }

  if (Rem > 0) {
    memcpy (Context->buf, DataCopy + DataSize - Rem, Rem);
    Context->num = Rem;
  }

  return 1;
}

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
  )
{
  UINTN  BlockSize;
  UINTN  Num;

  BlockSize = Context->block_size;
  Num       = Context->num;

  if (Context->md_size == 0) {
    return 1;
  }

  //
  // Pad the data with 10*1. Note that |Num| can be |BlockSize - 1|
  // in which case both byte operations below are performed on
  // same byte.
  //
  memset (Context->buf + Num, 0, BlockSize - Num);
  Context->buf[Num]            = Context->pad;
  Context->buf[BlockSize - 1] |= 0x80;

  (void)SHA3_absorb (Context->A, Context->buf, BlockSize, BlockSize);

  SHA3_squeeze (Context->A, MessageDigest, Context->md_size, BlockSize);

  return 1;
}
