/** @file  Big number API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <openssl/bn.h>

/**
  Allocate new Big Number.

  @retval New BigNum opaque structure or NULL on failure.
**/
VOID *
EFIAPI
BigNumInit (
  VOID
  )
{
  return BN_new ();
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
  return BN_bin2bn (Buf, (INT32)Len, NULL);
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
  return BN_bn2bin (Bn, Buf);
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
  if (Clear) {
    BN_clear_free (Bn);
  } else {
    BN_free (Bn);
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
  return (BOOLEAN)BN_add (BnRes, BnA, BnB);
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
  return (BOOLEAN)BN_sub (BnRes, BnA, BnB);
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_mod (BnRes, BnA, BnB, Ctx);
  BN_CTX_free (Ctx);

  return RetVal;
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_mod_exp (BnRes, BnA, BnP, BnM, Ctx);

  BN_CTX_free (Ctx);
  return RetVal;
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = FALSE;
  if (BN_mod_inverse (BnRes, BnA, BnM, Ctx) != NULL) {
    RetVal = TRUE;
  }

  BN_CTX_free (Ctx);
  return RetVal;
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_div (BnRes, NULL, BnA, BnB, Ctx);
  BN_CTX_free (Ctx);

  return RetVal;
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_mod_mul (BnRes, BnA, BnB, BnM, Ctx);
  BN_CTX_free (Ctx);

  return RetVal;
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
  return BN_cmp (BnA, BnB);
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
  return BN_num_bits (Bn);
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
  return BN_num_bytes (Bn);
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
  return (BOOLEAN)BN_is_word (Bn, Num);
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
  return (BOOLEAN)BN_is_odd (Bn);
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
  return BN_copy (BnDst, BnSrc);
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
  return BN_value_one ();
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
  return (BOOLEAN)BN_rshift (BnRes, Bn, (INT32)N);
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
  BN_set_flags (Bn, BN_FLG_CONSTTIME);
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_mod_sqr (BnRes, BnA, BnM, Ctx);
  BN_CTX_free (Ctx);

  return RetVal;
}

/**
  Create new Big Number computation context. This is an opaque structure
  which should be passed to any function that requires it. The BN context is
  needed to optimize calculations and expensive allocations.

  @retval Big Number context struct or NULL on failure.
**/
VOID *
EFIAPI
BigNumNewContext (
  VOID
  )
{
  return BN_CTX_new ();
}

/**
  Free Big Number context that was allocated with BigNumNewContext().

  @param[in]   BnCtx     Big number context to free.
**/
VOID
EFIAPI
BigNumContextFree (
  IN VOID  *BnCtx
  )
{
  BN_CTX_free (BnCtx);
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
  return (BOOLEAN)BN_set_word (Bn, Val);
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
  BOOLEAN  RetVal;
  BN_CTX   *Ctx;

  Ctx = BN_CTX_new ();
  if (Ctx == NULL) {
    return FALSE;
  }

  RetVal = (BOOLEAN)BN_mod_add (BnRes, BnA, BnB, BnM, Ctx);
  BN_CTX_free (Ctx);

  return RetVal;
}
