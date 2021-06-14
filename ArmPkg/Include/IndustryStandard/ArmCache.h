/** @file

  Copyright (c) 2020 - 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_CACHE_H_
#define ARM_CACHE_H_

#include <Uefi/UefiBaseType.h>

// The ARM Architecture Reference Manual for ARMv8-A defines up
// to 7 levels of cache, L1 through L7.
#define MAX_ARM_CACHE_LEVEL   7

/// Defines the structure of the CSSELR (Cache Size Selection) register
typedef union {
  struct {
    UINT32    InD       :1;  ///< Instruction not Data bit
    UINT32    Level     :3;  ///< Cache level (zero based)
    UINT32    TnD       :1;  ///< Allocation not Data bit
    UINT32    Reserved  :27; ///< Reserved, RES0
  } Bits; ///< Bitfield definition of the register
  UINT32 Data; ///< The entire 32-bit value
} CSSELR_DATA;

/// The cache type values for the InD field of the CSSELR register
typedef enum
{
  /// Select the data or unified cache
  CsselrCacheTypeDataOrUnified = 0,
  /// Select the instruction cache
  CsselrCacheTypeInstruction,
  CsselrCacheTypeMax
} CSSELR_CACHE_TYPE;

/// Defines the structure of the CCSIDR (Current Cache Size ID) register
typedef union {
  struct {
    UINT64    LineSize           :3;  ///< Line size (Log2(Num bytes in cache) - 4)
    UINT64    Associativity      :10; ///< Associativity - 1
    UINT64    NumSets            :15; ///< Number of sets in the cache -1
    UINT64    Unknown            :4;  ///< Reserved, UNKNOWN
    UINT64    Reserved           :32; ///< Reserved, RES0
  } BitsNonCcidx; ///< Bitfield definition of the register when FEAT_CCIDX is not supported.
  struct {
    UINT64    LineSize           :3;  ///< Line size (Log2(Num bytes in cache) - 4)
    UINT64    Associativity      :21; ///< Associativity - 1
    UINT64    Reserved1          :8;  ///< Reserved, RES0
    UINT64    NumSets            :24; ///< Number of sets in the cache -1
    UINT64    Reserved2          :8;  ///< Reserved, RES0
  } BitsCcidxAA64; ///< Bitfield definition of the register when FEAT_IDX is supported.
  struct {
    UINT64    LineSize           : 3;
    UINT64    Associativity      : 21;
    UINT64    Reserved           : 8;
    UINT64    Unallocated        : 32;
  } BitsCcidxAA32;
  UINT64 Data; ///< The entire 64-bit value
} CCSIDR_DATA;

/// Defines the structure of the AARCH32 CCSIDR2 register.
typedef union {
  struct {
    UINT32 NumSets               :24; ///< Number of sets in the cache - 1
    UINT32 Reserved              :8;  ///< Reserved, RES0
  } Bits; ///< Bitfield definition of the register
  UINT32 Data; ///< The entire 32-bit value
} CCSIDR2_DATA;

/** Defines the structure of the CLIDR (Cache Level ID) register.
 *
 * The lower 32 bits are the same for both AARCH32 and AARCH64
 * so we can use the same structure for both.
**/
typedef union {
  struct {
    UINT32    Ctype1   : 3; ///< Level 1 cache type
    UINT32    Ctype2   : 3; ///< Level 2 cache type
    UINT32    Ctype3   : 3; ///< Level 3 cache type
    UINT32    Ctype4   : 3; ///< Level 4 cache type
    UINT32    Ctype5   : 3; ///< Level 5 cache type
    UINT32    Ctype6   : 3; ///< Level 6 cache type
    UINT32    Ctype7   : 3; ///< Level 7 cache type
    UINT32    LoUIS    : 3; ///< Level of Unification Inner Shareable
    UINT32    LoC      : 3; ///< Level of Coherency
    UINT32    LoUU     : 3; ///< Level of Unification Uniprocessor
    UINT32    Icb      : 3; ///< Inner Cache Boundary
  } Bits; ///< Bitfield definition of the register
  UINT32 Data; ///< The entire 32-bit value
} CLIDR_DATA;

/// The cache types reported in the CLIDR register.
typedef enum {
  /// No cache is present
  ClidrCacheTypeNone = 0,
  /// There is only an instruction cache
  ClidrCacheTypeInstructionOnly,
  /// There is only a data cache
  ClidrCacheTypeDataOnly,
  /// There are separate data and instruction caches
  ClidrCacheTypeSeparate,
  /// There is a unified cache
  ClidrCacheTypeUnified,
  ClidrCacheTypeMax
} CLIDR_CACHE_TYPE;

#define CLIDR_GET_CACHE_TYPE(x, level) ((x >> (3 * (level))) & 0b111)

#endif /* ARM_CACHE_H_ */
