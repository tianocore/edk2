/** @file
  SMBIOS Processor Related Functions.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_PROCESSOR_H_
#define SMBIOS_PROCESSOR_H_

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>

/** Returns the maximum cache level implemented by the current CPU.

    @return The maximum cache level implemented.
**/
UINT8
SmbiosProcessorGetMaxCacheLevel (
  VOID
  );

/** Returns whether or not the specified cache level has separate I/D caches.

    @param CacheLevel The cache level (L1, L2 etc.).

    @return TRUE if the cache level has separate I/D caches, FALSE otherwise.
**/
BOOLEAN
SmbiosProcessorHasSeparateCaches (
  UINT8 CacheLevel
  );

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
  );

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
  );

/** Returns a value for the Processor ID field that conforms to SMBIOS
    requirements.

    @return Processor ID.
**/
UINT64
SmbiosGetProcessorId (VOID);

/** Returns the external clock frequency.

    @return The external CPU clock frequency.
**/
UINTN
SmbiosGetExternalClockFrequency (VOID);

/** Returns the SMBIOS ProcessorFamily field value.

    @return The value for the ProcessorFamily field.
**/
UINT8
SmbiosGetProcessorFamily (VOID);

/** Returns the ProcessorFamily2 field value.

    @return The value for the ProcessorFamily2 field.
**/
UINT16
SmbiosGetProcessorFamily2 (VOID);

/** Returns the SMBIOS Processor Characteristics.

    @return Processor Characteristics bitfield.
**/
PROCESSOR_CHARACTERISTIC_FLAGS
SmbiosGetProcessorCharacteristics (VOID);

#endif // SMBIOS_PROCESSOR_H_
