/** @file
  RDRAND Support Routines.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RdRand.h"

/**
  Generates a 64-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand64Step  (
  OUT UINT64   *Rand
  )
{
  UINT32  RandLow;
  UINT32  RandHigh;

  //
  // Generating a 64-bit rand on a 32-bit system by 
  // mapping two 32-bit RDRAND instructions.
  //
  if (!RdRand32Step (&RandLow)) {
    return FALSE;
  }
  if (!RdRand32Step (&RandHigh)) {
    return FALSE;
  }

  *Rand = (UINT64) RandLow | LShiftU64 ((UINT64)RandHigh, 32);

  return TRUE;
}

/**
  Calls RDRAND to request a word-length random number.

  @param[out]  Rand          Buffer pointer to store the random number.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        Random word generation succeeded.
  @retval EFI_NOT_READY      Failed to request random word.

**/
EFI_STATUS
EFIAPI
RdRandWord (
  OUT UINTN        *Rand,
  IN BOOLEAN       NeedRetry
  )
{
  return RdRand32 (Rand, NeedRetry);
}

/**
  Calls RDRAND to request multiple word-length random numbers.

  @param[in]   Length        Size of the buffer, in words, to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random words generation succeeded.
  @retval EFI_NOT_READY      Failed to request random words.

**/
EFI_STATUS
EFIAPI
RdRandGetWords (
  IN UINTN         Length,
  OUT UINTN        *RandBuffer
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  for (Index = 0; Index < Length; Index++) {
    //
    // Obtain one word-length (32-bit) Random Number with possible retry-loop.
    //
    Status = RdRand32 (RandBuffer, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    
    RandBuffer++;
  }

  return EFI_SUCCESS;
}
