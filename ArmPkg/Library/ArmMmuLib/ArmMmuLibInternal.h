/** @file
  Arm MMU library instance internal header file.

  Copyright (C) Microsoft Corporation. All rights reserved.
  Copyright (C) Google LLC. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_MMU_LIB_INTERNAL_H_
#define ARM_MMU_LIB_INTERNAL_H_

/**
  Internal helper to safely switch the translation base register

  @param[in]  RootTable       Address of the root page table
  @param[in]  Asid            The Address Space Identifier
**/
VOID
ArmSwitchTtbr (
  IN  UINT64  *RootTable,
  IN  UINTN   Asid
  );

#endif
