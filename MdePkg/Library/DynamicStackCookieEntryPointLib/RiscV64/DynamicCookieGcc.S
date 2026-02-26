#------------------------------------------------------------------------------
#
# Copyright (c) Xiang W <wangxiang@iscas.ac.cn>.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   DynamicCookie.S
#
# Abstract:
#
#   Generates random number through the Zkr extensions on a 64-bit RISC-V platform
#   to store a random value in the GCC __stack_check_guard stack cookie.
#
# Notes:
#
# If Zkr fails, the build time static stack cookie value will be used instead.
#
#------------------------------------------------------------------------------

#include <Base.h>
#include <Register/RiscV64/RiscVImpl.h>

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# _ModuleEntryPoint (
#   Parameters are passed through.
# )
#------------------------------------------------------------------------------
ASM_FUNC(_ModuleEntryPoint)
  lla   t0, __stack_chk_guard_init_done
  csrrw t6, CSR_STVEC, t0
  li    t0, 0
  li    t3, SEED_OPST_ES16
  li    t4, SEED_ENTROPY_MASK
  li    t5, __SIZEOF_POINTER__
__stack_chk_guard_init_loop:
  csrrw  t1, CSR_SEED, zero
  li    t2, SEED_OPST_MASK
  and   t2, t1, t2
  bgtu  t2, t3, __stack_chk_guard_init_done
  bltu  t2, t3, __stack_chk_guard_init_loop
  and   t1, t1, t4
  slli  t0, t0, 16
  or    t0, t0, t1
  addi  t5, t5, -2
  bgtz  t5, __stack_chk_guard_init_loop
  lla   t1, ASM_PFX(__stack_chk_guard)

  // Zero first byte of random cookie prevent string copy functions from clobbering
  // the stack cookie.
  slli  t0, t0, 8

  sd    t0, 0(t1)
  j     __stack_chk_guard_init_done
  .align 3
__stack_chk_guard_init_done:
  csrw  CSR_STVEC, t6
  j     ASM_PFX(_CModuleEntryPoint)     // Jump to the C module entry point

