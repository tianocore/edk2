#------------------------------------------------------------------------------
#
# Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(__ashlti3)

#/**
#  Left shift a 128-bit integer.
#
#  @param[in]  $a0  Low 64 bits of the value.
#  @param[in]  $a1  High 64 bits of the value.
#  @param[in]  $a2  Shift count.
#
#  @return     $a0  Low 64 bits of the shifted value.
#  @return     $a1  High 64 bits of the shifted value.
#**/
ASM_PFX(__ashlti3):
  beqz    $a2, Ret

  li.d    $t0, 64
  bge     $a2, $t0, ShiftGreaterOrEqual64

  sub.d   $t1, $t0, $a2
  srl.d   $t1, $a0, $t1
  sll.d   $a1, $a1, $a2
  or      $a1, $a1, $t1
  sll.d   $a0, $a0, $a2
  b       Ret

ShiftGreaterOrEqual64:
  li.d    $t1, 128
  bge     $a2, $t1, ShiftGreaterOrEqual128

  sub.d   $t1, $a2, $t0
  sll.d   $a1, $a0, $t1
  move    $a0, $zero
  b       Ret

ShiftGreaterOrEqual128:
  move    $a0, $zero
  move    $a1, $zero

Ret:
  jirl    $zero, $ra, 0

  .size   ASM_PFX(__ashlti3), . - ASM_PFX(__ashlti3)

  .end
