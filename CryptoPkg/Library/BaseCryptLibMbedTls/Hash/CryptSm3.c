/** @file
  SM3 Digest Implementations for MbedTLS based BaseCryptLib.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2024, Google LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Implementation based on http://www.gmbz.org.cn/upload/2018-07-24/1532401392982079739.pdf
**/

#include "InternalCryptLib.h"

#define SM3_BLOCK_SIZE  64

#define SM3_T1  0x79cc4519
#define SM3_T2  0x7a879d8a

STATIC CONST UINT32  mSm3Iv[] = {
  0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600, 0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
};

typedef struct {
  /// The SM3 state
  UINT32    State[8];

  /// The total number of consumed bytes to produce the digest
  UINTN     TotalInputSize;

  /// The number of buffered input bytes
  UINTN     NumBufferedBytes;

  /// Input buffer for incremental hashing
  UINT8     Buffer[SM3_BLOCK_SIZE];
} SM3_CTX;

/**
  Retrieves the size, in bytes, of the context buffer required for SM3 hash operations.

  @return  The size, in bytes, of the context buffer required for SM3 hash operations.

**/
UINTN
EFIAPI
Sm3GetContextSize (
  VOID
  )
{
  //
  // Retrieves Openssl SM3 Context Size
  //
  return (UINTN)(sizeof (SM3_CTX));
}

/**
  Initializes user-supplied memory pointed by Sm3Context as SM3 hash context for
  subsequent use.

  If Sm3Context is NULL, then return FALSE.

  @param[out]  Sm3Context  Pointer to SM3 context being initialized.

  @retval TRUE   SM3 context initialization succeeded.
  @retval FALSE  SM3 context initialization failed.

**/
BOOLEAN
EFIAPI
Sm3Init (
  OUT  VOID  *Sm3Context
  )
{
  SM3_CTX  *Ctx;

  //
  // Check input parameters.
  //
  if (Sm3Context == NULL) {
    return FALSE;
  }

  Ctx = (SM3_CTX *)Sm3Context;

  CopyMem (Ctx->State, mSm3Iv, sizeof (mSm3Iv));
  Ctx->TotalInputSize   = 0;
  Ctx->NumBufferedBytes = 0;

  return TRUE;
}

/**
  Makes a copy of an existing SM3 context.

  If Sm3Context is NULL, then return FALSE.
  If NewSm3Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  Sm3Context     Pointer to SM3 context being copied.
  @param[out] NewSm3Context  Pointer to new SM3 context.

  @retval TRUE   SM3 context copy succeeded.
  @retval FALSE  SM3 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sm3Duplicate (
  IN   CONST VOID  *Sm3Context,
  OUT  VOID        *NewSm3Context
  )
{
  //
  // Check input parameters.
  //
  if ((Sm3Context == NULL) || (NewSm3Context == NULL)) {
    return FALSE;
  }

  CopyMem (NewSm3Context, Sm3Context, sizeof (SM3_CTX));

  return TRUE;
}

/**
  Apply the P0 transformation from the SM3 spec.

  @param[in]        X       Input word

  @retval                   Output word
**/
STATIC
UINT32
P0 (
  IN  UINT32  X
  )
{
  return X ^ LRotU32 (X, 9) ^ LRotU32 (X, 17);
}

/**
  Apply the P1 transformation from the SM3 spec.

  @param[in]        X       Input word

  @retval                   Output word
**/
STATIC
UINT32
P1 (
  IN  UINT32  X
  )
{
  return X ^ LRotU32 (X, 15) ^ LRotU32 (X, 23);
}

/**
  Apply the FF transformation from the SM3 spec.

  @param[in]   Index      Round index
  @param[in]   X          First input word
  @param[in]   Y          Second input word
  @param[in]   Z          Third input word

  @retval                 Output word
**/
STATIC
UINT32
FF (
  IN  UINTN   Index,
  IN  UINT32  X,
  IN  UINT32  Y,
  IN  UINT32  Z
  )
{
  if (Index < 16) {
    return X ^ Y ^ Z;
  }

  // simplified (X & Y) | (X & Z) | (Y & Z)
  return (X & Y) | ((X | Y) & Z);
}

/**
  Apply the FF transformation from the SM3 spec.

  @param[in]   Index      Round index
  @param[in]   X          First input word
  @param[in]   Y          Second input word
  @param[in]   Z          Third input word

  @retval                 Output word
**/
STATIC
UINT32
GG (
  IN  UINTN   Index,
  IN  UINT32  X,
  IN  UINT32  Y,
  IN  UINT32  Z
  )
{
  if (Index < 16) {
    return X ^ Y ^ Z;
  }

  // simplified (X & Y) | (~X & Z)
  return (X & (Y ^ Z)) ^ Z;
}

/**
  Apply the core SM3 transform to a single block of data.

  @param[in, out]   State   The SM3 state to be updated.
  @param[in]        Data    The input data to apply SM3 to
**/
STATIC
VOID
Sm3Transform (
  IN OUT  UINT32       *State,
  IN      CONST UINT8  *Data
  )
{
  UINT32  W[68];
  UINT32  SS1;
  UINT32  SS2;
  UINT32  TT1;
  UINT32  TT2;
  UINT32  A, B, C, D, E, F, G, H;
  UINT32  T;
  UINTN   Index;

  for (Index = 0; Index < 16; Index++) {
    W[Index] = SwapBytes32 (ReadUnaligned32 (&((UINT32 *)Data)[Index]));
  }

  for (Index = 16; Index < ARRAY_SIZE (W); Index++) {
    W[Index]  = P1 (W[Index - 16] ^ W[Index - 9] ^ LRotU32 (W[Index - 3], 15));
    W[Index] ^= LRotU32 (W[Index - 13], 7) ^ W[Index - 6];
  }

  A = State[0];
  B = State[1];
  C = State[2];
  D = State[3];
  E = State[4];
  F = State[5];
  G = State[6];
  H = State[7];

  for (Index = 0, T = SM3_T1; Index < 64; Index++) {
    if (Index == 16) {
      T = SM3_T2;
    }

    SS1 = LRotU32 (LRotU32 (A, 12) + E + LRotU32 (T, Index % 32), 7);
    SS2 = SS1 ^ LRotU32 (A, 12);
    TT1 = FF (Index, A, B, C) + D + SS2 + (W[Index] ^ W[Index + 4]);
    TT2 = GG (Index, E, F, G) + H + SS1 + W[Index];

    D   = C;
    C   = LRotU32 (B, 9);
    B   = A;
    A   = TT1;
    H   = G;
    G   = LRotU32 (F, 19);
    F   = E;
    E   = P0 (TT2);
  }

  State[0] ^= A;
  State[1] ^= B;
  State[2] ^= C;
  State[3] ^= D;
  State[4] ^= E;
  State[5] ^= F;
  State[6] ^= G;
  State[7] ^= H;
}

/**
  Digests the input data and updates SM3 context.

  This function performs SM3 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be finalized
  by Sm3Final(). Behavior with invalid context is undefined.

  If Sm3Context is NULL, then return FALSE.

  @param[in, out]  Sm3Context     Pointer to the SM3 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize       Size of Data buffer in bytes.

  @retval TRUE   SM3 data digest succeeded.
  @retval FALSE  SM3 data digest failed.

**/
BOOLEAN
EFIAPI
Sm3Update (
  IN OUT  VOID        *Sm3Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  )
{
  SM3_CTX  *Ctx;
  UINTN    NumBufferedBytes;

  //
  // Check input parameters.
  //
  if (Sm3Context == NULL) {
    return FALSE;
  }

  Ctx                  = (SM3_CTX *)Sm3Context;
  Ctx->TotalInputSize += DataSize;

  //
  // Combine the already buffered input with the presented input to
  // fill a full block.
  //
  if (Ctx->NumBufferedBytes > 0) {
    NumBufferedBytes = MIN (DataSize, SM3_BLOCK_SIZE - Ctx->NumBufferedBytes);
    CopyMem (&Ctx->Buffer[Ctx->NumBufferedBytes], Data, NumBufferedBytes);
    Ctx->NumBufferedBytes += NumBufferedBytes;

    if (Ctx->NumBufferedBytes < SM3_BLOCK_SIZE) {
      return TRUE;
    }

    Sm3Transform (Ctx->State, Ctx->Buffer);
    Ctx->NumBufferedBytes = 0;

    Data     += NumBufferedBytes;
    DataSize -= NumBufferedBytes;
  }

  //
  // Apply the SM3 transform directly on as much input data as we can consume
  // as full blocks
  //
  while (DataSize >= SM3_BLOCK_SIZE) {
    Sm3Transform (Ctx->State, Data);

    Data     += SM3_BLOCK_SIZE;
    DataSize -= SM3_BLOCK_SIZE;
  }

  //
  // Copy the remaining data into the buffer
  //
  if (DataSize > 0) {
    CopyMem (Ctx->Buffer, Data, DataSize);
    Ctx->NumBufferedBytes = DataSize;
  }

  return TRUE;
}

/**
  Completes computation of the SM3 digest value.

  This function completes SM3 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the SM3 context cannot
  be used again.
  SM3 context should be already correctly initialized by Sm3Init(), and should not be
  finalized by Sm3Final(). Behavior with invalid SM3 context is undefined.

  If Sm3Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sm3Context     Pointer to the SM3 context.
  @param[out]      HashValue      Pointer to a buffer that receives the SM3 digest
                                  value (32 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.

**/
BOOLEAN
EFIAPI
Sm3Final (
  IN OUT  VOID   *Sm3Context,
  OUT     UINT8  *HashValue
  )
{
  SM3_CTX  *Ctx;
  UINTN    AvailableBytes;
  UINTN    Index;

  //
  // Check input parameters.
  //
  if ((Sm3Context == NULL) || (HashValue == NULL)) {
    return FALSE;
  }

  Ctx = (SM3_CTX *)Sm3Context;

  // The buffer is always processed directly once it is full, so we must have
  // at least one byte of space when we end up heere
  ASSERT (Ctx->NumBufferedBytes < SM3_BLOCK_SIZE);

  // Apply the input termination
  Ctx->Buffer[Ctx->NumBufferedBytes++] = 0x80;

  // Check if there is space left for a single UINT64 carrying the size of the
  // entire input
  AvailableBytes = SM3_BLOCK_SIZE - Ctx->NumBufferedBytes;
  if (AvailableBytes < sizeof (UINT64)) {
    ZeroMem (&Ctx->Buffer[Ctx->NumBufferedBytes], AvailableBytes);
    Sm3Transform (Ctx->State, Ctx->Buffer);

    AvailableBytes        = SM3_BLOCK_SIZE;
    Ctx->NumBufferedBytes = 0;
  }

  ZeroMem (&Ctx->Buffer[Ctx->NumBufferedBytes], AvailableBytes - sizeof (UINT64));
  *(UINT64 *)&Ctx->Buffer[SM3_BLOCK_SIZE - sizeof (UINT64)] = SwapBytes64 (Ctx->TotalInputSize << 3);
  Sm3Transform (Ctx->State, Ctx->Buffer);

  for (Index = 0; Index < ARRAY_SIZE (Ctx->State); Index++) {
    WriteUnaligned32 (&((UINT32 *)HashValue)[Index], SwapBytes32 (Ctx->State[Index]));
  }

  return TRUE;
}

/**
  Computes the SM3 message digest of a input data buffer.

  This function performs the SM3 message digest of a given data buffer, and places
  the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the SM3 digest
                           value (32 bytes).

  @retval TRUE   SM3 digest computation succeeded.
  @retval FALSE  SM3 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sm3HashAll (
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  )
{
  SM3_CTX  Ctx;

  return Sm3Init (&Ctx) && Sm3Update (&Ctx, Data, DataSize) && Sm3Final (&Ctx, HashValue);
}
