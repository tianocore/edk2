/** @file
  IA-32/x64 AsmRdSeedxx()
  Generates random seed through CPU RdSeed instruction.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"

/**
  Generates a 16-bit random seed through RDSEED instruction.

  if Seed is NULL, then ASSERT().

  @param[out]  Seed     Buffer pointer to store the seed data.

  @retval TRUE          RDSEED call was successful.
  @retval FALSE         Failed attempts to call RDSEED.

 **/
BOOLEAN
EFIAPI
AsmRdSeed16 (
  OUT     UINT16                    *Seed
  )
{
  ASSERT (Seed != NULL);
  return InternalX86RdSeed16 (Seed);
}

/**
  Generates a 32-bit random seed through RDSEED instruction.

  if Seed is NULL, then ASSERT().

  @param[out]  Seed     Buffer pointer to store the seed data.

  @retval TRUE          RDSEED call was successful.
  @retval FALSE         Failed attempts to call RDSEED.

**/
BOOLEAN
EFIAPI
AsmRdSeed32 (
  OUT     UINT32                    *Seed
  )
{
  ASSERT (Seed != NULL);
  return InternalX86RdSeed32 (Seed);
}

/**
  Generates a 64-bit random seed through RDSEED instruction.

  if Seed is NULL, then ASSERT().

  @param[out]  Seed     Buffer pointer to store the seed data.

  @retval TRUE          RDSEED call was successful.
  @retval FALSE         Failed attempts to call RDSEED.

**/
BOOLEAN
EFIAPI
AsmRdSeed64  (
  OUT     UINT64                    *Seed
  )
{
  ASSERT (Seed != NULL);
  return InternalX86RdSeed64 (Seed);
}
