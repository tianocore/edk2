/** @file
  ParallelHash related function and type declaration.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
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

#include "InternalCryptLib.h"

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
  SHA3_absorb can be called multiple times, but at each invocation
  largest multiple of |r| out of |len| bytes are processed. Then
  remaining amount of bytes is returned. This is done to spare caller
  trouble of calculating the largest multiple of |r|. |r| can be viewed
  as blocksize. It is commonly (1600 - 256*n)/8, e.g. 168, 136, 104,
  72, but can also be (1600 - 448)/8 = 144. All this means that message
  padding and intermediate sub-block buffering, byte- or bitwise, is
  caller's responsibility.
**/
size_t
SHA3_absorb (
  uint64_t             A[5][5],
  const unsigned char  *inp,
  size_t               len,
  size_t               r
  );

/**
  SHA3_squeeze is called once at the end to generate |out| hash value
  of |len| bytes.
**/
void
SHA3_squeeze (
  uint64_t       A[5][5],
  unsigned char  *out,
  size_t         len,
  size_t         r
  );

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the beginning of the string by inserting the length of the byte string
  before the byte string representation of input.

  @param[out] EncBuf  Result of left encode.
  @param[in]  Value   Input of left encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
LeftEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  );

/**
  Encode function from XKCP.

  Encodes the input as a byte string in a way that can be unambiguously parsed
  from the end of the string by inserting the length of the byte string after
  the byte string representation of input.

  @param[out] EncBuf  Result of right encode.
  @param[in]  Value   Input of right encode.

  @retval EncLen  Size of encode result in bytes.
**/
UINTN
EFIAPI
RightEncode (
  OUT UINT8  *EncBuf,
  IN  UINTN  Value
  );

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

/**
  Computes the CSHAKE-256 message digest of a input data buffer.

  This function performs the CSHAKE-256 message digest of a given data buffer, and places
  the digest value into the specified memory.

  @param[in]   Data               Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize           Size of Data buffer in bytes.
  @param[in]   OutputLen          Size of output in bytes.
  @param[in]   Name               Pointer to the function name string.
  @param[in]   NameLen            Size of the function name in bytes.
  @param[in]   Customization      Pointer to the customization string.
  @param[in]   CustomizationLen   Size of the customization string in bytes.
  @param[out]  HashValue          Pointer to a buffer that receives the CSHAKE-256 digest
                                  value.

  @retval TRUE   CSHAKE-256 digest computation succeeded.
  @retval FALSE  CSHAKE-256 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
CShake256HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  IN   UINTN       OutputLen,
  IN   CONST VOID  *Name,
  IN   UINTN       NameLen,
  IN   CONST VOID  *Customization,
  IN   UINTN       CustomizationLen,
  OUT  UINT8       *HashValue
  );

/**
  Complete computation of digest of each block.

  Each AP perform the function called by BSP.

  @param[in] ProcedureArgument Argument of the procedure.
**/
VOID
EFIAPI
ParallelHashApExecute (
  IN VOID  *ProcedureArgument
  );

/**
  Dispatch the block task to each AP.

**/
VOID
EFIAPI
DispatchBlockToAp (
  VOID
  );
