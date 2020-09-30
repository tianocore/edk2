/** @file
  Random number generator services that uses RdRand instruction access
  to provide high-quality random numbers.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "BaseRngLibInternals.h"

//
// Bit mask used to determine if RdRand instruction is supported.
//
#define RDRAND_MASK                  BIT30

/**
  The constructor function checks whether or not RDRAND instruction is supported
  by the host hardware.

  The constructor function checks whether or not RDRAND instruction is supported.
  It will ASSERT() if RDRAND instruction is not supported.
  It will always return RETURN_SUCCESS.

  @retval RETURN_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
  UINT32  RegEcx;

  //
  // Determine RDRAND support by examining bit 30 of the ECX register returned by
  // CPUID. A value of 1 indicates that processor support RDRAND instruction.
  //
  AsmCpuid (1, 0, 0, &RegEcx, 0);
  ASSERT ((RegEcx & RDRAND_MASK) == RDRAND_MASK);

  return RETURN_SUCCESS;
}

/**
  Generates a 16-bit random number.

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber16 (
  OUT     UINT16                    *Rand
  )
{
  return AsmRdRand16 (Rand);
}

/**
  Generates a 32-bit random number.

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber32 (
  OUT     UINT32                    *Rand
  )
{
  return AsmRdRand32 (Rand);
}

/**
  Generates a 64-bit random number.

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber64 (
  OUT     UINT64                    *Rand
  )
{
  return AsmRdRand64 (Rand);
}

