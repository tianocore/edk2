/** @file
  SM3 Digest Implementations for MbedTLS based BaseCryptLib.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2024, Google LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include "Sm3Core.h"

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
  //
  // Check input parameters.
  //
  if (Sm3Context == NULL) {
    return FALSE;
  }

  //
  // Openssl SM3 Context Initialization
  //
  ossl_sm3_init ((SM3_CTX *)Sm3Context);
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
  SM3_CTX      *Ctx;
  CONST UINT8  *Input;
  UINTN        NumBytes;
  UINTN        NumBlocks;

  //
  // Check input parameters.
  //
  if (Sm3Context == NULL) {
    return FALSE;
  }

  Input                = Data;
  Ctx                  = (SM3_CTX *)Sm3Context;
  Ctx->TotalInputSize += DataSize;

  //
  // Combine the already buffered input with the presented input to
  // fill a full block.
  //
  if (Ctx->NumBufferedBytes > 0) {
    NumBytes = MIN (DataSize, SM3_CBLOCK - Ctx->NumBufferedBytes);
    CopyMem (&Ctx->Buffer[Ctx->NumBufferedBytes], Input, NumBytes);
    Ctx->NumBufferedBytes += NumBytes;

    if (Ctx->NumBufferedBytes < SM3_CBLOCK) {
      return TRUE;
    }

    ossl_sm3_block_data_order (Ctx, Ctx->Buffer, 1);
    Ctx->NumBufferedBytes = 0;

    Input    += NumBytes;
    DataSize -= NumBytes;
  }

  //
  // Apply the SM3 transform directly on as much input data as we can consume
  // as full blocks
  //
  if (DataSize >= SM3_CBLOCK) {
    NumBlocks = DataSize / SM3_CBLOCK;
    ossl_sm3_block_data_order (Ctx, Input, NumBlocks);

    Input    += NumBlocks * SM3_CBLOCK;
    DataSize %= SM3_CBLOCK;
  }

  //
  // Copy the remaining data into the buffer
  //
  if (DataSize > 0) {
    CopyMem (Ctx->Buffer, Input, DataSize);
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

  //
  // Check input parameters.
  //
  if ((Sm3Context == NULL) || (HashValue == NULL)) {
    return FALSE;
  }

  Ctx = (SM3_CTX *)Sm3Context;

  // The buffer is always processed directly once it is full, so we must have
  // at least one byte of space when we end up heere
  ASSERT (Ctx->NumBufferedBytes < SM3_CBLOCK);

  // Apply the input termination
  Ctx->Buffer[Ctx->NumBufferedBytes++] = 0x80;

  // Check if there is space left for a single UINT64 carrying the size of the
  // entire input
  AvailableBytes = SM3_CBLOCK - Ctx->NumBufferedBytes;
  if (AvailableBytes < sizeof (UINT64)) {
    ZeroMem (&Ctx->Buffer[Ctx->NumBufferedBytes], AvailableBytes);
    ossl_sm3_block_data_order (Ctx, Ctx->Buffer, 1);

    AvailableBytes        = SM3_CBLOCK;
    Ctx->NumBufferedBytes = 0;
  }

  ZeroMem (&Ctx->Buffer[Ctx->NumBufferedBytes], AvailableBytes - sizeof (UINT64));
  *(UINT64 *)&Ctx->Buffer[SM3_CBLOCK - sizeof (UINT64)] = SwapBytes64 (Ctx->TotalInputSize << 3);
  ossl_sm3_block_data_order (Ctx, Ctx->Buffer, 1);

  WriteUnaligned32 (&((UINT32 *)HashValue)[0], SwapBytes32 (Ctx->A));
  WriteUnaligned32 (&((UINT32 *)HashValue)[1], SwapBytes32 (Ctx->B));
  WriteUnaligned32 (&((UINT32 *)HashValue)[2], SwapBytes32 (Ctx->C));
  WriteUnaligned32 (&((UINT32 *)HashValue)[3], SwapBytes32 (Ctx->D));
  WriteUnaligned32 (&((UINT32 *)HashValue)[4], SwapBytes32 (Ctx->E));
  WriteUnaligned32 (&((UINT32 *)HashValue)[5], SwapBytes32 (Ctx->F));
  WriteUnaligned32 (&((UINT32 *)HashValue)[6], SwapBytes32 (Ctx->G));
  WriteUnaligned32 (&((UINT32 *)HashValue)[7], SwapBytes32 (Ctx->H));

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
