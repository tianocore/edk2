#------------------------------------------------------------------------------
#
# Read RNG for LoongArch64
#
# Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmRdRng16)
ASM_GLOBAL ASM_PFX(AsmRdRng32)
ASM_GLOBAL ASM_PFX(AsmRdRng64)

#/**
#  Readed timer register and XOR, mixed the high and low bits,
#  then use the xorshift64 algorithm to generate a 64-bit
#  pseudo-random number.
#**/
ASM_PFX(AsmRdTimerRng64):
  rdtime.d   $t0, $zero
  rdtimeh.w  $t1, $zero
  xor        $t0, $t0, $t1

  //
  // Mix low 32-bit to high 32-bit
  //
  slli.d     $t1, $t0, 32
  xor        $t0, $t0, $t1

  //
  // A xorshift64 step is applied to decorrelate timer bits.
  // The shft constants (13, 7, 17) are taken from Marsaglia's
  // xorshift generator and provide a maximal-period 64-bit
  // linear PRNG suitable for non-cryptographic use.
  //
  slli.d     $t1, $t0, 13
  xor        $t0, $t0, $t1
  srli.d     $t1, $t0, 7
  xor        $t0, $t0, $t1
  slli.d     $t1, $t0, 17
  xor        $a0, $t0, $t1

  jirl       $zero, $ra, 0

#/**
#  Generates a 16-bit random number.
#**/
ASM_PFX(AsmRdRng16):
  addi.d $sp, $sp, -16
  st.d   $s0, $sp, 0
  st.d   $ra, $sp, 8

  move   $s0, $a0
  bl     AsmRdTimerRng64
  st.h   $a0, $s0, 0

  ld.d   $ra, $sp, 8
  ld.d   $s0, $sp, 0
  addi.d $sp, $sp, 16

  li.d   $a0, 1

  jirl   $zero, $ra, 0

#/**
#  Generates a 32-bit random number.
#**/
ASM_PFX(AsmRdRng32):
  addi.d $sp, $sp, -16
  st.d   $s0, $sp, 0
  st.d   $ra, $sp, 8

  move   $s0, $a0
  bl     AsmRdTimerRng64
  st.w   $a0, $s0, 0

  ld.d   $ra, $sp, 8
  ld.d   $s0, $sp, 0
  addi.d $sp, $sp, 16

  li.d   $a0, 1

  jirl $zero, $ra, 0

#/**
#  Generates a 64-bit random number.
#**/
ASM_PFX(AsmRdRng64):
  addi.d $sp, $sp, -16
  st.d   $s0, $sp, 0
  st.d   $ra, $sp, 8

  move   $s0, $a0
  bl     AsmRdTimerRng64
  st.d   $a0, $s0, 0

  ld.d   $ra, $sp, 8
  ld.d   $s0, $sp, 0
  addi.d $sp, $sp, 16

  li.d   $a0, 1

  jirl $zero, $ra, 0
.end
