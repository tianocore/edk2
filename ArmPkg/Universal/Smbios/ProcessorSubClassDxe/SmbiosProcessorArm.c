/** @file
  CacheArchitectureArm.c

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/ArmLib/ArmLibPrivate.h>

/** Gets the size of the specified cache.

    @param CacheLevel       The cache level (L1, L2 etc.). Zero based.
    @param InstructionCache Whether the cache is a dedicated instruction cache.
    @param DataCache        Whether the cache is a dedicated data cache.
    @param UnifiedCache     Whether the cache is a unified cache.

    @return The cache size.
**/
UINT64
ArmGetCacheSize (
  IN UINT8   CacheLevel,
  IN BOOLEAN InstructionCache,
  IN BOOLEAN DataCache,
  IN BOOLEAN UnifiedCache
  )
{
  CCSIDR_DATA  Ccsidr;
  CCSIDR2_DATA Ccsidr2;
  CSSELR_DATA  Csselr;
  BOOLEAN      CcidxSupported;
  UINT64       CacheSize;

  // Read the CCSIDR register to get the cache architecture
  Csselr.Data = 0;
  Csselr.Bits.Level = CacheLevel;
  Csselr.Bits.InD = (InstructionCache | UnifiedCache);

  Ccsidr.Data = ReadCCSIDR (Csselr.Data);

  CcidxSupported = ArmHasCcidx ();

  if (CcidxSupported) {
    Ccsidr2.Data = ReadCCSIDR2 (Csselr.Data);
    CacheSize = (UINT64)(1 << (Ccsidr.BitsCcidxAA32.LineSize + 4)) *
                                (Ccsidr.BitsCcidxAA32.Associativity + 1) *
                                (Ccsidr2.Bits.NumSets + 1);
  } else {
    CacheSize = (1 << (Ccsidr.BitsNonCcidx.LineSize + 4)) *
                        (Ccsidr.BitsNonCcidx.Associativity + 1) *
                        (Ccsidr.BitsNonCcidx.NumSets + 1);
  }

  return CacheSize;
}

/** Gets the associativity of the specified cache.

    @param CacheLevel       The cache level (L1, L2 etc.). Zero based.
    @param InstructionCache Whether the cache is a dedicated instruction cache.
    @param DataCache        Whether the cache is a dedicated data cache.
    @param UnifiedCache     Whether the cache is a unified cache.

    @return The cache associativity.
**/
UINT32
ArmGetCacheAssociativity (
  IN UINT8   CacheLevel,
  IN BOOLEAN InstructionCache,
  IN BOOLEAN DataCache,
  IN BOOLEAN UnifiedCache
  )
{
  CCSIDR_DATA  Ccsidr;
  CCSIDR2_DATA Ccsidr2;
  CSSELR_DATA  Csselr;
  BOOLEAN      CcidxSupported;
  UINT32       Associativity;

  // Read the CCSIDR register to get the cache architecture
  Csselr.Data = 0;
  Csselr.Bits.Level = CacheLevel;
  Csselr.Bits.InD = (InstructionCache | UnifiedCache);

  Ccsidr.Data = ReadCCSIDR (Csselr.Data);

  CcidxSupported = ArmHasCcidx ();

  if (CcidxSupported) {
    Ccsidr2.Data = ReadCCSIDR2 (Csselr.Data);
    Associativity = Ccsidr.BitsCcidxAA32.Associativity;
  } else {
    Associativity = Ccsidr.BitsNonCcidx.Associativity;
  }

  return Associativity;
}

