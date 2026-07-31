/** @file
  Random number generator services that uses the stdlib
  function rand() to provide random numbers.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

STATIC BOOLEAN  mRandSeeded = FALSE;

/**
 * Constructor for UnitTestHostRngLib.

  @retval TRUE         Support.
**/
EFI_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
  if (!mRandSeeded) {
    srand ((unsigned)time (NULL));
    mRandSeeded = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Generates a 16-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber16 (
  OUT     UINT16  *Rand
  )
{
  int  RandLo;

  RandLo = rand ();

  *Rand = (UINT16)RandLo;

  return TRUE;
}

/**
  Generates a 32-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber32 (
  OUT     UINT32  *Rand
  )
{
  int  RandLo;

  RandLo = rand ();

  *Rand = (UINT32)RandLo;

  return TRUE;
}

/**
  Generates a 64-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT     UINT64  *Rand
  )
{
  int  RandHi;
  int  RandLo;

  RandLo = rand ();
  RandHi = rand ();

  *Rand = (UINT64)(((UINT64)RandHi << 32) | (UINT32)RandLo);

  return TRUE;
}

/**
  Generates a 128-bit random number.

  if Rand is NULL, then ASSERT().

  @param[out] Rand     Buffer pointer to store the 128-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber128 (
  OUT     UINT64  *Rand
  )
{
  //
  // Read first 64 bits
  //
  if (!GetRandomNumber64 (Rand)) {
    return FALSE;
  }

  //
  // Read second 64 bits
  //
  return GetRandomNumber64 (++Rand);
}
