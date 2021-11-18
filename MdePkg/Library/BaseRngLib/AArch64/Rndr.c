/** @file
  Random number generator service that uses the RNDR instruction
  to provide pseudorandom numbers.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>

#include "ArmRng.h"
#include "BaseRngLibInternals.h"

STATIC BOOLEAN  mRndrSupported;

//
// Bit mask used to determine if RNDR instruction is supported.
//
#define RNDR_MASK  ((UINT64)MAX_UINT16 << 60U)

/**
  The constructor function checks whether or not RNDR instruction is supported
  by the host hardware.

  The constructor function checks whether or not RNDR instruction is supported.
  It will ASSERT() if RNDR instruction is not supported.
  It will always return EFI_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
  UINT64  Isar0;

  //
  // Determine RNDR support by examining bits 63:60 of the ISAR0 register returned by
  // MSR. A non-zero value indicates that the processor supports the RNDR instruction.
  //
  Isar0 = ArmReadIdIsar0 ();
  ASSERT ((Isar0 & RNDR_MASK) != 0);

  mRndrSupported = ((Isar0 & RNDR_MASK) != 0);

  return EFI_SUCCESS;
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
  OUT     UINT16  *Rand
  )
{
  UINT64  Rand64;

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
  OUT     UINT32  *Rand
  )
{
  UINT64  Rand64;

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
  OUT     UINT64  *Rand
  )
{
  return ArmRndr (Rand);
}

/**
  Checks whether RNDR is supported.

  @retval TRUE         RNDR is supported.
  @retval FALSE        RNDR is not supported.

**/
BOOLEAN
EFIAPI
ArchIsRngSupported (
  VOID
  )
{
  return mRndrSupported;
}
