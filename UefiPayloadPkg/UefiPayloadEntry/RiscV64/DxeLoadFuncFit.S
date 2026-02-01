# Copyright (c) Xiang W <wangxiang@iscas.ac.cn>.
# SPDX-License-Identifier: BSD-2-Clause-Patent

#include <Base.h>
#include <Register/RiscV64/RiscVImpl.h>

#define STACK_SIZE  0x4000

.align 12
.section .data.stack, "aw"
_stack:
  .zero STACK_SIZE

ASM_FUNC(_ModuleEntryPoint)
  lla   t0, ASM_PFX(_RiscV64BootHartID)
  sd    a0, 0(t0)
  lla   sp, _stack + STACK_SIZE
  mv    a0, a1
  call  ASM_PFX(FitUplEntryPoint)
