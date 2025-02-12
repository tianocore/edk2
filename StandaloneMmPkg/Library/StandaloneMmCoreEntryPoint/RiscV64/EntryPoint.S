/*
  Copyright (c) 2022 Ventana Micro Systems Inc.
  Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 */
.text
.align 3

#include <Register/RiscV64/RiscVImpl.h>
#include <StandaloneMmCoreEntryPoint.h>

ASM_FUNC (_ModuleEntryPoint)
  /*
   * a0: hart ID
   * a1: Boot info address
   */

  li    a2, BOOT_INFO_STACK_BASE_OFFSET
  add   a2, a1, a2
  ld    sp, 0(a2)
  call  CModuleEntryPoint

