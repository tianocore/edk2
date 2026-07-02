#------------------------------------------------------------------------------
#
# Copyright (c) Microsoft Corporation.
# Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   DynamicCookieGcc.S
#
# Abstract:
#
#   Generates random number through the RdTime instruction on a 64-bit LOONGARCH64 platform
#   to store a random value in the GCC __stack_check_guard stack cookie.
#   The first byte is 0'd to prevent string copy functions from clobbering
#   the stack cookie.
#
# Notes:
#
# Use the RngLib library to get random numbers. LoongArch64 currently does not support like
# `RdRand` or `RNDR` instructions, it can use the `RdTime` and a xorshift64 algorithms to
# generate pseudo-random numbers.
#
#------------------------------------------------------------------------------

#inlcude <Library/RngLib.h>

ASM_GLOBAL ASM_PFX (_ModuleEntryPoint)

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# _ModuleEntryPoint (
#   The parameters are saved to the stack when it calls other functions.
# )
#------------------------------------------------------------------------------
ASM_PFX(_ModuleEntryPoint):
  addi.d    $sp, $sp, -0x20
  st.d      $a0, $sp, 0x0
  st.d      $a1, $sp, 0x8
  st.d      $ra, $sp, 0x10

  la.pcrel  $a0, random_num
  bl        GetRandomNumber64

  la.pcrel  $a0, random_num
  ld.d      $t0, $a0, 0x0

  //
  // Zero the first byte of the random value
  //
  bstrins.d $t0, $zero, 8, 0

  la.local  $t1, __stack_chk_guard
  st.d      $t0, $t1, 0
  dbar      0

  ld.d      $a0, $sp, 0x0
  ld.d      $a1, $sp, 0x8
  ld.d      $ra, $sp, 0x10
  addi.d    $sp, $sp, 0x20

c_entry:
  b         _CModuleEntryPoint     // Jump to the C module entry point

random_num:
  .dword 0x0
.end
