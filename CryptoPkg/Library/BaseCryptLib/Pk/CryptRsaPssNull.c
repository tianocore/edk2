/** @file
  RSA-PSS Asymmetric Cipher Wrapper Implementation over OpenSSL.

  This file does not provide real capabilities for following APIs in RSA handling:
  1) RsaPssVerify

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Verifies the RSA signature with RSASSA-PSS signature scheme defined in RFC 8017.
  Implementation determines salt length automatically from the signature encoding.
  Mask generation function is the same as the message digest algorithm.
  Salt length should be equal to digest length.

  @param[in]  RsaContext      Pointer to RSA context for signature verification.
  @param[in]  Message         Pointer to octet message to be verified.
  @param[in]  MsgSize         Size of the message in bytes.
  @param[in]  Signature       Pointer to RSASSA-PSS signature to be verified.
  @param[in]  SigSize         Size of signature in bytes.
  @param[in]  DigestLen       Length of digest for RSA operation.
  @param[in]  SaltLen         Salt length for PSS encoding.

  @retval  TRUE   Valid signature encoded in RSASSA-PSS.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPssVerify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *Message,
  IN  UINTN        MsgSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize,
  IN  UINT16       DigestLen,
  IN  UINT16       SaltLen
  )
{
  ASSERT (FALSE);
  return FALSE;
}
