/** @file
  Big number API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <openssl/bn.h>

VOID *
EFIAPI
BigNumInit (
  VOID
  )
{
  return BN_new();
}

VOID *
EFIAPI
BigNumFromBin(
  IN CONST UINT8 *Buf,
  IN UINTN Len)
{
  return BN_bin2bn(Buf, (int)Len, NULL);
}

INTN
EFIAPI
BigNumToBin(
  IN VOID *Bn,
  OUT UINT8 *Buf)
{
  return BN_bn2bin(Bn, Buf);
}

VOID
EFIAPI
BigNumFree(
  IN VOID *Bn,
  IN BOOLEAN Clear
  )
{
  if (Clear)
    BN_clear_free(Bn);
  else
    BN_free(Bn);
}

EFI_STATUS
EFIAPI
BigNumAdd(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  return BN_add(BnRes, BnA, BnB) ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumSub(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  return BN_sub(BnRes, BnA, BnB) ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  int Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod(BnRes, BnA, BnB, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumExpMod(
  IN VOID *BnA,
  IN VOID *BnP,
  IN VOID *BnM,
  OUT VOID *BnRes
  )
{
  int Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod_exp(BnRes, BnA, BnP, BnM, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumInverseMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  )
{
  BIGNUM *Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod_inverse(BnRes, BnA, BnM, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumDiv(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  int Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_div(BnRes, NULL, BnA, BnB, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumMulMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  )
{
  int Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod_mul(BnRes, BnA, BnB, BnM, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

INTN
EFIAPI
BigNumCmp(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB)
{
  return BN_cmp(BnA, BnB);
}

UINTN
EFIAPI
BigNumBits(
  IN CONST VOID *Bn
  )
{
  return BN_num_bits(Bn);
}

UINTN
EFIAPI
BigNumBytes(
  IN CONST VOID *Bn
  )
{
  return BN_num_bytes(Bn);
}

BOOLEAN
EFIAPI
BigNumIsWord(
  IN CONST VOID *Bn,
  IN UINTN Num)
{
  return !!BN_is_word(Bn, Num);
}

BOOLEAN
EFIAPI
BigNumIsOdd(
  IN CONST VOID *Bn
  )
{
  return !!BN_is_odd(Bn);
}

VOID *
EFIAPI
BigNumCopy(
  OUT VOID *BnDst,
  IN CONST VOID *BnSrc
  )
{
  return BN_copy(BnDst, BnSrc);
}

CONST VOID *
EFIAPI
BigNumValueOne(
  VOID
  )
{
  return BN_value_one();
}

EFI_STATUS
EFIAPI
BigNumRShift(
  IN CONST VOID *Bn,
  IN UINTN n,
  OUT VOID *BnRes)
{
  //BN_rshift() does not modify the first argument, so we remove const
  if (BN_rshift((BIGNUM *) Bn, BnRes, (int)n) == 1)
    return EFI_SUCCESS;
  else
    return EFI_PROTOCOL_ERROR;
}

VOID
EFIAPI
BigNumConsttime(
  IN VOID *Bn
  )
{
  BN_set_flags(Bn, BN_FLG_CONSTTIME);
}

EFI_STATUS
EFIAPI
BigNumSqrMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnM,
  OUT VOID *BnRes)
{
  int Res;
  BN_CTX *Ctx;

  Ctx = BN_CTX_new();
  if (!Ctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod_sqr(BnRes, BnA, BnM, Ctx);
  BN_CTX_free(Ctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

VOID *
EFIAPI
BigNumNewContext(
  VOID
  )
{
  return BN_CTX_new();
}

VOID
EFIAPI
BigNumContextFree(
  IN VOID *BnCtx
)
{
  BN_CTX_free(BnCtx);
}

EFI_STATUS
EFIAPI
BigNumSetUint(
  IN VOID *Bn,
  IN UINTN Val
  )
{
  return BN_set_word(Bn, Val) == 1 ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}

EFI_STATUS
EFIAPI
BigNumAddMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  )
{
  int Res;
  BN_CTX *Bnctx;

  Bnctx = BN_CTX_new();
  if (!Bnctx)
    return EFI_OUT_OF_RESOURCES;

  Res = BN_mod_add(BnRes, BnA, BnB, BnM, Bnctx);
  BN_CTX_free(Bnctx);

  return Res ? EFI_SUCCESS : EFI_PROTOCOL_ERROR;
}
