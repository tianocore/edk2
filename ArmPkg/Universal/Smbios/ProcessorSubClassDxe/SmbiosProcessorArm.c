/** @file
  Functions for ARM processor information

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/ArmCache.h>
#include <Library/ArmLib.h>

#include "SmbiosProcessor.h"

/** Gets the size of the specified cache.

    @param CacheLevel       The cache level (L1, L2 etc.).
    @param DataCache        Whether the cache is a dedicated data cache.
    @param UnifiedCache     Whether the cache is a unified cache.

    @return The cache size.
**/
UINT64
SmbiosProcessorGetCacheSize (
  IN UINT8   CacheLevel,
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
  Csselr.Bits.Level = CacheLevel - 1;
  Csselr.Bits.InD = (!DataCache && !UnifiedCache);

  Ccsidr.Data = ReadCCSIDR (Csselr.Data);

  CcidxSupported = ArmHasCcidx ();

  if (CcidxSupported) {
    Ccsidr2.Data = ReadCCSIDR2 (Csselr.Data);
    CacheSize = (1 << (Ccsidr.BitsCcidxAA32.LineSize + 4)) *
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

    @param CacheLevel       The cache level (L1, L2 etc.).
    @param DataCache        Whether the cache is a dedicated data cache.
    @param UnifiedCache     Whether the cache is a unified cache.

    @return The cache associativity.
**/
UINT32
SmbiosProcessorGetCacheAssociativity (
  IN UINT8   CacheLevel,
  IN BOOLEAN DataCache,
  IN BOOLEAN UnifiedCache
  )
{
  CCSIDR_DATA  Ccsidr;
  CSSELR_DATA  Csselr;
  BOOLEAN      CcidxSupported;
  UINT32       Associativity;

  // Read the CCSIDR register to get the cache architecture
  Csselr.Data = 0;
  Csselr.Bits.Level = CacheLevel - 1;
  Csselr.Bits.InD = (!DataCache && !UnifiedCache);

  Ccsidr.Data = ReadCCSIDR (Csselr.Data);

  CcidxSupported = ArmHasCcidx ();

  if (CcidxSupported) {
    Associativity = Ccsidr.BitsCcidxAA32.Associativity + 1;
  } else {
    Associativity = Ccsidr.BitsNonCcidx.Associativity + 1;
  }

  return Associativity;
}

