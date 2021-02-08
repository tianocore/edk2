/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ARM_V7_LIB_H__
#define __ARM_V7_LIB_H__

#define ID_MMFR0_SHARELVL_SHIFT       12
#define ID_MMFR0_SHARELVL_MASK       0xf
#define ID_MMFR0_SHARELVL_ONE          0
#define ID_MMFR0_SHARELVL_TWO          1

#define ID_MMFR0_INNERSHR_SHIFT       28
#define ID_MMFR0_INNERSHR_MASK       0xf
#define ID_MMFR0_OUTERSHR_SHIFT        8
#define ID_MMFR0_OUTERSHR_MASK       0xf

#define ID_MMFR0_SHR_IMP_UNCACHED      0
#define ID_MMFR0_SHR_IMP_HW_COHERENT   1
#define ID_MMFR0_SHR_IGNORED         0xf

typedef VOID (*ARM_V7_CACHE_OPERATION)(UINT32);

VOID
ArmV7AllDataCachesOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  );

VOID
EFIAPI
ArmInvalidateDataCacheEntryBySetWay (
  IN  UINTN  SetWayFormat
  );

VOID
EFIAPI
ArmCleanDataCacheEntryBySetWay (
  IN  UINTN  SetWayFormat
  );

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryBySetWay (
  IN  UINTN   SetWayFormat
  );

/** Reads the ID_MMFR4 register.

   @return The contents of the ID_MMFR4 register.
**/
UINT32
EFIAPI
ArmReadIdMmfr4 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdPfr1 (
  VOID
  );

#endif // __ARM_V7_LIB_H__

