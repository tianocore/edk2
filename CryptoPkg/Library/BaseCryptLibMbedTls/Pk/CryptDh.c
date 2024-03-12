/** @file
  Diffie-Hellman Wrapper Implementation over MbedTLS.

  RFC 7919 - Negotiated Finite Field Diffie-Hellman Ephemeral (FFDHE) Parameters

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"
#include <mbedtls/dhm.h>
#include <mbedtls/bignum.h>

/**
  Allocates and Initializes one Diffie-Hellman Context for subsequent use.

  @return  Pointer to the Diffie-Hellman Context that has been initialized.
           If the allocations fails, DhNew() returns NULL.

**/
VOID *
EFIAPI
DhNew (
  VOID
  )
{
  mbedtls_dhm_context  *ctx;

  ctx = AllocateZeroPool (sizeof (mbedtls_dhm_context));
  if (ctx == NULL) {
    return NULL;
  }

  mbedtls_dhm_init (ctx);

  return ctx;
}

/**
  Release the specified DH context.

  If DhContext is NULL, then return FALSE.

  @param[in]  DhContext  Pointer to the DH context to be released.

**/
VOID
EFIAPI
DhFree (
  IN  VOID  *DhContext
  )
{
  mbedtls_dhm_free (DhContext);

  if (DhContext != NULL) {
    FreePool (DhContext);
  }
}

/**
  Generates DH parameter.

  Given generator g, and length of prime number p in bits, this function generates p,
  and sets DH context according to value of g and p.

  Before this function can be invoked, pseudorandom number generator must be correctly
  initialized by RandomSeed().

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[out]      Prime        Pointer to the buffer to receive the generated prime number.

  @retval TRUE   DH parameter generation succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  PRNG fails to generate random prime number with PrimeLength.

**/
BOOLEAN
EFIAPI
DhGenerateParameter (
  IN OUT  VOID   *DhContext,
  IN      UINTN  Generator,
  IN      UINTN  PrimeLength,
  OUT     UINT8  *Prime
  )
{
  mbedtls_dhm_context  *ctx;
  mbedtls_mpi          BnP;
  mbedtls_mpi          BnG;
  BOOLEAN              Status;
  size_t               Nbits;

  if ((DhContext == NULL) || (Prime == NULL) || (PrimeLength > INT_MAX)) {
    return FALSE;
  }

  ctx = (mbedtls_dhm_context *)DhContext;
  mbedtls_mpi_init (&BnP);
  mbedtls_mpi_init (&BnG);
  Nbits = PrimeLength;

  if (mbedtls_mpi_gen_prime (&BnP, Nbits, 1, MbedtlsRand, NULL) != 0) {
    Status = FALSE;
    goto Error;
  }

  if (mbedtls_mpi_read_binary (&BnG, (const unsigned char *)&Generator, 1) != 0) {
    Status = FALSE;
    goto Error;
  }

  if (mbedtls_dhm_set_group (ctx, &BnP, &BnG) != 0) {
    Status = FALSE;
    goto Error;
  }

  if (mbedtls_mpi_write_binary (&BnP, Prime, PrimeLength) != 0) {
    Status = FALSE;
    goto Error;
  }

  Status = TRUE;

Error:
  mbedtls_mpi_free (&BnP);
  mbedtls_mpi_free (&BnG);
  return Status;
}

/**
  Sets generator and prime parameters for DH.

  Given generator g, and prime number p, this function and sets DH
  context accordingly.

  If DhContext is NULL, then return FALSE.
  If Prime is NULL, then return FALSE.

  @param[in, out]  DhContext    Pointer to the DH context.
  @param[in]       Generator    Value of generator.
  @param[in]       PrimeLength  Length in bits of prime to be generated.
  @param[in]       Prime        Pointer to the prime number.

  @retval TRUE   DH parameter setting succeeded.
  @retval FALSE  Value of Generator is not supported.
  @retval FALSE  Value of Generator is not suitable for the Prime.
  @retval FALSE  Value of Prime is not a prime number.
  @retval FALSE  Value of Prime is not a safe prime number.

**/
BOOLEAN
EFIAPI
DhSetParameter (
  IN OUT  VOID         *DhContext,
  IN      UINTN        Generator,
  IN      UINTN        PrimeLength,
  IN      CONST UINT8  *Prime
  )
{
  mbedtls_dhm_context  *ctx;
  mbedtls_mpi          BnP;
  mbedtls_mpi          BnG;
  BOOLEAN              Status;

  //
  // Check input parameters.
  //
  if ((DhContext == NULL) || (Prime == NULL) || (PrimeLength > INT_MAX)) {
    return FALSE;
  }

  //
  // Set the generator and prime parameters for DH object.
  //
  ctx = (mbedtls_dhm_context *)DhContext;
  mbedtls_mpi_init (&BnP);
  mbedtls_mpi_init (&BnG);

  if (mbedtls_mpi_read_binary (&BnP, (const unsigned char *)Prime, (int)(PrimeLength)) != 0) {
    Status = FALSE;
    goto Error;
  }

  if (mbedtls_mpi_read_binary (&BnG, (const unsigned char *)&Generator, 1) != 0) {
    Status = FALSE;
    goto Error;
  }

  if (mbedtls_dhm_set_group (ctx, &BnP, &BnG) != 0) {
    Status = FALSE;
    goto Error;
  }

  Status = TRUE;

Error:
  mbedtls_mpi_free (&BnP);
  mbedtls_mpi_free (&BnG);
  return Status;
}

/**
  Generates DH public key.

  This function generates random secret exponent, and computes the public key, which is
  returned via parameter PublicKey and PublicKeySize. DH context is updated accordingly.
  If the PublicKey buffer is too small to hold the public key, FALSE is returned and
  PublicKeySize is set to the required buffer size to obtain the public key.

  If DhContext is NULL, then return FALSE.
  If PublicKeySize is NULL, then return FALSE.
  If PublicKeySize is large enough but PublicKey is NULL, then return FALSE.

  For FFDHE2048, the PublicSize is 256.
  For FFDHE3072, the PublicSize is 384.
  For FFDHE4096, the PublicSize is 512.

  @param[in, out]  DhContext      Pointer to the DH context.
  @param[out]      PublicKey      Pointer to the buffer to receive generated public key.
  @param[in, out]  PublicKeySize  On input, the size of PublicKey buffer in bytes.
                                  On output, the size of data returned in PublicKey buffer in bytes.

  @retval TRUE   DH public key generation succeeded.
  @retval FALSE  DH public key generation failed.
  @retval FALSE  PublicKeySize is not large enough.

**/
BOOLEAN
EFIAPI
DhGenerateKey (
  IN OUT  VOID   *DhContext,
  OUT     UINT8  *PublicKey,
  IN OUT  UINTN  *PublicKeySize
  )
{
  INT32                Ret;
  mbedtls_dhm_context  *ctx;
  UINTN                FinalPubKeySize;

  //
  // Check input parameters.
  //
  if ((DhContext == NULL) || (PublicKeySize == NULL)) {
    return FALSE;
  }

  if ((PublicKey == NULL) && (*PublicKeySize != 0)) {
    return FALSE;
  }

  ctx = DhContext;
  switch (mbedtls_mpi_size (&ctx->P)) {
    case 64:
      FinalPubKeySize = 64;
      break;
    case 256:
      FinalPubKeySize = 256;
      break;
    case 384:
      FinalPubKeySize = 384;
      break;
    case 512:
      FinalPubKeySize = 512;
      break;
    default:
      return FALSE;
  }

  if (*PublicKeySize < FinalPubKeySize) {
    *PublicKeySize = FinalPubKeySize;
    return FALSE;
  }

  *PublicKeySize = FinalPubKeySize;
  ZeroMem (PublicKey, *PublicKeySize);

  Ret = mbedtls_dhm_make_public (DhContext, (UINT32)*PublicKeySize, PublicKey, (UINT32)*PublicKeySize, MbedtlsRand, NULL);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Computes exchanged common key.

  Given peer's public key, this function computes the exchanged common key, based on its own
  context including value of prime modulus and random secret exponent.

  If DhContext is NULL, then return FALSE.
  If PeerPublicKey is NULL, then return FALSE.
  If KeySize is NULL, then return FALSE.
  If Key is NULL, then return FALSE.
  If KeySize is not large enough, then return FALSE.

  For FFDHE2048, the PeerPublicSize is 256.
  For FFDHE3072, the PeerPublicSize is 384.
  For FFDHE4096, the PeerPublicSize is 512.

  @param[in, out]  DhContext          Pointer to the DH context.
  @param[in]       PeerPublicKey      Pointer to the peer's public key.
  @param[in]       PeerPublicKeySize  Size of peer's public key in bytes.
  @param[out]      Key                Pointer to the buffer to receive generated key.
  @param[in, out]  KeySize            On input, the size of Key buffer in bytes.
                                      On output, the size of data returned in Key buffer in bytes.

  @retval TRUE   DH exchanged key generation succeeded.
  @retval FALSE  DH exchanged key generation failed.
  @retval FALSE  KeySize is not large enough.

**/
BOOLEAN
EFIAPI
DhComputeKey (
  IN OUT  VOID         *DhContext,
  IN      CONST UINT8  *PeerPublicKey,
  IN      UINTN        PeerPublicKeySize,
  OUT     UINT8        *Key,
  IN OUT  UINTN        *KeySize
  )
{
  INT32  Ret;

  //
  // Check input parameters.
  //
  if ((DhContext == NULL) || (PeerPublicKey == NULL) || (KeySize == NULL) || (Key == NULL)) {
    return FALSE;
  }

  if (PeerPublicKeySize > INT_MAX) {
    return FALSE;
  }

  Ret = mbedtls_dhm_read_public (DhContext, PeerPublicKey, PeerPublicKeySize);
  if (Ret != 0) {
    return FALSE;
  }

  Ret = mbedtls_dhm_calc_secret (DhContext, Key, *KeySize, KeySize, MbedtlsRand, NULL);
  if (Ret != 0) {
    return FALSE;
  }

  return TRUE;
}
