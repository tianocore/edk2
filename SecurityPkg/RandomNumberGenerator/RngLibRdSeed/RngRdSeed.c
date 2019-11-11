/** @file
  Random number generator services that uses RdSeed instruction access
  to provide non-deterministic random numbers, which are usually used
  for seeding other pseudo-random number generators.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>

//
// Bit mask used to determine if RdSeed instruction is supported.
//
#define RDSEED_MASK                  BIT18

//
// Limited retry number when valid random data is returned.
// It varies between 1 and 100 according to "Intel(R) DRGN Software Implementation
// Guide". Let's use the same value as RDRAND in BaseRngLib.
//
#define RDSEED_RETRY_LIMIT           10

/**
  The constructor function checks whether or not RDSEED instruction is supported
  by the host hardware.

  The constructor function checks whether or not RDSEED instruction is supported.
  It will ASSERT() if RDSEED instruction is not supported.

  @retval RETURN_SUCCESS      The processor supports RDSEED instruction.
  @retval RETURN_UNSUPPORTED  RDSEED instruction is not supported.

**/
RETURN_STATUS
EFIAPI
RngLibRdSeedConstructor (
  VOID
  )
{
  UINT32  RegEbx;

  //
  // Determine RDSEED support by examining bit 18 of the EBX register returned by
  // CPUID(EAX=7, ECX=0). BIT18 of EBX indicates that processor support RDSEED
  // instruction.
  //
  AsmCpuidEx (7, 0, NULL, &RegEbx, NULL, NULL);
  if ((RegEbx & RDSEED_MASK) != RDSEED_MASK) {
    ASSERT ((RegEbx & RDSEED_MASK) == RDSEED_MASK);
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
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
  OUT     UINT16                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  //
  // A loop to fetch a 16 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDSEED_RETRY_LIMIT; Index++) {
    if (AsmRdSeed16 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
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
  OUT     UINT32                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  //
  // A loop to fetch a 32 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDSEED_RETRY_LIMIT; Index++) {
    if (AsmRdSeed32 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
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
  OUT     UINT64                    *Rand
  )
{
  UINT32  Index;

  ASSERT (Rand != NULL);

  //
  // A loop to fetch a 64 bit random value with a retry count limit.
  //
  for (Index = 0; Index < RDSEED_RETRY_LIMIT; Index++) {
    if (AsmRdSeed64 (Rand)) {
      return TRUE;
    }
  }

  return FALSE;
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
  OUT     UINT64                    *Rand
  )
{
  ASSERT (Rand != NULL);

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
