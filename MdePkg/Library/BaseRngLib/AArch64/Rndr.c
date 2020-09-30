/** @file
  Random number generator service that uses the RNDR instruction
  to provide high-quality random numbers.

  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <Library/RngLib.h>

#include "BaseRngLibInternals.h"

//
// Bit mask used to determine if RNDR instruction is supported.
//
#define RNDR_MASK                  ((UINT64)MAX_UINT16 << 60U)

/**
  The constructor function checks whether or not RNDR instruction is supported
  by the host hardware.

  The constructor function checks whether or not RNDR instruction is supported.
  It will ASSERT() if RNDR instruction is not supported.
  It will always return RETURN_SUCCESS.

  @retval RETURN_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
RETURN_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
  UINT64 Isar0;
  //
  // Determine RNDR support by examining bits 63:60 of the ISAR0 register returned by
  // MSR. A non-zero value indicates that the processor supports the RNDR instruction.
  //
  Isar0 = ArmReadIdIsar0 ();
  ASSERT ((Isar0 & RNDR_MASK) != 0);
  (void)Isar0;

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
  UINT64 Rand64;

  if (ArchGetRandomNumber64 (&Rand64)) {
    *Rand = Rand64 & MAX_UINT16;
    return TRUE;
  }

  return FALSE;
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
  UINT64 Rand64;

  if (ArchGetRandomNumber64 (&Rand64)) {
    *Rand = Rand64 & MAX_UINT32;
    return TRUE;
  }

  return FALSE;
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
  return ArmRndr (Rand);
}

