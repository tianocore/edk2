/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>

#include <Chipset/AArch64.h>

#include "AArch64Lib.h"
#include "ArmLibPrivate.h"

/** Get bits from a value.

  Shift the input value from 'shift' bits and apply 'mask'.

  @param   value    The value to get the bits from.
  @param   shift    Index of the bits to read.
  @param   mask     Mask to apply to the value once shifted.

  @return  The desired bitfield from the value.
**/
#define GET_BITFIELD(value, shift, mask)    \
  ((value >> shift) & mask)

VOID
AArch64DataCacheOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN  SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();
  ArmDisableInterrupts ();

  AArch64AllDataCachesOperation (DataCacheOperation);

  ArmDataSynchronizationBarrier ();

  if (SavedInterruptState) {
    ArmEnableInterrupts ();
  }
}

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  )
{
  ASSERT (!ArmMmuEnabled ());

  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ASSERT (!ArmMmuEnabled ());

  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ASSERT (!ArmMmuEnabled ());

  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmCleanDataCacheEntryBySetWay);
}

/**
  Check whether the CPU supports the GIC system register interface (any version)

  @return   Whether GIC System Register Interface is supported

**/
BOOLEAN
EFIAPI
ArmHasGicSystemRegisters (
  VOID
  )
{
  return ((ArmReadIdAA64Pfr0 () & AARCH64_PFR0_GIC) != 0);
}

/** Checks if CCIDX is implemented.

   @retval TRUE  CCIDX is implemented.
   @retval FALSE CCIDX is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasCcidx (
  VOID
  )
{
  UINTN  Mmfr2;

  Mmfr2 = ArmReadIdAA64Mmfr2 ();
  return (((Mmfr2 >> 20) & 0xF) == 1) ? TRUE : FALSE;
}

/**
  Checks whether the CPU implements the Virtualization Host Extensions.

  @retval TRUE  FEAT_VHE is implemented.
  @retval FALSE FEAT_VHE is not mplemented.
**/
BOOLEAN
EFIAPI
ArmHasVhe (
  VOID
  )
{
  return ((ArmReadIdAA64Mmfr1 () & AARCH64_MMFR1_VH) != 0);
}

/**
  Checks whether the CPU implements the Trace Buffer Extension.

  @retval TRUE  FEAT_TRBE is implemented.
  @retval FALSE FEAT_TRBE is not mplemented.
**/
BOOLEAN
EFIAPI
ArmHasTrbe (
  VOID
  )
{
  return ((ArmReadIdAA64Dfr0 () & AARCH64_DFR0_TRBE) != 0);
}

/**
  Checks whether the CPU implements the Embedded Trace Extension.

  @retval TRUE  FEAT_ETE is implemented.
  @retval FALSE FEAT_ETE is not mplemented.
**/
BOOLEAN
EFIAPI
ArmHasEte (
  VOID
  )
{
  // The ID_AA64DFR0_EL1.TraceVer field identifies the presence of FEAT_ETE.
  return ((ArmReadIdAA64Dfr0 () & AARCH64_DFR0_TRACEVER) != 0);
}

/**
  Checks whether the CPU implements FEAT_AES.

  @retval TRUE  FEAT_AES is implemented.
  @retval FALSE FEAT_AES is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasAes (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_AES_SHIFT,
           ARM_ID_AA64ISAR0_EL1_AES_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_PMULL.

  @retval TRUE  FEAT_PMULL is implemented.
  @retval FALSE FEAT_PMULL is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasPmull (
  VOID
  )
{
  //
  // Only check BIT1 of AES field, bits [7:4]
  //
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_AES_SHIFT,
           ARM_ID_AA64ISAR0_EL1_AES_FEAT_PMULL_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SHA1.

  @retval TRUE  FEAT_SHA1 is implemented.
  @retval FALSE FEAT_SHA1 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSha1 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SHA1_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SHA1_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SHA256.

  @retval TRUE  FEAT_SHA256 is implemented.
  @retval FALSE FEAT_SHA256 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSha256 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SHA2_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SHA2_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SHA512.

  @retval TRUE  FEAT_SHA512 is implemented.
  @retval FALSE FEAT_SHA512 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSha512 (
  VOID
  )
{
  //
  // Only check BIT1 of SHA2 field, bits [15:12]
  //
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SHA2_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SHA2_FEAT_SHA512_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements CRC32 instruction.

  @retval TRUE  CRC32 instruction is implemented.
  @retval FALSE CRC32 instruction is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasCrc32 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_CRC32_SHIFT,
           ARM_ID_AA64ISAR0_EL1_CRC32_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_LSE.

  @retval TRUE  FEAT_LSE is implemented.
  @retval FALSE FEAT_LSE is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasLse (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_ATOMIC_SHIFT,
           ARM_ID_AA64ISAR0_EL1_ATOMIC_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_RDM.

  @retval TRUE  FEAT_RDM is implemented.
  @retval FALSE FEAT_RDM is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasRdm (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_RDM_SHIFT,
           ARM_ID_AA64ISAR0_EL1_RDM_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SHA3.

  @retval TRUE  FEAT_SHA3 is implemented.
  @retval FALSE FEAT_SHA3 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSha3 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SHA3_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SHA3_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SM3.

  @retval TRUE  FEAT_SM3 is implemented.
  @retval FALSE FEAT_SM3 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSm3 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SM3_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SM3_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_SM4.

  @retval TRUE  FEAT_SM4 is implemented.
  @retval FALSE FEAT_SM4 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasSm4 (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_SM4_SHIFT,
           ARM_ID_AA64ISAR0_EL1_SM4_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_DotProd.

  @retval TRUE  FEAT_DotProd is implemented.
  @retval FALSE FEAT_DotProd is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasDp (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_DP_SHIFT,
           ARM_ID_AA64ISAR0_EL1_DP_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_FHM.

  @retval TRUE  FEAT_FHM is implemented.
  @retval FALSE FEAT_FHM is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasFhm (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_FHM_SHIFT,
           ARM_ID_AA64ISAR0_EL1_FHM_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_FlagM.

  @retval TRUE  FEAT_FlagM is implemented.
  @retval FALSE FEAT_FlagM is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasFlagm (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_TS_SHIFT,
           ARM_ID_AA64ISAR0_EL1_TS_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_FlagM2.

  @retval TRUE  FEAT_FlagM2 is implemented.
  @retval FALSE FEAT_FlagM2 is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasFlagm2 (
  VOID
  )
{
  //
  // Only check BIT1 of TS field, bits [55:52]
  //
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_TS_SHIFT,
           ARM_ID_AA64ISAR0_EL1_TS_FEAT_FLAGM2_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_TLBIOS.

  @retval TRUE  FEAT_TLBIOS is implemented.
  @retval FALSE FEAT_TLBIOS is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasTlbios (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_TLB_SHIFT,
           ARM_ID_AA64ISAR0_EL1_TLB_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_TLBIRANGE.

  @retval TRUE  FEAT_TLBIRANGE is implemented.
  @retval FALSE FEAT_TLBIRANGE is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasTlbirange (
  VOID
  )
{
  //
  // Only check BIT1 of TLB field, bits [59:56]
  //
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_TLB_SHIFT,
           ARM_ID_AA64ISAR0_EL1_TLB_FEAT_TLBIRANGE_MASK
           ) != 0;
}

/**
  Checks whether the CPU implements FEAT_RNG.

  @retval TRUE  FEAT_RNG is implemented.
  @retval FALSE FEAT_RNG is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasRndr (
  VOID
  )
{
  return GET_BITFIELD (
           ArmReadIdAA64Isar0 (),
           ARM_ID_AA64ISAR0_EL1_RNDR_SHIFT,
           ARM_ID_AA64ISAR0_EL1_RNDR_MASK
           ) != 0;
}
