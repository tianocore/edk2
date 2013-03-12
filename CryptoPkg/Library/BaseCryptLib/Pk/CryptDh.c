/** @file
  Diffie-Hellman Wrapper Implementation over OpenSSL.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"
#include <openssl/dh.h>


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
  //
  // Allocates & Initializes DH Context by OpenSSL DH_new()
  //
  return (VOID *) DH_new ();
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
  //
  // Free OpenSSL DH Context
  //
  DH_free ((DH *) DhContext);
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

  @retval TRUE   DH pamameter generation succeeded.
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
  BOOLEAN RetVal;

  //
  // Check input parameters.
  //
  if (DhContext == NULL || Prime == NULL || PrimeLength > INT_MAX) {
    return FALSE;
  }

  if (Generator != DH_GENERATOR_2 && Generator != DH_GENERATOR_5) {
    return FALSE;
  }

  RetVal = (BOOLEAN) DH_generate_parameters_ex (DhContext, (UINT32) PrimeLength, (UINT32) Generator, NULL);
  if (!RetVal) {
    return FALSE;
  }

  BN_bn2bin (((DH *) DhContext)->p, Prime);

  return TRUE;
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

  @retval TRUE   DH pamameter setting succeeded.
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
  DH      *Dh;
  BIGNUM  *Bn;

  //
  // Check input parameters.
  //
  if (DhContext == NULL || Prime == NULL || PrimeLength > INT_MAX) {
    return FALSE;
  }
  
  if (Generator != DH_GENERATOR_2 && Generator != DH_GENERATOR_5) {
    return FALSE;
  }

  Bn = NULL;

  Dh = (DH *) DhContext;
  Dh->g = NULL;
  Dh->p = BN_new ();
  if (Dh->p == NULL) {
    goto Error;
  }
  
  Dh->g = BN_new ();
  if (Dh->g == NULL) {
    goto Error;
  }

  Bn = BN_bin2bn (Prime, (UINT32) (PrimeLength / 8), Dh->p);
  if (Bn == NULL) {
    goto Error;
  }

  if (BN_set_word (Dh->g, (UINT32) Generator) == 0) {
    goto Error;
  }

  return TRUE;

Error:

  if (Dh->p != NULL) {
    BN_free (Dh->p);
  }

  if (Dh->g != NULL) {
    BN_free (Dh->g);
  }

  if (Bn != NULL) {
    BN_free (Bn);
  }
  
  return FALSE;
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
  BOOLEAN RetVal;
  DH      *Dh;
  INTN    Size;

  //
  // Check input parameters.
  //
  if (DhContext == NULL || PublicKeySize == NULL) {
    return FALSE;
  }

  if (PublicKey == NULL && *PublicKeySize != 0) {
    return FALSE;
  }
  
  Dh = (DH *) DhContext;

  RetVal = (BOOLEAN) DH_generate_key (DhContext);
  if (RetVal) {
    Size = BN_num_bytes (Dh->pub_key);
    if ((Size > 0) && (*PublicKeySize < (UINTN) Size)) {
      *PublicKeySize = Size;
      return FALSE;
    }
    
    BN_bn2bin (Dh->pub_key, PublicKey);
    *PublicKeySize = Size;
  }

  return RetVal;
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
  BIGNUM  *Bn;
  INTN    Size;

  //
  // Check input parameters.
  //
  if (DhContext == NULL || PeerPublicKey == NULL || KeySize == NULL || Key == NULL) {
    return FALSE;
  }

  if (PeerPublicKeySize > INT_MAX) {
    return FALSE;
  }
  
  Bn = BN_bin2bn (PeerPublicKey, (UINT32) PeerPublicKeySize, NULL);
  if (Bn == NULL) {
    return FALSE;
  }

  Size = DH_compute_key (Key, Bn, DhContext);
  if (Size < 0) {
    BN_free (Bn);
    return FALSE;
  }

  if (*KeySize < (UINTN) Size) {
    *KeySize = Size;
    BN_free (Bn);
    return FALSE;
  }

  *KeySize = Size;
  BN_free (Bn);
  return TRUE;
}
