/** @file
  RSA PSS Asymmetric Cipher Wrapper Implementation over MbedTLS.

  This file implements following APIs which provide basic capabilities for RSA:
  1) RsaPssSign

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/rsa.h>
#include <Library/RngLib.h>

/**
  The MbedTLS function f_rng, which MbedRand implements, is not
  documented well.

  @param[in]       RngState  RngState.
  @param[in]       Output    Output.
  @param[in]       Len       Len.

  @retval  0                 success.
  @retval  non-zero          failed.

**/
INT32
MbedRand (
  VOID   *RngState,
  UINT8  *Output,
  UINTN  Len
  )
{
  BOOLEAN  Ret;
  UINT64   TempRand;

  Ret = FALSE;

  while (Len > 0) {
    // Use RngLib to get random number
    Ret = GetRandomNumber64 (&TempRand);

    if (!Ret) {
      return Ret;
    }

    if (Len >= sizeof (TempRand)) {
      *((UINT64 *)Output) = TempRand;
      Output             += sizeof (UINT64);
      Len                -= sizeof (TempRand);
    } else {
      CopyMem (Output, &TempRand, Len);
      Len = 0;
    }
  }

  return 0;
}

/**
  Carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme.

  This function carries out the RSA-SSA signature generation with EMSA-PSS encoding scheme defined in
  RFC 8017.
  Mask generation function is the same as the message digest algorithm.
  If the Signature buffer is too small to hold the contents of signature, FALSE
  is returned and SigSize is set to the required buffer size to obtain the signature.

  If RsaContext is NULL, then return FALSE.
  If Message is NULL, then return FALSE.
  If MsgSize is zero or > INT_MAX, then return FALSE.
  If DigestLen is NOT 32, 48 or 64, return FALSE.
  If SaltLen is not equal to DigestLen, then return FALSE.
  If SigSize is large enough but Signature is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]      RsaContext   Pointer to RSA context for signature generation.
  @param[in]      Message      Pointer to octet message to be signed.
  @param[in]      MsgSize      Size of the message in bytes.
  @param[in]      DigestLen    Length of the digest in bytes to be used for RSA signature operation.
  @param[in]      SaltLen      Length of the salt in bytes to be used for PSS encoding.
  @param[out]     Signature    Pointer to buffer to receive RSA PSS signature.
  @param[in, out] SigSize      On input, the size of Signature buffer in bytes.
                               On output, the size of data returned in Signature buffer in bytes.

  @retval  TRUE   Signature successfully generated in RSASSA-PSS.
  @retval  FALSE  Signature generation failed.
  @retval  FALSE  SigSize is too small.
  @retval  FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
RsaPssSign (
  IN      VOID         *RsaContext,
  IN      CONST UINT8  *Message,
  IN      UINTN        MsgSize,
  IN      UINT16       DigestLen,
  IN      UINT16       SaltLen,
  OUT     UINT8        *Signature,
  IN OUT  UINTN        *SigSize
  )
{
  INT32              Ret;
  mbedtls_md_type_t  md_alg;
  UINT8              HashValue[SHA512_DIGEST_SIZE];
  BOOLEAN            Status;
  UINTN              ShaCtxSize;
  VOID               *ShaCtx;

  if (RsaContext == NULL) {
    return FALSE;
  }

  if ((Message == NULL) || (MsgSize == 0) || (MsgSize > INT_MAX)) {
    return FALSE;
  }

  if (SaltLen != DigestLen) {
    return FALSE;
  }

  ZeroMem (HashValue, DigestLen);

  switch (DigestLen) {
    case SHA256_DIGEST_SIZE:
      md_alg     = MBEDTLS_MD_SHA256;
      ShaCtxSize = Sha256GetContextSize ();
      ShaCtx     = AllocatePool (ShaCtxSize);

      Status = Sha256Init (ShaCtx);
      if (!Status) {
        return FALSE;
      }

      Status = Sha256Update (ShaCtx, Message, MsgSize);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      Status = Sha256Final (ShaCtx, HashValue);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      FreePool (ShaCtx);
      break;

    case SHA384_DIGEST_SIZE:
      md_alg     = MBEDTLS_MD_SHA384;
      ShaCtxSize = Sha384GetContextSize ();
      ShaCtx     = AllocatePool (ShaCtxSize);

      Status = Sha384Init (ShaCtx);
      if (!Status) {
        return FALSE;
      }

      Status = Sha384Update (ShaCtx, Message, MsgSize);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      Status = Sha384Final (ShaCtx, HashValue);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      FreePool (ShaCtx);
      break;

    case SHA512_DIGEST_SIZE:
      md_alg     = MBEDTLS_MD_SHA512;
      ShaCtxSize = Sha512GetContextSize ();
      ShaCtx     = AllocatePool (ShaCtxSize);

      Status = Sha512Init (ShaCtx);
      if (!Status) {
        return FALSE;
      }

      Status = Sha512Update (ShaCtx, Message, MsgSize);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      Status = Sha512Final (ShaCtx, HashValue);
      if (!Status) {
        FreePool (ShaCtx);
        return FALSE;
      }

      FreePool (ShaCtx);
      break;

    default:
      return FALSE;
  }

  if (Signature == NULL) {
    //
    // If Signature is NULL, return safe SignatureSize
    //
    *SigSize = MBEDTLS_MPI_MAX_SIZE;
    return FALSE;
  }

  mbedtls_rsa_set_padding (RsaContext, MBEDTLS_RSA_PKCS_V21, md_alg);

  Ret = mbedtls_rsa_rsassa_pss_sign (
          RsaContext,
          MbedRand,
          NULL,
          md_alg,
          (UINT32)DigestLen,
          HashValue,
          Signature
          );
  if (Ret != 0) {
    return FALSE;
  }

  *SigSize = ((mbedtls_rsa_context *)RsaContext)->len;
  return TRUE;
}
