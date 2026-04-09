/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2021 - 2022, ARM Ltd. All rights reserved.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock ("DsdtTable.aml", "DSDT", 2, "ARMLTD", "ARM-KVMT", 1) {
  Scope (_SB) {
    //
    // Most ACPI tables for Kvmtool firmware are
    // dynamically generated. The AML code is also
    // generated at runtime for most components in
    // appropriate SSDTs.
    // Although there may not be much to describe
    // in the DSDT, the DSDT table is mandatory.
    // Therefore, add an empty stub for DSDT.
    //
  } // Scope (_SB)
}
