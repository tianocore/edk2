/** @file
  Big number API implementation based on MbedTLS

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "InternalCryptLib.h"
#include <mbedtls/bignum.h>

/**
  Allocate new Big Number.

  @retval New BigNum opaque structure or NULL on failure.
**/
VOID *
EFIAPI
BigNumInit (
  )
{
  mbedtls_mpi  *X;

  X = AllocateZeroPool (sizeof (mbedtls_mpi));

  mbedtls_mpi_init (X);

  return X;
}

/**
  Allocate new Big Number and assign the provided value to it.

  @param[in]   Buf    Big endian encoded buffer.
  @param[in]   Len    Buffer length.

  @retval New BigNum opaque structure or NULL on failure.
**/
VOID *
EFIAPI
BigNumFromBin (
  IN CONST UINT8  *Buf,
  IN UINTN        Len
  )
{
  mbedtls_mpi  *X;

  //
  // Check input parameters.
  //
  if ((Buf == NULL) || (Len > INT_MAX)) {
    return NULL;
  }

  X = AllocateZeroPool (sizeof (mbedtls_mpi));

  if (mbedtls_mpi_read_binary (X, Buf, Len) == 0 ) {
    return X;
  } else {
    return NULL;
  }
}

/**
  Convert the absolute value of Bn into big-endian form and store it at Buf.
  The Buf array should have at least BigNumBytes() in it.

  @param[in]   Bn     Big number to convert.
  @param[out]  Buf    Output buffer.

  @retval The length of the big-endian number placed at Buf or -1 on error.
**/
INTN
EFIAPI
BigNumToBin (
  IN CONST VOID  *Bn,
  OUT UINT8      *Buf
  )
{
  size_t  Len;

  if ((Bn == NULL) || (Buf == NULL)) {
    return -1;
  }

  Len = mbedtls_mpi_size (Bn);

  if (mbedtls_mpi_write_binary (Bn, Buf, Len) != 0) {
    return -1;
  }

  return Len;
}

/**
  Free the Big Number.

  @param[in]   Bn      Big number to free.
  @param[in]   Clear   TRUE if the buffer should be cleared.
**/
VOID
EFIAPI
BigNumFree (
  IN VOID     *Bn,
  IN BOOLEAN  Clear
  )
{
  mbedtls_mpi_free (Bn);
  if (Bn != NULL) {
    FreePool (Bn);
  }
}

/**
  Calculate the sum of two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA + BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumAdd (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_add_mpi (BnRes, BnA, BnB) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Subtract two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA - BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumSub (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_sub_mpi (BnRes, BnA, BnB) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Calculate remainder: BnRes = BnA % BnB.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result of BnA % BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_mod_mpi (BnRes, BnA, BnB) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Compute BnA to the BnP-th power modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnP     Big number (power).
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result of (BnA ^ BnP) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumExpMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnP,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnP == NULL) || (BnM == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_exp_mod (BnRes, BnA, BnP, BnM, NULL) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Compute BnA inverse modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnRes) % BnM == 1.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumInverseMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnM == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_inv_mod (BnRes, BnA, BnM) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Divide two Big Numbers.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[out]  BnRes   The result, such that BnA / BnB.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumDiv (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_div_mpi (BnRes, NULL, BnA, BnB) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Multiply two Big Numbers modulo BnM.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA * BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumMulMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnM == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_mul_mpi (BnRes, BnA, BnB) != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_mod_mpi (BnRes, BnRes, BnM) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Compare two Big Numbers.

  @param[in]   BnA     Big number.
  @param[in]   BnB     Big number.

  @retval 0          BnA == BnB.
  @retval 1          BnA > BnB.
  @retval -1         BnA < BnB.
**/
INTN
EFIAPI
BigNumCmp (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB
  )
{
  return mbedtls_mpi_cmp_mpi (BnA, BnB);
}

/**
  Get number of bits in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bits.
**/
UINTN
EFIAPI
BigNumBits (
  IN CONST VOID  *Bn
  )
{
  return mbedtls_mpi_bitlen (Bn);
}

/**
  Get number of bytes in Bn.

  @param[in]   Bn     Big number.

  @retval Number of bytes.
**/
UINTN
EFIAPI
BigNumBytes (
  IN CONST VOID  *Bn
  )
{
  return mbedtls_mpi_size (Bn);
}

/**
  Checks if Big Number equals to the given Num.

  @param[in]   Bn     Big number.
  @param[in]   Num    Number.

  @retval TRUE   iff Bn == Num.
  @retval FALSE  otherwise.
**/
BOOLEAN
EFIAPI
BigNumIsWord (
  IN CONST VOID  *Bn,
  IN UINTN       Num
  )
{
  if ((Bn == NULL) || (Num > INT_MAX)) {
    return FALSE;
  }

  if (mbedtls_mpi_cmp_int (Bn, Num) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Checks if Big Number is odd.

  @param[in]   Bn     Big number.

  @retval TRUE   Bn is odd (Bn % 2 == 1).
  @retval FALSE  otherwise.
**/
BOOLEAN
EFIAPI
BigNumIsOdd (
  IN CONST VOID  *Bn
  )
{
  mbedtls_mpi  X;
  mbedtls_mpi  TemBn;
  BOOLEAN      Result;

  //
  // Check input parameters.
  //
  if ((Bn == NULL) ||
      (((mbedtls_mpi *)Bn)->n == 0) || (((mbedtls_mpi *)Bn)->n > INT_MAX) ||
      (((mbedtls_mpi *)Bn)->p == NULL) ||
      ((((mbedtls_mpi *)Bn)->s != 1) && (((mbedtls_mpi *)Bn)->s != -1)))
  {
    return FALSE;
  }

  mbedtls_mpi_init (&X);
  mbedtls_mpi_init (&TemBn);

  if (mbedtls_mpi_copy (&TemBn, Bn) != 0) {
    Result =  FALSE;
    goto Done;
  }

  if (mbedtls_mpi_lset (&X, 2) != 0) {
    Result =  FALSE;
    goto Done;
  }

  if (mbedtls_mpi_mod_mpi (&TemBn, &TemBn, &X) != 0) {
    Result =  FALSE;
    goto Done;
  }

  if (mbedtls_mpi_cmp_int (&TemBn, 1) == 0) {
    Result =  TRUE;
  } else {
    Result =  FALSE;
  }

Done:
  mbedtls_mpi_free (&X);
  mbedtls_mpi_free (&TemBn);
  return Result;
}

/**
  Copy Big number.

  @param[out]  BnDst     Destination.
  @param[in]   BnSrc     Source.

  @retval BnDst on success.
  @retval NULL otherwise.
**/
VOID *
EFIAPI
BigNumCopy (
  OUT VOID       *BnDst,
  IN CONST VOID  *BnSrc
  )
{
  if ((BnDst == NULL) || (BnSrc == NULL)) {
    return NULL;
  }

  if (mbedtls_mpi_copy (BnDst, BnSrc) != 0) {
    return NULL;
  }

  return BnDst;
}

/**
  Get constant Big number with value of "1".
  This may be used to save expensive allocations.

  @retval Big Number with value of 1.
**/
CONST VOID *
EFIAPI
BigNumValueOne (
  VOID
  )
{
  mbedtls_mpi  *X;

  X = AllocateZeroPool (sizeof (mbedtls_mpi));

  if (mbedtls_mpi_lset (X, 1) == 0 ) {
    return X;
  } else {
    return NULL;
  }
}

/**
  Shift right Big Number.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   Bn      Big number.
  @param[in]   N       Number of bits to shift.
  @param[out]  BnRes   The result.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumRShift (
  IN CONST VOID  *Bn,
  IN UINTN       N,
  OUT VOID       *BnRes
  )
{
  mbedtls_mpi  TempBn;

  //
  // Check input parameters.
  //
  if ((Bn == NULL) || (BnRes == NULL) ||
      (((mbedtls_mpi *)Bn)->n == 0) || (((mbedtls_mpi *)Bn)->n > INT_MAX) ||
      (((mbedtls_mpi *)Bn)->p == NULL) ||
      ((((mbedtls_mpi *)Bn)->s != 1) && (((mbedtls_mpi *)Bn)->s != -1)))
  {
    return FALSE;
  }

  mbedtls_mpi_init (&TempBn);

  if (mbedtls_mpi_copy (&TempBn, Bn) != 0) {
    mbedtls_mpi_free (&TempBn);
    return FALSE;
  }

  if (mbedtls_mpi_shift_r (&TempBn, N) != 0) {
    mbedtls_mpi_free (&TempBn);
    return FALSE;
  }

  if (mbedtls_mpi_copy (BnRes, &TempBn) != 0) {
    mbedtls_mpi_free (&TempBn);
    return FALSE;
  }

  mbedtls_mpi_free (&TempBn);
  return TRUE;
}

/**
  Mark Big Number for constant time computations.
  This function should be called before any constant time computations are
  performed on the given Big number.

  @param[in]   Bn     Big number
**/
VOID
EFIAPI
BigNumConstTime (
  IN VOID  *Bn
  )
{
  ASSERT (FALSE);
}

/**
  Calculate square modulo.
  Please note, all "out" Big number arguments should be properly initialized
  by calling to BigNumInit() or BigNumFromBin() functions.

  @param[in]   BnA     Big number.
  @param[in]   BnM     Big number (modulo).
  @param[out]  BnRes   The result, such that (BnA ^ 2) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumSqrMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnM == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_mul_mpi (BnRes, BnA, BnA) != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_mod_mpi (BnRes, BnRes, BnM) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Create new Big Number computation context. This is an opaque structure
  which should be passed to any function that requires it. The BN context is
  needed to optimize calculations and expensive allocations.

  This API is not used in Mbedtls.
  It is only compatible for Openssl implementation.

  @retval Big Number context struct or NULL on failure.
**/
VOID *
EFIAPI
BigNumNewContext (
  VOID
  )
{
  mbedtls_mpi  *X;

  X = AllocateZeroPool (sizeof (mbedtls_mpi));

  mbedtls_mpi_init (X);

  return X;
}

/**
  Free Big Number context that was allocated with BigNumNewContext().

  This API is not used in Mbedtls.
  It is only compatible for Openssl implementation.

  @param[in]   BnCtx     Big number context to free.
**/
VOID
EFIAPI
BigNumContextFree (
  IN VOID  *BnCtx
  )
{
  mbedtls_mpi_free (BnCtx);

  if (BnCtx != NULL) {
    FreePool (BnCtx);
  }
}

/**
  Set Big Number to a given value.

  @param[in]   Bn     Big number to set.
  @param[in]   Val    Value to set.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumSetUint (
  IN VOID   *Bn,
  IN UINTN  Val
  )
{
  if ((Bn == NULL) || (Val > INT_MAX)) {
    return FALSE;
  }

  if (mbedtls_mpi_lset (Bn, Val) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Add two Big Numbers modulo BnM.

  @param[in]   BnA       Big number.
  @param[in]   BnB       Big number.
  @param[in]   BnM       Big number (modulo).
  @param[out]  BnRes     The result, such that (BnA + BnB) % BnM.

  @retval TRUE          On success.
  @retval FALSE         Otherwise.
**/
BOOLEAN
EFIAPI
BigNumAddMod (
  IN CONST VOID  *BnA,
  IN CONST VOID  *BnB,
  IN CONST VOID  *BnM,
  OUT VOID       *BnRes
  )
{
  if ((BnA == NULL) || (BnB == NULL) || (BnM == NULL) || (BnRes == NULL)) {
    return FALSE;
  }

  if (mbedtls_mpi_add_mpi (BnRes, BnA, BnB) != 0) {
    return FALSE;
  }

  if (mbedtls_mpi_mod_mpi (BnRes, BnRes, BnM) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}
