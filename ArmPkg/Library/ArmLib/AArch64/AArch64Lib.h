/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AARCH64_LIB_H_
#define AARCH64_LIB_H_

UINTN
EFIAPI
ArmReadIdAA64Dfr0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Dfr1 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Isar0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Isar1 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Isar2 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Mmfr0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Mmfr1 (
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

UINTN
EFIAPI
ArmReadIdAA64Pfr0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Pfr1 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdAA64Pfr2 (
  VOID
  );

#endif // AARCH64_LIB_H_
