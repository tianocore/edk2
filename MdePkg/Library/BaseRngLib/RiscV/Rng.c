/** @file
   Random number generator service that uses the SEED instruction
   to provide pseudorandom numbers.

   Copyright (c) 2024, Rivos, Inc.
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

   SPDX-License-Identifier: BSD-2-Clause-Patent
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>
#include <Register/RiscV64/RiscVEncoding.h>

#include "BaseRngLibInternals.h"
#define RISCV_CPU_FEATURE_ZKR_BITMASK  0x8

#define SEED_RETRY_LOOPS  100

// 64-bit CPU-based RNG using risc-v Zkr instruction

// Defined in Seed.S
extern UINT64
ReadSeed (
  VOID
  );

/**
   Gets seed value by executing trng instruction (CSR 0x15) amd returns
   the see to the caller 64bit value.

   @param[out] Out     Buffer pointer to store the 64-bit random value.
   @retval TRUE         Random number generated successfully.
   @retval FALSE        Failed to generate the random number.
 **/
STATIC
BOOLEAN
Get64BitSeed (
  OUT UINT64  *Out
  )
{
  UINT64  Seed;
  UINTN   Retry;
  UINTN   ValidSeeds;
  UINTN   NeededSeeds;
  UINT16  *Entropy;

  Retry       = SEED_RETRY_LOOPS;
  Entropy     = (UINT16 *)Out;
  NeededSeeds = sizeof (UINT64) / sizeof (UINT16);
  ValidSeeds  = 0;

  if (!ArchIsRngSupported ()) {
    DEBUG ((DEBUG_ERROR, "Get64BitSeed: HW not supported!\n"));
    return FALSE;
  }

  do {
    Seed = ReadSeed ();

    switch (Seed & SEED_OPST_MASK) {
      case SEED_OPST_ES16:
        Entropy[ValidSeeds++] = Seed & SEED_ENTROPY_MASK;
        if (ValidSeeds == NeededSeeds) {
          return TRUE;
        }

        break;

      case SEED_OPST_DEAD:
        DEBUG ((DEBUG_ERROR, "Get64BitSeed: Unrecoverable error!\n"));
        return FALSE;

      case SEED_OPST_BIST:           // fallthrough
      case SEED_OPST_WAIT:           // fallthrough
      default:
        continue;
    }
  } while (--Retry);

  return FALSE;
}

/**
   Constructor library which initializes Seeds and mStatus array.

   @retval EFI_SUCCESS  initialization was successful.
   @retval EFI_UNSUPPORTED Feature not supported.

 **/
EFI_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
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
  OUT UINT16  *Rand
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
  OUT UINT32  *Rand
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
  OUT UINT64  *Rand
  )
{
  UINT64  Y;

  if (!Get64BitSeed (&Y)) {
    return FALSE;
  }

  *Rand = Y;
  return TRUE;
}

/**
   Checks whether SEED is supported.

   @retval TRUE         SEED is supported.
 **/
BOOLEAN
EFIAPI
ArchIsRngSupported (
  VOID
  )
{
  return ((PcdGet64 (PcdRiscVFeatureOverride) & RISCV_CPU_FEATURE_ZKR_BITMASK) != 0);
}
