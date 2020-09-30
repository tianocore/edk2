#------------------------------------------------------------------------------
#
# ArmReadIdIsar0() for AArch64
#
# Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

.text
.p2align 2
GCC_ASM_EXPORT(ArmReadIdIsar0)

#/**
#  Reads the ID_AA64ISAR0 Register.
#
#  @return The contents of the ID_AA64ISAR0 register.
#
#**/
#UINT64
#EFIAPI
#ArmReadIdIsar0 (
#  VOID
#  );
#
ASM_PFX(ArmReadIdIsar0):
  mrs  x0, id_aa64isar0_el1 // Read ID_AA64ISAR0 Register
  ret


