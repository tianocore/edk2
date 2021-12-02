/** @file
  Header file for PPTT parser

  Copyright (c) 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ARM Architecture Reference Manual ARMv8 (D.a)
**/

#ifndef PPTT_PARSER_H_
#define PPTT_PARSER_H_

#if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)

/// Cache parameters allowed by the architecture with
/// ARMv8.3-CCIDX (Cache extended number of sets)
/// Derived from CCSIDR_EL1 when ID_AA64MMFR2_EL1.CCIDX==0001
#define PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX  (1 << 24)
#define PPTT_ARM_CCIDX_CACHE_ASSOCIATIVITY_MAX   (1 << 21)

/// Cache parameters allowed by the architecture without
/// ARMv8.3-CCIDX (Cache extended number of sets)
/// Derived from CCSIDR_EL1 when ID_AA64MMFR2_EL1.CCIDX==0000
#define PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX  (1 << 15)
#define PPTT_ARM_CACHE_ASSOCIATIVITY_MAX   (1 << 10)

/// Common cache parameters
/// Derived from CCSIDR_EL1
/// The LineSize is represented by bits 2:0
/// (Log2(Number of bytes in cache line)) - 4 is used to represent
/// the LineSize bits.
#define PPTT_ARM_CACHE_LINE_SIZE_MAX  (1 << 11)
#define PPTT_ARM_CACHE_LINE_SIZE_MIN  (1 << 4)

#endif // if defined (MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)

#endif // PPTT_PARSER_H_
