/** @file
  SLH-DSA Curve API implementation based on OpenSSL

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <openssl/evp.h>
#include <crypto/slh_dsa.h>

/**
 Instantiates an SLH-DSA EVP_PKEY object

  If Nid is not NID_SLHDSASHAKE256S, then return NULL.

  @param[in]  Nid  Numeric identifier value to denote SLHDSA-SHAKE256S

  @return     Pointer to the MLDSA EVP_PKEY object that has been initialized.
              If the allocations fails, SlhCreatePkeyObject() returns NULL.

**/
VOID *
EFIAPI
SlhCreatePkeyObject (
  IN UINTN  Nid,
  IN VOID   *Pkey
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Free the SLH-DSA EVP_PKEY object

  If Pkey is NULL, then return.

  @param[in]  Pkey  Pointer to the SLHDSA EVP_PKEY object to be freed.
**/
VOID
DestroySlhPkeyObject (
  IN VOID  *Pkey
  )
{
  ASSERT (FALSE);
}

/**
   Verifies the SLH-DSA signature. ML-DSA signatures are verified via 'pure' implementation.
   Meaning message digest cannot be computed ahead of time. Data is passed into the verification function

   If Pkey is NULL, then return FALSE.
   If Data is NULL, then return FALSE.
   If DataSize is 0, then return FALSE.
   If Sig is NULL, then return FALSE.
   If SigSize is 0, then return FALSE.

   @param[in]  Pkey       Pointer to SLH-DSA EVP_PKEY object.
   @param[in]  Data       Pointer to data being verified.
   @param[in]  DataSize   Size of data.
   @param[in]  Signature  Pointer to SLH-DSA signature.
   @param[in]  SigSize    Size of SLH-DSA signature.
   @param[in]  Nid        Nid for SLH-DSA Category

   EdCreatePkeyFromBinary  @retval  TRUE   Valid signature encoded in MLDSA.
   @retval  FALSE  Invalid signature or invalid input.

**/
BOOLEAN
EFIAPI
SlhDsaVerify (
  IN     CONST UINT8  *Signature,
  IN     CONST UINTN  SigLen,
  IN     CONST UINT8  *Data,
  IN     CONST UINTN  DataLen,
  IN     CONST VOID   *Pkey,
  IN     CONST UINTN  Nid
  )
{
  ASSERT (FALSE);
  return FALSE;
}
