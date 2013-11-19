/** @file
  Support routines for RDRAND instruction access.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RdRand.h"
#include "AesCore.h"

//
// Bit mask used to determine if RdRand instruction is supported.
//
#define RDRAND_MASK    0x40000000

/**
  Determines whether or not RDRAND instruction is supported by the host hardware.

  @retval EFI_SUCCESS          RDRAND instruction supported.
  @retval EFI_UNSUPPORTED      RDRAND instruction not supported.

**/
EFI_STATUS
EFIAPI
IsRdRandSupported (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      RegEax;
  UINT32      RegEbx;
  UINT32      RegEcx;
  UINT32      RegEdx;
  BOOLEAN     IsIntelCpu;

  Status     = EFI_UNSUPPORTED;
  IsIntelCpu = FALSE;
  
  //
  // Checks whether the current processor is an Intel product by CPUID.
  //
  AsmCpuid (0, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  if ((CompareMem ((CHAR8 *)(&RegEbx), "Genu", 4) == 0) &&
      (CompareMem ((CHAR8 *)(&RegEdx), "ineI", 4) == 0) &&
      (CompareMem ((CHAR8 *)(&RegEcx), "ntel", 4) == 0)) {
    IsIntelCpu = TRUE;
  }

  if (IsIntelCpu) {
    //
    // Determine RDRAND support by examining bit 30 of the ECX register returned by CPUID.
    // A value of 1 indicates that processor supports RDRAND instruction.
    //
    AsmCpuid (1, 0, 0, &RegEcx, 0);

    if ((RegEcx & RDRAND_MASK) == RDRAND_MASK) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

/**
  Calls RDRAND to obtain a 16-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand16 (
  OUT UINT16       *Rand,
  IN BOOLEAN       NeedRetry
  )
{
  UINT32      Index;
  UINT32      RetryCount;

  if (NeedRetry) {
    RetryCount = RETRY_LIMIT;
  } else {
    RetryCount = 1;
  }

  //
  // Perform a single call to RDRAND, or enter a loop call until RDRAND succeeds.
  //
  for (Index = 0; Index < RetryCount; Index++) {
    if (RdRand16Step (Rand)) {
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_READY;
}

/**
  Calls RDRAND to obtain a 32-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand32 (
  OUT UINT32       *Rand,
  IN BOOLEAN       NeedRetry
  )
{
  UINT32      Index;
  UINT32      RetryCount;

  if (NeedRetry) {
    RetryCount = RETRY_LIMIT;
  } else {
    RetryCount = 1;
  }

  //
  // Perform a single call to RDRAND, or enter a loop call until RDRAND succeeds.
  //
  for (Index = 0; Index < RetryCount; Index++) {
    if (RdRand32Step (Rand)) {
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_READY;
}

/**
  Calls RDRAND to obtain a 64-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand64 (
  OUT UINT64       *Rand,
  IN BOOLEAN       NeedRetry
  )
{
  UINT32      Index;
  UINT32      RetryCount;

  if (NeedRetry) {
    RetryCount = RETRY_LIMIT;
  } else {
    RetryCount = 1;
  }

  //
  // Perform a single call to RDRAND, or enter a loop call until RDRAND succeeds.
  //
  for (Index = 0; Index < RetryCount; Index++) {
    if (RdRand64Step (Rand)) {
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_READY;
}

/**
  Calls RDRAND to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RdRandGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  )
{
  EFI_STATUS  Status;
  UINT8       *Start;
  UINT8       *ResidualStart;
  UINTN       *BlockStart;
  UINTN       TempRand;
  UINTN       Count;
  UINTN       Residual;
  UINTN       StartLen;
  UINTN       BlockNum;
  UINTN       Index;

  ResidualStart = NULL;
  TempRand      = 0;

  //
  // Compute the address of the first word aligned (32/64-bit) block in the 
  // destination buffer, depending on whether we are in 32- or 64-bit mode.
  //
  Start = RandBuffer;
  if (((UINT32)(UINTN)Start % (UINT32)sizeof(UINTN)) == 0) {
    BlockStart = (UINTN *)Start;
    Count      = Length;
    StartLen   = 0;
  } else {
    BlockStart = (UINTN *)(((UINTN)Start & ~(UINTN)(sizeof(UINTN) - 1)) + (UINTN)sizeof(UINTN));
    Count      = Length - (sizeof (UINTN) - (UINT32)((UINTN)Start % sizeof (UINTN)));
    StartLen   = (UINT32)((UINTN)BlockStart - (UINTN)Start);
  }

  //
  // Compute the number of word blocks and the remaining number of bytes.
  //
  Residual = Count % sizeof (UINTN);
  BlockNum = Count / sizeof (UINTN);
  if (Residual != 0) {
    ResidualStart = (UINT8 *) (BlockStart + BlockNum);
  }

  //
  // Obtain a temporary random number for use in the residuals. Failout if retry fails.
  //
  if (StartLen > 0) {
    Status = RdRandWord ((UINTN *) &TempRand, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Populate the starting mis-aligned block.
  //
  for (Index = 0; Index < StartLen; Index++) {
    Start[Index] = (UINT8)(TempRand & 0xff);
    TempRand     = TempRand >> 8;
  }

  //
  // Populate the central aligned block. Fail out if retry fails.
  //
  Status = RdRandGetWords (BlockNum, (UINTN *)(BlockStart));
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Populate the final mis-aligned block.
  //
  if (Residual > 0) {
    Status = RdRandWord ((UINTN *)&TempRand, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    for (Index = 0; Index < Residual; Index++) {
      ResidualStart[Index] = (UINT8)(TempRand & 0xff);
      TempRand             = TempRand >> 8;
    }
  }

  return EFI_SUCCESS;
}

/**
  Creates a 128bit random value that is fully forward and backward prediction resistant,
  suitable for seeding a NIST SP800-90 Compliant, FIPS 1402-2 certifiable SW DRBG.
  This function takes multiple random numbers through RDRAND without intervening
  delays to ensure reseeding and performs AES-CBC-MAC over the data to compute the
  seed value.
  
  @param[out]  SeedBuffer    Pointer to a 128bit buffer to store the random seed.

  @retval EFI_SUCCESS        Random seed generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RdRandGetSeed128 (
  OUT UINT8        *SeedBuffer
  )
{
  EFI_STATUS  Status;
  UINT8       RandByte[16];
  UINT8       Key[16];
  UINT8       Ffv[16];
  UINT8       Xored[16];
  UINT32      Index;
  UINT32      Index2;

  //
  // Chose an arbitary key and zero the feed_forward_value (FFV)
  //
  for (Index = 0; Index < 16; Index++) {
    Key[Index] = (UINT8) Index;
    Ffv[Index] = 0;
  }

  //
  // Perform CBC_MAC over 32 * 128 bit values, with 10us gaps between 128 bit value
  // The 10us gaps will ensure multiple reseeds within the HW RNG with a large design margin.
  //
  for (Index = 0; Index < 32; Index++) {
    MicroSecondDelay (10);
    Status = RdRandGetBytes (16, RandByte);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Perform XOR operations on two 128-bit value.
    //
    for (Index2 = 0; Index2 < 16; Index2++) {
      Xored[Index2] = RandByte[Index2] ^ Ffv[Index2];
    }

    AesEncrypt (Key, Xored, Ffv);
  }

  for (Index = 0; Index < 16; Index++) {
    SeedBuffer[Index] = Ffv[Index];
  }

  return EFI_SUCCESS;
}

/**
  Generate high-quality entropy source through RDRAND.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
EFI_STATUS
EFIAPI
RdRandGenerateEntropy (
  IN UINTN         Length,
  OUT UINT8        *Entropy
  )
{
  EFI_STATUS  Status;
  UINTN       BlockCount;
  UINT8       Seed[16];
  UINT8       *Ptr;

  Status     = EFI_NOT_READY;
  BlockCount = Length / 16;
  Ptr        = (UINT8 *)Entropy;

  //
  // Generate high-quality seed for DRBG Entropy
  //
  while (BlockCount > 0) {
    Status = RdRandGetSeed128 (Seed);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (Ptr, Seed, 16);

    BlockCount--;
    Ptr = Ptr + 16;
  }

  //
  // Populate the remained data as request.
  //
  Status = RdRandGetSeed128 (Seed);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  CopyMem (Ptr, Seed, (Length % 16));

  return Status;
}
