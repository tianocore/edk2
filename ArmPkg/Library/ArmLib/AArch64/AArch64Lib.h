/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __AARCH64_LIB_H__
#define __AARCH64_LIB_H__

typedef VOID (*AARCH64_CACHE_OPERATION)(UINTN);

VOID
AArch64AllDataCachesOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
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

UINTN
EFIAPI
ArmReadIdAA64Pfr0 (
  VOID
  );

/** Reads the ID_AA64MMFR2_EL1 register.

   @return The contents of the ID_AA64MMFR2_EL1 register.
**/
UINTN
EFIAPI
ArmReadIdAA64Mmfr2 (
  VOID
  );

#endif // __AARCH64_LIB_H__

